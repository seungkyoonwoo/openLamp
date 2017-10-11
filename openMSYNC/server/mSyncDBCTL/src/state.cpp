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

#include <stdio.h>
#include <Windows.h>

#include "Sync.h"
#include "state.h"

struct S_STATEFUNC g_stateFunc[] = {
/* NONE_STATE = -1,         */    
/* READ_MESSAGE_BODY_STATE, */    { ReadMessageBody,  "ReadMessageBody" },	
/* AUTHENTICATE_USER_STATE, */    { AuthenticateUser, "AuthenticateUser" }, 
/* WRITE_FLAG_STATE,	    */    { WriteFlag,	      "WriteFlag" },		
/* RECEIVE_FLAG_STATE,      */    { ReceiveFlag,      "ReceiveFlag" },	
/* SET_APPLICATION_STATE,   */    { SetApplication,   "SetApplication" },	
/* SET_TABLENAME_STATE,	    */    { SetTableName,     "SetTableName" },		
/* PROCESS_SCRIPT_STATE,	*/    { ProcessScript,    "ProcessScript" },		
/* UPLOAD_OK_STATE,         */    { UploadOK,         "UploadOK" },		
/* DOWNLOAD_PROCESSING_STATE, */  { DownloadProcessingMain,"DownloadProcessingMain" },
/* MAKE_DOWNLOAD_MESSAGE_STATE,*/ { MakeDownloadMessage,   "MakeDownloadMessage" },
/* WRITE_MESSAGE_BODY_STATE, */   { WriteMessageBody,   "WriteMessageBody" },	
/* END_OF_RECORD_STATE = 99,  */
/* END_OF_STATE = 100   */
};


/*****************************************************************************/
//! ReadMessageBody
/*****************************************************************************/
/*! \breif  Packet�� size ��ŭ socket���κ��� packet ������ �д� �Լ�
 ************************************
 * \param	pNetParam(in,out)	: ReadFrNet���� ���
 * \param	pOdbcParam(in)		: not used
 ************************************
 * \return	void
 ************************************
 * \note	Packet�� size ��ŭ socket���κ��� packet ������ �д� �Լ�
 *			output�� pNetParam->receiveBuffer �̴�
 *****************************************************************************/
int ReadMessageBody(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int n ;
    unsigned int    hint;	
    
    n = ReadFrNet(pNetParam->socketFd, (char *)&hint, sizeof(unsigned int));
    if(n<0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive];\n", 
            pOdbcParam->order);
        return(-1);
    }
    else if(n==0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Timeout];\n",
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n!=sizeof(unsigned int))
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
            pOdbcParam->order);
        return(-1) ;
    }
    
    pNetParam->receiveDataLength  = (int)ntohl(hint) ;
    memset(pNetParam->receiveBuffer, '\0', HUGE_STRING_LENGTH+1);
    
    n = ReadFrNet(pNetParam->socketFd, 
        pNetParam->receiveBuffer, pNetParam->receiveDataLength);  
    if(n<0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive];\n", 
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n==0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Timeout];\n",
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n!=pNetParam->receiveDataLength)
    {
        ErrLog("n : %d, pNetParam->receiveDataLength : %d \n", 
            n, pNetParam->receiveDataLength);
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
            pOdbcParam->order);
        return(-1) ;
    }  
    
    pNetParam->receiveBuffer[pNetParam->receiveDataLength] ='\0';
    return(pNetParam->nextState);
}

