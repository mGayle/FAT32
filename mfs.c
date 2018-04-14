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

#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 100
#define MAX_NUM_ARGUMENTS 11

char *token[MAX_NUM_ARGUMENTS]; 
int token_count; 

struct __attribute__((__packed__)) DirectoryEntry {
	char		DIR_Name[11]; 
	uint8_t		DIR_Attr; 
	uint8_t		Unused1[8]; 
	uint16_t	DIR_FirstClusterHigh; 
	uint8_t		Unused2[4]; 
	uint16_t	DIR_FirstClusterLow; 
	uint32_t	DIR_FileSize; 
};
struct Node{
	int dir_offset; 
	int par_offset; 
	struct Node *next; 
	struct Node *prev; 
};
struct Node *head = NULL;
struct Node *current = NULL;  
struct Node *previous = NULL; 

bool 		isEmpty(struct Node*); 
struct Node* 	insertEnd( struct Node*, int, int ); 
//struct Node *deleteLast(); 
void  		displayList( struct Node* ); 
struct Node*  	makeDummyList( struct Node* );  
struct Node* 	get_prev_node( struct Node* ); 
struct Node* 	deleteLast( struct Node* ); 


struct DirectoryEntry dir[16]; 
struct DirectoryEntry *d = dir; 


FILE *	open_file(); 
void 	toke(); 
void 	getInfo(FILE*); 
char *	volume; 
void 	printInfo(void); 
long 	getBPBsize(); 
void 	getDirectories(FILE *, int); 
void 	printDirectories(FILE *);
int 	get_match(struct DirectoryEntry *, char *);
void 	do_stat(struct DirectoryEntry *, char *); 
int 	do_cd(FILE*, struct DirectoryEntry *, char *); 
void 	do_volume(); 
void 	do_ls(FILE *); 


char 	BS_OEMName[8]; 
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
char **	make_file_list(FILE *); 

char 	cur_dir[11]; 
char * 	cur_dir_ptr;

int 	current_offset = NULL; 
int 	next_offset = NULL;    

int main()
{
	FILE *fp; 
	int i;  
	bs_vol_ptr   = malloc(11 * sizeof(char)); 
	bs_vol_ptr   = BS_VolLab; 
	cur_dir_ptr  = malloc(11 * sizeof(char)); 
	cur_dir_ptr  = NULL; 
	
		
	head	 = (struct Node*) malloc(sizeof(struct Node)); 	
	current	 = (struct Node*) malloc(sizeof(struct Node)); 	
	previous = (struct Node*) malloc(sizeof(struct Node)); 	
	
  	while (token[0] == NULL)
  	{	
		if( cur_dir_ptr != NULL ){
    			printf("mfs %s> ", cur_dir_ptr); 
		}
		else{
			printf( "mfs> ");
		} 
		toke(); 

		if (strcmp(token[0], "open") == 0)
		{
			fp = open_file(token[0], token[1]);
			getInfo(fp);
			volume = BS_VolLab;  
	
			BPBsize = getBPBsize(); 
			unsigned int rootAddress; 
			rootAddress = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +
					(BPB_RsvdSecCnt * BPB_BytsPerSec);  
	 		getDirectories(fp, rootAddress);
			cur_dir_ptr = "root";
			next_offset = rootAddress;    
			current_offset = 2; 
			printf("\nBEFORE MAKE DUMMY LIST\n"); 
			head = insertEnd( head, next_offset, current_offset  ); 
			printf("\nAFTER MAKE DUMMY LIST\n"); 
			displayList(head);
 		}
		else if (strcmp(token[0], "info") == 0)
		{
			printInfo(); 
		}
		else if (strcmp(token[0], "stat" ) == 0)
		{
		 	do_stat( d, token[1]);
		}
		else if (strcmp(token[0], "ls" ) == 0){
			array_ptr = make_file_list(fp);
		}
		else if (strcmp(token[0], "volume" ) == 0){
			do_volume(); 
		}
		else if (strcmp( token[0], "cd" ) == 0) {
			if(strcmp( token[1], ".." ) != 0) {
				current_offset = next_offset; 		
				next_offset = do_cd(fp, d, token[1]);	//get the offset for new directory.
				getDirectories(fp, next_offset);  
				//printDirectories(fp);
				cur_dir_ptr = token[1];  
				head = insertEnd(head, next_offset, current_offset); //add link to offset list.
				displayList(head);
			}
			else if(strcmp( token[1], "..") == 0) {
				struct Node * node; 
				node = get_prev_node(head); 	
				next_offset = node->dir_offset; 
				current_offset = node->par_offset; 
				head = deleteLast(head); 

				getDirectories(fp, next_offset ); 
				displayList(head); 
			}	
		}
		token[0] = NULL;
	}	
  
  	getc(stdin); 
	
	fclose( fp ); 
  	return 0; 
}

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
	
