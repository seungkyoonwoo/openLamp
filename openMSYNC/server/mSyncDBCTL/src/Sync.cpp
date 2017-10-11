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


#include	<stdio.h>
#include	<stdlib.h>		
#include	<Winsock2.h>
#include	<process.h>

#include	"Sync.h"
#include	"state.h"

#include	"../../version.h"

#define APP_EXIT 0
typedef struct _Sync_Param{
    int sockfd;
    int count;
    char clientAddr[16];
}Sync_Param;

Sync_Param *param;

SQLCHAR		*dbuid, *dbdsn, *dbpasswd;

int dbType;
#ifdef SERVICE_CONSOLE
HANDLE g_hEvent;
#endif

static int serverSocketFd;
extern int	REMAIN_DAYS ;

void ProcessMain(HWND hWnd);


/*****************************************************************************/
//! MemoryAllocation
/*****************************************************************************/
/*! \breif  sendBuffer, receiveBuffer, recordBuffer�� �޸𸮸� �Ҵ��� �� ���
 ************************************
 * \param	order(in)	:
 * \param	**ptr(out)	: Pointer to the Memory that allocated
 * \param	length(in)	: not used
 ************************************
 * \return	void
 ************************************
 * \note	SInitializeParameter() �Լ� �ȿ��� sendBuffer, receiveBuffer, \n
 *			recordBuffer�� �޸𸮸� �Ҵ��� �� ���Ǵ� �Լ�
 *****************************************************************************/
int MemoryAllocation(int order, char **ptr, int length)
{
	*ptr = (char *)calloc(length, sizeof(char));	
	if (*ptr ==NULL)
    {
		ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
               order, length);
		return -1;
	}
	return 0;
}

/*****************************************************************************/
//! InitializeParameter
/*****************************************************************************/
/*! \breif  �Ķ���� �ʱ�ȭ
 ************************************
 * \param	pNetParam(out)	:
 * \param	pOdbcParam(out)	:
 * \param	sockfd(out)		:
 * \param	order(out)		:
 ************************************
 * \return	void
 ************************************
 * \note	Network parameter�� ODBC parameter�� �ʱ�ȭ�ϴ� �Լ��� \n
 *			client�κ��� connection�� �ξ��� thread�� �Ҵ�� ���� \n
 *			ProcessThread()���� ȣ��ȴ�.
 *****************************************************************************/
int InitializeParameter(NETWORK_PARAMETER *pNetParam,
                        ODBC_PARAMETER *pOdbcParam, int sockfd, int order)
{
	memset(pOdbcParam, 0x00, sizeof(ODBC_PARAMETER));	
    pNetParam->socketFd = sockfd;
    pNetParam->sendDataLength = 0 ;
    pNetParam->receiveDataLength = 0;
    pNetParam->sendBufferSize = HUGE_STRING_LENGTH;
    pNetParam->receiveBufferSize = HUGE_STRING_LENGTH;
    pNetParam->nextState = AUTHENTICATE_USER_STATE;
    pNetParam->sendBuffer = NULL;
    pNetParam->receiveBuffer = NULL;
    pNetParam->recordBuffer = NULL;
    
    pOdbcParam->dbdsn = dbdsn;
    pOdbcParam->dbuid = dbuid;
    pOdbcParam->dbpasswd = dbpasswd;
    pOdbcParam->UserDBOpened = FALSE;
    pOdbcParam->Authenticated = FALSE;
    pOdbcParam->connectTime = NULL;
    pOdbcParam->order = order;
    pOdbcParam->mode = 0;	

    pOdbcParam->dbType = dbType;
    pOdbcParam->lastSyncTime = NULL;
    pOdbcParam->hflag4Processing = 0;
    pOdbcParam->hflag4System = 0;
    
    memset(pOdbcParam->uid, 0x00, sizeof(pOdbcParam->uid));	
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->sendBuffer), HUGE_STRING_LENGTH+4+1) < 0)
    {
        return (-1);
    }
    
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->receiveBuffer), HUGE_STRING_LENGTH+1)<0)
    {
        return (-1);
    }
    
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->recordBuffer), HUGE_STRING_LENGTH+1)<0)
    {
        return (-1);
    }
    
    return (0);
}
void FreeMemories(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    if(pNetParam->sendBuffer)	
    {
        free(pNetParam->sendBuffer);
        pNetParam->sendBuffer = NULL;
    }
    
    if(pNetParam->receiveBuffer)
    {
        free(pNetParam->receiveBuffer);
        pNetParam->receiveBuffer = NULL;
    }
    
    if(pNetParam->recordBuffer)
    {
        free(pNetParam->recordBuffer);
        pNetParam->recordBuffer = NULL;
    }
}

