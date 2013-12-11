// RecordBuffer.h: interface for the CRecordBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_)
#define AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_

#include <winsock2.h>   // Необходимо для инициализации DirectSound
#include <mmsystem.h>   // Необходимо для инициализации DirectSound
#include <mmreg.H>      // Необходимо для инициализации DirectSound
#include <dsound.h>     // Необходимо для инициализации DirectSound
#include <mmsystem.h>

#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

#define REC_BUFFER_NUMBER	2			// Кол-во DMA буферов
#define REC_BUFFER_SIZE 	1200			// Размер буфера в словах
#define REC_SAMPLING_RATE 	48000		// Частота дискретизации
#define REC_CHANAL_NUMBER	1			// Моно / стерео
#define REC_BITSPERSAMPLE   8           // Уровень квантования        

class CDemodulatorDlg;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRecordBuffer  
{
public:
	CRecordBuffer(CDemodulatorDlg* mpDlg);
	virtual ~CRecordBuffer();

	BOOL GetDirectSoundInfo(WAVEFORMATEX &Waveformat);
	BOOL InitDirectSound();
	BOOL StartRec();


	HRESULT HandleNotification();

	static UINT RecordProcessStream(LPVOID pVoid);
	
protected:

	bool ControlRecordThread;

	LPDIRECTSOUNDCAPTURE		pDSObject;			// - Указ на объект DirectSound
	LPDIRECTSOUNDCAPTUREBUFFER	pDSBuffer;			// - Указ на буфер 
	LPDIRECTSOUNDNOTIFY			pDSNotify;			// - Указ на объект реализующий события   
	WAVEFORMATEX*				pWfex;				// - Указ структуру описывающую <тип звука>

	DSCBUFFERDESC				DSBufferDesc;						  // Структура использующаяся в CreateCaptureBuffer методе
	DSBPOSITIONNOTIFY			DSPositionNotify[REC_BUFFER_NUMBER + 1];  // +1 для события останов буффер 
	HANDLE						hNotificationEvents[REC_BUFFER_NUMBER]; 

	DWORD						CaptureBufferSize;          // Размер буффера 
	DWORD						CaptureBufferOffset;	    // Место от куда писать (в буфере)
	DWORD						NotifySize;					// Модуль счета, по которому формируется событие

	CDemodulatorDlg*			pDemDlg;

};

#endif // !defined(AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_)
