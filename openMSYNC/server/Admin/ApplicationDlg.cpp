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

// ApplicationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "ApplicationDlg.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "LeftTreeView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CApplicationDlg dialog


CApplicationDlg::CApplicationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CApplicationDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CApplicationDlg)
	m_strAppName = _T("");
	m_strVersion = _T("");
	//}}AFX_DATA_INIT
}


void CApplicationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CApplicationDlg)
	DDX_Control(pDX, IDC_APPVER, m_ctlVersion);
	DDX_Control(pDX, IDC_APPNAME, m_ctlAppName);
	DDX_Text(pDX, IDC_APPNAME, m_strAppName);
	DDV_MaxChars(pDX, m_strAppName, 30);
	DDX_Text(pDX, IDC_APPVER, m_strVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CApplicationDlg, CDialog)
	//{{AFX_MSG_MAP(CApplicationDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CApplicationDlg message handlers

BOOL CApplicationDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_nAction == COPY_LEFT || m_nAction == COPY_RIGHT)	// sync script copy�� ���
	{
		((CStatic *)GetDlgItem(IDC_APPINFO))->SetWindowText(" �� Application ���� �Է� ");

		UpdateData(FALSE);
	}
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CApplicationDlg::SetValue(int nIndex, 
                               CString strVerID, CString strAppVer, int nAct)
{
	m_nIndex = nIndex;
	m_strAppVerID = strVerID;		// select�� ���� key
	m_strBeforeAppVer = strAppVer;	// change�� item�� ã�� ���� ����
	m_nAction = nAct;
}


void CApplicationDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);
	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd(); 
	
	if(m_strAppName == "")
	{
		AfxMessageBox("Application �̸��� �Է��Ͽ� �ֽʽÿ�.");
		m_ctlAppName.SetFocus();
		return;
	}	
	else if(m_strVersion.Find(".")>0 || atoi(m_strVersion.GetBuffer(0)) < 1 
            || atoi(m_strVersion.GetBuffer(0)) > 50)
	{
		AfxMessageBox("Version�� ���� 1���� 50 ������ ���� ���Դϴ�.");
		m_ctlVersion.SetFocus();
		return;
	}

	char szQuery[QUERY_MAX_LEN];
	if (m_nAction == COPY_LEFT || m_nAction == COPY_RIGHT)// sync script copy�� ���
	{
		CSQLDirect aSD1, aSD2, aSD3, aSD4;
		if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE 
            || aSD3.ConnectToDB() == FALSE || aSD4.ConnectToDB() == FALSE) 
		{
			AfxMessageBox("���翡 �����Ͽ����ϴ�");
			return;
		}

		// ���ο� VersionID�� ����
		sprintf(szQuery, "SELECT max(VersionID)+1 FROM openMSYNC_Version");
		if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
		{
			aSD1.RollBack();
			aSD2.RollBack();
			aSD3.RollBack();
			aSD4.RollBack();
			return;
		}
		if(aSD1.FetchSQL() != SQL_SUCCESS)
		{
			aSD1.RollBack();
			aSD2.RollBack();
			aSD3.RollBack();
			aSD4.RollBack();
			return;	
		}
		CString strNewVersionID = aSD1.GetColumnValue(1);

		// openMSYNC_Version�� insert
		sprintf(szQuery, "INSERT INTO openMSYNC_Version VALUES(%s, '%s', %s)",
				strNewVersionID, m_strAppName, m_strVersion);
		if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)  
		{
			aSD1.RollBack();
			aSD2.RollBack();
			aSD3.RollBack();
			aSD4.RollBack();
			return;
		}		

		sprintf(szQuery, "SELECT DBID, SvrDSN, DSNUserID, DSNPwd, CliDSN "
                "FROM openMSYNC_DSN WHERE VersionID = %s ORDER BY SvrDSN",
                m_strAppVerID);
		if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
		{
			AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
			aSD1.RollBack();
			aSD2.RollBack();
			aSD3.RollBack();
			aSD4.RollBack();
			return;
		}
		// �ش� DBID�� ���õ� Table, script ���� ��� �� ID�� ���� �޾� ����
		while(aSD1.FetchSQL() == SQL_SUCCESS)
		{
			CString strDBID = aSD1.GetColumnValue(1);
			CString strDSNName = aSD1.GetColumnValue(2);
			CString strDSNUid = aSD1.GetColumnValue(3);
			CString strDSNPwd = aSD1.GetColumnValue(4);
			CString strCliDSN = aSD1.GetColumnValue(5);
			if(InsertDSNTableScript(&aSD2, &aSD3, &aSD4, strNewVersionID, 
                strDSNName, strDSNUid, strDSNPwd, strCliDSN, strDBID) == FALSE)
			{
				aSD1.RollBack();
				aSD2.RollBack();
				aSD3.RollBack();
				aSD4.RollBack();
				return;
			}
		}
		aSD1.Commit();
		aSD2.Commit();
		aSD3.Commit();
		aSD4.Commit();
		// ���� TreeView�� �߰�
		CString strAppVer;
		strAppVer.Format("%s.%s", m_strAppName, m_strVersion); // Application.Version
		// Application.Version �߰�
		HTREEITEM	hDSNParent, hTreeScriptParent;
		hTreeScriptParent = pMainFrame->m_pLeftTreeView->GetTreeScriptParent();
		hDSNParent = pMainFrame->m_pLeftTreeView->AddItemToTree(hTreeScriptParent, 
            strAppVer, pMainFrame->m_pLeftTreeView->n_TreeApp);
		pMainFrame->m_pLeftTreeView->MakeDSNTableTree(&aSD2, &aSD3, 
            strNewVersionID, hDSNParent);
	
		if(m_nAction == COPY_RIGHT)
		{
			// ������ ListView�� �߰�
			char *szColValue[2];
			szColValue[0] = new char[strAppVer.GetLength()+1];
			strcpy(szColValue[0], strAppVer.GetBuffer(0));		// strAppVer
			szColValue[1] = new char[strNewVersionID.GetLength()+1];
			strcpy(szColValue[1], strNewVersionID.GetBuffer(0));// VersionID
			pMainFrame->m_pRightListView->AddStringToList(szColValue);
			for(int i=0 ; i<2 ; i++)			
				delete szColValue[i];
			
			UpdateData(FALSE);		
		}
	}
	else
	{		
		// Application ������ ���
		CSQLDirect aSD;

		if(aSD.ConnectToDB() == FALSE)
		{
			AfxMessageBox("���忡 �����Ͽ����ϴ�");
			return;
		}

		sprintf(szQuery, "UPDATE openMSYNC_Version SET Application = '%s', "
            "Version = %s WHERE VersionID = %s", 
			m_strAppName, m_strVersion, m_strAppVerID);
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
		{
			AfxMessageBox("���忡 �����Ͽ����ϴ�");
			return;
		}
		CString strAppVer ;
		strAppVer.Format("%s.%s", m_strAppName, m_strVersion);

		if(m_nAction == MODIFY_RIGHT)
		{
			// ApplicationVersion, VersionID�ϱ� 0��°
			pMainFrame->m_pRightListView->ModifyColValue(m_nIndex, 0, strAppVer);
			pMainFrame->m_pLeftTreeView->ChangeItemText(m_strBeforeAppVer, strAppVer);	
		}
		else
			pMainFrame->m_pLeftTreeView->ChangeItemText(strAppVer);	
		aSD.Commit();
	}
	CDialog::OnOK();
}

