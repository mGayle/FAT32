#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

//Borrowed from Prof Bakker's code. Used for toke() function. 
#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 100
#define MAX_NUM_ARGUMENTS 11

//Holds info for the current directory's directory entries. 
struct __attribute__((__packed__)) DirectoryEntry {
	char		DIR_Name[11]; 
	uint8_t		DIR_Attr; 
	uint8_t		Unused1[8]; 
	uint16_t	DIR_FirstClusterHigh; 
	uint8_t		Unused2[4]; 
	uint16_t	DIR_FirstClusterLow; 
	uint32_t	DIR_FileSize; 
};
struct DirectoryEntry dir[16]; 
struct DirectoryEntry *d = dir; 


//This is used to keep up with the directories that have been 
//visited and the order in which they were visited. It's needed
//when we have to do 'cd ..'.
struct Node{
	int dir_offset; 	//current dir offset
	int par_offset; 	//parent dir offset
	char name[11]; 		//name of directory
	struct Node *next; 	//points to next node
};
struct Node *head = NULL;
struct Node *current = NULL;  

/****Function Prototypes**************/
int		is_open; 
bool 		isEmpty(struct Node*); 
struct Node* 	insertEnd( struct Node*, char *, int, int ); 
void  		displayList( struct Node* ); 
struct Node* 	get_prev_node( struct Node* ); 
struct Node* 	deleteLast( struct Node* ); 


FILE *	open_file( char * ); 
void 	close_file( FILE * ); 
void 	toke(); 
void 	getInfo( FILE* ); 
char *	volume; 
void 	printInfo( void ); 
long 	getBPBsize(); 
void 	getDirectories( FILE *, int ); 
void 	printDirectories( FILE * );
int 	get_match( struct DirectoryEntry *, char * );
void 	do_stat( struct DirectoryEntry *, char * ); 
int 	do_cd( FILE*, struct DirectoryEntry *, char * ); 
void 	do_volume(); 
void 	do_ls( FILE * ); 
int 	check_cd( char * ); 
char **	make_file_list(FILE *); 

char * 	cd_mode1( FILE *, char * ); 
char * 	cd_mode2( FILE *, char * ); 
char *	cd_mode3( FILE *, char * );
char *	cd_mode4( FILE *, char * ); 

int 	LBAToOffset( int32_t ); 
int16_t NextLB( uint32_t, FILE * ); 


/****Global Variables***************/
char *	token[MAX_NUM_ARGUMENTS]; 
int 	token_count; 


int16_t BPB_BytsPerSec; //must = 512
int8_t 	BPB_SecPerClus;  //must be a power of 2 greater than 0
int16_t BPB_RsvdSecCnt; //must not be 0. Typically = 32
int8_t 	BPB_NumFATs; 	//must = 2   
int16_t BPB_RootEntCnt; //must be set to 0
char	BS_VolLab[11];   
char *	bs_vol_ptr; 
int32_t BPB_FATSz32;      //32-bit count of sectors occupied by ONE FAT	
int32_t BPB_RootClus;     //Cluster number of the first cluster of root dir. 
long 	BPBsize; 

char *	dir_ptr; 
char **	array_ptr = NULL; 

char 	cur_dir[11]; 		//holds the current directory name
char * 	cur_dir_ptr;
char	filename[11]; 		//global to help change values with functions
char *  filename_ptr; 		


int 	current_offset = NULL; 
int 	next_offset = NULL;    

int 	rootFlag; 	//trips when we've returned to root after a cd /filename/flskfj command
			//so we don't return to root on the second '/'
int 	done; 		//keeps track of whether we've processed the whole cd argument
char *	token_ptr; 


