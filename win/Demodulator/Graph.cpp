// Graph.cpp: implementation of the CGraph class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Demodulator.h"
#include "DemodulatorDlg.h"
#include "Graph.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CGraph::CGraph(CDC *hdc,int x1,int y1,int x2,int y2,int R,int G,int B,int W,int mX,int mY)
{
   posX1 = x1;
   posY1 = y1;
   posX2 = x2;
   posY2 = y2;

   OldPoint.x = 0;
   OldPoint.y = 125;

   dc = hdc;

//////////////////////////////////////////////////////////////////////////////////////////////////
//****************************** Создание виртуальных окон ***************************************

  memDC_static.CreateCompatibleDC(hdc);                  
  memBmp_static.CreateCompatibleBitmap(hdc,posX2 - posX1,posY2 - posY1);
  memDC_static.SelectObject(&memBmp_static);
  memDC_static.PatBlt(0,0,posX2 - posX1,posY2 - posY1,BLACKNESS);       

  memDC_dynamic.CreateCompatibleDC(hdc);                  
  memBmp_dynamic.CreateCompatibleBitmap(hdc,posX2 - posX1,posY2 - posY1);
  memDC_dynamic.SelectObject(&memBmp_dynamic);
  memDC_dynamic.PatBlt(0,0,posX2 - posX1,posY2 - posY1,BLACKNESS);

//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************

  CPen	TempPen;

  TempPen.CreatePen(PS_SOLID,1,RGB(80,80,50)); 

  int StepX = (x2 - x1) / mX;

  for(int i = 0; i <= posX2; i+=StepX)
  {
	memDC_dynamic.SelectObject(&TempPen);
	memDC_dynamic.MoveTo(i,0);
	memDC_dynamic.LineTo(i,y2);
  }

  int StepY = (y2 - y1) / mY;

  for(int j = 0; j <= posX2; j+=StepY)
  {
	memDC_dynamic.SelectObject(&TempPen);
	memDC_dynamic.MoveTo(0,j);
	memDC_dynamic.LineTo(x2,j);
  }

  memDC_dynamic.SelectStockObject(NULL_PEN); 
  TempPen.DeleteObject();

//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************* Создание Pen *********************************************

  Pen.CreatePen(PS_SOLID,W,RGB(R,G,B)); 
  memDC_static.SelectObject(&Pen);
}

CGraph::~CGraph()
{
   memDC_static.SelectStockObject(NULL_PEN); 
   Pen.DeleteObject();
}


void CGraph::DrawVirtualScreen()
{
	dc->SetBkMode(TRANSPARENT);
    dc->BitBlt(posX1,posY1,posX2,posY2,&memDC_static,0,0,SRCCOPY);               // Отрисовка на дисплей
}

///////////////////////////////////////////////////////////////////////////////////////////
//							!!! ОТРИСОВКА ВРЕМЕНИ !!!
///////////////////////////////////////////////////////////////////////////////////////////

void CGraph::PackBuffer1(BYTE *pBuffer,int BufferSize,int Mashtab)
{
  BYTE SampleData; 
  OldPoint.x = 0;

  memDC_static.BitBlt(0,0,posX2 - posX1,posY2 - posY1,&memDC_dynamic,0,0,SRCCOPY);

  for(int i = 0; i < BufferSize; i++)
  {

	SampleData = *(pBuffer + i);
		
	memDC_static.MoveTo(OldPoint.x, OldPoint.y);
	memDC_static.LineTo(i*Mashtab,  SampleData);
		
	OldPoint.x = i*Mashtab;
	OldPoint.y = SampleData;

	}
  if(PhotoParam)
	  DrawVirtualScreen();        // Отрисовка на экран

}

///////////////////////////////////////////////////////////////////////////////////////////
//							!!! ОТРИСОВКА СПЕКТРА !!!
///////////////////////////////////////////////////////////////////////////////////////////

void CGraph::PackBuffer2(BYTE *pBuffer,int BufferSize,int Mashtab)
{
//  WORD SampleData; 
  
  memDC_static.BitBlt(0,0,posX2 - posX1,posY2 - posY1,&memDC_dynamic,0,0,SRCCOPY);


  for(int i = 0; i < BufferSize; i++) 
  {
	memDC_static.MoveTo(BufferSize+(i*Mashtab),255 - *(pBuffer++));
	memDC_static.LineTo(BufferSize+(i*Mashtab),255);

	memDC_static.MoveTo(BufferSize-(i*Mashtab),255 - *(pBuffer++));
	memDC_static.LineTo(BufferSize-(i*Mashtab),255);
  }



  if(PhotoParam) DrawVirtualScreen();        // Отрисовка на экран

}

