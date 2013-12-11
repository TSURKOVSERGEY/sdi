// Wave.h: interface for the CWave class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVE_H__D3B52437_4C01_46A2_AD8F_E602AECE4CD7__INCLUDED_)
#define AFX_WAVE_H__D3B52437_4C01_46A2_AD8F_E602AECE4CD7__INCLUDED_

#include <mmsystem.h>   // Необходимо для WAVEFORMATEX 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CWave  
{

public:
	void FileWrite(BYTE *pBuffer, UINT BufferSize);

	CWave();
	virtual ~CWave();

	void InitWave(WAVEFORMATEX Waveformat);

protected:
	DWORD FileSize;

	CFile *pWaveFile;

struct WAVEHEADER{

	char  id_riff[4];
    DWORD len_riff;
	char  id_chuck[4];
	char  fmt[4];
	DWORD len_chuck;

} WaveHeader; 

struct WAVEINFO{

    WORD  wFormatTag; 
    WORD  nChannels; 
    DWORD nSamplesPerSec; 
    DWORD nAvgBytesPerSec; 
    WORD  nBlockAlign; 
    WORD  wBitsPerSample; 

}  WaveInfo;

struct WAVEDATA{

	char  id_data[4];
    DWORD len_data;

} WaveData; 

};

#endif // !defined(AFX_WAVE_H__D3B52437_4C01_46A2_AD8F_E602AECE4CD7__INCLUDED_)
