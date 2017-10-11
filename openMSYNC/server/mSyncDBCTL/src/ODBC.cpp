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

#include <windows.h>
#include <stdio.h>
#include "Sync.h"
#include "ODBC.h"

/*****************************************************************************/
//! ConnectToDB
/*****************************************************************************/
/*! \breif  DB�� connect �ϴ� �κ�
 ************************************
 * \param	Mode(in)	: ALL_HANDLES ���� �ƴ����� üũ
 * \param	henv(out)	: ODBC environment handle
 * \param	hdbc(out)	: ODBC connection handle
 * \param	hstmt(out)	: ODBC statement handle(�� �Լ������� �Ⱦ��̴µ�) (?)
 * \param	hflag(out)	: handle���� ����(ȯ�� ������ ��������, ���Ῡ�� ���)
 * \param	dsn(in)		: data source	
 * \param	uid(in)		: db user's id
 * \param	passwd(in)	: db user's passwd
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	ODBC�� DB�� connect�� ����ϴ� �Լ��� environment handle, \n
 *			connection handle�� �Ҵ� �ް� transaction auto commit�� off�� \n
 *			setting �� �� DB�� connect(SQLConnect())�� ����� return �Ѵ�. 
 *****************************************************************************/
int ConnectToDB(int Mode, SQLHENV *henv, SQLHDBC *hdbc, 
	    		SQLHSTMT *hstmt, int *hflag,
		    	SQLCHAR *dsn, SQLCHAR *uid, SQLCHAR *passwd)
{	
    SQLRETURN   retcode;
    // Allocate environment handle ==> henv 
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
        return -1;  // error returned 
    
    *hflag = hENV;
    // Set the ODBC version environment attribute 
    retcode = 
        SQLSetEnvAttr(*henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        return -1; // error returned 
    
    // Allocate connection handle ==> hdbc 
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, *henv, hdbc); 
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
        return -1; // error returned 
    *hflag = *hflag | hDBC;
    
    // Set login timeout to 5 seconds. 
    retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_LOGIN_TIMEOUT, (void *)10, 0);
    
    if(Mode == ALL_HANDLES)
    {
        retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_AUTOCOMMIT, 
                                    (void *)SQL_AUTOCOMMIT_OFF, 0);
    }
    
    retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_CONNECTION_POOLING, 
                                (void *)SQL_CP_OFF, 0);    
    
    // Connect to data source 
    retcode = SQLConnect(*hdbc, dsn, SQL_NTS, uid, SQL_NTS, passwd, SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        SQLCHAR			SQLSTATE[6], Msg[256];
        SQLINTEGER		NativeError;
        SQLSMALLINT		MsgLen;
        
        Msg[0] = '\0';
        SQLSTATE[0] = '\0';
        SQLGetDiagRec(SQL_HANDLE_DBC, *hdbc, 1, SQLSTATE, &NativeError, 
            Msg, sizeof(Msg), &MsgLen);
        
        if (Msg[strlen((char *)Msg)-1]=='\n')
        {        
            Msg[strlen((char *)Msg)-1] = '\0';
        }
        
        ErrLog("SYSTEM;DBERROR[%d:%s]:%s;\n", retcode, SQLSTATE, Msg);	
        return -1; // error returned 
    }
    *hflag = *hflag | hCONNECT;

    return 0;	
}

/*****************************************************************************/
//! FreeHandle
/*****************************************************************************/
/*! \breif  DB���� ������ ����
 ************************************
 * \param	henv(in)	: ODBC environment handle
 * wparam	hdbc(in)	: ODBC connection handle
 * \param	hstmt(in)	: ?
 * \param	hflag(out)	: DB connection�� ���¸� ��Ÿ���� �÷���(ConnectToDB ����)
 ************************************
 * \return	void
 ************************************
 * \note	Connect�� �Ǿ����� Ȯ���ϰ� disconnect(SQLDisconnect()), \n
 *			connection handle, environment handle�� �Ҵ�Ǿ����� �����Ѵ�.
 *****************************************************************************/
void FreeHandle(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt, int hflag)
{
    int ret;
    
    if((hflag&hCONNECT) == hCONNECT)
    {
        ret = SQLDisconnect(hdbc);    
        if(ret<0) ErrLog("SYSTEM;Disconnect error : %d;\n", ret);
    }
    
    if((hflag&hDBC) == hDBC)
    {
        ret = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }
    
    if((hflag&hENV) == hENV)
    {
        ret = SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}

/*****************************************************************************/
//! FreeHandles
/*****************************************************************************/
/*! \breif  DB���� ������ ����
 ************************************
 * \param	flag(in)		: DB connection�� ����
 * \param	pOdbcParam(in)	: UserDB�� Ŀ�ؼ��� ���ǵǾ� �ִ� ����ü
 ************************************
 * \return	void 
 ************************************
 * \note	UserDB�� open�Ǿ� ������ userDB�� ���� �� handle���� \n
 *			FreeHandle() �Լ��� �̿��Ͽ� �����ϰ� system DB�� ���� \n
 *			handle�� FreeHandle() �Լ��� �̿��Ͽ� �����Ѵ�
 *****************************************************************************/
void FreeHandles(int flag, ODBC_PARAMETER *pOdbcParam)
{
    int ret=0;	
    
    if(pOdbcParam->UserDBOpened)
    {
        ret = SQLEndTran(SQL_HANDLE_DBC, 
                         pOdbcParam->hdbc4Processing, SQL_COMMIT);
        if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hdbc4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, ret);
        }	
        
        FreeHandle(pOdbcParam->henv4Processing, pOdbcParam->hdbc4Processing, 
                   pOdbcParam->hstmt4Processing, pOdbcParam->hflag4Processing);
    }
    
    if(flag == ALL_HANDLES) 
    {
        FreeHandle(pOdbcParam->henv4System, pOdbcParam->hdbc4System, 
                   pOdbcParam->hstmt4System, pOdbcParam->hflag4System);
    }	
}

