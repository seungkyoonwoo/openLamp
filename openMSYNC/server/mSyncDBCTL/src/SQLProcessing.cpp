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

#include	<windows.h>
#include	<stdio.h>

#include	"Sync.h"
#include	"ODBC.h"
#include    <mbstring.h>


int	GetSyncTime(ODBC_PARAMETER *pOdbcParam, 
                char *ptr, int dataLength, int mode);

unsigned int  gl_MaxUserCount;

/*****************************************************************************/
//! delete_space
/*****************************************************************************/
/*! \breif  ������ �ڿ� ������ space ������ ����
 ************************************
 * \param	src(in)	: ���ڿ� 
 * \param	len(in)	: ���ڿ��� ���� 
 ************************************
 * \return  void
 ************************************
 * \note	������ �ڿ� ������ space ������ �����Ѵ�. 
 *****************************************************************************/
void delete_space(char *src, int len)
{
    int	i = 0;
    
    if (src == NULL)	return;		
    if (strlen(src) > 0)
    {
        for (i = len-1 ; i > 0 && src[i] == ' ' ; i--);
            src[i+1] = '\0';
    }
}

/*****************************************************************************/
//! SetMaxUserCount
/*****************************************************************************/
/*! \breif  �ִ� ����� ���� ���� ��Ű�� �Լ�  \n
 ************************************
 * \param	max : �����ϰ��� �ϴ� �ִ� ����� �� 
 ************************************
 * \return	void
 ************************************
 * \note	MaxUserCount�� ���� max�� ���� ��Ų��. \n
 *			Auth.ini�� ������ ���� �ִ� ����� �� ���� setting �Ѵ�.
 *****************************************************************************/
void SetMaxUserCount(int max)
{
	gl_MaxUserCount = max;
}

/*****************************************************************************/
//! UpdateConnectFlag
/*****************************************************************************/
/*! \breif update connectionFlag, DBSyncTime  in openMSYNC_User table 
 ************************************
 * \param	pOdbcParam(in)	: User's DB Connection ������ ��� �ִ� �ʵ� 
 * \param	flag(in)		: User�� �α��� ����
 ************************************
 * \return  int : 0, -1
 ************************************
 * \note	Client�� connect, disconnect �� ���� �ҷ����� �Լ���\n
 *			openMSYNC_User���̺��� ConnectionFlag ���� CONNECT_STATE,\n
 *			DISCONNECT_ERROR_STATE, DISCONNECT_STATE ������ �������ش�. 
 *			�������� disconnect ������ ��� �Ҵ��ߴ� �޸𸮵鵵 ��ȯ�Ѵ�.
 *****************************************************************************/
int	UpdateConnectFlag(ODBC_PARAMETER *pOdbcParam, int flag)
{
    SQLRETURN	returncode;
    SQLCHAR		Statement[QUERY_MAX_LEN + 1];
    int			ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1); 
    if (flag == CONNECT_STATE)
    {
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' where UserID='%s'",
            flag, pOdbcParam->uid);
    }
    else if (flag == DISCONNECT_ERROR_STATE)
    {    
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' where UserID='%s'",
            flag, pOdbcParam->uid);
    }
    else
    { // DISCONNECT_STATE�� ��� Last Sync time�� update
        SQLCHAR		buffer1[100];
        SQLCHAR		buffer2[100];
        
        memset(buffer1, 0x00, sizeof(buffer1));	
        memset(buffer2, 0x00, sizeof(buffer2));
        
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' ", flag);

        if(pOdbcParam->syncFlag==ALL_SYNC || pOdbcParam->syncFlag==MOD_SYNC)
        {  
            if(pOdbcParam->mode & DBSYNC)			
            {
                sprintf((char *)buffer2, 
                        ", DBSyncTime='%s'", pOdbcParam->connectTime);
            }
        }
        
        sprintf((char *)Statement, "%s %s %s where UserID='%s'", 
            Statement, buffer1, buffer2, pOdbcParam->uid);
    }
    
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
    }
    FreeStmtHandle(pOdbcParam, SYSTEM);
    
    if(flag != CONNECT_STATE)
    {
        if(pOdbcParam->connectTime) 
        {
            free(pOdbcParam->connectTime);
            pOdbcParam->connectTime = NULL;
        }
        if(pOdbcParam->lastSyncTime)	
        {
            free(pOdbcParam->lastSyncTime);
            pOdbcParam->lastSyncTime = NULL;
        }
    }
    return (ret);
}

/*****************************************************************************/
//! CheckCurrentUserCount 
/*****************************************************************************/
/*! \breif ���� connection user�� ���� üũ�Ͽ� ���� ���� ���� 
 ************************************
 * \param pOdbcParam(in)	: DB ���� ����
 ************************************
 * \return  int : \n
 *			0 < error \n
 *			0 >= success \n
 *			EXCESS_USER �ʰ���, 0, -1
 ************************************
 * \note	openMSYNC_User ���̺� ��ϵǾ� �ִ� ����� ���� MaxUser ���� \n
 *			���Ͽ� ���� ��ϵǾ� �ִ� ����ڰ� �� ���� �ʰ��ϸ� \n
 *			EXCESS_USER��, �ƴ� ��� 0�� return �Ѵ�. \n
 *			�˰���\n
 *			1. connected user�� ������ �о� ���� SQL ���� ���� \n
 *			2. column Binding\n
 *****************************************************************************/
int	 CheckCurrentUserCount(ODBC_PARAMETER *pOdbcParam)
{
    SQLUINTEGER		DCount;
    SQLINTEGER		DCountLenOrInd;		
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1);
    
    // 1. connected user�� ������ �о� ���� SQL ���� ����    
    sprintf((char *)Statement,"select count(*) from openMSYNC_User");
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckCount_END;
    }
    // 2. column Binding
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_ULONG, &DCount, 0, &DCountLenOrInd);
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);
    // SQL_ERROR�� return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && 
        returncode != SQL_NO_DATA )
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckCount_END;
    }	

    // 4. count ��    
    if (DCount > gl_MaxUserCount)
    {
        ErrLog("SYSTEM [%d];Allowed maximun users = %d;\n", 
                pOdbcParam->order, gl_MaxUserCount);
        ret = EXCESS_USER;
    }

CheckCount_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

/*****************************************************************************/
//! CheckUserInfo 
/*****************************************************************************/  
/*! \breif  LD_USER table�� password�� ���Ͽ� ����� ���� 
 ************************************
 * \param	pOdbcParam(in)	: DB ���� ����(uid, passwd)
 * \param	passwd(in)		: user's passwd 
 ************************************
 * \return  return -1, 0, NO_USER, INVALID_USER
 ************************************
 * \note	Connect�� client�� userID�� openMSYNC_User ���̺� ��ϵǾ� �ִ���\n 
 *			Ȯ���ϰ� ��ϵ� ������� ��� password�� �Է¹��� ���� ������\n
 *			�����ϴ� �۾��� �����Ѵ�. ��ϵǾ� ���� ���� ������� ���\n
 *			NO_USER�� return �ϰ� password ���� �߸��� ��� INVALID_USER��\n
 *			return �ϸ� �������� ��� 0�� return ���ش�.
 *****************************************************************************/
