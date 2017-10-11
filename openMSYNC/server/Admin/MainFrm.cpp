/* 
    This file is part of openMSync Server module, DB synchronization software.

    Copyright (C) 2012 Inervit Co., Ltd.
        support@inervit.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Admin.h"

#include "MainFrm.h"
#include "DBConnectDlg.h"
#include "UserDlg.h"

#include "LeftTreeView.h"
#include "RightListView.h"
#include "RightView.h"
#include "SQLDirect.h"

#include "AddDSNDlg.h"
#include "odbcinst.h"  // SQLCreateDataSource() odbccp32.lib�� ���� link
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_MENU_CREATEDSN, OnMenuCreatedsn)
	ON_COMMAND(ID_MENU_REGSYNCSCRIPT, OnMenuNewSyncScript)
	ON_COMMAND(ID_MENU_DBCONNECT, OnMenuDBConnect)
	ON_COMMAND(ID_MENU_DBDISCONNECT, OnMenuDBDisConnect)
	ON_COMMAND(ID_MENU_REGUSERPROFILE, OnMenuNewUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_strDSNName = _T("");
	m_strPWD = _T("");
	m_strLoginID = _T("");
	m_bConnectFlag = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.MakeEdit(8, ID_IP_TEXT, CSize(50, 50));  // 2003.8.20 mSyncIP�� Toolbar�� ����
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
  /* supiajin 3/24 toolbar�� docking ���� �ʰ� �����ǵ��� */
//	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_wndToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style &= ~FWS_ADDTOTITLE;  // supiajin 3/13 ������� �ȳ����� program �̸��� ������
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

// OnCreate()�� CFrameWnd::OnCreate()=>OnCreateHelper() �ȿ��� ȣ��� 
// �ΰ��� view�� ������ �κ�
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CRect rect;

	GetClientRect(&rect);
	CSize size1(MulDiv(rect.Width(), 20, 100), rect.Height());
	CSize size2(MulDiv(rect.Width(), 80, 100), ::GetSystemMetrics(SM_CYSCREEN));

	// 2���� view�� �����ϴ� spliter â �����
	if(!m_wndSplitter.CreateStatic(this, 1,2))
	{
		TRACE0("Failed to CreateStatic Splitter \n");
		return FALSE;
	}
	// CLeftTreeView�� ���� view �����
	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftTreeView), size1, pContext))
	{
		TRACE0("Failed to create CLeftTreeView pane\n");
		return FALSE;
	}
	// CRightView�� ������ view�� parent �����
	if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRightView), size2, pContext))
	{
		TRACE0("Failed to create CRightView pane\n");
		return FALSE;
	}

	m_pLeftTreeView = (CLeftTreeView *)m_wndSplitter.GetPane(0, 0);
	m_pRightView = (CRightView *)m_wndSplitter.GetPane(0, 1);

	m_pRightView->SetTabImage();

	// ���� display list�� �ִ� CRightListView ���
	m_pRightListView  = (CRightListView *)m_pRightView->AddView("Details", 
        RUNTIME_CLASS(CRightListView), pContext, 0);

	UpdateData(FALSE);
	return TRUE;
}
void CMainFrame::OnMenuCreatedsn() 
{
	// TODO: Add your command handler code here
	HWND hWnd = this->m_hWnd;

	if (SQLCreateDataSource(hWnd, NULL)==TRUE)
		MessageBox("DSN�� ���������� �����Ǿ����ϴ�.");
}

void CMainFrame::OnMenuNewSyncScript() 
{
	// TODO: Add your command handler code here
	if(CheckDBSetting() == FALSE) return;

    CAddDSNDlg dlg;
    HTREEITEM	hItem;
	hItem = m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();
	CString  strAppVer = m_pLeftTreeView->GetTreeCtrl().GetItemText(hItem);

	if(strAppVer == "SyncScript Manager")
		dlg.m_nAction = ADD_LIST;
	else
        dlg.m_nAction = NO_ACTION;

    dlg.m_bNewApp = TRUE;
    dlg.DoModal();
    delete dlg;
}

