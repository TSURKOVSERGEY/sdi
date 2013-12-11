// Demodulator.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Demodulator.h"
#include "DemodulatorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorApp

BEGIN_MESSAGE_MAP(CDemodulatorApp, CWinApp)
	//{{AFX_MSG_MAP(CDemodulatorApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorApp construction

CDemodulatorApp::CDemodulatorApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDemodulatorApp object

CDemodulatorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorApp initialization


BOOL CDemodulatorApp::InitInstance()
{
	AfxEnableControlContainer();


#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CDemodulatorDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
/*



BOOL CDemodulatorApp::InitInstance()
{
	AfxEnableControlContainer();

	chat 


   CCommandLineInfo cmdInfo;

   ParseCommandLine(cmdInfo);

   if (!ProcessShellCommand(cmdInfo))

      return FALSE;

   cmdInfo.ParseParam




	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CDemodulatorDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}


	
	DialogBoxParam()

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

InitInstance DialogBoxParam
*/