int main()
{
	FILE *fp; 
	int i;  
	is_open = 0; 		//no files are open, so set to 0. 
	bs_vol_ptr   = malloc(11 * sizeof(char)); 
	bs_vol_ptr   = BS_VolLab; 
	cur_dir_ptr  = malloc(11 * sizeof(char)); 
	cur_dir_ptr  = NULL; 
		
		
	head	 = (struct Node*) malloc(sizeof(struct Node)); 	
	current	 = (struct Node*) malloc(sizeof(struct Node)); 	
	
  	while (token[0] == NULL)
  	{	
		if( cur_dir_ptr != NULL ){
    			printf("mfs %s> ", cur_dir_ptr);  //Print current working directory  
		}					  // with prompt. 
		else{
			printf( "mfs> ");
		} 
		toke(); 
		//If command if "open filename" 
		if (strcmp(token[0], "open") == 0)	
		{
			fp = open_file( token[1] );	//Open the image file.
			is_open = 1;  
			getInfo(fp);			//Get the BPB info. 
			volume = BS_VolLab;  		//Get the volume name. 
	
			BPBsize = getBPBsize(); 	//Calculate BPBsize for later use. 
			unsigned int rootAddress; 	//Calculate root address. 
			rootAddress = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +
					(BPB_RsvdSecCnt * BPB_BytsPerSec);  
	 		getDirectories(fp, rootAddress); //Populate dir[16] with root's directories. 	
			cur_dir_ptr = "root";		 //Set cur_dir_ptr to "root" so it can be 
			next_offset = rootAddress;    	 //	displayed in mfs> prompt. 	
			current_offset = 2; 		 //Set root's parent offset to 2. 
			//Make a node to record this directory's offset info and its parent's offset info. 
			//This is used when "cd" is called. 
			head = insertEnd( head, cur_dir_ptr, next_offset, current_offset  ); 
 		}
		//If command is "info".
		else if (strcmp(token[0], "info") == 0)
		{
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {
				printInfo();    //Call printInfo. Prints info specified in assignment specs. 
			}
		}
		//If command is "stat filename". 
		else if (strcmp(token[0], "stat" ) == 0)
		{
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {	
		 		do_stat( d, token[1]); //Call do_stat on the filename. 
			}
		}
		//If command is "ls". 
		else if (strcmp(token[0], "ls" ) == 0){
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {
				array_ptr = make_file_list(fp); //Call make_file_list(). Files and directories
			}
		}					       // are printed from this function. 
		//If command is "volume". 
		else if (strcmp(token[0], "volume" ) == 0){
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {
				do_volume(); 			//Call do_volume to handle it. Prints volume name. 
			}
		} 
		//If command is "cd". 
		else if (strcmp( token[0], "cd" ) == 0) {  
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {
				rootFlag = 0;			//rootFlag is set to 0 at beginning of cd
								//   because this could possibly be a 'cd /filename.' 
								//   command. 
				done = 0;			//done is set to 0 until we know we're done processing
								//  the entire string.  
				int length = strlen(token[1]);  
				token_ptr = token[1]; 		//Use pointer to the string during processing. 
				//This while loop runs until we have processed all parts of the string. 
				//The cd_mode functions process it in parts and feed the string back 
				//	to this loop until it's all been processed. 
				while( !done ){			//While we haven't processed the whole string...

					int cd_mode; 		//Figure out what mode we need to run in. 
					cd_mode = check_cd( token_ptr ); 
			
					//mode 2: cd ..
					if( cd_mode == 2 ){
						token_ptr = cd_mode2(fp, token_ptr); 
					}
					//mode 3: cd ../filename
					else if( cd_mode == 3 ) {
						token_ptr = cd_mode3(fp, token_ptr); 
					}	
					//mode 1: cd filename
					else if( cd_mode == 1 ) {
						token_ptr = cd_mode1(fp, token_ptr);
					}
					//mode 4: cd /filename/filename
					else if( cd_mode == 4 ) {
						token_ptr = cd_mode4(fp, token_ptr); 
					}
					else{
						printf("invalid input\n"); 
					}
				}
			}
		}
		else if ( strcmp( token[0], "close" ) == 0 ) {
			if( !is_open )
			{
				printf("Error: File system image must be opened first.\n"); 
			}
			else {
				close_file( fp ); 
			}
		}
		token[0] = NULL;	//set to null so continues to prompt user in while loop
	}	
  	getc(stdin); 
  	return 0; 
}