void CMainFrame::OnMenuDBConnect() 
{
	// TODO: Add your command handler code here
	if(m_bConnectFlag)
		AfxMessageBox("���� ���� ������ �����Ͽ� �ֽʽÿ�.");
	else
    {
		CDBConnectDlg  pDBDlg(this);
        // �� �ȿ��� m_bConnectFlag�� TRUE���� FALSE���� setting
		pDBDlg.DoModal();	
		m_bConnectFlag = TRUE;
	}
}

void CMainFrame::OnMenuDBDisConnect() 
{
	// TODO: Add your command handler code here
	if(m_bConnectFlag == FALSE)
		AfxMessageBox("���� ���� ������ �Ͽ� �ֽʽÿ�.");
	else{
		m_strDSNName = _T("");
		m_strPWD = _T("");
		m_strLoginID = _T("");
		m_pLeftTreeView->InitializeSyncScriptTree();
		m_pRightListView->GetListCtrl().DeleteAllItems();
		svDBType[0] = 0x00;
		m_bConnectFlag = FALSE;
	}
}

void CMainFrame::OnMenuNewUser() 
{
	// TODO: Add your command handler code here
	if(CheckDBSetting() == FALSE) return;
	CUserDlg	pUserDlg(this);

	HTREEITEM	hItem;
	hItem = m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();
	CString  strAppVer = m_pLeftTreeView->GetTreeCtrl().GetItemText(hItem);

	if(strAppVer == "User Profile")
		pUserDlg.SetState(ID_NEW_USER, ADD_LIST);
	else
		pUserDlg.SetState(ID_NEW_USER, NO_ACTION);
	pUserDlg.DoModal();
}

void CMainFrame::ChangeTitleText(CString strTitle)
{
	m_pRightView->SetTabLabel(0,strTitle);
	UpdateData(FALSE);
}

void CMainFrame::ShowSystemDSN(CComboBox *pWnd)
{
    /*    HKEY_LOCAL_MACHINE\Software\ODBC\ODBC.INI */
	LONG		lResult;
	HKEY		hKey;
	CHAR		achClass[MAX_PATH] = "";
	DWORD		cchClassName = MAX_PATH,cSubKeys,cbMaxSubKey,cchMaxClass;
	DWORD		cValues, cchMaxValue, cbMaxValueData, cbSecurityDescriptor;
	FILETIME	ftLastWriteTime;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",0,KEY_READ,&hKey);

	RegQueryInfoKey	(hKey,
				achClass,
				&cchClassName,
				NULL,&cSubKeys,
				&cbMaxSubKey,
				&cchMaxClass,
				&cValues,
				&cchMaxValue,
				&cbMaxValueData,
				&cbSecurityDescriptor,
				&ftLastWriteTime);

	DWORD		dwBuffer;
	char		szSubKeyName[MAX_PATH];
	pWnd->ResetContent();
	for (DWORD nCount=0 ; nCount < cValues ; nCount++)
	{
		dwBuffer = MAX_PATH;
		lResult = RegEnumValue(hKey,nCount,
            (LPTSTR)szSubKeyName,&dwBuffer,NULL,NULL,NULL, NULL);
		if ((pWnd != NULL) && (szSubKeyName != (LPCTSTR)""))
		{
			pWnd->AddString( (LPCTSTR)szSubKeyName );
		}
	}
	pWnd->SetCurSel(0);
	RegCloseKey(hKey);
}

BOOL CMainFrame::CheckDBSetting()
{
	if(m_strDSNName.GetLength() == 0 || 
		m_strLoginID.GetLength() == 0 || 
		m_strPWD.GetLength() == 0 )
	{
		AfxMessageBox("���� DB ������ ���� ȯ���� �����ϼ���");
		return FALSE;
	}
	return TRUE;
}

BOOL CMainFrame::DeleteDSNTableScript(CSQLDirect *aSD1, 
                                      CSQLDirect *aSD2, CString strDBID)
{
	// aSD1�� DSN�� db, aSD2�� Table�� db
	char szQuery[QUERY_MAX_LEN];

	sprintf(szQuery, 
        "SELECT TableID FROM openMSYNC_Table WHERE DBID = %s", strDBID);
	if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
		return FALSE;
	}
	while(aSD1->FetchSQL() == SQL_SUCCESS)
	{
		CString strTableID = aSD1->GetColumnValue(1);
		if(DeleteTableScript(aSD2, strTableID) == FALSE) return FALSE;
	}
	sprintf(szQuery, "DELETE FROM openMSYNC_DSN WHERE DBID = %s", strDBID);
	if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS) 
	{
		AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
		return FALSE;
	}
	return TRUE;
}

