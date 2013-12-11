// DemodulatorDlg.h : header file
//

#if !defined(AFX_DEMODULATORDLG_H__C225668F_64D3_42C6_B273_C7B97F594443__INCLUDED_)
#define AFX_DEMODULATORDLG_H__C225668F_64D3_42C6_B273_C7B97F594443__INCLUDED_

#include "RecordBuffer.h"			// Added by Serge
#include "PlayBuffer.h"				// Added by Serge
#include "Graph.h"					// Added by Serge
#include "Wave.h"					// Added by Serge
#include "DigitalProcessing.h"		// Added by Serge

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorDlg dialog

class CDemodulatorDlg : public CDialog
{
// Construction
public:
	void close(void);
	CDemodulatorDlg(CWnd* pParent = NULL);	// standard constructor

	CGraph				*pGraph1;
	CGraph				*pGraph2;
	CWave				*pWave;
	CRecordBuffer		*pRecordBuffer;
	CPlayBuffer			*pPlayBuffer;
	CDigitalProcessing  *pDigitalProcessing;

		
	char fname[100];

// Dialog Data
	//{{AFX_DATA(CDemodulatorDlg)
	enum { IDD = IDD_DEMODULATOR_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDemodulatorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDC dc;

	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDemodulatorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEMODULATORDLG_H__C225668F_64D3_42C6_B273_C7B97F594443__INCLUDED_)