FILE* open_file(char* cmmd, char* file_name){
	FILE *fp; 
	struct stat buffer; 
	int status; 
	status = stat(file_name, &buffer); 
	if (status < 0)
	{
		printf("ERROR: file system image not found\n"); 
	}
	else{
		fp = fopen(file_name, "r");
		if(!fp)
		{
			perror("ERROR: something: ");
		}  
	}		
	return fp; 
}
   
void   getInfo(FILE *fp){
	//BS_OEMName = "MSWIN4.1"; 

	fseek( fp, 11, SEEK_SET); 
	fread( &BPB_BytsPerSec, 1, 2, fp ); 

	fseek( fp, 16, SEEK_SET ); 
	fread( &BPB_NumFATs, 1, 1, fp ); 

	fseek( fp, 13, SEEK_SET ); 
	fread( &BPB_SecPerClus, 1, 1, fp ); 

	fseek( fp, 14, SEEK_SET ); 
	fread( &BPB_RsvdSecCnt, 1, 2, fp ); 

	fseek( fp, 17, SEEK_SET ); 
	fread( &BPB_RootEntCnt, 1, 2, fp ); 

	fseek( fp, 71, SEEK_SET ); 
	int i; 
	for ( i = 0; i < 11; i++ ){
		fread( &bs_vol_ptr[i], 1, 1, fp ); 
	}
	fseek( fp, 36, SEEK_SET); 
	fread( &BPB_FATSz32, 1, 4, fp ); 

	fseek( fp, 44, SEEK_SET); 
	fread( &BPB_RootClus, 1, 4, fp ); 
	
//	return fp; 
}



void printInfo(void){


	printf( "\nBPB_BytesPerSec: %d\n", BPB_BytsPerSec );
	printf( "BPB_SecPerClus:\t %d\n", BPB_SecPerClus ); 
	printf( "BPB_RsvdSecCnt:\t %d\n", BPB_RsvdSecCnt ); 
	printf( "BPB_NumFATs:\t %d\n", BPB_NumFATs );  
	printf( "BPB_FATSz32:\t %d\n", BPB_FATSz32 );
}

long    getBPBsize(void){
	long BPBsize = 0;
	BPBsize = BPB_RsvdSecCnt * BPB_BytsPerSec; 
	return BPBsize; 			
}

void getDirectories(FILE *fp, int offset){
	printf( "OFFSET: %d \n", offset ); 
	int i; 
	for (i = 0; i < 16; i++ ){
		memset( &dir[i], 0, 32);  
	}
	fseek( fp, offset , SEEK_SET );  
	for ( i = 0; i < 16; i++ ){
		fread( &dir[i], 32, 1, fp ); 
	}
	printDirectories(fp); 
	
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

void do_stat( struct DirectoryEntry *directory, char *filename){
	int index; 
	index = get_match( directory, filename); 
	printf("NAME\t\tATTRIBUTE\t\tCLUSTER START\t\tFILE SIZE\n"); 
	printf("%s\t\t%d\t\t\t%d\t\t\t%d\n", filename, directory[index].DIR_Attr, 
					     directory[index].DIR_FirstClusterLow,
					      directory[index].DIR_FileSize); 
}





char** make_file_list(FILE *fp){
	int i;
	char ** name_array_ptr = NULL;  
	name_array_ptr = malloc(16 * sizeof(char*));
	for( i = 0; i < 16; i++){
		name_array_ptr[i] = malloc(11 * sizeof(char)); 
	} 
//	char temp[11]; 
//	char *temp_ptr; 
//	temp_ptr = malloc(11 * sizeof(char)); 	
//	temp_ptr = temp; 
	dir_ptr = dir[0].DIR_Name; 
//	memcpy(name_array_ptr[0], dir_ptr, 11 ); 
//	memcpy(temp_ptr, name_array_ptr[0], 11);
//	memcpy(name_array_ptr[0], temp_ptr, 11); 
//	name_array_ptr[0] = temp_ptr;		
	for( i = 0; i < 16; i++ ){ 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			dir_ptr = dir[i].DIR_Name;   
			memcpy(name_array_ptr[i], dir_ptr, 11); //copy dir name to name[11]
			printf("\n%s\n", name_array_ptr[i]); 
		}
		else{
			 name_array_ptr[i] = NULL; 
		}	
	}
	return name_array_ptr; 
}