//code provided by Prof Bakker. Takes input and parces it.
void toke(){
	char *cmd_str = (char*) malloc( MAX_COMMAND_SIZE ); 
	while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) ); 
	int token_count = 0; 
	
	char *arg_ptr; 
	char *working_str = strdup( cmd_str ); 
	
	char *working_root = working_str; 

	while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
		(token_count<MAX_NUM_ARGUMENTS))
	{
		token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE ); 
		if( strlen(token[token_count] ) == 0) 
		{
		token[token_count] = NULL; 
		}
		token_count++; 
	}
	free( working_root ); 
}

/* Function	: open_file
 * Parameters	: char* file_name - the name of the file that the user wants to open. 
 * Returns	: FILE * - pointer to the file that the function opened. 
 * Description	: This takes the filename that has been provided by the user
 * and opens it. Returns error if not found. 
 */	

FILE* open_file( char* file_name){
	FILE *fp; 
	struct stat buffer; 
	int status; 
	status = stat(file_name, &buffer); 
	if (status < 0)				//if the file does not exist
	{
		printf("ERROR: file system image not found\n"); 
	}
	else{
		fp = fopen(file_name, "r");
		if(!fp)				//if there's some other problem
		{				//when trying to open the file	
			perror("ERROR: could not open file\n ");
		}  
	}		
	return fp; 
}

/*
 * Function	: getInfo
 * Parameters	: FILE *fp - a file pointer to the file being used 
 * Returns	: void
 * Description	: This function seeks to the appropriate spots in the 
 * boot sector of the file image and finds the info needed. 
 */   
void   getInfo(FILE *fp){

	fseek( fp, 11, SEEK_SET); 		//set pointer to 11th byte 
	fread( &BPB_BytsPerSec, 1, 2, fp ); 	//read that data into variable..

	fseek( fp, 16, SEEK_SET ); 		//set pointer to 16th byte
	fread( &BPB_NumFATs, 1, 1, fp ); 	//read that data into variable...

	fseek( fp, 13, SEEK_SET ); 
	fread( &BPB_SecPerClus, 1, 1, fp ); 	//...

	fseek( fp, 14, SEEK_SET ); 
	fread( &BPB_RsvdSecCnt, 1, 2, fp ); 

	fseek( fp, 17, SEEK_SET ); 
	fread( &BPB_RootEntCnt, 1, 2, fp ); 

	fseek( fp, 71, SEEK_SET ); 		//set pointer to byte 71
	int i; 					//read one byte at a time bc its chars
	for ( i = 0; i < 11; i++ ){
		fread( &bs_vol_ptr[i], 1, 1, fp ); 
	}
	fseek( fp, 36, SEEK_SET); 
	fread( &BPB_FATSz32, 1, 4, fp ); 

	fseek( fp, 44, SEEK_SET); 
	fread( &BPB_RootClus, 1, 4, fp ); 
}

/* 
 * Function	: printInfo
 * Parameters	: void
 * Returns	: void
 * Description	: This funtion prints the information specified 
 * in the "info" requirement. Gets the data from the variables
 * created in getInfo(). 
 */
void printInfo(void){


	printf( "\nBPB_BytesPerSec: %d\n" , BPB_BytsPerSec );
	printf( "BPB_SecPerClus:\t %d\n"  , BPB_SecPerClus ); 
	printf( "BPB_RsvdSecCnt:\t %d\n"  , BPB_RsvdSecCnt ); 
	printf( "BPB_NumFATs:\t %d\n"	  , BPB_NumFATs );  
	printf( "BPB_FATSz32:\t %d\n"	  , BPB_FATSz32 );
}

long    getBPBsize(void){
	long BPBsize = 0;
	BPBsize = BPB_RsvdSecCnt * BPB_BytsPerSec; 
	return BPBsize; 			
}

/*
 * Function	: getDirectories
 * Parameters	: FILE *fp - pointer to file we're using.
 *		  int offset - the offset to the directory whose files and
 *		  directories we're retrieving. 
 * Returns	: void
 * Description	: This function goes to the current directory position
 * and retrieves the subdirectories and files there and populates the 
 * directory struct with this data. 
 */

