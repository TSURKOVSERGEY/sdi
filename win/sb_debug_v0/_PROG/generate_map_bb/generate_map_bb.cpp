// generate_map_bb.cpp : Defines the entry point for the console application.
//

#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <windows.h>


typedef char           int8_t;
typedef unsigned char  uint8_t; 
typedef short          int16_t;
typedef unsigned short uint16_t;
typedef int            int32_t; 
typedef unsigned int   uint32_t; 
typedef long		   int64_t;
typedef unsigned long  uint64_t;

#define MAX_BLOCK            4096
#define BLOCK_GOOD           0x1
#define BLOCK_BAD            0x2

#pragma pack(push,1)
 typedef struct
 { uint32_t bad_block_number;
   uint8_t  block_address[MAX_BLOCK];
   uint32_t crc;
 } bad_block_map_struct;
#pragma pack(pop)  


bad_block_map_struct *pmap_bb;
FILE *fp[2];


int main(int argc, char* argv[])
{

	int index;


    pmap_bb = new bad_block_map_struct;

	memset(pmap_bb,0,sizeof(bad_block_map_struct));

    fp[0] = fopen("map_bb.mas","w+");
	fp[1] = fopen("map_bb.ini","r");

	if(fp[1] == NULL)
	{	printf("\n");
		printf("Error open map_bb.ini \n");
		printf("\n");
      	printf("Press any key to exit \n");
      	printf("\n");
		while(!kbhit());
		return 0;
	}


	for(int i = 0; i < MAX_BLOCK; i++) 
	{
		pmap_bb->block_address[i] = BLOCK_GOOD;
	}

 
	printf("\n");

	while(fscanf(fp[1],"%d",&index) > 0)
	{
        printf(" page adr %d",index);

		index = index >> 6;

        printf(" block adr %d",index);
       	printf("\n");

		if(index < MAX_BLOCK)
		{
			pmap_bb->block_address[index] = BLOCK_BAD;
		    pmap_bb->bad_block_number++;
		}
		else
		{
			printf("\n");
        	printf("Error index array (%d) \n",index);
        	printf("\n");
		}

	}




    fwrite(pmap_bb,1,sizeof(bad_block_map_struct),fp[0]);

	fclose(fp[0]);
	fclose(fp[1]);

	delete pmap_bb;
    
	printf("\n");
	printf("Generate done\n");
    printf("\n");
	printf("press any key for exit !!!\n");
    printf("\n");
	
	while(!kbhit());

	return 0;
}

