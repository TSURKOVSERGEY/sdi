// Demodulator.h : main header file for the DEMODULATOR application
//

#if !defined(AFX_DEMODULATOR_H__9944CD94_188C_4D14_8DA0_E5F3CCE8EE5E__INCLUDED_)
#define AFX_DEMODULATOR_H__9944CD94_188C_4D14_8DA0_E5F3CCE8EE5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorApp:
// See Demodulator.cpp for the implementation of this class
//

class CDemodulatorApp : public CWinApp
{
public:
	CDemodulatorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDemodulatorApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDemodulatorApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEMODULATOR_H__9944CD94_188C_4D14_8DA0_E5F3CCE8EE5E__INCLUDED_)
