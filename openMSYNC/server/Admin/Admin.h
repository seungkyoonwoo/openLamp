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

// Admin.h : main header file for the ADMIN application
//

#if !defined(AFX_ADMIN_H__D94F1100_05FA_4D87_9F25_7FDD1AD2493B__INCLUDED_)
#define AFX_ADMIN_H__D94F1100_05FA_4D87_9F25_7FDD1AD2493B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
extern char *svDBType;

/////////////////////////////////////////////////////////////////////////////
// CAdminApp:
// See Admin.cpp for the implementation of this class
//

#define SCRIPT_LEN 4000
#define QUERY_MAX_LEN (SCRIPT_LEN+1024)
//#define QUERY_MAX_LEN 8092
class CAdminApp : public CWinApp
{
public:
	CAdminApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdminApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CAdminApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADMIN_H__D94F1100_05FA_4D87_9F25_7FDD1AD2493B__INCLUDED_)