/*****************************************************************************/
//! ProcessThread
/*****************************************************************************/
/*! \breif  Client�κ��� connection�� �ξ��� �� thread�� child process��	\n
 *			�Ҵ�ǰ� ���εǴ� �Լ� \n
 *			sync ������ ���.
 ************************************
 * \param	param(in,out)	: network paramter
 ************************************
 * \return	void
 ************************************
 * \note	Client�κ��� connection�� �ξ��� �� thread�� child process��\n
 *			�Ҵ�ǰ� ���εǴ� �Լ��� sync ������ ó���ϴ� thread�� main \n
 *			�Լ��� ���̴�. 
 *			Client���� connect ������ ȣ��Ǵ� ���� mSync_AuthRequest()�̹Ƿ�\n
 *			ret ��������READ_MESSAGE_SIZE�� setting�Ͽ� �� �ܰ���� \n
 *			�����ϵ��� �Ѵ�. ReadMessageSize() �Լ���  READ_MESSAGE_BODY�� \n
 *			return�ϹǷ� ���� �ܰ�� packet ������ �д� ReadMessageBody()�� \n
 *			ȣ���ϰ� �ǰ� socket���κ��� ���� ������ ������ \n
 *			pNetParam->nextState�� ������ ��� ���� ó���� �ϴ� \n
 *			AUTHENTICATE_USER�� ȣ���Ѵ�. ��ó�� state.cpp�� �����Ǿ� �ִ� \n
 *			�� state���� client�� API�� �ش��ϴ� ������� ó���Ѵ�.\n
 *			Sync�� ��� �ܰ谡 ����Ǹ� DB�� ���� handle���� �����ϰ� 
 *			�Ҵ��ߴ� �޸𸮵鵵 �����ϰ� thread�� �����Ѵ�.
 *****************************************************************************/
unsigned  __stdcall	ProcessThread(Sync_Param *param)
{	
    NETWORK_PARAMETER	networkParameter;
    ODBC_PARAMETER		odbcParameter;
    int		ret = READ_MESSAGE_BODY_STATE;
    int		state;
        
    // ���� �ʱ�ȭ �� �޸� �Ҵ�
    if(InitializeParameter(&networkParameter, 
                           &odbcParameter, param->sockfd, param->count) < 0)
    {
        ret = -1;
        goto SYNC_END;
    }
    
    ErrLog("SYSTEM;%dth client[%s] is Connected;\n", 
           odbcParameter.order, param->clientAddr);
    
    // sync�� �� ������ ó�� : �ڼ��� ������ state.cpp�� ����
    // �� function�� �̸��� func[]�� ���    
    while(1) 
    {
        if (ret == END_OF_STATE || ret < 0)	break ;
        ret = g_stateFunc[ret].pFunc(&networkParameter, &odbcParameter);		
    }	
    
    // sync �ܰ� ���� : ret <0�̸� �����̰� END_OF_STATE�� ���� ����
    if (ret < 0)
        state = DISCONNECT_ERROR_STATE;
    else 
        state = DISCONNECT_STATE;
        
#if  defined(COMMIT_AFTER_SYNC_ALL)
    // ��ü ����ȭ �Ŀ� rollback�̳� commit�� ���Ѵ�.
    if(odbcParameter.UserDBOpened)
    {
        if(ret < 0)
        {		
            ret = SQLEndTran(SQL_HANDLE_DBC, 
                             odbcParameter.hdbc4Processing, SQL_ROLLBACK);
            if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
            {
                DisplayError(odbcParameter.hstmt4Processing, 
                             odbcParameter.uid, odbcParameter.order, ret); 	
                ret = -1;
            }
        }	
        else 
        {
            ret = SQLEndTran(SQL_HANDLE_DBC, 
                             odbcParameter.hdbc4Processing, SQL_COMMIT);
            if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
            {
                DisplayError(odbcParameter.hstmt4Processing, 
                             odbcParameter.uid, odbcParameter.order, ret);
                state = DISCONNECT_ERROR_STATE;
                ret = -1;
            }
        }	
    }
#endif

    // user�� �ִ� ��쿡 update
    // firstUserDB�� True�ΰ� invalid user�̰ų� no user�̰ų� update�� ������ ���
    // ���� �ް� openMSYNC_User ���̺� update�� �����Ǹ� firstUserDB�� TRUE�� �ȴ�.
    if(odbcParameter.Authenticated == TRUE ) 
    {
        if(UpdateConnectFlag(&odbcParameter, state) <0) 
            ret = -1;
    }
    // �� handle�� �����ϰ� DB���� ������ �����Ѵ�
    FreeHandles(ALL_HANDLES, &odbcParameter);
    
SYNC_END:
    if(ret <0)
    {
        ErrLog("SYSTEM;%dth client[%s] is Disconnected => Error Occurred;\n", 
               odbcParameter.order, param->clientAddr);
    }
    else
    {
        ErrLog("SYSTEM;%dth client[%s] is Disconnected;\n", 
               odbcParameter.order, param->clientAddr);
    }
    
    closesocket(networkParameter.socketFd);    
    
    FreeMemories(&networkParameter, &odbcParameter);

    return	0;
}


