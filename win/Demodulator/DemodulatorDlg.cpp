// DemodulatorDlg.cpp : implementation file
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
// CDemodulatorDlg dialog

CDemodulatorDlg::CDemodulatorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDemodulatorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDemodulatorDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemodulatorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDemodulatorDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDemodulatorDlg, CDialog)
	//{{AFX_MSG_MAP(CDemodulatorDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDemodulatorDlg message handlers

BOOL CDemodulatorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	CDC* pDC = GetDC();
	dc.Attach(pDC->GetSafeHdc());

 
	char **argv = NULL;
	int    argc, BuffSize;
    WCHAR  *wcCommandLine;
    LPWSTR *argw;
    wcCommandLine = GetCommandLineW(); 
    argw = CommandLineToArgvW( wcCommandLine, &argc);
	argv = (char **)GlobalAlloc( LPTR, argc + 1 );
    for(int i=0; i<argc; i++)
	{
		BuffSize = WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK, argw[i], -1, NULL, 0, NULL, NULL );
        argv[i] = (char *)GlobalAlloc( LPTR, BuffSize );  
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK, argw[i], BuffSize * sizeof( WCHAR ) ,argv[i], BuffSize, NULL, NULL );
        if(i > 0) sprintf(fname,argv[i],BuffSize);
	}
 

 	sprintf(fname,"adpcm_MyPlayer_channel_0.adm");


  	pGraph1				= NULL;
	pGraph2				= NULL;
	pWave				= NULL;
	pPlayBuffer			= NULL;
	pRecordBuffer		= NULL;
	pDigitalProcessing	= NULL;


//	pRecordBuffer		= new CRecordBuffer(this);
	pPlayBuffer			= new CPlayBuffer(this);
	pDigitalProcessing  = new CDigitalProcessing(this); 	
	pGraph1				= new CGraph(&dc,10,
										 10,
										 610,
										 266,
										 250, // <R>
										 250, // <G>
										 0,	  // <B>
										 1,	  // Толщина графика	
										 10,  // Сетка по X
										 5);  // Сетка по Y	

	pGraph2				= new CGraph(&dc,10,
										 276,
										 610,
										 531,
										 50,  // <R>
										 200, // <G>
										 50,  // <B>
										 2,	  // Толщина графика	
										 10,  // Сетка по X
										 5);  // Сетка по Y	





	
	
	
	pPlayBuffer->InitDirectSound(m_hWnd);





		
	return TRUE;  // return TRUE  unless you set the focus to a control
}

//  If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDemodulatorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{

	if(pDigitalProcessing->fopen_status)
	{
		if(pGraph1) pGraph1->DrawVirtualScreen();
		
		if(pGraph2) pGraph2->DrawVirtualScreen();

		pPlayBuffer->StartPlay();
	}
	else
	{   CDialog::OnPaint();

	    MessageBox("Ошибка открытия файла"); 

	  	EndDialog(0);
	}
	



	  CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDemodulatorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDemodulatorDlg::OnClose() 
{

	if(pRecordBuffer)
		delete pRecordBuffer; 

	if(pPlayBuffer)
		delete pPlayBuffer; 

	if(pGraph1)
		delete pGraph1; 

	if(pGraph2)
		delete pGraph2; 

	if(pWave)
		delete pWave; 

	if(pDigitalProcessing)
		delete pDigitalProcessing; 

	CDialog::OnClose();
}

void CDemodulatorDlg::close()
{

	OnClose();
}