int	CheckUserInfo(ODBC_PARAMETER *pOdbcParam, SQLCHAR *passwd)
{
    SQLCHAR			DPasswd[PASSWD_LEN + 1];
    SQLINTEGER		DPasswdLenOrInd, DDBSyncTimeOrInd;
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    int		ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)	
        return (-1);
    
    // 1. userID�� password�� DBSyncTime �о���� SQL ���� ����
    sprintf((char *)Statement,
        "select UserPwd, DBSyncTime  from openMSYNC_User where UserID='%s'", 
        pOdbcParam->uid);
        
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckUser_END;
    }
    
    // 2. column Binding
    // password binding
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_CHAR, DPasswd , PASSWD_LEN + 1, &DPasswdLenOrInd);
    SQLDescribeCol(pOdbcParam->hstmt4System, 
                   2, (unsigned char *)szBuffer, sizeof(szBuffer), &szBufferLen,
                   &SQLDataType, &ColumnSize, &DecimalDigits, &Nullable);
    
    pOdbcParam->lastSyncTime = NULL;
    pOdbcParam->lastSyncTime = (char *)calloc(ColumnSize+4, sizeof(char));    
    if(pOdbcParam->lastSyncTime==NULL)
    {
        ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n",
               pOdbcParam->order, ColumnSize+4);
        ret = -1;
        goto CheckUser_END;
    }

    SQLBindCol(pOdbcParam->hstmt4System, 
               2, SQL_C_CHAR, pOdbcParam->lastSyncTime, 
               (ColumnSize+1)*sizeof(char), &DDBSyncTimeOrInd);
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);
    // SQL_ERROR�� return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System,
            pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckUser_END;
    }	
    SQLCloseCursor(pOdbcParam->hstmt4System);
    
    // openMSYNC_User table�� ���� user
    if (returncode == SQL_NO_DATA) 
    {
        ret = NO_USER;
        goto CheckUser_END;
    }
    delete_space((char *)DPasswd, DPasswdLenOrInd);	
    
    // 4. password ��
    if (strcmp((char *)passwd, (char *)DPasswd))
        ret = INVALID_USER;		
    
CheckUser_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return ret;
}

/*****************************************************************************/
//! GetVersionInfo 
 /*****************************************************************************/
/*! \breif  openMSYNC_Version table���� version or VersionID�� fetch\n
 *			2003. 2. 18
 ************************************
 * \param	pOdbcParam(in)	: DB ���� ����
 * \param	Statement(in)	: versionID�� version ���� �������� query��
 * \param	newVersion(out)	: versionID�� version
 ************************************
 * \return  int \n
 *			0 < error \n
 *			0 >= sucess \n
 *			return : 0, -1, SQL_NO_DATA
 ************************************
 * \note	�̸� ������ ���� Statement ������ �����Ͽ� versionID�� version\n
 *			���� �˻��Ͽ� newVersion ������ �������ش�. �ش� version ������ 
 *			���� ��� SQL_NO_DATA�� return ���ش�.\n
 *			CheckVersion() �Լ����� ���� ȣ��ȴ�.
 *			���� �˰���\n
 *			openMSYNC_Version table���� version or versionID�� fetch
 *****************************************************************************/
int GetVersionInfo(ODBC_PARAMETER *pOdbcParam, 
                   SQLCHAR *Statement, SQLUINTEGER *newVersion)
{
    SQLRETURN		returncode;
    SQLINTEGER		newVersionLenOrInd;	
    
    // 1. �ش� statement ����
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
            pOdbcParam->uid, pOdbcParam->order, returncode);
        return (-1);
    }
    // 2. column Binding
    SQLBindCol(pOdbcParam->hstmt4System, 
        1, SQL_C_ULONG, newVersion , 0, &newVersionLenOrInd);	
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System) ;	
    // SQL_ERROR�� return -1
    if(returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
            pOdbcParam->uid, pOdbcParam->order, returncode);
        
        return (-1);
    }	
    
    SQLCloseCursor(pOdbcParam->hstmt4System);
    // record�� ���� ��� max()�� NULL���� SQL_NO_DATA�� return ���� �ʴ´�.
    if (returncode == SQL_NO_DATA || newVersionLenOrInd == SQL_NULL_DATA)
    { 
        return (SQL_NO_DATA);
    }	
    return 0;
}

/*****************************************************************************/
//! CheckVersion 
 /*****************************************************************************/
/*! \breif	���� �˻�
 ************************************
 * \param	pOdbcParam(in,out)	: pOdbcParam->versionID(out)
 * \param	application(in)		: 
 * \param	version(in)			 :
 ************************************
 * \return	<0, on error
			>0, on versionID�� ã�� ��� new version ID
			=0, upgrade�� version�� ���� ���
 ************************************
 * \note	Parameter�� ���� application, version�� ���� �˻縦 �ϴ� �Լ���\n
 *			���� versionID�� �˻��ϰ� �ش� version ���� ���� ������ ��ϵǾ�\n
 *			�ִ��� Ȯ���Ͽ� ���� ������ ������ �� ������ versionID�� �˻��Ͽ�\n
 *			pOdbcParam->newVersionID�� �������ش�. \n
 *			Upgrade�� version�� ���� ��� 0�� return �ǰ� upgrade�� version��\n
 *			��ϵǾ� ������ �� version�� return�ϹǷ� >0���� �ȴ�.
 *			- application, version�� versionID�� ���ϰ� ���� application�� ����\n
 *			  ���� version�� ��ϵǾ� �ִ��� ���� Ȯ��
 *****************************************************************************/
int	CheckVersion(ODBC_PARAMETER *pOdbcParam, SQLCHAR *application, int version)
{
    SQLRETURN		returncode;
    SQLUINTEGER		newVersion;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1);
    
    // 1. versionID ���ϴ� SQL ���� ����
    sprintf((char *)Statement,
            "select VersionID from openMSYNC_Version "
            "where Application = '%s' and Version = %d", 
            application, version);
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion))<0 ||
        returncode == SQL_NO_DATA)
    {
        if(returncode == SQL_NO_DATA)
            ErrLog("SYSTEM [%d];Application '%s'�� ���� '%d'��" 
                   "���� ������ ã�� �� �����ϴ�. ;\n", 
                   pOdbcParam->order, application, version);
        ret = -1;
        goto CheckVersion_END;
    }	
    pOdbcParam->VersionID = newVersion;
    
    // 2. ���� ���� version�� �ִ��� ���ϴ� SQL ���� ����
    sprintf((char *)Statement,
            "select max(Version) from openMSYNC_Version"
            " where Application = '%s' and Version > %d", 
            application, version);
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion)) <0)
    {
        ret = -1;
        goto CheckVersion_END;
    }	
    // ������ 0�� return
    if(returncode == SQL_NO_DATA)		goto CheckVersion_END;
    
    // 3. ���� ���� version�� versionID ���ϴ� SQL ���� ����
    sprintf((char *)Statement,
            "select VersionID from openMSYNC_Version"
            "where application = '%s' and version = %d", 
            application, newVersion);
    ret = newVersion;
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion)) <0 ||
       returncode == SQL_NO_DATA)
    {
        if(returncode == SQL_NO_DATA)
            ErrLog("SYSTEM [%d];���׷��̵带 ���� Application"
                   "'%s'�� ���� '%d'�� ���� ������ ã�� �� �����ϴ�.;\n", 
                   pOdbcParam->order, application, newVersion);
        ret = -1;
        goto CheckVersion_END;
    }	

    // 4. ���ο� version�� return�ϰ� versionID�� ����
    pOdbcParam->newVersionID = newVersion;
    
CheckVersion_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

