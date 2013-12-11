// PlayBuffer.cpp: implementation of the CPlayBuffer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Demodulator.h"
#include "PlayBuffer.h"
#include "DemodulatorDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////////////////
CPlayBuffer::CPlayBuffer(CDemodulatorDlg* mpDlg)
{	pDemDlg = mpDlg;
}

//////////////////////////////////////////////////////////////////////
// Destruction
//////////////////////////////////////////////////////////////////////
CPlayBuffer::~CPlayBuffer()
{

///////////////////////////////////////////////////////////////////////////////////////
//	������� ������ (�������� � �������� ����������� ������)

	ControlPlayThread = false;	
	
    pDSBuffer->Stop();


    for(int TimeOut = 0xfffff; TimeOut > 0; TimeOut--) // ��������� ��������� ������
	{
		if(ControlPlayThread)
			break;
		else 
			Sleep(0);
	}

///////////////////////////////////////////////////////////////////////////////////////
// ����������� ����������� DirectSound ������������ ��������

	pDSBuffer->Stop();


	if(pWfex)
		delete	pWfex;

	if(pDSNotify)
		pDSNotify->Release();

	if(pDSBuffer)
		pDSBuffer->Release();

	if(pDSObject)
		pDSObject->Release();

}


/////////////////////////////////////////////////////////////////////////////////////
//	    					������������� DIRECT_SOUND 
/////////////////////////////////////////////////////////////////////////////////////
BOOL CPlayBuffer::InitDirectSound(HWND hWin)
{
	pDSObject				= NULL; // - ���� �� ������ DirectSound
	pDSBuffer				= NULL; // - ���� �� �����
	pDSNotify				= NULL; // - ���� �� ������ ����������� �������
	pWfex					= NULL; // - ���� ��������� ����������� <��� �����>
	
	LPDIRECTSOUNDBUFFER		pDSBPrimary = NULL;
	
	NotifySize				= PLAY_BUFFER_SIZE;	// ������ �����, �� �������� ����������� �������
	SoundBufferSize	     	= 0;				// ������ Capture �������
	SoundBufferOffset		= 0;				// ����� �� ���� ������ (� ������)  !!!!

///////////////////////////////////////////////////////////////////////////////////////
//	Co����� � ��������������� ����� DirectSound <DirectSoundCreate>

   if(FAILED(DirectSoundCreate(NULL,&pDSObject,NULL)))
		return FALSE;

///////////////////////////////////////////////////////////////////////////////////////
//	Co����� � ��������������� ����� SoundBuffer <CreateSoundBuffer>

   	pWfex = new WAVEFORMATEX;

	pWfex->cbSize		   = 0; 
	pWfex->nChannels	   = PLAY_CHANAL_NUMBER;
	pWfex->nSamplesPerSec  = PLAY_SAMPLING_RATE;
	pWfex->wBitsPerSample  = PLAY_BITSPERSAMPLE;
	pWfex->wFormatTag	   = WAVE_FORMAT_PCM;
    pWfex->nBlockAlign	   = pWfex->wBitsPerSample / 8 * pWfex->nChannels;
    pWfex->nAvgBytesPerSec = pWfex->nSamplesPerSec * pWfex->nBlockAlign;   

    NotifySize -= NotifySize % pWfex->nBlockAlign;     // ������������ �������� �������
    SoundBufferSize = NotifySize * PLAY_BUFFER_NUMBER; // ������������ ������

    ZeroMemory(&DSBufferDesc,	 sizeof(DSBUFFERDESC));
	DSBufferDesc.dwSize		   = sizeof(DSBUFFERDESC);	 // ������ ����� ��������� � ������
	DSBufferDesc.dwBufferBytes = SoundBufferSize;		 // ������ CaptureBuffer � ������
	DSBufferDesc.dwFlags       = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
    DSBufferDesc.lpwfxFormat   = pWfex;

	if(FAILED(pDSObject->CreateSoundBuffer(&DSBufferDesc,&pDSBuffer,NULL)))
		return FALSE;
	
	if(FAILED(pDSObject->SetCooperativeLevel(hWin, DSSCL_PRIORITY)))
		return FALSE;

//////////////////////////////////////////////////////////////////////////////////////
// ������� ������� ������� ������� ����� ��������������

	hNotificationEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hNotificationEvents[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	if(FAILED(pDSBuffer->QueryInterface(IID_IDirectSoundNotify,(VOID**)&pDSNotify)))
		return FALSE;

	for(int i = 0; i < PLAY_BUFFER_NUMBER; i++)
	{
		DSPositionNotify[i].dwOffset = (NotifySize * i) + NotifySize - 1;
		DSPositionNotify[i].hEventNotify = hNotificationEvents[0];			 
	}

	DSPositionNotify[i].dwOffset		= DSBPN_OFFSETSTOP;
	DSPositionNotify[i].hEventNotify	= hNotificationEvents[1];

	if((pDSNotify->SetNotificationPositions(PLAY_BUFFER_NUMBER ,DSPositionNotify)))
		return FALSE;

///////////////////////////////////////////////////////////////////////////////////////
// ������� ����� ����� �������

	ControlPlayThread = true;					  

	AfxBeginThread(PlayProcessStream,this,THREAD_PRIORITY_NORMAL); // ������ ������ 

	return TRUE;

}


//////////////////////////////////////////////////////////////////////
// ProcessStream < ����� ����������� ������� > 
//////////////////////////////////////////////////////////////////////
UINT CPlayBuffer::PlayProcessStream(LPVOID pVoid)
{
	CPlayBuffer* pthis = reinterpret_cast<CPlayBuffer*>(pVoid);

	HANDLE*	hNotificationEvents = pthis->hNotificationEvents; // ���� ������ ������ ������� �������
	
	DWORD   dwResult; // <- ��� ������������ �������� ���������� ���� TIME-OUT 

///////////////////////////////////////////////////////////////////////////////////

	while(pthis->ControlPlayThread)  // �������� ���������� ������
	{ 
		dwResult = MsgWaitForMultipleObjects(2,hNotificationEvents,FALSE,INFINITE,QS_ALLEVENTS);

		if(dwResult == WAIT_OBJECT_0)
		{
			if(FAILED(pthis->HandleNotification()))
			{	
				pthis->ControlPlayThread  = false;

				MessageBox( NULL, "������ ��� ������ � ��������� ��������� DirectSound. "
					"���������� ��������� ���� ������.", "������������� DirectSound", MB_OK | MB_ICONERROR );
			}
		}
	}

///////////////////////////////////////////////////////////////////////////////////

	pthis->ControlPlayThread = true;  // ����������� ����� �� ������
	
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// ��������� ������� !!!!!! ���������� �� DMA ������ !!!!!! 
////////////////////////////////////////////////////////////////////////////
HRESULT CPlayBuffer::HandleNotification()
{
	HRESULT		hresult;
	DWORD		CurrentBufferPos;	//  �������  �������  � ������
	DWORD		GarantReadPos;		//  ������� ����������� ������
	LONG		DataSize;			//  ����� ������ ��������� ��� ������

	VOID*   pbCaptureData	 = NULL;
	DWORD   dwCaptureLength;
	VOID*   pbCaptureData2   = NULL;
	DWORD   dwCaptureLength2;

	if(!pDSBuffer)	
		return S_FALSE;  // ���� Buffer �� ������ �����

// ���������� ������� � �������
	if(FAILED(hresult = pDSBuffer->GetCurrentPosition(&CurrentBufferPos,&GarantReadPos)))
		return hresult;

	DataSize = GarantReadPos - SoundBufferOffset;
	
	if(DataSize < 0)
		DataSize += SoundBufferSize;

// ������������ ������� �����
	DataSize -= (DataSize % NotifySize);

	if(DataSize == 0) 
		return S_FALSE;

// ���������� ������ ������ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if(FAILED(pDSBuffer->Lock(SoundBufferOffset,DataSize, 
							  &pbCaptureData, &dwCaptureLength, 
							  &pbCaptureData2, &dwCaptureLength2,0L)))
		return S_OK;

// ����� ������������ /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

	if(pDemDlg->pDigitalProcessing)
			pDemDlg->pDigitalProcessing->OutputStream((unsigned short*)pbCaptureData,dwCaptureLength);

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// ��������������� ������ ������
	pDSBuffer->Unlock(pbCaptureData,dwCaptureLength,NULL,0);

// ����������� ��������� ������� ������
	SoundBufferOffset += dwCaptureLength; 
	SoundBufferOffset %= SoundBufferSize; // ����������� �����

	return S_OK;

}

///////////////////////////////////////////////////////////////////////////////////////
// ��������� ������ ������
///////////////////////////////////////////////////////////////////////////////////////
BOOL CPlayBuffer::StartPlay()
{
	if(FAILED(pDSBuffer->Play(0,0,DSBPLAY_LOOPING))) // ������ ������
	  return FALSE;
	else
	  return TRUE;

}