DWORD WINAPI win_ProcessThread(LPVOID param)
{	
	Sync_Param* pSyncParam =  *((Sync_Param **)param);

	param = 0x00;
	ProcessThread(pSyncParam);
	free(pSyncParam);
	
	_endthread();
	return 0;
}

/*****************************************************************************/
//! SyncMain
/*****************************************************************************/
/*! \breif  Server Socket�� initialize�ϰ� bind()������ thread ���� ó��
 ************************************
 * \param	svUid(in)		:
 * \param	svDsn(in)		:
 * \param	svPort(in)		:
 * \parram	hWnd(in)		:
 ************************************
 * \return	int
 ************************************
 * \note	Server�� ���� auth.ini ������ �о� ��� ������ �������� \n
 *			Ȯ���� �� ProcessMain()�� ȣ���Ѵ�. \n 
 *****************************************************************************/
/*****************************************************************************
 Server Socket�� initialize�ϰ� client�� connect�� ����Ǹ� 
 thread ���� ó���ϵ��� ProcessMain()�� ȣ���Ѵ�. 
 �������� �κ��� ProcessMain()�� ����.  
*****************************************************************************/
__DECL_PREFIX_MSYNC int SyncMain(S_mSyncSrv_Info *pSrvInfo, HWND hWnd)
{
    int		    SERVER_PORT;
    char		version[VERSION_LENGTH+1];
    
    SERVER_PORT = pSrvInfo->nPortNo;
    
    sprintf(version,
            "%d.%d.%d.%d", MAJOR_VER, MINOR_VER, RELEASE_VER, BUGPATCH_VER);
    
    // socket�� �ʱ�ȭ�ϰ� ������ ��� ��
    if ((serverSocketFd = InitServerSocket(SERVER_PORT)) < 0) 
    {
        ErrLog("SYSTEM;Start Up mSync ver %s [port:%d]: FAIL.;\n", 
               version, SERVER_PORT);
        SendMessage(hWnd, WM_COMMAND, APP_EXIT, 0);
        exit (0);
    }

    ErrLog("SYSTEM;Start Up mSync ver %s [port:%d];\n", version, SERVER_PORT);
    ErrLog("SYSTEM;�ִ� ����� ���� %d���Դϴ�;\n", pSrvInfo->nMaxUser);
    
    // max User count�� timeout ���� �����Ѵ�.
    SetMaxUserCount(pSrvInfo->nMaxUser);
    SetTimeout(pSrvInfo->ntimeout);
    
    dbuid = (SQLCHAR *)pSrvInfo->szUid;
    dbpasswd = (SQLCHAR *)pSrvInfo->szPasswd;
    dbdsn = (SQLCHAR *)pSrvInfo->szDsn;
    
    dbType = pSrvInfo->nDbType;
    
    ProcessMain(hWnd);
    return	0;
}

#ifdef SERVICE_CONSOLE
void SetEventHandle(HANDLE gEvent){
	g_hEvent = gEvent;
}
#endif

/*****************************************************************************
// Sync Process�� Main �Լ� : client�� ���ӵǸ� thread�� �ϳ� �����
// ProcessThread()���� sync ������ ����ϵ��� �Ѵ�.
*****************************************************************************/
void ProcessMain(HWND hWnd)
{
	char	clientAddr[16];
	int		newsockfd;
	int		ttt=1;	
	int		childPid = -1;

#ifdef SERVICE_CONSOLE
	// ���α׷����� ó���� ������ timeout�� �ּ� �ֱ������� 
	// service�� stop /�Ǿ����� �� �̺�Ʈ�� Ȯ���Ѵ�.
	DWORD dwWait;
	struct timeval tv;
	fd_set rset;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
#endif

	while(1)
	{		
		// ���� ���α׷��� ���� �ֱ������� �̺�Ʈ�� Ȯ���ϰ�
		// Unix�� windows���� client�� ���� ������ 
        // ProcessNetwork()���� ��� ��ٸ���. 
#ifdef SERVICE_CONSOLE
		dwWait = WaitForSingleObject(g_hEvent, 0);
		if(WAIT_OBJECT_0 == dwWait)
		{
			break;
		}
	
		FD_ZERO(&rset);
		FD_SET((u_int)serverSocketFd, &rset);
		if(select(serverSocketFd+1, &rset, NULL, NULL, &tv)<=0)
			continue;
#endif

		if ((newsockfd = ProcessNetwork(clientAddr)) < 0)	
			continue;

		HANDLE		hThread;

		param = (Sync_Param *)malloc(sizeof(Sync_Param));
		param->sockfd = newsockfd;
		param->count = ttt++;
		strcpy(param->clientAddr,clientAddr);
				
		hThread = (HANDLE)_beginthread(
            (void(__cdecl *)(void *))win_ProcessThread,
		    0,
			(void*)(&param));
		
		if (hThread == NULL)
		{
			free(param);
			param = NULL;

			ErrLog("SYSTEM;Thread Creation ERROR;\n");
			closesocket(newsockfd);
		}
	}
}
