// PlayBuffer.h: interface for the CPlayBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_)
#define AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_

#include <winsock2.h>   // Необходимо для инициализации DirectSound
#include <mmsystem.h>   // Необходимо для инициализации DirectSound
#include <mmreg.H>      // Необходимо для инициализации DirectSound
#include <dsound.h>     // Необходимо для инициализации DirectSound
#include <mmsystem.h>

#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

#define PLAY_BUFFER_NUMBER	4			// Кол-во DMA буферов
#define PLAY_BUFFER_SIZE 	800			// Размер буфера в словах
#define PLAY_SAMPLING_RATE 	8000		// Частота дискретизации
#define PLAY_CHANAL_NUMBER	1			// Моно / стерео
#define PLAY_BITSPERSAMPLE  16          // Уровень квантования        

class CDemodulatorDlg;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPlayBuffer  
{

public:
	CPlayBuffer(CDemodulatorDlg* mpDlg);
	virtual ~CPlayBuffer();

	BOOL StartPlay();
	BOOL InitDirectSound(HWND hWin);
	HRESULT HandleNotification();
	static UINT PlayProcessStream(LPVOID pVoid);

	LPDIRECTSOUNDBUFFER		pDSBuffer;			// - Указ на объект реализующий события  

protected:

	bool ControlPlayThread;

	LPDIRECTSOUND			pDSObject;			// - Указ на объект DirectSound
 
	LPDIRECTSOUNDNOTIFY		pDSNotify;			// - Указ на объект реализующий события   
	WAVEFORMATEX*			pWfex;				// - Указ структуру описывающую <тип звука>

    DSBUFFERDESC			DSBufferDesc;		// Структура использующаяся в CreateSoundBuffer методе

	DSBPOSITIONNOTIFY		DSPositionNotify[PLAY_BUFFER_NUMBER + 1];  // +1 для события останов буффер 
	HANDLE					hNotificationEvents[PLAY_BUFFER_NUMBER]; 

	DWORD					SoundBufferSize;    // Размер буффера 
	DWORD					SoundBufferOffset;  // Место от куда писать (в буфере)
	DWORD					NotifySize;			// Модуль счета, по которому формируется событие

	CDemodulatorDlg*		pDemDlg;

};

#endif // !defined(AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_)