BOOL CMainFrame::DeleteTableScript(CSQLDirect *aSD, CString strTableID)
{
	char szQuery[QUERY_MAX_LEN];

	sprintf(szQuery, 
        "DELETE FROM openMSYNC_Script WHERE TableID = %s", strTableID);
	if(aSD->ExecuteSQL(szQuery) != SQL_SUCCESS) 
	{	
		AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
		return FALSE;
	}	
	sprintf(szQuery, 
        "DELETE FROM openMSYNC_Table WHERE TableID = %s", strTableID);
	if(aSD->ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
		return FALSE;
	}		
	return TRUE;
}

int CMainFrame::GetFileCount(CString strItem)
{
	int idx1 = strItem.Find("(",0);	
	int idx2 = strItem.Find(")",0);
	int nFileCount = atoi((char *)(LPCSTR)strItem.Mid(idx1+1, idx2-idx1));	
	return nFileCount;	
}

void CMainFrame::GetDBServerType(CString strDSNName)
{
    /*    HKEY_LOCAL_MACHINE\Software\ODBC\ODBC.INI */
	LONG		lResult;
	HKEY		hKey;
	CHAR		achClass[MAX_PATH] = "";
	DWORD		cchClassName = MAX_PATH,cSubKeys,cbMaxSubKey,cchMaxClass;
	DWORD		cValues, cchMaxValue, cbMaxValueData, cbSecurityDescriptor;
	FILETIME	ftLastWriteTime;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",0,KEY_READ,&hKey);

	RegQueryInfoKey	(hKey,
				achClass,
				&cchClassName,
				NULL,&cSubKeys,
				&cbMaxSubKey,
				&cchMaxClass,
				&cValues,
				&cchMaxValue,
				&cbMaxValueData,
				&cbSecurityDescriptor,
				&ftLastWriteTime);

	DWORD		dwBuffer, dwType;
	char		szSubKeyName[MAX_PATH], szSubValueName[MAX_PATH], svDsn[MAX_PATH];
	sprintf(svDsn, strDSNName);
	for (DWORD nCount=0 ; nCount < cValues ; nCount++)
	{
		dwBuffer = MAX_PATH;
		lResult = RegEnumValue(hKey,nCount,
            (LPTSTR)szSubKeyName,&dwBuffer,NULL,NULL,NULL, NULL);
		if(!strcmp(szSubKeyName, svDsn)){
			dwBuffer = MAX_PATH;
			RegQueryValueEx (hKey, (LPSTR)szSubKeyName, 
                NULL,  &dwType, (BYTE *)szSubValueName, &dwBuffer);
			if(strstr(szSubValueName, "Oracle")){
				strcpy(svDBType, "Oracle");
				m_nDBType = ORACLE;
			}
			else if(strstr(szSubValueName, "SQL Server")){
				strcpy(svDBType, "SQL Server");
				m_nDBType = MSSQL;
			}
			else if(strstr(szSubValueName, "MySQL")){
				strcpy(svDBType, "MySQL");
				m_nDBType = MYSQL;
			}
			else if(strstr(szSubValueName, "Access")){
				strcpy(svDBType, "MS Access");
				m_nDBType = ACCESS;
			}
			else if(strstr(szSubValueName, "Adaptive Server Enterprise") 
                || strstr(szSubValueName, "ASE"))
            {
				strcpy(svDBType, "Sybase");
				m_nDBType = SYBASE;
			}			
			else if(strstr(szSubValueName, "CUBRID")){
				strcpy(svDBType, "CUBRID");
				m_nDBType = CUBRID;
			}
			else if(strstr(szSubValueName, "DB2")){
				strcpy(svDBType, "DB2");
				m_nDBType = DB2;
			}

			break;
		}		
	}
	RegCloseKey(hKey);
}