BOOL CApplicationDlg::InsertDSNTableScript(CSQLDirect *aSD1, 
                                           CSQLDirect *aSD2, 
                                           CSQLDirect *aSD3, 
                                           CString strNewVersionID, 
                                           CString strDSNName, 
                                           CString strDSNUid, 
                                           CString strDSNPwd, 
                                           CString strCliDSN, 
                                           CString strDBID)
{
	// aSD1�� DSN�� db, aSD2�� Table�� db, aSD3�� script�� db
	char szQuery[QUERY_MAX_LEN];

	// ���ο� DSN�� ����
	sprintf(szQuery, "SELECT max(DBID)+1 FROM openMSYNC_DSN");
	if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)		return FALSE;
	if(aSD1->FetchSQL() != SQL_SUCCESS)		return FALSE;
			
	CString strNewDBID = aSD1->GetColumnValue(1);

	sprintf(szQuery, "INSERT INTO openMSYNC_DSN VALUES(%s, %s, '%s', '%s', '%s', '%s')",
		strNewDBID, strNewVersionID, strDSNName, strDSNUid, strDSNPwd, strCliDSN);	
	
	if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)	return FALSE;
	
	// ���� table ���� �о� copy �ϱ�
	sprintf(szQuery, "SELECT TableID, SvrTableName, CliTableName "
        "FROM openMSYNC_Table WHERE DBID = %s ORDER BY SvrTableName", strDBID);
	if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ�");
		return FALSE;
	}
	while(aSD1->FetchSQL() == SQL_SUCCESS)	// SQL_NO_DATA�� ��������
	{
		CString strTableID = aSD1->GetColumnValue(1);
		CString strSvrTableName = aSD1->GetColumnValue(2);
		CString strCliTableName = aSD1->GetColumnValue(3);
		if(InsertTableScript(aSD2, aSD3, strNewDBID, strSvrTableName, 
            strCliTableName, strTableID) == FALSE) 
        {
            return FALSE;
        }
	}
	return TRUE;
}

BOOL CApplicationDlg::InsertTableScript(CSQLDirect *aSD1, CSQLDirect *aSD2, 
                                        CString strDBID, CString strSvrTableName,
                                        CString strCliTableName, CString strTableID)
{
    char szQuery[QUERY_MAX_LEN];
    
    CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd(); 
    
    // ���ο� table ����
    sprintf(szQuery, "SELECT max(TableID)+1 FROM openMSYNC_Table");
    if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS) return FALSE;
    if(aSD1->FetchSQL() != SQL_SUCCESS) return FALSE;
    CString  strNewTableID = aSD1->GetColumnValue(1);
    
    sprintf(szQuery, "INSERT INTO openMSYNC_Table VALUES(%s, %s, '%s', '%s')",
        strNewTableID, strDBID, strSvrTableName, strCliTableName);
    if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)  return FALSE;	
    
    sprintf(szQuery, "SELECT Event, Script FROM openMSYNC_Script WHERE TableID = %s", strTableID);
    if(aSD1->ExecuteSQL(szQuery) != SQL_SUCCESS)
    {
        AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ� : script ���� �б�");
        return FALSE;
    }
    
    CString strScript, strEvent;
    while(aSD1->FetchSQL() == SQL_SUCCESS)	// SQL_NO_DATA�� ��������
    {
        strEvent = aSD1->GetColumnValue(1);
        strScript = aSD1->GetColumnValue(2);
        strScript.Replace("'", "''");       
       
        if(pMainFrame->m_nDBType == SYBASE)
            sprintf(szQuery, "INSERT INTO openMSYNC_Script VALUES(%s, upper('%s'), '%s')",
            strNewTableID, strEvent, strScript);
        else
            sprintf(szQuery, "INSERT INTO openMSYNC_Script VALUES(%s, '%s', '%s')",
            strNewTableID, strEvent, strScript);
        
        if(aSD2->ExecuteSQL(szQuery) != SQL_SUCCESS)
        {
            AfxMessageBox("���� �߿� ������ �߻��Ͽ����ϴ� : script ���� ����");
            return FALSE;
        }	
    }	
    return TRUE;
}