void getDirectories(FILE *fp, int offset){
	int i; 
	for (i = 0; i < 16; i++ ){	//clear the structure so no garbage
		memset( &dir[i], 0, 32);  
	}
	fseek( fp, offset , SEEK_SET );  // go to the new directory location
	for ( i = 0; i < 16; i++ ){      // and populate its structure with
		fread( &dir[i], 32, 1, fp ); // data on its files and subdirectories
	}
}


int get_match(struct DirectoryEntry *directory, char *filename){
	int inputLength; 
	int listNameLength; 
 	int length[16]; 	

	inputLength = strlen(filename); 
	int i = 0; 
	int j = 0; 
	while( i < 16 ){
		while(j < strlen(directory[i].DIR_Name)){ 
			if( directory[i].DIR_Name[j] == 0x20 ){
				length[i] = j;
				break;  
			}
			length[i] = j; 
			j++; 
		}
		i++;
		j = 0 ; 	
	}
	i = 0; 
	j = 0; 
 	int match = 0; 
	while( i < 16 ){
		if (length[i] == inputLength){
			j = 0;  
			while(directory[i].DIR_Name[j] == filename[j]){
				match = 1; 		
				j++; 
				if(inputLength == j){
					return i; 
				}
						
			}
			match = 0; 
		}
		i++;
	
	}
	return -1; 
}

/*
 * Function	: do_stat
 * Parameters	: struct DirectoryEntry *directory - the pointer to the 
 *			current directory. 
 *	  	  char *filename - the name of the file in that directory 
 *			whose stats have been requested. 
 * Returns	: void
 * Description	: This displays the attribute, cluster starting number and 
 * file size of the file that's been specified by the user. It uses get_match()
 * to find the index of the file of interest within the current directory's 
 * list. 
 */
void do_stat( struct DirectoryEntry *directory, char *filename ){
	int index; 					//find the index of the 
	index = get_match( directory, filename); 	//file of interest. 
	printf( "NAME\t\tATTRIBUTE\t\tCLUSTER START\t\tFILE SIZE\n" ); 
	printf( "%s\t\t%d\t\t\t%d\t\t\t%d\n", filename, directory[index].DIR_Attr, 
					     directory[index].DIR_FirstClusterLow,
					     directory[index].DIR_FileSize      ); 
}

/*
 * Function	: make_file_list
 * Parameters	: FILE *fp - a pointer to the image file we're using
 * Returns	: char ** - a pointer to the list of files made by this function. 
 * Description	: This file takes current directory list of directory entries and
 * retrieves just the filenames and puts them in an array. Then returns the array. 
 * This is for the "ls" command. 
 */
char** make_file_list( FILE *fp ) {
	int i;
	char ** name_array_ptr = NULL;  //pointer to new array of directory entries. 
	name_array_ptr = malloc( 16 * sizeof( char* ) );
	for( i = 0; i < 16; i++ ){		//allocate memory for this array. 
		name_array_ptr[i] = malloc( 11 * sizeof( char ) ); 
	} 
	dir_ptr = dir[0].DIR_Name; 			//points to the filenames.
	for( i = 0; i < 16; i++ ){ 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 //only get the names
			|| dir[i].DIR_Attr == 0x20 ){ 		//with these attributes
			dir_ptr = dir[i].DIR_Name;   
			memcpy( name_array_ptr[i], dir_ptr, 11 ); //copy dir name to name[11].
			printf( "\n%s\n", name_array_ptr[i] );    //print from this function
		}						  //for 'ls' command.
		else{						//enter NULL if they don't have 
			 name_array_ptr[i] = NULL; 		//those attributes so that indices 
		}						//for filenames still correspond to 
	}							//their indices in directoryEntry array.
	return name_array_ptr; 
}

/*
 * Function	: printDirectories
 * Parameters	: FILE *fp
 * Returns	: void
 * Description	: This function just prints the directories in directoryEntry array
 * along with some of their information. It's not required by assignment specs but 
 * comes in handy sometimes.  
 */
