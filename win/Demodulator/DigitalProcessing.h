// DigitalProcessing.h: interface for the CDigitalProcessing class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIGITALPROCESSING_H__30D189AA_5FF5_47BA_9F78_A7FCE9003154__INCLUDED_)
#define AFX_DIGITALPROCESSING_H__30D189AA_5FF5_47BA_9F78_A7FCE9003154__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define pi	3.141

class CDemodulatorDlg;




#define MAX_SAMPLE 112        


class CDigitalProcessing  
{
public:
	
	BYTE TimeDataMass[1000];
	BYTE FrequencyDataMass[1000];

	CDigitalProcessing(CDemodulatorDlg* mpDlg);
	virtual ~CDigitalProcessing();

	void OutputStream(unsigned short *pBuffer, UINT BufferSize);
	void InputStream(BYTE *pBuffer, UINT BufferSize);
	signed long ADPCMDecoder(char code);
	unsigned short Filter(signed long data);

	CDemodulatorDlg*			pDemDlg;

    struct
	{
		short prevsample;
	    char  previndex;
        char  reserved;
  
	} state;

    CFile file;
	int  fsize; 
	int  fopen_status;


protected:
	void Dpf(UINT BufferSize);
};

#endif // !defined(AFX_DIGITALPROCESSING_H__30D189AA_5FF5_47BA_9F78_A7FCE9003154__INCLUDED_)
