// Graph.h: interface for the CGraph class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPH_H__F545EC90_EDEA_49C4_83DA_4D44089B0055__INCLUDED_)
#define AFX_GRAPH_H__F545EC90_EDEA_49C4_83DA_4D44089B0055__INCLUDED_

class CDemodulatorDlg;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGraph  
{
public:
	
	CGraph(CDC *hdc,int x1,int y1,int x2,int y2,int R,int G,int B,int W,int mX,int mY);
	virtual ~CGraph();

	void PackBuffer1(BYTE* pBuffer,int BufferSize,int Mashtab);
	void PackBuffer2(BYTE *pBuffer,int BufferSize,int Mashtab);
	void PackBuffer2(BYTE *pBuffer,int BufferSize);

	void DrawVirtualScreen();
	
protected:

	bool PhotoParam;

	int posX1,posY1,posX2,posY2;

	CPen				Pen;
	CPoint				OldPoint;
	CDC					*dc;
	CBitmap				memBmp_dynamic;
	CBitmap				memBmp_static;
	CDC					memDC_dynamic;
	CDC					memDC_static;

};

#endif // !defined(AFX_GRAPH_H__F545EC90_EDEA_49C4_83DA_4D44089B0055__INCLUDED_)