/*****************************************************************************/
//! AuthenticateUser
/*****************************************************************************/
/*! \breif  Packet�� size ��ŭ socket���κ��� packet ������ �д� �Լ�
 ************************************
 * \param	pNetParam(in)	: receiveBuffer (UserID|PASSWD|application|version)�� input���� ���
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)\n
 *			ACK_FLAG, NACK_FLAG, UPGRADE_FLAG (valid), -1 (error)
 ************************************
 * \note	Socket���κ��� ���� receiveBuffer �ȿ��� \n
 *			userid|password|application|version ������ ��� �ִ�. �� ���� \n
 *			parsing �� �ڿ� ���� ���� ��ϵ� ����� ����MaxUser�� �ʰ����� \n
 *			�ʴ��� Ȯ���ϰ�(CheckCurrentUserCount()) openMSYNC_User table�κ��� \n
 *			userid�� password�� Ȯ���� ��(CheckUserInfo()), application�� \n
 *			version�� üũ�Ѵ�(CheckVersion()).
 *			�̿� ���� üũ ������ ����� �°� ����� ������ �����ϸ� \n
 *			ACK_FLAG, �����ϸ� NACK_FLAG, ����� ���� �ʰ��ϸ� XACK_FLAG, \n
 *			upgrade�� version�� �����ϸ� UPGRADE_FLAG�� client�� �����ϵ���\n
 *			WRITE_FLAG�� return�Ͽ� �ܰ踦 �����Ѵ�. 
 *****************************************************************************/