/*****************************************************************************/
//! AllocStmtHandle
/*****************************************************************************/
/*! \breif  SQL ������ ó���ϱ� ���ؼ� �ʿ��� statement handle�� �Ҵ��ϴ� �Լ�
 ************************************
 * \param	pOdbcParam(out) : statement handle�� �Ҵ� �ޱ� ���� ����ü
 * \param	flag(in)		: �÷���
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	SQL ������ ó���ϱ� ���ؼ� �ʿ��� statement handle�� �Ҵ��ϴ� �Լ�
 *****************************************************************************/
int	AllocStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag)
{
    SQLRETURN returncode;
    if(flag==SYSTEM)
    {   // Allocate statement handle ==> hstmt
        returncode = SQLAllocHandle(SQL_HANDLE_STMT, 
            pOdbcParam->hdbc4System, &(pOdbcParam->hstmt4System)); 
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO) 
            return -1 ; // error returned 
    }
    else
    {	// Allocate statement handle ==> hstmt
        returncode = SQLAllocHandle(SQL_HANDLE_STMT, 
            pOdbcParam->hdbc4Processing, &(pOdbcParam->hstmt4Processing)); 
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO) 
            return -1 ; // error returned 
    }
    return 0;
}


/*****************************************************************************/
//! FreeStmtHandle
/*****************************************************************************/
/*! \breif  Statement handle�� �����ϴ� �Լ�
 ************************************
 * \param	pOdbcParam(in) : statement handle�� ���� ��Ű�� ����ü
 * \param	flag(in)		: �÷���
  ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Statement handle�� �����ϴ� �Լ�
 *****************************************************************************/
void FreeStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag)
{
	if(flag==SYSTEM)
		SQLFreeHandle(SQL_HANDLE_STMT, pOdbcParam->hstmt4System);
	else
		SQLFreeHandle(SQL_HANDLE_STMT, pOdbcParam->hstmt4Processing);
}

/*****************************************************************************/
//! OpenSystemDB
/*****************************************************************************/
/*! \breif  System DB�� connection�� ����
 ************************************
 * \param	pOdbcParam(in) : statement handle�� ���� ��Ű�� ����ü
 * \param	flag(in)		: �÷���
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	ConnectToDB() �Լ��� �̿��Ͽ� ������ �Ķ���͸� �����ؼ�\n
 *			System DB�� connection�� �δ´�.
 *****************************************************************************/
int OpenSystemDB(ODBC_PARAMETER *pOdbcParam)
{	
    // Connect to System DB �� ���� 
    if(ConnectToDB(SYSTEM, &(pOdbcParam->henv4System), 
                   &(pOdbcParam->hdbc4System), &(pOdbcParam->hstmt4System), 
                   &(pOdbcParam->hflag4System), pOdbcParam->dbdsn, 
                   pOdbcParam->dbuid, pOdbcParam->dbpasswd) < 0)
    {
        ErrLog("SYSTEM [%d];Connect to System DB[%s] : FAIL...;\n", 
                pOdbcParam->order, pOdbcParam->dbdsn);
        return (-1);
    }
    return (0);
}

/*****************************************************************************/
//!  OpenUserDB 
/*****************************************************************************/
/*! \breif  User DB�� connection�� �δ� �Լ�
 ************************************
 * \param	pOdbcParam(out)	: 
 * \param	dsn(in)			: data source	
 * \param	dsnuid(in)		: data source user's id
 * \param	dsnpasswd(in)	: data source user's passwd
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	ConnectToDB() �Լ��� �̿��Ͽ� ������ �Ķ���͸� �����ؼ� \n
 *			User DB�� connection�� �δ´�.
 *****************************************************************************/
int	OpenUserDB(ODBC_PARAMETER *pOdbcParam, 
               SQLCHAR *dsn, SQLCHAR *dsnuid, SQLCHAR *dsnpasswd)
{
	
	if(ConnectToDB(ALL_HANDLES, &(pOdbcParam->henv4Processing), 
                   &(pOdbcParam->hdbc4Processing), 
                   &(pOdbcParam->hstmt4Processing), 
                   &(pOdbcParam->hflag4Processing), dsn, dsnuid, dsnpasswd)<0)
    {
		ErrLog("%d_%s;Connect to User DB[%s] : FAIL...;\n", 
            pOdbcParam->order, pOdbcParam->uid, dsn);
		return (-1);
	}	
	return 0;
}