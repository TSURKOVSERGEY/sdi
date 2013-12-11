// DigitalProcessing.cpp: implementation of the CDigitalProcessing class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Demodulator.h"
#include "DigitalProcessing.h"
#include "DemodulatorDlg.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDigitalProcessing::CDigitalProcessing(CDemodulatorDlg* mpDlg)
{	pDemDlg = mpDlg;
   
 
		if(!file.Open(pDemDlg->fname,CFile::modeRead))
		{
			fopen_status = 0;
		}
		else 
		{
			fsize = file.GetLength();
			fopen_status = 1;
		}

}

CDigitalProcessing::~CDigitalProcessing()
{
}

void CDigitalProcessing::InputStream(BYTE *pBuffer, UINT BufferSize)
{
  for(unsigned int i = 0; i < BufferSize; i+=2)
  {
	  TimeDataMass[i] = *(pBuffer + i);
  }

	Dpf(BufferSize);
	pDemDlg->pGraph1->PackBuffer1((BYTE*)pBuffer,BufferSize,1); // Отрисовка во временной обдасти
//  pDemDlg->pWave->FileWrite((BYTE*)pBuffer,BufferSize); 

}


#define ADPCM_BLOCK_SIZE 4036

#define WIDTH 1

unsigned short CDigitalProcessing::Filter(signed long data)
{
	static short f[WIDTH];

	 short result = 2048;

	f[0] = (short)data;
	
	for(int i = (WIDTH - 1); i >= 1; i--) 
	{
		f[i] = f[i-1];
	}



	for(int j = (WIDTH -1); j >= 0; j--) 
	{
		result +=f[j];

	}

	return result = result / WIDTH;

//	return (unsigned short) data;

}

void CDigitalProcessing::OutputStream(unsigned short *pBuffer, UINT BufferSize)
{
	static int index = 0;

	unsigned char sample;

  if(fsize <= 0) 
  {
	 pDemDlg->pPlayBuffer->pDSBuffer->Stop();
	 pDemDlg->EndDialog(0);
	return;
  }


  for(unsigned int i = 0; i < (BufferSize / 2); i+=2)
  {

	  if(index++ == 0) file.Read(&state,4);
	  else if(index == ADPCM_BLOCK_SIZE-4) index = 0;

	  file.Read(&sample,1);
      fsize--;

	  *(pBuffer + i)     = Filter(ADPCMDecoder(sample & 0xf));
	  *(pBuffer + i+1)   = Filter(ADPCMDecoder(sample >> 4));

	  TimeDataMass[i]   = (unsigned char)(*(pBuffer + i) / 256) -128;
	  TimeDataMass[i+1] = (unsigned char)(*(pBuffer + i+1) / 256) -128;

  }


  pDemDlg->pGraph1->PackBuffer1(TimeDataMass,BufferSize,2); // Отрисовка во временной обдасти

  Dpf(BufferSize);

}


////////////////////////////////////////////////////////////////////////////////  
// ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР * ДЕКОДЕР
////////////////////////////////////////////////////////////////////////////////  
signed long CDigitalProcessing::ADPCMDecoder(char code)
{
	static const int IndexTable[16] = 
	{
		-1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8
	};


	static const long StepSizeTable[89] = 
	{
	 	 7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
        19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
        50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
       130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
       337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
       876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
      2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
      5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
     15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};


  long step;                      
  signed long predsample;         
  signed long diffq;           
  int index;

  predsample = state.prevsample;
  index = state.previndex;

  step = StepSizeTable[index];

// ОБРАТНЫЙ КВАНТОВАТЕЛЬ /////////////////////////////////////////////////////// 
  
  diffq = step >> 3;
  
  if(code & 4) diffq += step;
  
  if(code & 2) diffq += step >> 1;

  if(code & 1) diffq += step >> 2;
  
// ПРЕДСКАЗСТЕЛЬ ///////////////////////////////////////////////////////////////
   
  if(code & 8) predsample -= diffq;
  else         predsample += diffq;

  if(predsample > 32767)       predsample = 32767;
  else if(predsample < -32768) predsample = -32768;
  
  index += IndexTable[code];

  if(index < 0)  index = 0;
  
  if(index > 88) index = 88;

  state.prevsample = (short) predsample;
  state.previndex = index;

  return (predsample);
}

//////////////////////////////////////////////////////////////////////
// Дискретное преобразование ФУРЬЕ (в комплексной форме)
//////////////////////////////////////////////////////////////////////
void CDigitalProcessing::Dpf(UINT BufferSize)
{


	double  Re	   = 0x0;
	double  Im	   = 0x0;
	double  Z      = 0x0;

	
	int    N	   = 600; 
	int    Mashtab = 20;


	for(int i = 0 ; i < N/2; i++) 
	{
		Re = 0;
		Im = 0;

		for(int j = 0 ; j < N; j++)
		{
		  Re += TimeDataMass[j] * cos(2*pi*i*j/N); 
		  Im -= TimeDataMass[j] * sin(2*pi*i*j/N); 
		}

		Re = Re / N;
		Im = Im / N;

		Z = (sqrt(Re*Re + Im*Im)) * Mashtab;

        FrequencyDataMass[i] = (unsigned char)Z;
	}

	pDemDlg->pGraph2->PackBuffer2(FrequencyDataMass,N/2,2); // Отрисовка в частотной области
    
}