void printDirectories(FILE *fp){
	int i; 
	printf("\n"); 
	for ( i = 0; i < 16; i++ ){
		printf("\n"); 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			printf( "HEX FIRST CHAR: %x\n"	, dir[i].DIR_Name[0] ); 
			printf( "FILE NAME: %s\n"	, dir[i].DIR_Name ); 
			printf( "ATTRIB BYTE: %x\n"	, dir[i].DIR_Attr ); 
			printf( "FstClusHI: %d\n"	, dir[i].DIR_FirstClusterHigh ); 
			printf( "FstClusLO: %d\n"	, dir[i].DIR_FirstClusterLow ); 
			printf( "FILE SIZE: %d\n" 	, dir[i].DIR_FileSize ); 
		}
	}
}

/*
 * Function	: do_volume
 * Parameters	: void
 * Returns	: void
 * Description	: This prints the volume name of the image file. If it has no
 * name, it prints the error message. 
 */
void do_volume( void ){
	if ( BS_VolLab[0] == 0x20 ){ 	//if the volume name bytes are just spaces, no name.
		printf( "Volume:\t Error: volume name not found.\n" ); 
	}
	else{
		printf( "BS_VolLab:\t%s\n ", volume );   //this was retrieved during "getInfo()". 
	}	
}


/*
 * Function 	: do_cd
 * Parameters	: FILE *fp - pointer to image file we're working with. 
 * 		  struct DirectoryEntry *directory - point to current directory struct. 
 *		  char* filename - pointer of the name of the file that we want to change to. 
 * Returns	: int - the address of the new file's info. i
 * Description	: When cd is called by user, this finds the location of the file that the user
 * wants to cd to. 
 */
 int do_cd( FILE * fp, struct DirectoryEntry *directory, char* filename ){
	int index; 				//get the index of the file within the 
	index = get_match( directory, filename ); //current directory struct.  	 
	uint16_t clusterNum = directory[index].DIR_FirstClusterLow; //get it's lo clus number. 
	fseek( fp, clusterNum, SEEK_SET); 		//go there in the image file.
							//don't think this is necessary. :/ 
	int offset; 				//get the address of the new file 
	offset =  ( BPB_RsvdSecCnt * BPB_BytsPerSec ) 
		+ ( BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec )
		+ ( clusterNum * BPB_BytsPerSec * BPB_SecPerClus ); 
	return offset; 				//return this address/offset.
}


/*
 * Function	: isEmpty	
 * Parameters	: struct Node* head - a pointer to the linked list of addresses. 
 * Returns	: bool - to indicate whether the list is empty. 
 * Description	: This assists in creating and maintaining the dynamic linked list 
 * of offsets that have been used. This list is needed for cd .. to keep up with parent
 * offsets. 
 */
bool isEmpty(struct Node* head){
	return (head == NULL); 
}

/*
 * Function 	: insertEnd
 * Parameters	: struct Node* head - pointer to the linked list of offsets. 
 * 		  char* cur_dir_ptr - pointer to keep up with the current node. 
 *		  int dir_off - the offset of the directory we're moving to. 
 *		  int par_off - the offset of the parent directory, which is the 
 * 				directory we're about to leave. 	
 * Returns	: struct Node* - pointer to the head of the linked list. 
 * Description	: This function inserts a new node into the linked list so that
 * we can record the address of the directory. 		
 */	
struct Node * insertEnd( struct Node* head, char* cur_dir_ptr, int dir_off, int par_off ){
	current = head; 
	struct Node *link = ( struct Node* ) malloc( sizeof( struct Node ) ); 
	link->dir_offset = dir_off;  	//make a new link and populate it with  
	link->par_offset = par_off;	//offset info. 
	strcpy( link->name, cur_dir_ptr ); //including the name of the directory 
	link->next = NULL; 

	if ( isEmpty( head ) ){
		head = link;
	}
	else{
		while( current->next != NULL ){  //find the end of list. 
			current = current->next; 
		}
	current->next = link;  			//add the new link there. 
	}
	return head; 				//return the list. 
}

/*
 * Function 	: deleteLast	
 * Parameters	: struct Node* head - pointer to linked list we're working on. 
 * Returns	: struct Node * - pointer to the linked list we modified. 
 * Description	: This deletes the last node of the linked list. This is used 
 * when we cd back to the parent directory. 
 */