/*****************************************************************************/
//! GetDBConnectTime 
 /*****************************************************************************/
/*! \breif	2003.2.18 fileTime�� dbConnectionTime�� ��� ����\n
 *			DBSYNC�� ��� pOdbcParam->connectTime, FILESYNC�� ��� pOdbcParam->fileTime
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->versionID
 * \param	hstmt(in)		: 
 * \param	flag(in)		:
 * \param	connectTime(in)	:
 ************************************
 * \return	<0, on error \n
 *			>0, on versionID�� ã�� ��� new version ID \n
 *			=0, upgrade�� version�� ���� ��� \n
 ************************************
 * \note	���� DB�� system time�� ���ϴ� �Լ��� �� �����ͺ��̽��� �´� \n
 *			������ �����Ͽ� ���� �ð��� ���Ѵ�. �ð��� ������ ���ۿ�\n
 *			�޸𸮸� �Ҵ��� �� '9999-99-99'�� �ʱ�ȭ���ְ� \n
 *			���� DB(���� �����ͺ��̽� �ý����� ����DSN)�� �����ؼ� ����ȭ��\n
 *			�ϴ� ��� �� DB�� system time�� �������� ���� ���� �����Ƿ� ��\n
 *			DB�� system time �߿� ���� ���� ���� ������ �� �ֵ��� ���� ����\n
 *			���� ������ �����ϴ� ���� �ƴ϶� ���Ͽ� ������ ������ �����Ѵ�.\n
 *			�̴� �� ���� ����ȭ ������ ���� �ڿ� ���� ����ȭ �ð�����\n
 *			����Ǵµ� ���� ����ȭ ��û ��, ����� �����͸� ����ȭ �ϱ�\n
 *			���� �˻� ���ǿ� ���̱� �����̴�.
 *****************************************************************************/
int	GetDBConnectTime(ODBC_PARAMETER *pOdbcParam, SQLHSTMT *hstmt, int flag, char **connectTime)
{
    SQLINTEGER		DConnectTimeOrInd;		
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    char			*date = NULL;
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, flag) < 0) 
        return (-1);
    
    // 1. ���� �ð��� �о���� SQL ���� ����
    if(pOdbcParam->dbType == DBSRV_TYPE_ORACLE)		
        sprintf((char *)Statement,"select SYSDATE from dual");
    else if(pOdbcParam->dbType == DBSRV_TYPE_MSSQL || 
            pOdbcParam->dbType == DBSRV_TYPE_SYBASE )
        sprintf((char *)Statement,"select getdate()");		
    else if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL)
        sprintf((char *)Statement,"select sysdate()");		
    else if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS)
        sprintf((char *)Statement,"select Now()");	
    else if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
        sprintf((char *)Statement,"select SYSDATE");	
    else if(pOdbcParam->dbType == DBSRV_TYPE_DB2)
        sprintf((char *)Statement,"SELECT CURRENT DATE FROM SYSIBM.SYSDUMMY1");
    
    returncode = SQLExecDirect(*hstmt, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(*hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetDBTime_END2;
    }

    // 2. column Binding
    SQLDescribeCol(*hstmt, 
                   1, (unsigned char *)szBuffer, sizeof(szBuffer), &szBufferLen,
                   &SQLDataType, &ColumnSize, &DecimalDigits, &Nullable);
    if(*connectTime == NULL)
    {
        *connectTime = (char *)calloc(ColumnSize+1, sizeof(char));		
        if (*connectTime==NULL)
        {
            ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
                pOdbcParam->order, ColumnSize+1);
            ret = -1;
            goto GetDBTime_END2;
        }
        sprintf(*connectTime, "9999-99-99");
    }
    
    date = (char *)calloc(ColumnSize+1, sizeof(char));
    if (date==NULL)
    {
        ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
            pOdbcParam->order, ColumnSize+1);
        ret = -1;
        goto GetDBTime_END2;
    }	
    SQLBindCol(*hstmt, 
        1, SQL_C_CHAR, date, (ColumnSize+1)*sizeof(char), &DConnectTimeOrInd);
    // 3. SQL Fetch
    returncode = SQLFetch(*hstmt) ;	
    // SQL_ERROR�� return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(*hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        
        ret = -1;
        goto GetDBTime_END1;
    }	
    if(strcmp(*connectTime, date)>0)
        strcpy(*connectTime, date);
    
GetDBTime_END1:
    free(date);	
GetDBTime_END2:
    FreeStmtHandle(pOdbcParam, flag);
    return (ret);
}

/*****************************************************************************/
//! GetStatement 
 /*****************************************************************************/
/*! \breif	�ش� event�� statement�� ���ϴ� �Լ�
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: �ش� �̺�Ʈ�� statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 *			=0, upgrade�� version�� ���� ���
 *			statement�� ���� ��� SQL_NO_DATA�� return���ش�.
 ************************************
 * \note	�ش� event�� statement�� ���ϴ� �Լ��� statement�� ���� ���\n
 *			SQL_NO_DATA�� return���ش�.
 *****************************************************************************/
int	GetStatement(ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLCHAR		Query[QUERY_MAX_LEN + 1];
    char		buff[QUERY_MAX_LEN + 1];
    SQLRETURN	returncode;
    int			RowCount=0;
    SDWORD		StmtLen;
    int			ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)	
        return	(-1);
    
    sprintf((char *)Query, "SELECT Script ");
    strcat((char *)Query, "FROM openMSYNC_Script S, openMSYNC_Table T ");
    strcat((char *)Query, "WHERE T.TableID = S.TableID ");
    if( pOdbcParam->dbType == DBSRV_TYPE_ORACLE || 
        pOdbcParam->dbType == DBSRV_TYPE_DB2 )
    {   // upper case�� �ٲٵ� ���� ���� �״�� ����
        char *tableUp;
        tableUp = _strupr(_strdup(pOdbcParam->tableName));
        sprintf((char *)buff, "AND T.CliTableName ='%s' ", tableUp);
        free(tableUp);
    }
    else
    {
        sprintf((char *)buff, 
            "AND T.CliTableName ='%s' ", pOdbcParam->tableName);
    }    
    strcat((char *)Query, buff);
    
    sprintf((char *)buff, "AND T.DBID = %d ", pOdbcParam->DBID);
    strcat((char *)Query, buff);
    
    if(pOdbcParam->dbType == DBSRV_TYPE_SYBASE)
        sprintf((char *)buff, "AND S.Event = upper('%c')", pOdbcParam->event);
    else
        sprintf((char *)buff, "AND S.Event = '%c'", pOdbcParam->event);

    strcat((char *)Query, buff);
    
    // ������ �ҹ��ڷ�             //
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Query, SQL_NTS);		
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System,
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetStmt_END;
    }
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_CHAR, Statement, SCRIPT_LEN+1, &StmtLen);
    
    returncode = SQLFetch(pOdbcParam->hstmt4System) ;	
    if (returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode!=SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System,
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        
        ret = -1;
        goto GetStmt_END;
    }
    /* �ش� script�� ���� ��� */
    if (returncode == SQL_NO_DATA) 
        ret = SQL_NO_DATA;
    
GetStmt_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return	(ret);	
}

