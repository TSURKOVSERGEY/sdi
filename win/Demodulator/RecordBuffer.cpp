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
//	Останов потока (задержка и контроль безопасного выхода)

	pDSBuffer->Stop();

	ControlRecordThread = false;					  

    for(int TimeOut = 0xfffff; TimeOut > 0; TimeOut--) // Подождать остановки потока
	{
		if(ControlRecordThread)
			break;
		else 
			Sleep(0);
	}

///////////////////////////////////////////////////////////////////////////////////////
// Уничтожение интерфейсов DirectSound освобождение ресурсов

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
BOOL CRecordBuffer::InitDirectSound()
{

	pDSObject				= NULL; // - Указ на объект DirectSound
	pDSBuffer				= NULL; // - Указ на буфер
	pDSNotify				= NULL; // - Указ на объект реализующий события
	pWfex					= NULL; // - Указ структуру описывающую <тип звука>
	
	NotifySize				= REC_BUFFER_SIZE;	// Модуль счета, по которому формируется событие
	CaptureBufferSize		= 0;				// Размер Capture буффера
	CaptureBufferOffset		= 0;				// Место от куда писать (в буфере)  !!!!

///////////////////////////////////////////////////////////////////////////////////////
//	Coздаем и инеициализируем обект DirectSound <DirectSoundCaptureCreate>

	if(FAILED(DirectSoundCaptureCreate(NULL,&pDSObject,NULL)))
		return FALSE;

///////////////////////////////////////////////////////////////////////////////////////
//	Coздаем и инеициализируем обект CaptureBuffer <CreateCaptureBuffer>

	pWfex = new WAVEFORMATEX;

	pWfex->cbSize		   = 0; 
	pWfex->nChannels	   = REC_CHANAL_NUMBER;
	pWfex->nSamplesPerSec  = REC_SAMPLING_RATE;
	pWfex->wBitsPerSample  = REC_BITSPERSAMPLE;
	pWfex->wFormatTag	   = WAVE_FORMAT_PCM;
	pWfex->nBlockAlign	   = pWfex->wBitsPerSample / 8 * pWfex->nChannels;
	pWfex->nAvgBytesPerSec = pWfex->nSamplesPerSec * pWfex->nBlockAlign;   

	NotifySize -= NotifySize % pWfex->nBlockAlign;       // Выравнивание счетчика событий
	CaptureBufferSize = NotifySize * REC_BUFFER_NUMBER;	 // Выравнивание буфера

	ZeroMemory(&DSBufferDesc, sizeof(DSBufferDesc));
	DSBufferDesc.dwSize		   = sizeof(DSBufferDesc);	 // Размер самой структуры в байтах
	DSBufferDesc.dwBufferBytes = CaptureBufferSize;		 // Размер CaptureBuffer в байтах
	DSBufferDesc.lpwfxFormat   = pWfex;					 // Указ. WAVEFORMATEX структуру 

	if(FAILED(pDSObject->CreateCaptureBuffer(&DSBufferDesc,&pDSBuffer,NULL)))
		return false;

///////////////////////////////////////////////////////////////////////////////////////
// Создаем объекты событий которые будут использоваться

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
// Создаем поток ловли событий

	ControlRecordThread = true;					  

	AfxBeginThread(RecordProcessStream,this,THREAD_PRIORITY_TIME_CRITICAL); // Запуск потока 

    return TRUE;  
}



//////////////////////////////////////////////////////////////////////
// ProcessStream < Поток отслеживает события > 
//////////////////////////////////////////////////////////////////////
UINT CRecordBuffer::RecordProcessStream(LPVOID pVoid)
{
	CRecordBuffer* pthis = reinterpret_cast<CRecordBuffer*>(pVoid);

	HANDLE*	hNotificationEvents = pthis->hNotificationEvents; // Указ массив описат ОБЪЕКТА событий
	
	DWORD   dwResult; // <- Тут возвращается значение ПРЕРЫВАНИЯ либо TIME-OUT 

///////////////////////////////////////////////////////////////////////////////////
	while(pthis->ControlRecordThread)  // Проверка выполнения потока
	{ 
		dwResult = MsgWaitForMultipleObjects(2,hNotificationEvents,FALSE,INFINITE,QS_ALLEVENTS);

		if(dwResult == WAIT_OBJECT_0)
		{
			if(FAILED(pthis->HandleNotification()))
			{	
				pthis->ControlRecordThread  = false;

				MessageBox( NULL, "Ошибка при работе с объектами извещений DirectSound. "
					"Приложение завершило свою работу.", "Использование DirectSound", MB_OK | MB_ICONERROR );
			}
		}
	}
///////////////////////////////////////////////////////////////////////////////////

	pthis->ControlRecordThread = true;  // Подтвердить выход из потока
	
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// Обработка событий !!!!!! ПРЕРЫВАНИЯ ОТ DMA БУФЕРА !!!!!! 
////////////////////////////////////////////////////////////////////////////
HRESULT CRecordBuffer::HandleNotification()
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
		return S_FALSE;  // Если CaptureBuffer не создан выйти

// Определить позицию в буффере
	if(FAILED(hresult = pDSBuffer->GetCurrentPosition(&CurrentBufferPos,&GarantReadPos)))
		return hresult;

	DataSize = GarantReadPos - CaptureBufferOffset;
	
	if(DataSize < 0)
		DataSize += CaptureBufferSize;

// Выравнивание границы блока
	DataSize -= (DataSize % NotifySize);

	if(DataSize == 0) 
		return S_FALSE;

// Блокировка буфера записи !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if(FAILED(pDSBuffer->Lock(CaptureBufferOffset,DataSize, 
							  &pbCaptureData, &dwCaptureLength, 
							  &pbCaptureData2, &dwCaptureLength2,0L)))
		return S_OK;

// Вызов обработчиков /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

	if(pDemDlg->pDigitalProcessing)
		pDemDlg->pDigitalProcessing->InputStream((BYTE*)pbCaptureData,dwCaptureLength);

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// Разблокирование буфера записи
	pDSBuffer->Unlock(pbCaptureData,dwCaptureLength,NULL,0);

// Перемещение начальной позиции записи
	CaptureBufferOffset += dwCaptureLength; 
	CaptureBufferOffset %= CaptureBufferSize; // Циклический буфер

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
// Запускаем работу буфера
////////////////////////////////////////////////////////////////////////////
BOOL CRecordBuffer::StartRec()
{
	if(FAILED(pDSBuffer->Start(DSCBSTART_LOOPING))) // Запуск записи
	  return false;
	else
	  return true;
}
