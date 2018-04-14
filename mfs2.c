#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

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
struct DirectoryEntry dir[16]; 
struct DirectoryEntry *d = dir; 


FILE *open_file(); 
void toke(); 
FILE *getInfo(FILE*); 
void printInfo(); 
long getBPBsize(); 
void getDirectories(FILE *); 
void printDirectories(FILE *);
int get_match(char **, struct DirectoryEntry *, char *);

 
char BS_OEMName[8]; 
int16_t BPB_BytsPerSec; //must = 512
int8_t BPB_SecPerClus;  //must be a power of 2 greater than 0
int16_t BPB_RsvdSecCnt; //must not be 0. Typically = 32
int8_t BPB_NumFATs; 	//must = 2   
int16_t BPB_RootEntCnt; //must be set to 0
char BS_VolLab[11];   
int32_t BPB_FATSz32;      //32-bit count of sectors occupied by ONE FAT	
int32_t BPB_RootClus;     //Cluster number of the first cluster of root dir. 
		//usually = 2 
long BPBsize; 

char *dir_ptr; 
char **array_ptr = NULL; 
char** make_file_list(FILE *); 

int main()
{
	FILE *fp; 
	int i;  
  	while (token[0] == NULL)
  	{	
    		printf("mfs> "); 
		toke(); 

		if (strcmp(token[0], "open") == 0)
		{
			fp = open_file(token[0], token[1]);
		}
		fp = getInfo(fp); 
		if (strcmp(token[0], "info") == 0)
		{
			printInfo(); 
		}
		getDirectories(fp); 
		if (strcmp(token[0], "stat" ) == 0)
		{
			array_ptr = make_file_list(fp); 
		 	int index = get_match(array_ptr, d, token[1]);
			printf("INDEX:\t%d\n", index); 
		}
		if (strcmp(token[0], "ls" ) == 0)
		{
			array_ptr = make_file_list(fp);
			printf("%s\n", array_ptr[0]);	//BUG HERE.  
			i = 0; 
			while( i < 16 ){
				if(array_ptr[i] != NULL){	//DOESN'T PRINT FIRST ELEMENT ???
					printf("%s\n", array_ptr[i]); 
				}
				i++; 
			} 
		}
//	printDirectories(fp); 
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
   
FILE*   getInfo(FILE *fp){
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

	fseek( fp, 42, SEEK_SET ); 
	int i;  
	for( i = 0; i < 11; i++ )
	{
	fread( &BS_VolLab[i], 1, 1, fp ); 
	}

	fseek( fp, 36, SEEK_SET); 
	fread( &BPB_FATSz32, 1, 4, fp ); 

	fseek( fp, 44, SEEK_SET); 
	fread( &BPB_RootClus, 1, 4, fp ); 
	return fp; 
}



void printInfo(void){

	//BS_OEMName = "MSWIN4.1"; 

	printf( "\nBPB_BytesPerSec: %d\n", BPB_BytsPerSec );
	printf( "BPB_NumFATs:\t %d\n", BPB_NumFATs );  
	printf( "BPB_SecPerClus:\t %d\n", BPB_SecPerClus ); 
	printf( "BPB_RsvdSecCnt:\t %d\n", BPB_RsvdSecCnt ); 
	printf( "BPB_RootEntCnt:\t %d\n", BPB_RootEntCnt ); 
	printf( "BS_VolLab:\t " ); 
	int i; 
	for( i = 0; i < 11; i++ )
	{
	printf( "%d", BS_VolLab[i] );   
	}
	printf( "\n" ); 
	printf( "BPB_FATSz32:\t %d\n", BPB_FATSz32 );
	printf( "BPB_RootClus:\t %d\n", BPB_RootClus );
}

long    getBPBsize(void){
	long BPBsize = 0;
	BPBsize = BPB_RsvdSecCnt * BPB_BytsPerSec; 
	return BPBsize; 			
}

void getDirectories(FILE *fp){
	char name[11]; 
	char *names; 


	BPBsize = getBPBsize(); 
	unsigned int rootAddress; 
	rootAddress = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +
			(BPB_RsvdSecCnt * BPB_BytsPerSec);  
	fseek( fp, rootAddress , SEEK_SET );  
	int i; 
	for ( i = 0; i < 16; i++ ){
		fread( &dir[i], 32, 1, fp ); 
	}
}
int get_match(char **nameList, struct DirectoryEntry *directory, char *filename){
	printf("\nIN STAT\n"); 
	int inputLength; 
	int listNameLength; 
 	int length[16]; 	

	inputLength = strlen(filename); 
	int i = 0; 
	int j = 0; 
	while( i < 16 ){
		printf("iteration: %d  ", i);
		while(j < strlen(directory[i].DIR_Name)){ 
			if( directory[i].DIR_Name[j] == 0x20 ){
//				printf("   IT EQUALS 20   "); 
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
			printf("\n");
			j = 0;  
			while(directory[i].DIR_Name[j] == filename[j]){
				match = 1; 		
				j++; 
				if(inputLength == j){
					printf("THE MATCH IS %s\n", directory[i].DIR_Name); 
					return i; 
				}
						
			}
			match = 0; 
		}
		i++;
	
	}
	return -1; 
}




//	for( i = 0; i < 16; i++ ){
//		if (filename == dir[i].DIR_Name[0]){
//			printf("ATTRIBUTE: %d\n", dir[i].DIR_Attr );
//			printf("CLUSTER NUMBER: %d\n", dir[i].DIR_FirstClusterLow ); 
//			printf("SIZE: %d\n", dir[i].DIR_FileSize ); 
//		}
//	} 


//NEXT MAKE FILE LIST. SEPARATE MAKING LIST AND LS COMMAND. MAKE LIST OF FILES IN CURRENT DIR
//IF NOT FOR PRINTING IN LS, MAKE = NULL SO YOU CAN KEEP INDICES CORRECT. 
//FOR LS, PRINT FILE LIST IF NAME != NULL

char** make_file_list(FILE *fp){
	int i;
	char ** name_array_ptr = NULL;  
	name_array_ptr = malloc(16 * sizeof(char*));
	for( i = 0; i < 16; i++){
		name_array_ptr[i] = malloc(11 * sizeof(char)); 
	} 
	char temp[11]; 
	char *temp_ptr; 
	temp_ptr = malloc(11 * sizeof(char)); 	
	temp_ptr = temp; 
	dir_ptr = dir[0].DIR_Name; 
	memcpy(name_array_ptr[0], dir_ptr, 11 ); 
	memcpy(temp_ptr, name_array_ptr[0], 11);
	memcpy(name_array_ptr[0], temp_ptr, 11); 
	name_array_ptr[0] = temp_ptr;		
//	printf("first element: %s\n", name_array_ptr[0]); 
	for( i = 1; i < 16; i++ ){ 
		if( dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 
			|| dir[i].DIR_Attr == 0x20 ){ 
			dir_ptr = dir[i].DIR_Name;   
			memcpy(name_array_ptr[i], dir_ptr, 11); //copy dir name to name[11]
		}
		else{
			 name_array_ptr[i] = NULL; 
		}	
	}
	return name_array_ptr; 
}

//void do_ls(char **arr){
//	int i; 
//	for( i = 0; i < 16; i++){
//		printf("%s\n", arr[i]); 
//	}
//} 

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