/*****************************************************************************/
//! BindColumns 
/*****************************************************************************/
/*! \breif	�ش� event�� statement�� ���ϴ� �Լ�
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: �ش� �̺�Ʈ�� statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 *			=0, upgrade�� version�� ���� ���
 ************************************
 * \note	Select a, b, c from table; �������� �����͸� fetch�ؿ� ��\n
 *			a, b, c �ʵ��� ��(column)�� ������ ������ �ʿ��ϴ�. \n
 *			Fetch ���࿡ �ռ� ��Ű�� ������ �̿��Ͽ� �� �ʵ��� type�� \n
 *			���ؼ�(SQLDescribeCol()) ������ ũ���� �޸𸮸� �Ҵ��� �ڿ�\n
 *			�� column�� bind �����ش�(SQLBindCol()).
 *****************************************************************************/
int	BindColumns(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam)
{
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		i, SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    SQLRETURN		returncode;
    
    pOdbcParam->ColumnArray = 
        (SQLPOINTER *) malloc(pOdbcParam->NumCols * sizeof(SQLPOINTER));
    pOdbcParam->ColumnLenArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumCols * sizeof(SQLINTEGER));
    pOdbcParam->ColumnLenOrIndArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumCols * sizeof(SQLINTEGER));
    
    for (i = 0 ; i < pOdbcParam->NumCols; i++) 
        pOdbcParam->ColumnArray[i] = NULL;
    
    for (i = 0 ; i < pOdbcParam->NumCols; i++) 
    {
        returncode = SQLDescribeCol(hstmt, 
                                    i + 1, 
                                    (unsigned char *)szBuffer, 
                                    sizeof(szBuffer), 
                                    &szBufferLen, 
                                    &SQLDataType, 
                                    &ColumnSize, 
                                    &DecimalDigits, 
                                    &Nullable);
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        
        // TODO: 
        //   �ӽ÷� varchar, char Ÿ���� �ʵ��� binding ������ ũ��� 
        //   TEXT_TYPE_LEN ���� ���, ������ ũ��� ���� �ʿ� ��.
        if (SQLDataType == SQL_LONGVARCHAR || SQLDataType == SQL_WLONGVARCHAR)
            ColumnSize = TEXT_TYPE_LEN;        
        else if (SQLDataType == SQL_TYPE_TIMESTAMP)
            ColumnSize = TIMESTAMP_TYPE_LEN;

        // TODO:
        //   ColumnSize�� DataType���κ��� ������ char buffer�� ũ�⸦ ���Ѵ�.
        /*
        if( SQLDataType == SQL_WCHAR || SQLDataType == SQL_WVARCHAR )
        {
            pOdbcParam->ColumnLenArray[i] = (ColumnSize+1)*sizeof(SQLWCHAR);
        }
        else
        {
            ;
        }
        */
        pOdbcParam->ColumnLenArray[i] = ColumnSize*2+4;
        
        pOdbcParam->ColumnArray[i] = 
            (SQLPOINTER *)calloc(pOdbcParam->ColumnLenArray[i], sizeof(char));
        
        if(pOdbcParam->ColumnArray[i]==NULL)
        {
            ErrLog("%d_%s;Memory Allocation Error : size = %dbytes;\n", 
                   pOdbcParam->order, pOdbcParam->uid, ColumnSize+4);
            return	-1;
        }
        
        // SQL ���� �� ������� �޾ƿ� ������ �غ��Ͽ� ������� binding
        // TODO:
        //   Stored Procedure ���� 4��° ������ ������� ������ ������
        //   ũŰ ������ ������ �߻� ���� �ʿ�.
        returncode = SQLBindCol(hstmt, 
                                i + 1, 
                                SQL_C_CHAR, 
                                pOdbcParam->ColumnArray[i], 
                                pOdbcParam->ColumnLenArray[i], 
                                &(pOdbcParam->ColumnLenOrIndArray[i]));
                
        if(returncode != SQL_SUCCESS && 
           returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
    }
    
    return	0;
}

/*****************************************************************************/
//! BindParameters 
/*****************************************************************************/
/*! \breif	�ش� event�� statement�� ���ϴ� �Լ�
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: �ش� �̺�Ʈ�� statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 *			=0, upgrade�� version�� ���� ���
 ************************************
 * \note	Select a from table where b=? and c=? �����̳�\n
 *			insert into table(b, c) values(?, ?) ������ ������ �� b, c�� \n
 *			�ش��ϴ� ������ parameter�� �Ѵ�. Column�� ���������� �� ���� \n
 *			������ �� ������ �ʿ��ϸ� sql ������ prepare �ϰ� �ݺ� ������ \n
 *			���� �� ���� ���� �ٲ� �� �� �ֵ��� bind ���ѵ� �ʿ䰡 �ִ�. 
 *			BindColumns()�� ���������� SQLDescribeParam() �Լ��� �̿��Ͽ�\n
 *			�Ķ������ ������ ũ�⸦ ���� �޸𸮸� �Ҵ��ϰ� SQLBindParameter() 
 *			�Լ��� �̿��Ͽ� bind �����ش�.
 *****************************************************************************/
int	BindParameters(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam)
{
    SQLSMALLINT	i;	
    SQLSMALLINT	SQLDataType, DecimalDigits, Nullable;
    SQLUINTEGER	ParamSize;
    SQLRETURN	returncode;
    
    pOdbcParam->ParamArray = 
        (SQLPOINTER *) malloc(pOdbcParam->NumParams * sizeof(SQLPOINTER));
    pOdbcParam->ParamLenArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumParams * sizeof(SQLINTEGER));
    pOdbcParam->ParamLenOrIndArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumParams * sizeof(SQLINTEGER));
    
    for(i=0 ; i<pOdbcParam->NumParams; i++) 
        pOdbcParam->ParamArray[i] = NULL;
    
    for (i = 0; i < pOdbcParam->NumParams; i++) 
    {
        // Describe the parameter.        
        returncode = SQLDescribeParam(hstmt, 
                                      i + 1, 
                                      &SQLDataType, 
                                      &ParamSize, 
                                      &DecimalDigits, 
                                      &Nullable);
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        
        if (SQLDataType == SQL_LONGVARCHAR || SQLDataType == SQL_WLONGVARCHAR)
            ParamSize = TEXT_TYPE_LEN;
        else if (SQLDataType == SQL_TYPE_TIMESTAMP)
        {            
            // ���� �������� type�� DATE�� ���
            // SQLDescribeParam�Ŀ� ������(where��)�� ��� VARCHAR�� ���������� 
            // �������� ��� SQL_TYPE_TIMESTAMP���� ���������� �ؼ� 
            // script�� DATE type�� ��� to_date API�� �̿��ؼ� ó���ؾ� ��.
            if(pOdbcParam->dbType == DBSRV_TYPE_ORACLE)
            {
                ParamSize = 999;
                SQLDataType = SQL_VARCHAR;
            }
            else
                ParamSize = TIMESTAMP_TYPE_LEN;
        }
		else if (pOdbcParam->dbType == DBSRV_TYPE_SYBASE)
		{
			if (SQLDataType == SQL_NTS)
			{
				ParamSize = 255;
			}
		}
                
        if( SQLDataType == SQL_WCHAR || SQLDataType == SQL_WVARCHAR )
        {
            pOdbcParam->ParamArray[i] = 
                (SQLPOINTER)calloc(ParamSize+1, sizeof(SQLWCHAR));        
            pOdbcParam->ParamLenArray[i] = ((ParamSize + 1)*sizeof(SQLWCHAR));
        }
        else
        {
            pOdbcParam->ParamArray[i] = 
                (SQLPOINTER)calloc(ParamSize+1, sizeof(char));
            pOdbcParam->ParamLenArray[i] = ParamSize + 1;
        }
                
        if (pOdbcParam->ParamArray[i]==NULL)
        {	
            ErrLog("%d_%s;Memory Allocation Error : size = %dbytes;\n", 
                   pOdbcParam->order, pOdbcParam->uid, ParamSize+1);			
            return	-1;
        }

        pOdbcParam->ParamLenOrIndArray[i] = SQL_NTS;

        // Bind the memory to the parameter.         
        returncode = SQLBindParameter(hstmt, 
                                        i + 1, 
                                        SQL_PARAM_INPUT, 
                                        SQL_C_CHAR, 
                                        SQLDataType, 
                                        ParamSize,	
                                        DecimalDigits, 
                                        pOdbcParam->ParamArray[i], 
                                        pOdbcParam->ParamLenArray[i], 
                                        &(pOdbcParam->ParamLenOrIndArray[i]));
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
    }
    
    return	0;

}

