// RecordBuffer.cpp: implementation of the CRecordBuffer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Demodulator.h"
#include "RecordBuffer.h"
#include "DemodulatorDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////////////////
CRecordBuffer::CRecordBuffer(CDemodulatorDlg* mpDlg)
{	pDemDlg = mpDlg;
}

//////////////////////////////////////////////////////////////////////
// Destruction
//////////////////////////////////////////////////////////////////////
CRecordBuffer::~CRecordBuffer()
{

///////////////////////////////////////////////////////////////////////////////////////
//	������� ������ (�������� � �������� ����������� ������)

	pDSBuffer->Stop();

	ControlRecordThread = false;					  

    for(int TimeOut = 0xfffff; TimeOut > 0; TimeOut--) // ��������� ��������� ������
	{
		if(ControlRecordThread)
			break;
		else 
			Sleep(0);
	}

///////////////////////////////////////////////////////////////////////////////////////
// ����������� ����������� DirectSound ������������ ��������

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
BOOL CRecordBuffer::InitDirectSound()
{

	pDSObject				= NULL; // - ���� �� ������ DirectSound
	pDSBuffer				= NULL; // - ���� �� �����
	pDSNotify				= NULL; // - ���� �� ������ ����������� �������
	pWfex					= NULL; // - ���� ��������� ����������� <��� �����>
	
	NotifySize				= REC_BUFFER_SIZE;	// ������ �����, �� �������� ����������� �������
	CaptureBufferSize		= 0;				// ������ Capture �������
	CaptureBufferOffset		= 0;				// ����� �� ���� ������ (� ������)  !!!!

///////////////////////////////////////////////////////////////////////////////////////
//	Co����� � ��������������� ����� DirectSound <DirectSoundCaptureCreate>

	if(FAILED(DirectSoundCaptureCreate(NULL,&pDSObject,NULL)))
		return FALSE;

///////////////////////////////////////////////////////////////////////////////////////
//	Co����� � ��������������� ����� CaptureBuffer <CreateCaptureBuffer>

	pWfex = new WAVEFORMATEX;

	pWfex->cbSize		   = 0; 
	pWfex->nChannels	   = REC_CHANAL_NUMBER;
	pWfex->nSamplesPerSec  = REC_SAMPLING_RATE;
	pWfex->wBitsPerSample  = REC_BITSPERSAMPLE;
	pWfex->wFormatTag	   = WAVE_FORMAT_PCM;
	pWfex->nBlockAlign	   = pWfex->wBitsPerSample / 8 * pWfex->nChannels;
	pWfex->nAvgBytesPerSec = pWfex->nSamplesPerSec * pWfex->nBlockAlign;   

	NotifySize -= NotifySize % pWfex->nBlockAlign;       // ������������ �������� �������
	CaptureBufferSize = NotifySize * REC_BUFFER_NUMBER;	 // ������������ ������

	ZeroMemory(&DSBufferDesc, sizeof(DSBufferDesc));
	DSBufferDesc.dwSize		   = sizeof(DSBufferDesc);	 // ������ ����� ��������� � ������
	DSBufferDesc.dwBufferBytes = CaptureBufferSize;		 // ������ CaptureBuffer � ������
	DSBufferDesc.lpwfxFormat   = pWfex;					 // ����. WAVEFORMATEX ��������� 

	if(FAILED(pDSObject->CreateCaptureBuffer(&DSBufferDesc,&pDSBuffer,NULL)))
		return false;

///////////////////////////////////////////////////////////////////////////////////////
// ������� ������� ������� ������� ����� ��������������

	hNotificationEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hNotificationEvents[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	if(FAILED(pDSBuffer->QueryInterface(IID_IDirectSoundNotify,(VOID**)&pDSNotify)))
		return false;

	for(int i = 0; i < REC_BUFFER_NUMBER; i++)
	{
		DSPositionNotify[i].dwOffset = (NotifySize * i) + NotifySize - 1;
		DSPositionNotify[i].hEventNotify = hNotificationEvents[0];			 
	}

	DSPositionNotify[i].dwOffset		= DSBPN_OFFSETSTOP;
	DSPositionNotify[i].hEventNotify	= hNotificationEvents[1];

	if((pDSNotify->SetNotificationPositions(REC_BUFFER_NUMBER + 1,DSPositionNotify)))
		return false;

///////////////////////////////////////////////////////////////////////////////////////
// ������� ����� ����� �������

	ControlRecordThread = true;					  

	AfxBeginThread(RecordProcessStream,this,THREAD_PRIORITY_TIME_CRITICAL); // ������ ������ 

    return TRUE;  
}



//////////////////////////////////////////////////////////////////////
// ProcessStream < ����� ����������� ������� > 
//////////////////////////////////////////////////////////////////////
UINT CRecordBuffer::RecordProcessStream(LPVOID pVoid)
{
	CRecordBuffer* pthis = reinterpret_cast<CRecordBuffer*>(pVoid);

	HANDLE*	hNotificationEvents = pthis->hNotificationEvents; // ���� ������ ������ ������� �������
	
	DWORD   dwResult; // <- ��� ������������ �������� ���������� ���� TIME-OUT 

///////////////////////////////////////////////////////////////////////////////////
	while(pthis->ControlRecordThread)  // �������� ���������� ������
	{ 
		dwResult = MsgWaitForMultipleObjects(2,hNotificationEvents,FALSE,INFINITE,QS_ALLEVENTS);

		if(dwResult == WAIT_OBJECT_0)
		{
			if(FAILED(pthis->HandleNotification()))
			{	
				pthis->ControlRecordThread  = false;

				MessageBox( NULL, "������ ��� ������ � ��������� ��������� DirectSound. "
					"���������� ��������� ���� ������.", "������������� DirectSound", MB_OK | MB_ICONERROR );
			}
		}
	}
///////////////////////////////////////////////////////////////////////////////////

	pthis->ControlRecordThread = true;  // ����������� ����� �� ������
	
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// ��������� ������� !!!!!! ���������� �� DMA ������ !!!!!! 
////////////////////////////////////////////////////////////////////////////
HRESULT CRecordBuffer::HandleNotification()
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
		return S_FALSE;  // ���� CaptureBuffer �� ������ �����

// ���������� ������� � �������
	if(FAILED(hresult = pDSBuffer->GetCurrentPosition(&CurrentBufferPos,&GarantReadPos)))
		return hresult;

	DataSize = GarantReadPos - CaptureBufferOffset;
	
	if(DataSize < 0)
		DataSize += CaptureBufferSize;

// ������������ ������� �����
	DataSize -= (DataSize % NotifySize);

	if(DataSize == 0) 
		return S_FALSE;

// ���������� ������ ������ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if(FAILED(pDSBuffer->Lock(CaptureBufferOffset,DataSize, 
							  &pbCaptureData, &dwCaptureLength, 
							  &pbCaptureData2, &dwCaptureLength2,0L)))
		return S_OK;

// ����� ������������ /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

	if(pDemDlg->pDigitalProcessing)
		pDemDlg->pDigitalProcessing->InputStream((BYTE*)pbCaptureData,dwCaptureLength);

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// ��������������� ������ ������
	pDSBuffer->Unlock(pbCaptureData,dwCaptureLength,NULL,0);

// ����������� ��������� ������� ������
	CaptureBufferOffset += dwCaptureLength; 
	CaptureBufferOffset %= CaptureBufferSize; // ����������� �����

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
BOOL CRecordBuffer::GetDirectSoundInfo(WAVEFORMATEX &Waveformat)
{
	if(pWfex == NULL)
		return false;

	Waveformat = *pWfex;
		
	return true;

}

////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
// ��������� ������ ������
////////////////////////////////////////////////////////////////////////////
BOOL CRecordBuffer::StartRec()
{
	if(FAILED(pDSBuffer->Start(DSCBSTART_LOOPING))) // ������ ������
	  return false;
	else
	  return true;
}
