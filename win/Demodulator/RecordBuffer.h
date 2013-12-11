// RecordBuffer.h: interface for the CRecordBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_)
#define AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_

#include <winsock2.h>   // ���������� ��� ������������� DirectSound
#include <mmsystem.h>   // ���������� ��� ������������� DirectSound
#include <mmreg.H>      // ���������� ��� ������������� DirectSound
#include <dsound.h>     // ���������� ��� ������������� DirectSound
#include <mmsystem.h>

#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

#define REC_BUFFER_NUMBER	2			// ���-�� DMA �������
#define REC_BUFFER_SIZE 	1200			// ������ ������ � ������
#define REC_SAMPLING_RATE 	48000		// ������� �������������
#define REC_CHANAL_NUMBER	1			// ���� / ������
#define REC_BITSPERSAMPLE   8           // ������� �����������        

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

	LPDIRECTSOUNDCAPTURE		pDSObject;			// - ���� �� ������ DirectSound
	LPDIRECTSOUNDCAPTUREBUFFER	pDSBuffer;			// - ���� �� ����� 
	LPDIRECTSOUNDNOTIFY			pDSNotify;			// - ���� �� ������ ����������� �������   
	WAVEFORMATEX*				pWfex;				// - ���� ��������� ����������� <��� �����>

	DSCBUFFERDESC				DSBufferDesc;						  // ��������� �������������� � CreateCaptureBuffer ������
	DSBPOSITIONNOTIFY			DSPositionNotify[REC_BUFFER_NUMBER + 1];  // +1 ��� ������� ������� ������ 
	HANDLE						hNotificationEvents[REC_BUFFER_NUMBER]; 

	DWORD						CaptureBufferSize;          // ������ ������� 
	DWORD						CaptureBufferOffset;	    // ����� �� ���� ������ (� ������)
	DWORD						NotifySize;					// ������ �����, �� �������� ����������� �������

	CDemodulatorDlg*			pDemDlg;

};

#endif // !defined(AFX_RECORDBUFFER_H__14F8474A_5EEC_47FF_A5C3_652861D5AE40__INCLUDED_)