/*****************************************************************************/
//! PrepareStmt
/*****************************************************************************/
/*! \breif	SQL ���� �����ϱ� ���� prepare�� ó�����ִ� �Լ�
 ************************************
 * \param	hstmt(in)		:	
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 * \param	Statement(in)	:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	SQL ������ �����ϱ⿡ �ռ� prepare ���ִ� �Լ��� parameter�� \n
 *			column�� �ִ� ��� �׿� ���� binding ó������ ���ش�.
 *****************************************************************************/
int	PrepareStmt(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    
    returncode = SQLPrepare(hstmt, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        return	-1;
    }
    
    SQLNumParams(hstmt, &(pOdbcParam->NumParams));
    if (pOdbcParam->NumParams > 0)
    {
        if(BindParameters(hstmt, pOdbcParam) < 0)	
            return	-1;
    }
        
    // mySQL�� ��� select �ܿ� BindColumn �ϸ� auto_increment�� ���� ��� 
    // �ϳ��� row�� �����. ���� download �ÿ��� BindColumn�� �ʿ��ϹǷ�
    // select ���� ��쿡�� ȣ���ϵ� �Ѵ�.
    // stored procedure�� �����ϱ� ���ؼ� {�� �߰�
    if(!strnicmp((char *)Statement, "select", 6) || 
       !strncmp((char *)Statement, "{", 1)          )
    {
        SQLNumResultCols(hstmt, &(pOdbcParam->NumCols));
        if (pOdbcParam->NumCols > 0) 
        {
            if (BindColumns(hstmt, pOdbcParam) < 0)	
                return -1;
        }
    }
    else 
        pOdbcParam->NumCols = 0;

    return 0;
}

/*****************************************************************************/
//! GetParameters
/*****************************************************************************/
/*! \breif	delimeter�� ���� field�� parsing�ϰ� pOdbcParam->ParamArray[]�� �־��ش�.
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 * \param	ptr(in)			: �ϳ��� ����
 * \param	dataLength(in)	:	
 * \param	gnIdx(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	Client�κ��� upload ���� data stream���κ��� FIELD_DEL�� \n
 *			RECORD_DEL�� �̿��Ͽ� �� �ʵ� ���� parsing ���ִ� �Լ��� parsing\n
 *			�� �� �ʵ� ������ BindParameters()�� ���� binding �� ������ \n
 *			pOdbcParam->ParamArray[]�� ����ȴ�.
 *****************************************************************************/
int	
GetParameters(ODBC_PARAMETER *pOdbcParam, char *ptr, int dataLength, int *gnIdx)
{
    int i;
    int Idx = *gnIdx, s_Idx = *gnIdx;
    char		field[HUGE_STRING_LENGTH+1];
    int	length;
    
    for(i=0 ; i<pOdbcParam->NumParams && Idx < dataLength; )
    {
        while(ptr[Idx]!=FIELD_DEL && ptr[Idx]!=RECORD_DEL && Idx<dataLength)
            Idx++;

        memset(field, 0x00, HUGE_STRING_LENGTH+1);
        length = Idx-s_Idx;
        strncpy(field, ptr+s_Idx, length);
        field[length] = 0x00;
        if((int)strlen(field)>(pOdbcParam->ParamLenArray[i]-1)) 
        {
            ErrLog("%s;The size of data in DB is %d. "
                   "But the size of data[%s] from %s is %d;\n", 
                    pOdbcParam->uid, pOdbcParam->ParamLenArray[i]-1, 
                    field, pOdbcParam->uid, strlen(field));
            return (-1);
        }
        strcpy((char *)pOdbcParam->ParamArray[i++], field);
        if(ptr[Idx] == RECORD_DEL) 
            break;
        Idx++;		// delimeter skip
        s_Idx = Idx;		
    }

    if(pOdbcParam->event == DOWNLOAD_FLAG ||
       pOdbcParam->event == DOWNLOAD_DELETE_FLAG)
    {
        if (i == pOdbcParam->NumParams-1)
        {
            if(pOdbcParam->syncFlag == ALL_SYNC )
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    strcpy((char *)pOdbcParam->ParamArray[i++], "1970-01-01");
                }
                else
                {
                    strcpy((char *)pOdbcParam->ParamArray[i++], "1900-01-01");
                }
            }
            else
            {
                strcpy((char *)pOdbcParam->ParamArray[i++], 
                       pOdbcParam->lastSyncTime);
            }
        }
    }
    else  if(ptr[Idx] != RECORD_DEL)
    {
        ErrLog("%d_%s;The format of data is wrong."
               " There is no Record Delimeter.;\n",
               pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }
    
    if(i!=pOdbcParam->NumParams)
    {
        ErrLog("%d_%s;[SCRIPT ERROR] Client���� ���� parameter�� ������"
            " script�� ���ǵ� parameter�� ������ ���� �ʽ��ϴ�.;\n", 
            pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }

#ifdef DEBUG
    for(i=0 ; i<pOdbcParam->NumParams ; i++)
        ErrLog("%s;%s/(%d);\n",pOdbcParam->uid,pOdbcParam->ParamArray[i], 
               strlen((char *)pOdbcParam->ParamArray[i]));
#endif

    // break�� ���������� RECORD_DEL�� pointing �ϹǷ� ���� index ���� 1 ����
    *gnIdx = Idx+1;  
    return (0);
}

/*****************************************************************************/
//! FreeArrays
/*****************************************************************************/
/*! \breif	BindParameters()�� BindColumns()�� ���� �Ҵ��� �޸𸮵��� ����
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 ************************************
 * \return	void
  ************************************
 * \note	BindParameters()�� BindColumns()�� ���� �Ҵ��� �޸𸮵��� �������ش�.
 *****************************************************************************/
void FreeArrays(ODBC_PARAMETER *pOdbcParam)
{
    int	i;
    if (pOdbcParam->NumParams)
    {
        for (i = 0; i < pOdbcParam->NumParams; i++)
        {
            if(pOdbcParam->ParamArray[i])	
                free(pOdbcParam->ParamArray[i]);
        }
        free(pOdbcParam->ParamArray);
        free(pOdbcParam->ParamLenArray);
        free(pOdbcParam->ParamLenOrIndArray);
    }
    if (pOdbcParam->NumCols)
    {
        for (i = 0; i < pOdbcParam->NumCols; i++)	
        {
            if(pOdbcParam->ColumnArray[i])
                free(pOdbcParam->ColumnArray[i]);
        }
        free(pOdbcParam->ColumnArray);
        free(pOdbcParam->ColumnLenArray);
        free(pOdbcParam->ColumnLenOrIndArray);
    }
}

/*****************************************************************************/
//! ReplaceParameterToData
/*****************************************************************************/
/*! \breif	delimeter�� ���� field�� parsing �ϰ� ? �κп� �־� statement�� �ٲ��ش�.
 ************************************
 * \param	pOdbcParam(in)		: 
 * \param	Statement(in)		: �ϳ��� ����
 * \param	newStatement(out)	:	
 * \param	ptr(out)				:	
 * \param	dataLength(in)		:
 * \param	gnIdx(in)			:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	�����ͺ��̽��� ACCESS�� ��� SQLDescribeParam() �Լ��� ODBC���� \n
 *			�������� �ʱ� ������ '?'�� �ش��ϴ� �κп� ���� ���� assign �ؼ�\n
 *			�������־�� �Ѵ�. �̷� �� ȣ��Ǵ� �Լ��� data stream�� \n
 *			parsing �Ͽ� �� �ʵ� ���� ���� ���忡 ���� �����ؼ� ���ο� \n
 *			statement�� �ٲ��ش�. 
 *****************************************************************************/
int 
ReplaceParameterToData(ODBC_PARAMETER *pOdbcParam, 
                       SQLCHAR *Statement, SQLCHAR *newStatement, 
                       char *ptr, int dataLength, int *gnIdx)
{	
    int Idx = *gnIdx, s_Idx = *gnIdx;
    char field[HUGE_STRING_LENGTH+1];
    char tmpStatement[HUGE_STRING_LENGTH+1];
    int	length;
    char *sptr;
    
    strcpy(tmpStatement, (char *)Statement);
    sptr = strtok(tmpStatement, "?");
    sprintf((char *)newStatement, "%s", sptr);
    
    for( ; sptr && Idx < dataLength; )
    {
        while(ptr[Idx]!=FIELD_DEL && ptr[Idx]!=RECORD_DEL && Idx<dataLength)
            Idx++;

        memset(field, 0x00, HUGE_STRING_LENGTH+1);
        length = Idx-s_Idx;
        strncpy(field, ptr+s_Idx, length);
        field[length] = 0x00;
        sptr = strtok(NULL, "?");
        sprintf((char *)newStatement, "%s%s", newStatement, field);
        if(sptr) 
            strcat((char *)newStatement, sptr);        
        
        if(ptr[Idx] == RECORD_DEL) 
            break;
        Idx++;		// delimeter skip
        s_Idx = Idx;
    }	

    if(pOdbcParam->event == DOWNLOAD_FLAG || 
       pOdbcParam->event == DOWNLOAD_DELETE_FLAG)
    {
        sptr = strtok(NULL, "?");
        if (sptr) 
        {
            if(pOdbcParam->syncFlag == ALL_SYNC)
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    sprintf((char *)newStatement, 
                            "%s1970-01-01%s", newStatement, sptr);
                }
                else
                {
                    sprintf((char *)newStatement, 
                            "%s1900-01-01%s", newStatement, sptr);
                }
            }
            else
            {
                sprintf((char *)newStatement, 
                        "%s%s%s", newStatement, pOdbcParam->lastSyncTime, sptr);
            }

            sptr = strtok(NULL, "?");
        }
    }

    // break�� ���������� RECORD_DEL�� pointing �ϹǷ� ���� index ���� 1 ����
    *gnIdx = Idx+1;  
    return (0);
}

