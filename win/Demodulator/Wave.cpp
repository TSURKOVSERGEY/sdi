// Wave.cpp: implementation of the CWave class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Demodulator.h"
#include "Wave.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWave::CWave()
{
	pWaveFile = NULL;
}

CWave::~CWave()
{
	if(pWaveFile == NULL)
		return;

	strcpy(WaveHeader.id_riff,"RIFF");
	WaveHeader.len_riff =  FileSize + sizeof(WaveHeader) - 8;   // Длина файла без этого заголовка
	strcpy(WaveHeader.id_chuck,"WAVE");							// Индификатор "WAVE" = 0x45564157
	strcpy(WaveHeader.fmt,"fmt ");								// Индификатор "fmt " = 0x20746D66
	WaveHeader.len_chuck  = 0x10;								// Длина этого куска WAV - файла

	strcpy(WaveData.id_data,"data");							// Индификатор "data "  
	WaveData.len_data		= FileSize;
	
	pWaveFile->Seek(0,CFile::begin);
	pWaveFile->Write(&WaveHeader,sizeof(WaveHeader));
	pWaveFile->Write(&WaveInfo,sizeof(WaveInfo));
	pWaveFile->Write(&WaveData,sizeof(WaveData));
	pWaveFile->Close();

	delete pWaveFile;
}

void CWave::InitWave(WAVEFORMATEX Waveformat)
{
	WaveInfo.wFormatTag		 = Waveformat.wFormatTag;
	WaveInfo.nChannels		 = Waveformat.nChannels;
	WaveInfo.nSamplesPerSec  = Waveformat.nSamplesPerSec;
	WaveInfo.nAvgBytesPerSec = Waveformat.nAvgBytesPerSec;
	WaveInfo.nBlockAlign     = Waveformat.nBlockAlign;
	WaveInfo.wBitsPerSample  = Waveformat.wBitsPerSample;

	pWaveFile = new CFile("SoundTest.wav", CFile::modeCreate | CFile::modeWrite);
	pWaveFile->Write(&WaveHeader,sizeof(WaveHeader));
	pWaveFile->Write(&WaveInfo,sizeof(WaveInfo));
	pWaveFile->Write(&WaveData,sizeof(WaveData));

	FileSize = 0x0;
}

void CWave::FileWrite(BYTE *pBuffer, UINT BufferSize)
{
	pWaveFile->Write(pBuffer,BufferSize);
	FileSize = FileSize + BufferSize;
}