struct Node *deleteLast( struct Node* head ){
	struct Node* cur = head; 
	struct Node* prev; 
	if( head->next == NULL ){  	//if there's only one entry... 
		free( head ); 		
		head = NULL; 
	}
	else{				//otherwise, traverse until end is found.
		while( cur->next != NULL ){ //prev follows behing cur to the end. 
			prev = cur; 
			cur = cur->next; 
		} 
		free( prev->next ); 	//when end is found, free the last node
		prev->next = NULL; 	//and set the last link to NULL.
	}
	return head; 
}


/*
 * Function	: get_prev_node
 * Parameters	: struct Node* head - pointer to linked list of visited directories. 
 * Returns	: struct Node* - pointer to this linked list. 
 * Description	: This finds the parent directory to the current directory for the 
 * "cd .." type commands. It traverses the linked list and returns the second to last
 * node. 
 */
struct Node* get_prev_node(struct Node* head){
	struct Node* prev = (struct Node*) malloc(sizeof(struct Node)); 
	struct Node* curr = (struct Node*) malloc(sizeof(struct Node)); 
	if( isEmpty(head) ){
		printf("\nError: can't go back.\n"); 
	}
	else{
		prev = head; 		//prev follows curr down the linked list. 
		curr = head->next; 	
		while( curr->next != NULL ){
			prev = curr; 
			curr = curr->next; 
		}
	}				//when the end is found, prev is returned. 
		return prev; 
}
/*
 * Function	: check_cd
 * Parameters	: char *input - the string that followed "cd" command from user. 
 * Returns	: int - integer to indicate what kind of cd should be performed. 
 * Description	: this reads the cd command and determines what cd mode is to be performed. 
 * mode 1 : cd filename
 * mode 2 : cd ..
 * mode 3 : cd ../filename
 * mode 4 : cd /filename
 */

int check_cd(char* input){
	
	int length = strlen( input );	//get length of input  
	//check for mode 2.....
	if ( length == 2 && (( strcmp( input, ".." )) == 0 ) ) {  
		return 2; 
	}
	//check for mode 3.....
	else if( length > 2 && ( input[0] == '.' ) && ( input[1] == '.' ) 
					           && ( input[2] == '/' ) ){
		return 3; 
	}
	//check for error. Return -1 if no mode. 
	else if( length > 2 && ( input[0] == '.' ) && ( input[1] == '.' ) 
				           && ( input[2] != '/' ) ){
		printf("Error: Invalid input\n"); 
		return -1; 
	}
	//check for mode 4...
	else if( input[0] == '/' ){
		return 4; 
	}
	//if none of these, it's mode 1.
	else {
		return 1; 
	}
}
		
/*
 * Function	: cd_mode1
 * Parameters	: FILE *fp - pointer to image file we're working with. 
 *		  char *token_ptr - pointer to the string we're working with. 
 * Returns	: char * - pointer to the string we're working with. It may 
 *			have been modified here. 
 * Description	: This function handles the mode 1 cd command. ( cd filename )			
 * 
 */
char * cd_mode1(FILE *fp, char* token_ptr ){
	memset(filename, 0, 11); 	//Clear out filename string
	//parce to get filename
	filename_ptr = filename;
	int count = 0; 
	int i = 0; 
	while(i < strlen(token_ptr)){		//figure out how long the filename is.
		if ((token_ptr[i] != '/') && (token_ptr[i] != '\0')){ //there may be more
			count ++; 				//characters after the filename
			i ++; 					//if it's a "cd filename/filename"
		}						//command, so we need to get just 
		else {						//the next filename only here. 
			break; 
		}
	}						//put that filename in filename_ptr.
	memcpy(filename_ptr, token_ptr, count); 
	
	current_offset = next_offset; 			//"Next_offset" is the address of the directory
							//  we're in now. We need to move that to "current_offset
							//  so we can put the upcoming directory in "next_offset".  
	next_offset = do_cd(fp, d, filename_ptr);	//Get the offset for new directory.
	getDirectories(fp, next_offset);  		//Populate dir[16] with the new directory's data.
	cur_dir_ptr = filename_ptr;  			//This is so we can display current directory with
							//  	mfs> prompt. 
	head = insertEnd(head, cur_dir_ptr, next_offset, current_offset); //Add this directory's offset info to
									  // 	the linked list.  
	token_ptr = token_ptr + strlen(filename); //Subtract the part we just processed from the string we're
	if(strlen(token_ptr) > 0 ){	          // 	processing. 
		done = 0; 			//Have we processed the whole path? If not, done = 0 
	}
	else if( strlen(token_ptr) == 0 ) {     //If we have, set done = 1. 
		done = 1; 
	}
	rootFlag = 1; 			//So we know not to send this argument to root anytime from now.  
	return token_ptr;		//If the input is "filename/filename, this tells the program that
					// the "/" was not originally at the beginning. 
} 