/*****************************************************************************/
//! ProcessStmt
/*****************************************************************************/
/*! \breif	statement�� ����� �ٸ��ִ� �κ�
 ************************************
 * \param	dataBuffer(in)		: 
 * \param	dataLength(in)		: �ϳ��� ����
 * \param	pOdbcParam(out)		:	
 * \param	Statement(om)		:	
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	�� �̺�Ʈ�� �ش��ϴ� statement�� ���� �� statement handle�� �ϳ�\n
 *			�Ҵ� �޾� SQL ������ prepare �ϰ� parameter�� columns�� binding\n
 *			�� �� �������ش�.
 *			�����ͺ��̽��� ACCESS�� ���� SQLPrepare()�� ������ prepare \n
 *			���ְ� parameter �� ������ ODBC �Լ��� ���� ���� �� �����Ƿ� \n
 *			ReplaceParameterToData()�Լ��� �̿��Ͽ� '?' ���� ������ �ʵ� \n
 *			������ ġȯ�� ������ ��� SQLExecDirect ()�� �������ش�. 
 *			�� �� �����ͺ��̽��� ���ؼ��� PrepareStmt() �Լ��� �̿��Ͽ� \n
 *			parameter�� column���� binding ���ְ� GetParameters() �Լ��� \n
 *			�̿��Ͽ� binding �� ���ۿ� ������ ������ assign �� �� \n
 *			SQLExecute() �Լ��� �������ش�. ��, SQL ������ stored procedure��\n 
 *			���� �ѹ� alloc�� statement handle�� ��� �̿��� �� �����Ƿ� \n
 *			�Ź� free�ϰ� �ٽ� alloc, prepare, execute�� �ϵ��� �Ѵ�.
 *****************************************************************************/