void printDirectories(FILE *fp){
	int i; 
	printf("\n"); 
	for ( i = 0; i < 16; i++ ){
		printf("\n"); 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			printf("HEX FIRST CHAR: %x\n", dir[i].DIR_Name[0]); 
			printf("FILE NAME: %s\n", dir[i].DIR_Name); 
			printf("ATTRIB BYTE: %x\n", dir[i].DIR_Attr); 
			printf("FstClusHI: %d\n", dir[i].DIR_FirstClusterHigh); 
			printf("FstClusLO: %d\n", dir[i].DIR_FirstClusterLow); 
			printf("FILE SIZE: %d\n", dir[i].DIR_FileSize); 
		}
	}
}

void do_volume(void){
	if (BS_VolLab[0] == 0x20 ){
		printf("Volume:\t Error: volume name not found.\n"); 
	}
	else{
		printf( "BS_VolLab:\t%s\n ", volume ); 
	}	
}
 
int do_cd(FILE * fp, struct DirectoryEntry *directory, char* filename){
	int index; 
	index = get_match(directory, filename); 
	uint16_t clusterNum = directory[index].DIR_FirstClusterLow; 
	fseek( fp, clusterNum, SEEK_SET); 

	int offset; 
	offset =  ( BPB_RsvdSecCnt * BPB_BytsPerSec ) 
		+ ( BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec )
		+ ( clusterNum * BPB_BytsPerSec * BPB_SecPerClus ); 
	return offset; 	
}

void do_ls(FILE *fp){
	int i; 
	printf("\n"); 
	for ( i = 0; i < 16; i++ ){
		printf("\n"); 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			printf("%s\n", dir[i].DIR_Name); 
		}
	}
}

bool isEmpty(struct Node* head){
	return (head == NULL); 
}
//BUG: doesn't detect of starting with an empty list. first node values are just (0 ,0)
struct Node * insertEnd(struct Node* head, int dir_off, int par_off){
	current = head; 
	//create link 
	struct Node *link = (struct Node*) malloc(sizeof(struct Node)); 
	link->dir_offset = dir_off; 
	link->par_offset = par_off;
	link->next = NULL; 

	if ( isEmpty(head) ){
	//make it the last link
		printf("\nEMPTY\n"); 
		head = link;
	}
	else{
		//find end of list
		while(current->next != NULL){
			current = current->next; 
		}
	current->next = link;  
	}
	return head; 
}

struct Node *deleteLast(struct Node* head){
	struct Node* temp = head; 
	struct Node* t; 
	//if only one link
	if( head->next == NULL ){
		free( head ); 
		head = NULL; 
	}
	else{
		while(temp->next != NULL){
			t = temp; 
			temp = temp->next; 
		}
		free(t->next); 
		t->next = NULL; 
	}
	return head; 
}

void displayList(struct Node *head){
	//start from beginning
	struct Node *ptr = (struct Node*) malloc(sizeof(struct Node)); 
 	ptr = head; 
	//navigate till end of list
	printf("\n[ "); 
	
	while(ptr != NULL){
		printf("(%d, %d) ", ptr->dir_offset, ptr->par_offset); 
		ptr = ptr->next; 
	}
	printf(" ]\n"); 
}

struct Node * makeDummyList(struct Node *head){
	head = insertEnd( head, 15, 40); 
	head = insertEnd( head, 60, 30); 
	head = insertEnd( head, 77, 10); 
	head = insertEnd( head, 23, 24);
	return head;  
}
//returns the second to last node for cd ..
struct Node* get_prev_node(struct Node* head){
	struct Node* prev = (struct Node*) malloc(sizeof(struct Node)); 
	struct Node* curr = (struct Node*) malloc(sizeof(struct Node)); 
	if( isEmpty(head) ){
		printf("\nEMPTY! FIX THIS!\n"); 
	}
	else{
		prev = head; 
		curr = head->next; 
		while( curr->next != NULL ){
			prev = curr; 
			curr = curr->next; 
		}
	}
		return prev; 
}




//ROOT DIRECTORY PARENT OFFSET
	