/*
 * Function	: cd_mode2
 * Parameters	: FILE *fp - pointer to the image file we're working with. 
 *		  char *token_ptr - pointer to the string we're processing for cd command. 
 * Returns 	: char * - pointer to the string we're processing. Possibly modified here. 
 * Description	: This handles cd mode 2 (cd .. ). 
 */
char * cd_mode2(FILE *fp, char *token_ptr ){			
	struct Node * node; 
	node = get_prev_node(head);	//Find the previous directory's offset in the  	
	next_offset = node->dir_offset; // 	offset info linked list. 
	current_offset = node->par_offset; 
	cur_dir_ptr = node->name; 	//Update cur_dir_ptr.
	head = deleteLast(head); 	//Delete last node from list. 
				
	getDirectories(fp, next_offset );	//Populate the current directory's dir[16]. 
	done = 1;			//If we're in this function, then we're certain we've  
	return token_ptr; 		// processed the whole string and done is set to 1. 
}



/*
 * Function	: cd_mode3
 * Parameters	: FILE *fp - pointer to image file we're working with. 
 *		  char *token_ptr - pointer to string we're processing for cd command. 
 * Returns	: char * - pointer to string we're processing. Possibly modified here. 
 * Description	: This function handles cd mode 3 (cd ../filename). 
 */
char * cd_mode3(FILE *fp, char *token_ptr ){				
	
	rootFlag = 1; 			//So we don't treat it like cd /filename.
	token_ptr = token_ptr + 3;      //Get filename by chopping off '../' .
	token_ptr = cd_mode2(fp, token_ptr);  //Process the ".." part with cd_mode2. 
	token_ptr = cd_mode1(fp, token_ptr);  //Process the "filename" part with cd_mode1. 

	return token_ptr; 			
}

/*
 * Function 	: cd_mode4
 * Parameters	: FILE *fp - pointer to the image file we're working with. 
 *		  char *token_ptr - pointer to the string we're processing for cd command. 
 * Returns	: char * - ponter to the string we're processing. 
 * Description	: This function handles cd mode 4 (/filename/filename). 
 */
char * cd_mode4(FILE *fp, char *token_ptr) {
	if( rootFlag == 0) {			//This means '/' was the first character. 
		while( head->next->next != NULL ){	//So go to root. 
			token_ptr = cd_mode2(fp, token_ptr ); //Process each visited directory with 
		}					      //  mode 2 on the way back to root.  	
		token_ptr = token_ptr + 1; 		      //Adjust string after we processed the '/' part. 	
		rootFlag = 1; 	//Set flag so if there's another '/' in the argument, we don't go back to root
	}
	else if( rootFlag == 1 ){		//If this isn't a 'start at root' cd command, 
		token_ptr = token_ptr + 1; 	//  just adjust the string and send back to cd loop 
	}					//  to continue processing if there's still more string left. 
	if(( strlen(token_ptr) > 0 )){		
		done = 0; 
	}
	else if(( strlen(token_ptr)) == 0 ) {   //Or set to 'done' if the string is all processed. 
		done = 1; 
	}
	return token_ptr; 	
}
/*
 *Function	: LBAToOffset
 *Parameters	: The current sector number that points to a block of data
 *Returns	: The value of the address for that block of data
 *Description	: Finds the starting address of a block of data given teh sector number
 *corresponding to that data block.
 *(credit: borrowed from class slides.)
 */