int	ProcessStmt(char *dataBuffer, int dataLength, ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    int		record_count =0;
    int		gnIdx = 0;
    int		ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, PROCESSING) < 0) 
        return (-1);
        
    if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
    {    
        SQLCHAR newStatement[HUGE_STRING_LENGTH+1];
        
        returncode = 
            SQLPrepare(pOdbcParam->hstmt4Processing, Statement, SQL_NTS);
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hstmt4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        SQLNumParams(pOdbcParam->hstmt4Processing, &(pOdbcParam->NumParams));
        pOdbcParam->NumCols = 0;

        // SQLDescribeParam() �Լ��� Access & Sybase������ �������� �����Ƿ�
        // parameter�� �ش��ϴ� ?�� ���� ���� �����ͷ� ä���� 
        // statement�� ���� �� SQLExecDirect()�Լ��� �������ֵ��� �Ѵ�.
        while(gnIdx < dataLength)
        {		
            if(ReplaceParameterToData(pOdbcParam, Statement, newStatement, 
                                      dataBuffer, dataLength, &gnIdx) < 0  )
            {
                ret = -1;
                pOdbcParam->NumParams = 0;
                goto ProcessStmt_END;
            }			
            
            // ������ �ҹ��ڷ�               
            returncode = SQLExecDirect(pOdbcParam->hstmt4Processing, 
                                       newStatement, SQL_NTS);
            
            if(returncode!=SQL_SUCCESS && 
               returncode!=SQL_SUCCESS_WITH_INFO && returncode!=SQL_NO_DATA)
            {
                DisplayError(pOdbcParam->hstmt4Processing, 
                             pOdbcParam->uid, pOdbcParam->order, returncode);
                ret = -1;
                pOdbcParam->NumParams = 0;
                goto ProcessStmt_END;
            }		
            record_count++;
        }
        pOdbcParam->NumParams = 0;
    }
    else
    {        
        if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement)<0)
        {
            ret = -1;
            goto ProcessStmt_END;
        }
        
        while (gnIdx < dataLength)
        {		
            if(pOdbcParam->NumParams) 
            {
                if(GetParameters(pOdbcParam, dataBuffer, dataLength, &gnIdx)<0)
                {
                    ret = -1;
                    goto ProcessStmt_END;
                }
            }

            returncode = SQLExecute(pOdbcParam->hstmt4Processing);
            if(returncode != SQL_SUCCESS && 
               returncode != SQL_SUCCESS_WITH_INFO && 
               returncode != SQL_NO_DATA)
            {
                DisplayError(pOdbcParam->hstmt4Processing, 
                             pOdbcParam->uid, pOdbcParam->order, returncode);
                ret = -1;
                goto ProcessStmt_END;
            }		

            record_count++;

            // upload script�� parameter�� ���� ��� �ѹ� ������ �ϰ� ����
            if(!pOdbcParam->NumParams)
                break; 

           // Upload�� stored procedure�� ��� 
           // SQLPrepare, SQLExecute�� ��� �� �� �����Ƿ� Free ���ְ� 
           // �ٽ� Alloc, Prepare, Execute�� �ݺ��Ѵ� 
            if(pOdbcParam->dbType == DBSRV_TYPE_MSSQL)
            {		
                if(!strncmp((char *)Statement, "{", 1))
                { 
                    FreeStmtHandle(pOdbcParam, PROCESSING);
                    FreeArrays(pOdbcParam);	
                    if (AllocStmtHandle(pOdbcParam, PROCESSING) < 0) 
                        return (-1);
                    if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement) < 0) 
                    {
                        ret = -1;
                        goto ProcessStmt_END;
                    }
                }
            }
        } 
    }

    if(pOdbcParam->event==UPDATE_FLAG)
        ErrLog("%d_%s;Upload_Update : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);
    else if(pOdbcParam->event==DELETE_FLAG)
        ErrLog("%d_%s;Upload_Delete : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);
    else if(pOdbcParam->event==INSERT_FLAG)
        ErrLog("%d_%s;Upload_Insert : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);	
    
ProcessStmt_END:
    FreeStmtHandle(pOdbcParam, PROCESSING);
    FreeArrays(pOdbcParam);
    return	(ret);
}

/*****************************************************************************/
//! FetchResult
/*****************************************************************************/
/*! \breif	recordBuffer��, recordBuffer�� query�� ���� ����� �����ش�.
 ************************************
 * \param	pOdbcParam(in)		: DB�� ������ �ִ� ������
 * \param	recordBuffer(out)	: ����
 * \param	recordBuffer(out)	: ���� 
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 *			-1(error), 1(find), SQL_NO_DATA (no_file)
 ************************************
 * \note	PrepareStmt() �Լ��� prepare, execute �� cursor�κ��� �����͸�\n
 *			�ϳ��� fetch�Ͽ� pOdbcParam->ColumnArray[]�� ��� �ִ� �� �ʵ� \n
 *			������ recordBuffer�� ���ش�. �� �ʵ带 �� ������ FIELD_DEL�� \n
 *			�̿��ϰ� recordBuffer�� client�� ���� sendBuffer�� �� ������ \n
 *			RECORD_DEL�� �����ڷ� �̿��Ѵ�. 
 *			SendBuffer�� HUGE_STRING_LENGTH (8192 byte) ������ ũ�� 1�� \n
 *			return �Ͽ� socket�� ���� client�� �����͸� ������ �� �ֵ��� �Ѵ�. 
 *			�� �̻� ���� �����Ͱ� ���� �ÿ� 2�� return ���ش�.
 *****************************************************************************/
int	FetchResult(ODBC_PARAMETER *pOdbcParam, char *sendBuffer, char *recordBuffer)
{
    SQLRETURN	returncode;
    int			nByte=0;
    int			i;
    
    if (pOdbcParam->NumRows > 0)
    {  // ������ fetch�� ����Ÿ�� ī���Ѵ�. ù��° call�� ���� skip
        sprintf(sendBuffer, "%s", recordBuffer);
        nByte = strlen(recordBuffer);	
    }
    memset(recordBuffer, '\0', HUGE_STRING_LENGTH+1);	
    // download data fetch 
    while((returncode = SQLFetch(pOdbcParam->hstmt4Processing))!=SQL_NO_DATA &&
          (returncode == SQL_SUCCESS) ) 
    {	
        for (i = 0; i < pOdbcParam->NumCols; i++) 
        {
            delete_space((char *)pOdbcParam->ColumnArray[i], 
                         pOdbcParam->ColumnLenOrIndArray[i]);
            strcat(recordBuffer, (char *)pOdbcParam->ColumnArray[i]);
            sprintf(recordBuffer, "%s%c", recordBuffer, FIELD_DEL);            
            memset(pOdbcParam->ColumnArray[i], 
                   0x00, pOdbcParam->ColumnLenArray[i]);
        }
        
        recordBuffer[strlen(recordBuffer)-1] = RECORD_DEL;
        pOdbcParam->NumRows++;
        
        if (nByte+strlen(recordBuffer) > HUGE_STRING_LENGTH)	
            break;
        nByte += strlen(recordBuffer);
        
        strcat(sendBuffer, recordBuffer);
        memset(recordBuffer, '\0', HUGE_STRING_LENGTH+1);
    }
    
    if (returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);        
        return -1;
    }

    if (returncode == SQL_NO_DATA && nByte == 0)	
        return	2;
    else
        return (1);
}

/*****************************************************************************/
//! GetSyncTime
/*****************************************************************************/
/*! \breif	�޵� parameter�� �ִ� ��� GetParameters()�� ȣ���Ͽ� ���� ���ϰ� ����\n
 *			���� syncTime�� assign
 ************************************
 * \param	pOdbcParam(out)	:	
 * \param	ptr(in)			: parameter �� or NO_PARAM)
 * \param	dataLength(in)	:
 * \param	mode(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	Download�� script�� prepare �� �� ȣ��Ǵ� �Լ��� select ���忡\n
 *			���� ���� ���� ��� pOdbcParam->NumParams ���� 0�̹Ƿ� �ٷ� \n
 *			return �ǰ� ���� ���� ������ client�κ��� ���� ���� parameter�� \n
 *			���� ���(NO_PARAM�� ���޵�), parameter�� ������ �� ����� \n
 *			�ڵ����� sync time�� �־� �ִ� �۾��� �Ѵ�. 
 *			���� parameter�� ������ �� ���� �ƴѵ� client�κ��� ���� ���� \n
 *			parameter�� ���� ���� error ó���Ѵ�.
 *			����, ���� ���� parameter�� �ִ� ���� GetParameters() �Լ��� \n
 *			�̿��Ͽ� parameter�� binding �����ش�.
 *****************************************************************************/
int	 
GetSyncTime(ODBC_PARAMETER *pOdbcParam, char *ptr, int dataLength, int mode)
{
    int gnIdx=0;
    
    if (pOdbcParam->NumParams == 0)		
        return 0;  // SyncTime ���� ��ü sync �ϴ� �ɷ� ������ �� ���

    if (!strcmp(ptr, "NO_PARAM"))
    {  // ���� ���� parameter�� ���� ���
        if (pOdbcParam->NumParams == 1)
        { // �ϳ��� syncTime�� �־� �ش�.
            if(mode == PROCESSING)
            {
                if(pOdbcParam->syncFlag == ALL_SYNC)	
                {
                    if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                        strcpy((char *)pOdbcParam->ParamArray[0], "1970-01-01");
                    else
                        strcpy((char *)pOdbcParam->ParamArray[0], "1900-01-01");
                }
                else
                {
                    strcpy((char *)pOdbcParam->ParamArray[0], 
                            pOdbcParam->lastSyncTime);
                }
            }
        }
        else
        { // script�� parmaeter�� ������ �Ѱ��� �ƴѵ� parameter��
          // ���� ���� �����Ƿ� default�� syncTime�� �־� �༭�� �ȵȴ�.
            ErrLog("%d_%s;[SCRIPT ERROR] PDA�κ��� ���� parameter�� ������" 
                   " script�� ���ǵ� parameter�� ������ ���� �ʽ��ϴ�.;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
            return	(-1);	
        }
    }
    else
    {   // ���� ���� parameter�� �ִ� ���
        if(GetParameters(pOdbcParam, ptr, dataLength, &gnIdx)<0) 
            return -1;
    }
    
    return (0);
}

/*****************************************************************************/
//! PrepareDownload
/*****************************************************************************/
/*! \breif	Download�� ȣ��Ǵ� �Լ�
 ************************************
 * \param	dataBuffer(out)	: parameter �� or NO_PARAM)
 * \param	pOdbcParam(out)	:	
 * \param	dataLength(in)	:
 * \param	Statement(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	DownloadProcessingMain() �Լ����� �ҷ����� �Լ��� ACCESS\n
 *			�����ͺ��̽��� ��� ���� ���� ������ '?' ���� ������ parameter��\n
 *			ġȯ��Ű��(ReplaceParameterToData()), ���� ���� parameter�� ����\n
 *			���� sync time���� ġȯ���ش�. 
 *			�ٸ� �����ͺ��̽��� PrepareStmt()�� GetSyncTime()���� parameter��\n
 *			�����ϰ� binding�� �ڿ� SQLExecute()�� �������ش�.
 *****************************************************************************/
int	
PrepareDownload(char *dataBuffer, 
                ODBC_PARAMETER *pOdbcParam, int dataLength, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    
    pOdbcParam->NumRows = 0;
        
    if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS || 
       pOdbcParam->dbType == DBSRV_TYPE_CUBRID   )
    {        
        int gnIdx =0;
        SQLCHAR newStatement[HUGE_STRING_LENGTH+1];
        char *sptr;
        
        pOdbcParam->NumCols = 0;
        char tmpStatement[HUGE_STRING_LENGTH+1];
        
        if(!strcmp(dataBuffer,"NO_PARAM"))
        {
            // Statement : select * from employee where updateDate>=CDate('?')
            strcpy(tmpStatement, (char *)Statement);
            // select ..... where updatedate>=CDate( ������ �κ� or ?�� ���� ��� ��ü ����
            sptr = strtok(tmpStatement, "?");  
            sprintf((char *)newStatement, "%s", sptr);
            sptr = strtok(NULL, "?");	// ') �κ�
            if(sptr)
            {	// ') �� �ִ� ���
                if(pOdbcParam->syncFlag == ALL_SYNC)
                {
                    if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                    {
                        sprintf((char *)newStatement, 
                                "%s1970-01-01%s", newStatement, sptr);
                    }
                    else
                    {
                        sprintf((char *)newStatement, 
                                "%s1900-01-01%s", newStatement, sptr);
                    }
                }
                else
                {
                    sprintf((char *)newStatement, "%s%s%s", 
                            newStatement, pOdbcParam->lastSyncTime, sptr);
                }

                sptr = strtok(NULL, "?");  // NULL�̾�� ����
                if(sptr)
                {
                    ErrLog("%d_%s;[SCRIPT ERROR] PDA�κ��� ���� parameter��"
                            " ������ script�� ���ǵ� parameter�� "
                            "������ ���� �ʽ��ϴ�.;\n", 
                            pOdbcParam->order, pOdbcParam->uid);
                    return	(-1);
                }
            }
            else if( sptr == NULL )
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    if(pOdbcParam->syncFlag == ALL_SYNC)
                    {
                        sprintf((char *)newStatement, 
                                "%s'1970-01-01'", newStatement);
                    }
                }
            }            
        }
        else 
        {
            if(ReplaceParameterToData(pOdbcParam, Statement, newStatement, 
                                      dataBuffer, dataLength, &gnIdx) < 0 )
            {
                return (-1);	
            }
        }

        if (PrepareStmt(pOdbcParam->hstmt4Processing, 
                        pOdbcParam, newStatement) < 0)
        {
            return	(-1);
        }
    }
    else 
    {
        if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement)<0)
            return	(-1);
        if (GetSyncTime(pOdbcParam, dataBuffer, dataLength, PROCESSING) < 0)
            return	(-1);
    }

    returncode = SQLExecute(pOdbcParam->hstmt4Processing);
    if(returncode != SQL_SUCCESS && 
       returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4Processing, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        return (-1);			
    }
    
    return 0;
}