int AuthenticateUser(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    SQLCHAR		passwd[PASSWD_LEN+1], application[APPLICATION_ID_LEN+1];
    int version;
    int ret;
    
    /* ReceiveBuffer���� UserID|PASSWD|application|version ��� ���� */	
    /* 1. SystemDB open */
    if(OpenSystemDB(pOdbcParam)<0) 
        return (-1);
   
    /* 2. ���� ���� Data(receiveBuffer) Parsing */
    if(ParseData(pNetParam->receiveBuffer, 
                (char *)pOdbcParam->uid, (char *)passwd, 
                (char *)application, &version)<0)
    {	// �� ������ parsing�ؼ� ����
        ErrLog("%d_%s;[Authenticate User]Data Format is Wrong;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }
   
    pNetParam->nextState = RECEIVE_FLAG_STATE;
    
    /* 3. User count check */
    ret = CheckCurrentUserCount(pOdbcParam);
    if(ret<0)  
        return (-1);
    else if(ret == EXCESS_USER)
    {
        pOdbcParam->event = XACK_FLAG;
        return (WRITE_FLAG_STATE);
    }

    /* 4. User ID/passwd check */
    ret = CheckUserInfo(pOdbcParam, passwd);
    if(ret<0)
    {
        if(ret == INVALID_USER) 
        {
            ErrLog("%d_%s;[Authenticate User]Invalid User : Password is wrong!;\n",
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(ret == NO_USER) 
        {
            ErrLog("%d_%s;[Authenticate User]There is no user ID[%s];\n", 
                   pOdbcParam->order, pOdbcParam->uid, pOdbcParam->uid);
        }
        else
        {
            ErrLog("%d_%s;[Authenticate User]System Error;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
            return (-1);
        }
        
        pOdbcParam->event = NACK_FLAG;
        return (WRITE_FLAG_STATE);	// userID�� passwd����
    }

    /* 5. Version Check & versionID ���ϱ� */
    ret = CheckVersion(pOdbcParam, application, version);
    if(ret <0)
    {
        pOdbcParam->event = NACK_FLAG;
        return (WRITE_FLAG_STATE);
    }
    else if(ret >0)
    {
        pOdbcParam->event = UPGRADE_FLAG;
        ErrLog("%d_%s;���׷��̵带 ���� Application '%s'�� ���� '%d'�� �����մϴ�.;\n", 
               pOdbcParam->order, pOdbcParam->uid, application, ret);
        return (WRITE_FLAG_STATE);
    }
    /* 6. ConnectionFlag�� CONNET_STATE�� update */
    if(UpdateConnectFlag(pOdbcParam, CONNECT_STATE)<0)
    {
        ErrLog("%d_%s;[Authenticate User]System Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return -1;
    }
    pOdbcParam->Authenticated = TRUE;
    
    ErrLog("%d_%s;[%s:%d] sync start.;\n", 
           pOdbcParam->order, pOdbcParam->uid, application, version);
    pOdbcParam->event = ACK_FLAG;
    return (WRITE_FLAG_STATE); 
}

/*****************************************************************************/
//! WriteFlag
/*****************************************************************************/
/*! \breif  Flags ���� socket�� ���ִ� �ܰ��̴�.
 ************************************
 * \param	pNetParam(in)	:							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Flag ���� socket�� ���ִ� �Լ��̴�.\n
 *			�̶� ���� packet�� �ִ� ��� WRITE_MESSAGE_SIZE �ܰ�� �����ϰ� \n
 *			packet�� ���� �ʿ䰡 ���� ���� pNetParam->nextState�� ������ \n
 *			�ܰ�� �����Ѵ�. 
 *****************************************************************************/
int WriteFlag(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret;
    char a[2] = {0x00,};
    
    sprintf(a, "%c", pOdbcParam->event);
    
    ret = WriteToNet(pNetParam->socketFd, a, sizeof(a));
    if(ret < 0) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1);
    }
    
    if(a[0]==OK_FLAG)	
        return (END_OF_STATE);    
    if(a[0] == NACK_FLAG || a[0] == ACK_FLAG  || a[0] == XACK_FLAG || 
       a[0] == UPGRADE_FLAG || a[0] == END_OF_TRANSACTION || a[0] == END_FLAG)
    { // MSG�� ���� �ʿ䰡 ���� ��� ������ state�� �̵�
        return (pNetParam->nextState);
    }
    
    return (WRITE_MESSAGE_BODY_STATE);
}

/*****************************************************************************/
//! ReceiveFlag
/*****************************************************************************/
/*! \breif  Socket���κ��� flag ���� �о�, �� FLAG�� ���� ������ ���� �ܰ�� �̵�\n
 ************************************
 * \param	pNetParam(in)	:							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Socket���κ��� flag ���� �д� �Լ��� �� flag�� ���� ������ ���� \n
 *			�ܰ�� �����Ѵ�.
 *****************************************************************************/
int ReceiveFlag(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int     n;
    char    buf[2];
    int     cnt=0;
    
re :
    memset(buf, 0x00, sizeof(buf));
    n = ReadFrNet(pNetParam->socketFd, buf, 2);
    if( n<0 ) 
    {
        ErrLog("%d_%s;Network Communication Error [Receive];\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    else if( n==0 ) 
    {
        ErrLog("%d_%s;Network Communication Error [Receive:Timeout];\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    else if(n!=2)
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
               pOdbcParam->order);
        return(-1) ;
    }

    if(buf[0] == APPLICATION_FLAG)
    { //'S'
        return (SET_APPLICATION_STATE);
    }
    else if(buf[0] == TABLE_FLAG)
    { // 'M'
        return (SET_TABLENAME_STATE);
    }
    else if(buf[0] == UPDATE_FLAG || 
            buf[0] == INSERT_FLAG || buf[0] == DELETE_FLAG)
    { // 'U', 'I', 'D'
        pOdbcParam->event = buf[0];
        return (PROCESS_SCRIPT_STATE);
    }
    else if( buf[0]== END_FLAG )
    { // 'E'
        return(UPLOAD_OK_STATE);
    }
    else if(buf[0] == ACK_FLAG)
    { // 'A'
        pOdbcParam->event = END_OF_TRANSACTION;
        pNetParam->nextState = END_OF_STATE;
        return (WRITE_FLAG_STATE);
    }
    else 
    {
        ErrLog("%d_%s;Network Communication Error [Invalid Flag:%c];\n", 
               pOdbcParam->order, pOdbcParam->uid, buf[0]);
        if( cnt<5 ) 
        {
            cnt++;
            goto re;
        }
        return(-1) ;
    }		
}

/*****************************************************************************/
//! SetApplication
/*****************************************************************************/
/*! \breif  flag�� APPLICATION_FLAG�� ��� ����Ǵ� �ܰ�
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG�κ��� ȣ��Ǵ� �Լ��� ���� flag�� APPLICATION_FLAG��\n
 *			��� ����Ǵ� �ܰ�� READ_MESSAGE_SIZE���� packet�� ������ �д� \n
 *			READ_MESSAGE_BODY���� �����ϸ� pNetParam->nextState�� \n
 *			END_OF_STATE�̹Ƿ� while loop�� ���� ���´�.\n
 *			ReceiveBuffer���� DBSync+Dsn ���� ����Ǿ� �����Ƿ� \n
 *          application Type�� ���� userDB�� open�ϰų� \n
 *			DB connection time�� �о� ���� ���� ���ش�. 
 *			DB sync�� ��� ���� �ܰ�� table �̸��� �б� ���� RECEIVE_FLAG��\n
 *			�����Ѵ�.\n
 *****************************************************************************/
int SetApplication(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret = READ_MESSAGE_BODY_STATE;
    SQLCHAR dsn[DSN_LEN + 1];
    SQLCHAR dsnuid[DSN_UID_LEN + 1];
    SQLCHAR dsnpasswd[DSN_PASSWD_LEN + 1];
    char appType[2];
    int  applicationType;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)	
        return -1;
    
    // receiveBuffer���� 
    // DBSYNC�� ��� DBSYNC+DSN    
    sprintf(appType, "%1.1s", &pNetParam->receiveBuffer[0]);
    applicationType = atoi(appType);
    
    if(applicationType == DBSYNC)
    {
        if(pOdbcParam->UserDBOpened)
        { // ���� ���� �ִ� ����� ������ �ݾ� �ش�.
            FreeHandles(USERDB_ONLY, pOdbcParam);  
            pOdbcParam->UserDBOpened = FALSE;
        }

        // ������ ���õǾ� �ִ� DSN 
        if(GetDSN(pOdbcParam, 
                 (char *)pNetParam->receiveBuffer+1, dsn, dsnuid, dsnpasswd)<0)
        {
            return (-1);
        }

        if(OpenUserDB(pOdbcParam, dsn, dsnuid, dsnpasswd)<0) 
        {
            return (-1); 
        }

        pOdbcParam->UserDBOpened = TRUE;
        pOdbcParam->mode = pOdbcParam->mode | DBSYNC;

        if(GetDBConnectTime(pOdbcParam, 
                            &(pOdbcParam->hstmt4Processing), 
                            PROCESSING, &(pOdbcParam->connectTime))<0)
        {
            return (-1);
        }

        ErrLog("%d_%s;[DBSync] DSN : %s start.;\n", 
               pOdbcParam->order, pOdbcParam->uid, dsn);
    }
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! SetTableName
/*****************************************************************************/
/*! \breif  flag�� TABLE_FLAG�� ��� ����
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG�κ��� ȣ��Ǵ� �Լ�\n
 *			�̶� ���� flag�� TABLE_FLAG�� ��� ����Ǵ� �ܰ�� 
 *			SetApplication() �Լ��� ���� READ_MESSAGE_SIZE, READ_MESSAGE_BODY\n
 *			�ܰ踦 ���� syncMode�� tableName ���� ��´�.\n
 *			Upload data stream�� ����ϸ� RECEIVE_FLAG�� �����Ѵ�.
 *****************************************************************************/
int	SetTableName(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    char syncFlag[2];
    int	 ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)	
        return -1;
    
    sprintf(syncFlag, "%1.1s", &pNetParam->receiveBuffer[0]);
    pOdbcParam->syncFlag = atoi(syncFlag);
    
    
    // TO DO:
    //    table ���� ALL, MOD sync �����ϵ��� ���� 
    // if(pOdbcParam->syncFlag==ALL_SYNC)
    //     strcpy(pOdbcParam->lastSyncTime,"1900-01-01");
    
    strcpy((char *)pOdbcParam->tableName, (char *)pNetParam->receiveBuffer+1);
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! ProcessScript
/*****************************************************************************/
/*! \breif  flag�� UPDATE_FLAG, DELETE_FLAG, INSERT_FLAG�� ��� ����\n
 *			Script�� ���� ��� �޼����� �α��ϰ� ���� ����Ÿ�� ��� \n
 *			�ݿ����� ������ ���� �ܰ�� ����
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG�κ��� ȣ��Ǵ� �Լ��� ���� flag�� UPDATE_FLAG, \n
 *			DELETE_FLAG, INSERT_FLAG�� ��� ����Ǵ� �ܰ�� record���� \n
 *			packet�� socket���κ��� �о�(READ_MESSAGE_SIZE, READ_MESSAGE_BODY)\n
 *			loop�� �������� �� GetStatement() �Լ��� �ݿ��� ������ ���ϰ� \n
 *			ProcessStmt() �Լ����� ���� �����ͺ��̽��� �� record���� ������ 
 *			�ݿ��Ѵ�. ���� packet�� ����ϸ� RECEIVE_FLAG�� �����Ѵ�. 
 *****************************************************************************/
int ProcessScript(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    SQLCHAR	Statement[SCRIPT_LEN+1];
    int	ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);
    }	

    if(ret < 0)	
        return (-1);
    
    memset(Statement, 0x00, SCRIPT_LEN+1);
    if((ret = GetStatement(pOdbcParam, Statement))<0) 
        return (-1);
    
    if(ret == SQL_NO_DATA)
    { // script�� ���� ��� 
        if(pOdbcParam->event==UPDATE_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Update;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(pOdbcParam->event==DELETE_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Delete;\n",
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(pOdbcParam->event==INSERT_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Insert;\n", 
                   pOdbcParam->order, pOdbcParam->uid);	
        }
    }
    else
    {        
        if(ProcessStmt(pNetParam->receiveBuffer, 
                       pNetParam->receiveDataLength, pOdbcParam, Statement)<0)
            return (-1);
    }
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! UploadOK
/*****************************************************************************/
/*! \breif  flag�� END_FLAG�� ���\n
 *			upload�� ������ �ڿ��� OK_FLAG 'K'�� ������ client�κ��� \n
 *			OK_FLAG 'K'�� ���� �ڿ� download�� �Ѿ��.
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG���� ���� flag�� END_FLAG�� ��� upload�� �������� \n
 *			�� �� ������ �̶� ȣ��Ǵ� �Լ���. Download script�� ������ ����\n
 *			�ʿ��� parameter�� �ִ��� Ȯ���ϱ� ���� socket���κ��� packet�� \n
 *			�޴´�. \n
 *			Parameter�� ������ NO_PARAM�̶�� �޽����� ��� �ȴ� \n
 *			(READ_MESSAGE_SIZE, READ_MESSAGE_BODY). \n
 *			���������� �� �ܰ谡 ������ pNetParam->nextState�� ������ ��ó��\n
 *			WRITE_FLAG(OK_FLAG) �ܰ�� �����ϰ� END_OF_STATE��while loop�� \n
 *			���� ���´�. �� �ܰ迡�� ���� �ÿ��� upload �ܰ迡 ������ \n
 *			transaction�� rollback���� ó���ϰ� ���� �ÿ���commit���� ó����\n
 *			�� DOWNLOAD �ܰ�� �����Ѵ�.
 *****************************************************************************/
int UploadOK(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int	ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = WRITE_FLAG_STATE;
    pOdbcParam->event = OK_FLAG;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)
    {
#if defined(COMMIT_AFTER_SYNC_ALL)
       /* no Action */; // ��ü ����ȭ �Ŀ� rollback�̳� commit�� ���Ѵ�.
#else
        ret = SQLEndTran(SQL_HANDLE_DBC, 
                         pOdbcParam->hdbc4Processing, SQL_ROLLBACK);
        if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hstmt4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, ret); 	
        }
#endif
        return -1;
    }	
    
#if defined(COMMIT_AFTER_SYNC_ALL)
    /* no Action */; // ��ü ����ȭ �Ŀ� rollback�̳� commit�� ���Ѵ�.
#else
    ret = SQLEndTran(SQL_HANDLE_DBC, pOdbcParam->hdbc4Processing, SQL_COMMIT);
    if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4Processing, 
                     pOdbcParam->uid, pOdbcParam->order, ret);
        return (-1);
    }	
#endif
    
    pOdbcParam->event = DOWNLOAD_FLAG;	
    ErrLog("%d_%s;Upload processing [%s] is completed;\n", 
           pOdbcParam->order, pOdbcParam->uid, pOdbcParam->tableName);
    
    return (DOWNLOAD_PROCESSING_STATE);
}

/*****************************************************************************/
//! MakeDownloadMessage
/*****************************************************************************/
/*! \breif  DownloadProcessingMain()���� ȣ��Ǵ� �� �ܰ� 
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	DownloadProcessingMain()���� ȣ��Ǵ� �� �ܰ�� FetchResult()��\n
 *			���� sendBuffer�� ���� packet�� ����� socket�� download packet��\n
 *			�� �� �ֵ��� WRITE_FLAG �ܰ�� �����Ѵ�.
 *****************************************************************************/
int 
MakeDownloadMessage(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret;
    memset(pNetParam->sendBuffer, '\0', HUGE_STRING_LENGTH+4+1);
    ret = FetchResult(pOdbcParam, pNetParam->sendBuffer+4, pNetParam->recordBuffer);
    
    // ������ ��� return <0
    // ����Ÿ�� ������ return 1
    // ����Ÿ�� ���� ��� return 2
    pNetParam->sendDataLength = strlen(pNetParam->sendBuffer+4);
    
    if(ret <0)			
        return (-1);
    else if(ret == 2) 
    {
        pOdbcParam->event = END_FLAG;
        pNetParam->nextState = END_OF_RECORD_STATE;
    }
    else
        pNetParam->nextState = MAKE_DOWNLOAD_MESSAGE_STATE;
    
    return (WRITE_FLAG_STATE);
}

/*****************************************************************************/
//! DownloadProcessingMain
/*****************************************************************************/
/*! \breif  Upload�� ���������� ���� �� ����Ǵ� �ܰ�
 ************************************
 * \param	pNetParam(in)	: network �Ķ����							
 * \param	pOdbcParam(in)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Upload�� ���������� ���� �� ����Ǵ� �ܰ�� download script��\n
 *			DB�κ��� ���(GetStatement()) cursor�� prepare�ϰ� \n
 *			MAKE_DOWNLOAD_MESSAGE �ܰ���� �����Ͽ� socket�� download stream��\n
 *			����� ���ش�(WRITE_FLAG, WRITE_MESSAGE_SIZE, WRITE_MESSAGE_BODY)\n
 *			���� �����Ͱ� �� �̻� ���ų� script�� ���� ���� END_FLAG�� \n
 *			�����ϰ� END_OF_RECORD�� return�ϰ� �Ǿ� while loop�� ���� ���´�.\n 
 *			���������� download �ܰ踦 ��ġ�� deleted record���� download�ϱ�\n
 *			���ؼ� pNetParam->event�� DOWNLOAD_DELETE_FLAG�� �Ͽ� �ٽ� �ѹ� \n
 *			�� �ܰ踦 �����ϵ��� �ϴµ�(DOWNLOAD_PROCESSING) ���� deleted \n
 *			record�� ó������ �ʴ� ����� client���� 
 *			mSync_ReceiveDownloadData()�� �ѹ��� ȣ���ϰ� �ǰ� \n
 *			download_delete�� script�� �ۼ����� �ʾ����Ƿ� �ٷ� RECEIVE_FLAG \n
 *			�ܰ�� �����Ѵ�.
 *****************************************************************************/
int 
DownloadProcessingMain(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int		ret;	
    SQLCHAR	Statement[SCRIPT_LEN+1];
    char	event = pOdbcParam->event;
    
    memset(Statement, 0x00, SCRIPT_LEN+1);
    pOdbcParam->NumCols = 0;
    pOdbcParam->NumParams = 0;
    
    if((ret = GetStatement(pOdbcParam, Statement))<0) 
        return (-1);
    if(ret != SQL_NO_DATA)
    {	// script�� �ִ� ���
        if(AllocStmtHandle(pOdbcParam, PROCESSING)<0) 
            return (-1);	
        if(PrepareDownload(pNetParam->receiveBuffer, pOdbcParam, 
                           pNetParam->receiveDataLength, Statement) < 0)
        {
            ret = -1;
            goto Download_End;
        }
        
        ret = MAKE_DOWNLOAD_MESSAGE_STATE;
        while(1) 
        {
            if(ret == END_OF_RECORD_STATE || ret<0)
                break ;	
            ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);			
        }	
        
Download_End:
        if(pOdbcParam->NumRows)	
            SQLCloseCursor(pOdbcParam->hstmt4Processing);
        FreeStmtHandle(pOdbcParam, PROCESSING);
        FreeArrays(pOdbcParam);
        
        if(ret < 0) 
            return -1;

        if(event == DOWNLOAD_FLAG)
        {
            pOdbcParam->event = DOWNLOAD_DELETE_FLAG;
            pNetParam->nextState = DOWNLOAD_PROCESSING_STATE;
            if(pOdbcParam->syncFlag == ALL_SYNC)
            {
                ErrLog("%d_%s;Download ALL_SYNC processing [%s] is"
                       " completed => %d rows downloaded;\n", 
                       pOdbcParam->order, pOdbcParam->uid, 
                       pOdbcParam->tableName, pOdbcParam->NumRows);
            }
            else
            {
                ErrLog("%d_%s;Download MOD_SYNC processing [%s] is"
                       " completed => %d rows downloaded;\n", 
                       pOdbcParam->order, pOdbcParam->uid, 
                       pOdbcParam->tableName, pOdbcParam->NumRows);			
            }
        }
        else
        {
            pNetParam->nextState = RECEIVE_FLAG_STATE;
            ErrLog("%d_%s;Download_Delete processing [%s] is"
                   " completed => %d rows downloaded;\n", 
                   pOdbcParam->order, pOdbcParam->uid, 
                   pOdbcParam->tableName, pOdbcParam->NumRows);
        }
    }
    else
    {	// script�� ���� ���
        ret = WRITE_FLAG_STATE;		
        pOdbcParam->event = END_FLAG;
        pNetParam->nextState = END_OF_RECORD_STATE;
        
        ErrLog("%d_%s;There is no Script for Download;\n", pOdbcParam->order, pOdbcParam->uid);		
        while(1) 
        {
            if(ret == END_OF_RECORD_STATE || ret<0)
                break ;	
            ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);
        }
        // DOWNLOAD_FLAG('F')�� ��쿣 DOWNLOAD_DELETE_FLAG�� �̵��ϰ�
        // DOWNLOAD_DELETE_FLAG('R')�� ��쿣 �ٷ� RECEIVE_ACK�� �̵�
        if(ret < 0) 
            return -1;
        if(event == DOWNLOAD_DELETE_FLAG)            
            return (RECEIVE_FLAG_STATE);
        
        pOdbcParam->event = DOWNLOAD_DELETE_FLAG;
        pNetParam->nextState = DOWNLOAD_PROCESSING_STATE;		
    }
    return (pNetParam->nextState);
}

/*****************************************************************************/
//! WriteMessageBody
/*****************************************************************************/
/*! \breif  SendBuffer�� socket�� ���ִ� �Լ��� packet�� size + content�� ����
 ************************************
  * \param	pNetParam(in,out)	: network �Ķ����							
 * \param	pOdbcParam(in,out)	: DB ������ ���ؼ� �����
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	SendBuffer�� socket�� ���ִ� �Լ��� packet�� size + content�� \n
 *			��������.
 ****************************************************************************/
int 
WriteMessageBody(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret ;
    unsigned int tmpSize ;
    
    if(pNetParam->sendDataLength > pNetParam->sendBufferSize) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1);
    }
    tmpSize = htonl((unsigned int)pNetParam->sendDataLength) ;
    memcpy(pNetParam->sendBuffer, (char *)&tmpSize, sizeof(unsigned int));
    
    ret = WriteToNet(pNetParam->socketFd, 
                     pNetParam->sendBuffer, pNetParam->sendDataLength+4 );
    if(ret < 0) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    
    return(pNetParam->nextState) ;
}

