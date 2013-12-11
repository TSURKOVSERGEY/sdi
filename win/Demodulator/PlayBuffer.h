// PlayBuffer.h: interface for the CPlayBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_)
#define AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_

#include <winsock2.h>   // ���������� ��� ������������� DirectSound
#include <mmsystem.h>   // ���������� ��� ������������� DirectSound
#include <mmreg.H>      // ���������� ��� ������������� DirectSound
#include <dsound.h>     // ���������� ��� ������������� DirectSound
#include <mmsystem.h>

#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

#define PLAY_BUFFER_NUMBER	4			// ���-�� DMA �������
#define PLAY_BUFFER_SIZE 	800			// ������ ������ � ������
#define PLAY_SAMPLING_RATE 	8000		// ������� �������������
#define PLAY_CHANAL_NUMBER	1			// ���� / ������
#define PLAY_BITSPERSAMPLE  16          // ������� �����������        

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

	LPDIRECTSOUNDBUFFER		pDSBuffer;			// - ���� �� ������ ����������� �������  

protected:

	bool ControlPlayThread;

	LPDIRECTSOUND			pDSObject;			// - ���� �� ������ DirectSound
 
	LPDIRECTSOUNDNOTIFY		pDSNotify;			// - ���� �� ������ ����������� �������   
	WAVEFORMATEX*			pWfex;				// - ���� ��������� ����������� <��� �����>

    DSBUFFERDESC			DSBufferDesc;		// ��������� �������������� � CreateSoundBuffer ������

	DSBPOSITIONNOTIFY		DSPositionNotify[PLAY_BUFFER_NUMBER + 1];  // +1 ��� ������� ������� ������ 
	HANDLE					hNotificationEvents[PLAY_BUFFER_NUMBER]; 

	DWORD					SoundBufferSize;    // ������ ������� 
	DWORD					SoundBufferOffset;  // ����� �� ���� ������ (� ������)
	DWORD					NotifySize;			// ������ �����, �� �������� ����������� �������

	CDemodulatorDlg*		pDemDlg;

};

#endif // !defined(AFX_PLAYBUFFER_H__13545D32_DDAF_4C81_AD0A_F2F234F90BDD__INCLUDED_)
