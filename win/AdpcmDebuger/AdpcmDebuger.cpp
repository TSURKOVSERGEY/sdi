// AdpcmDebuger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <conio.h>
#include "AdpcmDebuger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_PAGES           16320
#define MAX_SAMPLE          224
#define MAX_BUFFER          36
#define MAX_CHANNEL         16


#define ADPCM_PAGE_ID       0x2
#define SUPER_BLOCK_ID      0xa1b2c3d4

typedef char           int8_t;
typedef unsigned char  uint8_t; 
typedef short          int16_t;
typedef unsigned short uint16_t;
typedef int            int32_t; 
typedef unsigned int   uint32_t; 
typedef long		   int64_t;
typedef unsigned long  uint64_t;


#pragma pack(push,1)
 typedef struct  
 {
	 unsigned int   riffsig;   // 1179011410 'RIFF' 
     unsigned int   filesize;  // размер файла начиная со следующего байта (-8)
     unsigned int   wavesig;   // 1163280727 'WAVE'

     unsigned int   fmtsig;    // 544501094 'fmt'
     unsigned int   fmtsize;   // (subchunk size) со след байта до "data"
     unsigned short type;      // WAVE_FORMAT_PCM
     unsigned short nch;       // число каналов, 1 или 2
     unsigned int   freq;      // частота сэмплов
     unsigned int   rate;      // датарейт
     unsigned short block;     // размер блока (размер сэмпла * число каналов)
     unsigned short bits;      // бит на сэмпл 4, 8 или 16
     unsigned short sbsize;    // кол-во дополнительных байт (2)
  	 unsigned short extra_byte;

     unsigned int   datasig;   // 1635017060 data
     unsigned int   datasize;

 } wave_header;
#pragma pack(pop)



#pragma pack(push,1)
 typedef struct
 { int16_t  prevsample;
   int8_t   previndex;
   int8_t   id;
   int8_t   adpcm_data[MAX_SAMPLE / 2 * MAX_BUFFER]; // 18 * 112 = 2016 + 32(free)
   uint32_t time[2];
   uint32_t crc;   
 } adpcm_page_struct;
#pragma pack(pop) 

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

void GetTime(int ms, int *h, int *m, int *s)
{
	*s = (ms/10)    - (60 * ((ms/10) / 60));
	*m = (ms/10/60) - (60 * ((ms/10) /3600));
	*h = (ms/10/3600);	
}

#define MY_PLAYER

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	CString              buffer;
    CFile                file[3] = {NULL,NULL,NULL};
	wave_header          wave_hdr;
	adpcm_page_struct    aps;


	int                  my_player;
    int                  fsize;
	unsigned int         channel_id = 0;
	char                 fname[100];


	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}


	printf("\n adpcm debuger \n");

	FILE *fp; 


	if((fp = fopen("AdpcmDebuger.ini","r")) == NULL)
	{
		printf("\n failure open initial file << AdpcmDebuger.ini >> \n ");
		Sleep(2000);
		return nRetCode;
	}


	fscanf(fp,"%s %d %d",fname,&channel_id,&my_player);

	if(!file[1].Open("nand_page.dat",CFile::modeRead))
	{
		printf("\n failure open input file << nand_page.dat >> \n ");
		Sleep(2000);
		return nRetCode;
	}
	
	
	if(channel_id > 15)
	{
		printf("\n failure read channel_id param \n ");
		Sleep(2000);
		return nRetCode;
	}


	my_player &= 0x1;


	sprintf(fname,"adpcm_WinPlayer_channel_%d.dat",channel_id);

	file[0].Open(fname,CFile::modeCreate | CFile::modeWrite);


    if(my_player)
	{
		sprintf(fname,"adpcm_MyPlayer_channel_%d.adm",channel_id);
	    file[2].Open(fname,CFile::modeCreate | CFile::modeWrite);
	}

	CFileStatus status;

    file[2].GetStatus(status);

	if(status.m_attribute == 0)
	{
		printf("\n failure write output file \n ");
		Sleep(1000);
		return nRetCode;
	}

	memset(&wave_hdr,0,sizeof(wave_hdr));
    
	file[0].Write(&wave_hdr,sizeof(wave_hdr));
	
	fsize = file[1].GetLength();
    
	int h,m,s;

	int i = 0;

	while(fsize > 0)
	{
		if(!file[1].Read(&aps,sizeof(aps)))
		{
			Sleep(0);
		
		}
  	    fsize -= sizeof(aps);

		GetTime(aps.time[0],&h,&m,&s);

		printf("\r num = %d ch = %d     time %d : %d : %d \n",i++,channel_id,h,m,s);

		buffer.Format("\r %d \n",aps.id);
		OutputDebugString(buffer);
			
		if(aps.id != channel_id) 
		{
			continue;
		}
		else
		{
			file[0].Write(&aps,sizeof(aps)-12);

			if(my_player) file[2].Write(&aps,sizeof(aps)-12);
    
		}


	}

	wave_hdr.riffsig    = 0x46464952;            // сигнатура ('RIFF')
	wave_hdr.filesize   = file[0].GetLength()-8; // размер файла
	wave_hdr.wavesig    = 0x45564157;            // тип фалйа ('WAVE')
	wave_hdr.fmtsig     = 0x20746D66;            // сигнатура ('fmt')
	wave_hdr.fmtsize    = 20;                    // смещение до 'fact'
	wave_hdr.type       = 0x11;                  // WAVE_FORMAT_PCM
	wave_hdr.nch        = 0x1;                   // 1 канал = моно
	wave_hdr.freq       = 8000;                  // частота сэмплов
	wave_hdr.rate       = 4055;                  // байт в секунду
    wave_hdr.block      = sizeof(aps)-12;        // без crc и времени
	wave_hdr.bits       = 4;                     // 8 бит на сэмпл
	wave_hdr.sbsize     = 2;                     // кол-во дополнительных байт
    wave_hdr.extra_byte = 0;                     // выборок на блок на канал 
	wave_hdr.datasig    = 0x61746164;       
	wave_hdr.datasize   = file[0].GetLength() - sizeof(wave_hdr);            

	file[0].SeekToBegin();
    file[0].Write(&wave_hdr,sizeof(wave_hdr));

	file[0].Close();
	file[1].Close();

			
	if(my_player) file[2].Close();

	printf("\n");
    printf("\n unpacking done PRESS ANY KEY TO EXIT");

	
    while(!kbhit());
	
	
	//int i = getch();



	return nRetCode;
}


/*

				if(brw++ == 0) 
				{
					file[0].Write(&ams.adpcm_block[channel_id],4);
				}
				else if(brw == 6) brw = 0;
					
			    file[0].Write(&ams.adpcm_block[channel_id].adpcm_data[0],MAX_SAMPLE / 2);  

			    wave_hdr.block = ((MAX_SAMPLE / 2) * 6) + 4;
				*/