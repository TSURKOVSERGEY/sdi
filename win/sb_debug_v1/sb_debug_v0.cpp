// sb_debug_v0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "conio.h"
#include "windows.h"
#include "nand_sb_driver.h"

extern int sb_number;

unsigned char page_buffer[2048];

int main(int argc, char* argv[])
{



	SuperBlock_Config();
	

	
	while(!kbhit())
	{
		WriteNandPage((adpcm_message_struct*) page_buffer);
		printf("\r write page number % d \n",sb_number);
		Sleep(0);
	}

	getch();


	printf("press any key for exit !!!\n");
	

    OnClose();

	return 0;
}

