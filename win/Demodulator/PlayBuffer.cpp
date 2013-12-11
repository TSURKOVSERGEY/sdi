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
//	Останов потока (задержка и контроль безопасного выхода)

	ControlPlayThread = false;	
	
    pDSBuffer->Stop();


    for(int TimeOut = 0xfffff; TimeOut > 0; TimeOut--) // Подождать остановки потока
	{
		if(ControlPlayThread)
			break;
		else 
			Sleep(0);
	}

///////////////////////////////////////////////////////////////////////////////////////
// Уничтожение интерфейсов DirectSound освобождение ресурсов

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
//	    					ИНИЦИАЛИЗАЦИЯ DIRECT_SOUND 
/////////////////////////////////////////////////////////////////////////////////////
BOOL CPlayBuffer::InitDirectSound(HWND hWin)
{
	pDSObject				= NULL; // - Указ на объект DirectSound
	pDSBuffer				= NULL; // - Указ на буфер
	pDSNotify				= NULL; // - Указ на объект реализующий события
	pWfex					= NULL; // - Указ структуру описывающую <тип звука>
	
	LPDIRECTSOUNDBUFFER		pDSBPrimary = NULL;
	
	NotifySize				= PLAY_BUFFER_SIZE;	// Модуль счета, по которому формируется событие
	SoundBufferSize	     	= 0;				// Размер Capture буффера
	SoundBufferOffset		= 0;				// Место от куда писать (в буфере)  !!!!

///////////////////////////////////////////////////////////////////////////////////////
//	Coздаем и инеициализируем обект DirectSound <DirectSoundCreate>

   if(FAILED(DirectSoundCreate(NULL,&pDSObject,NULL)))
		return FALSE;

///////////////////////////////////////////////////////////////////////////////////////
//	Coздаем и инеициализируем обект SoundBuffer <CreateSoundBuffer>

   	pWfex = new WAVEFORMATEX;

	pWfex->cbSize		   = 0; 
	pWfex->nChannels	   = PLAY_CHANAL_NUMBER;
	pWfex->nSamplesPerSec  = PLAY_SAMPLING_RATE;
	pWfex->wBitsPerSample  = PLAY_BITSPERSAMPLE;
	pWfex->wFormatTag	   = WAVE_FORMAT_PCM;
    pWfex->nBlockAlign	   = pWfex->wBitsPerSample / 8 * pWfex->nChannels;
    pWfex->nAvgBytesPerSec = pWfex->nSamplesPerSec * pWfex->nBlockAlign;   

    NotifySize -= NotifySize % pWfex->nBlockAlign;     // Выравнивание счетчика событий
    SoundBufferSize = NotifySize * PLAY_BUFFER_NUMBER; // Выравнивание буфера

    ZeroMemory(&DSBufferDesc,	 sizeof(DSBUFFERDESC));
	DSBufferDesc.dwSize		   = sizeof(DSBUFFERDESC);	 // Размер самой структуры в байтах
	DSBufferDesc.dwBufferBytes = SoundBufferSize;		 // Размер CaptureBuffer в байтах
	DSBufferDesc.dwFlags       = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
    DSBufferDesc.lpwfxFormat   = pWfex;

	if(FAILED(pDSObject->CreateSoundBuffer(&DSBufferDesc,&pDSBuffer,NULL)))
		return FALSE;
	
	if(FAILED(pDSObject->SetCooperativeLevel(hWin, DSSCL_PRIORITY)))
		return FALSE;

//////////////////////////////////////////////////////////////////////////////////////
// Создаем объекты событий которые будут использоваться

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
// Создаем поток ловли событий

	ControlPlayThread = true;					  

	AfxBeginThread(PlayProcessStream,this,THREAD_PRIORITY_NORMAL); // Запуск потока 

	return TRUE;

}


//////////////////////////////////////////////////////////////////////
// ProcessStream < Поток отслеживает события > 
//////////////////////////////////////////////////////////////////////
UINT CPlayBuffer::PlayProcessStream(LPVOID pVoid)
{
	CPlayBuffer* pthis = reinterpret_cast<CPlayBuffer*>(pVoid);

	HANDLE*	hNotificationEvents = pthis->hNotificationEvents; // Указ массив описат ОБЪЕКТА событий
	
	DWORD   dwResult; // <- Тут возвращается значение ПРЕРЫВАНИЯ либо TIME-OUT 

///////////////////////////////////////////////////////////////////////////////////

	while(pthis->ControlPlayThread)  // Проверка выполнения потока
	{ 
		dwResult = MsgWaitForMultipleObjects(2,hNotificationEvents,FALSE,INFINITE,QS_ALLEVENTS);

		if(dwResult == WAIT_OBJECT_0)
		{
			if(FAILED(pthis->HandleNotification()))
			{	
				pthis->ControlPlayThread  = false;

				MessageBox( NULL, "Ошибка при работе с объектами извещений DirectSound. "
					"Приложение завершило свою работу.", "Использование DirectSound", MB_OK | MB_ICONERROR );
			}
		}
	}

///////////////////////////////////////////////////////////////////////////////////

	pthis->ControlPlayThread = true;  // Подтвердить выход из потока
	
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// Обработка событий !!!!!! ПРЕРЫВАНИЯ ОТ DMA БУФЕРА !!!!!! 
////////////////////////////////////////////////////////////////////////////
HRESULT CPlayBuffer::HandleNotification()
{
	HRESULT		hresult;
	DWORD		CurrentBufferPos;	//  Текущая  позиция  в буфере
	DWORD		GarantReadPos;		//  Позиция безопасного чтения
	LONG		DataSize;			//  Объем данных доступных для чтения

	VOID*   pbCaptureData	 = NULL;
	DWORD   dwCaptureLength;
	VOID*   pbCaptureData2   = NULL;
	DWORD   dwCaptureLength2;

	if(!pDSBuffer)	
		return S_FALSE;  // Если Buffer не создан выйти

// Определить позицию в буффере
	if(FAILED(hresult = pDSBuffer->GetCurrentPosition(&CurrentBufferPos,&GarantReadPos)))
		return hresult;

	DataSize = GarantReadPos - SoundBufferOffset;
	
	if(DataSize < 0)
		DataSize += SoundBufferSize;

// Выравнивание границы блока
	DataSize -= (DataSize % NotifySize);

	if(DataSize == 0) 
		return S_FALSE;

// Блокировка буфера записи !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if(FAILED(pDSBuffer->Lock(SoundBufferOffset,DataSize, 
							  &pbCaptureData, &dwCaptureLength, 
							  &pbCaptureData2, &dwCaptureLength2,0L)))
		return S_OK;

// Вызов обработчиков /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

	if(pDemDlg->pDigitalProcessing)
			pDemDlg->pDigitalProcessing->OutputStream((unsigned short*)pbCaptureData,dwCaptureLength);

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// Разблокирование буфера записи
	pDSBuffer->Unlock(pbCaptureData,dwCaptureLength,NULL,0);

// Перемещение начальной позиции записи
	SoundBufferOffset += dwCaptureLength; 
	SoundBufferOffset %= SoundBufferSize; // Циклический буфер

	return S_OK;

}

///////////////////////////////////////////////////////////////////////////////////////
// Запускаем работу буфера
///////////////////////////////////////////////////////////////////////////////////////
BOOL CPlayBuffer::StartPlay()
{
	if(FAILED(pDSBuffer->Play(0,0,DSBPLAY_LOOPING))) // Запуск записи
	  return FALSE;
	else
	  return TRUE;

}