/*****************************************************************************/
//! GetDSN
/*****************************************************************************/
/*! \breif	cliDSN�� pOdbcParam->VersionID�� ������ ���Ѵ�.
 ************************************
 * \param	pOdbcParam(in,out)	: in(VersionID, cliDSN), out(DBID)
 * \param	dsn(out)			:
 * \param	dsn_uid(out)		:
 * \param	dsn_passwd(out)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID�� ã�� ��� new version ID
 ************************************
 * \note	mSync_SetDSN()���� �Է¹��� dsn�� ������ ��� �Լ��� server�� \n
 *			DSN, DB�� ������ ����� id, password ���� ���´�.
 *****************************************************************************/
int	GetDSN(ODBC_PARAMETER *pOdbcParam, 
           char *cliDSN, SQLCHAR *dsn, SQLCHAR *dsn_uid, SQLCHAR *dsn_passwd)
{
    SQLCHAR		Statement[QUERY_MAX_LEN + 1];
    SQLRETURN	returncode;
    SQLINTEGER	dsnLenOrInd, dbidLenOrInd, dsnuidLenOrInd, dsnpasswdLenOrInd;
    int ret = 0;
    
    if(AllocStmtHandle(pOdbcParam, SYSTEM)<0) 
        return (-1);
    
    // SQL ���� ����
    sprintf((char *)Statement,
            "select DBID, SvrDSN, DSNUserID, DSNPwd from openMSYNC_DSN "
            "where VersionID = %d and CliDSN = '%s'",
            pOdbcParam->VersionID, cliDSN);	
    
    // ������ �ҹ��ڷ� 
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);    
    if(returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetDSN_END;
    }

    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_ULONG, &(pOdbcParam->DBID), 0, &dbidLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System, 
               2, SQL_C_CHAR, dsn, DSN_LEN+1, &dsnLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System,
               3, SQL_C_CHAR, dsn_uid, DSN_UID_LEN+1, &dsnuidLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System, 
               4, SQL_C_CHAR, dsn_passwd, DSN_PASSWD_LEN+1, &dsnpasswdLenOrInd);
    
    // SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);	

    // SQL_ERROR�� return -1
    if(returncode != SQL_SUCCESS && 
       returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);        
        ret = -1;
        goto GetDSN_END;
    }	
    // openMSYNC_Version table�� ���� application or version
    if(returncode == SQL_NO_DATA) 
    {
        ErrLog("%d_%s;There is no DSN of '%s' which is defined in client;\n", 
               pOdbcParam->order, pOdbcParam->uid, cliDSN);
        ret = -1;
        goto GetDSN_END;
    }
    
    delete_space((char *)dsn, dsnLenOrInd);
    delete_space((char *)dsn_uid, dsnuidLenOrInd);
    delete_space((char *)dsn_passwd, dsnpasswdLenOrInd);
    
GetDSN_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