int LBAToOffset( int32_t sector ){
	return (( sector - 2 ) * BPB_BytsPerSec ) + ( BPB_BytsPerSec * BPB_RsvdSecCnt ) 
			+ ( BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec ); 
}

/*
 *Name: NextLB
 *Purpose: Given a logical block address, look into the first FAT and return the logical
 *block address of the block in the file. If there is no further blocks then return -1. 
 *(credit: borrowed from class slides.)
 */
int16_t NextLB( uint32_t sector, FILE *fp ){
	uint32_t FATAddress = ( BPB_BytsPerSec * BPB_RsvdSecCnt ) + ( sector * 4 ); 
	int16_t val; 
	fseek( fp, FATAddress, SEEK_SET ); 
	fread( &val, 2, 1, fp ); 
	return val; 
}

void close_file( FILE *fp ){
	if ( is_open ){		//If it's open, ....
		while( head->next->next != NULL ){	//Clear linked list and free nodes.  
			head = deleteLast( head ); 			
		}
		cur_dir_ptr = NULL; 			//So no directory is printed with prompt. 	       	
		fclose( fp ); 	//  close it. 
		is_open = 0;    //Record that we've closed the file.  	
	}
	else {			//If it's not open, Error. 
		printf( "Error: File system not open. " ); 
	}
} 

	
/*
void cd_error( ){
	printf("Error: invalid input.\n");  
}
*/






		
	/*
					struct Node * node; 
					node = get_prev_node(head); 	
					next_offset = node->dir_offset; 
					current_offset = node->par_offset; 
					cur_dir_ptr = node->name; 
					head = deleteLast(head); 
					getDirectories(fp, next_offset ); 
					//get filename (chop off '../'
					char *token_ptr; 
					token_ptr = token[1]; 
					token_ptr = token_ptr + 3; 
					//cd mode 3 protocol
					current_offset = next_offset; 		
					next_offset = do_cd(fp, d, token_ptr);	//get the offset for new directory.
					getDirectories(fp, next_offset);  
					cur_dir_ptr = token_ptr;  
					head = insertEnd(head, cur_dir_ptr, next_offset, current_offset); //add link to offset list.
					
					displayList(head); 
				}*/
	
/*
 * Function	: do_ls
 * Parameters	: FILE *fp
 * Returns	: void
 * Description	: This prints the 
 */

/*
void do_ls(FILE *fp){
	int i; 
	printf("\n"); 
	for ( i = 0; i < 16; i++ ){
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			printf("%s\n", dir[i].DIR_Name); 
		}
	}
	printf("\n"); 
}

*/
/*
 * Function	: displayList
 * Parameters	: struct Node *head - pointer to linked list of offsets used.
 * Returns	: void
 * Description	: This just prints the info from the linked list that keeps track of
 * the directories we've visited. Not required by assignment specs. 
 */
/*
void displayList(struct Node *head){
	struct Node *ptr = (struct Node*) malloc(sizeof(struct Node)); 
 	ptr = head; 
	printf("\n[ "); 
	
	while(ptr != NULL){
		printf("(%d, %d) ", ptr->dir_offset, ptr->par_offset); 
		ptr = ptr->next; 
	}
	printf(" ]\n"); 
}
*/
/*
struct Node * makeDummyList(struct Node *head){
	head = insertEnd( head, 15, 40); 
	head = insertEnd( head, 60, 30); 
	head = insertEnd( head, 77, 10); 
	head = insertEnd( head, 23, 24);
	return head;  
}
*/
//	if( strlen( token_ptr ) > 0 ){
//		done = 0; 
//	}
//	else if ( strlen( token_ptr ) == 0 ) {
//		done = 1;
//	} 
	
/*
	CHOPPED FROM CD_MODE 3

	current_offset = next_offset; 		
	next_offset = do_cd(fp, d, token_ptr);	//get the offset for new directory.
	getDirectories(fp, next_offset);  
	cur_dir_ptr = token_ptr;  
	head = insertEnd(head, cur_dir_ptr, next_offset, current_offset); //add link to offset list.
*/
