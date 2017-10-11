//
// mSyncDemoApp.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/mSyncclient.h"
#include "msyncDemo.h"


struct SyncSrv_Info
{
    char _szSrvAddr[16];
    int _nPort;
    char _appName[128];
    int _version;
};

struct virtualClinet_Info
{
    char _db_name[128];
    char _userid[32];
    char _passwd[32];
};

static struct SyncSrv_Info g_SrvInfo = {
    "192.168.4.9",
    7788,
    "demoApp",
    1
};

static struct virtualClinet_Info g_ClientInfo[2] = {
    {"/mSyncClientDB/contactdb1", "sync_user1", "passwd"},
    {"/mSyncClientDB/contactdb2", "sync_user2", "passwd"}
};

struct virtualClinet_Info *g_pClientInfo = NULL;

static char g_clientDSN[] = "demoApp_clientDSN";
static char g_serverTable[] = "contact_place";
static int g_sync_mode = ALL_SYNC;
static int g_timeout = 1000;

/* ************************************************************************ 
/* SCRIPT 
** UPLOAD_INSERT
**   INSERT INTO contact_place (ID, username, Company,name, e_mail,Company_phone,Mobile_phone,Address,mTime, dflag  ) VALUES (?,?,?,?,?,?,?,?,getdate(), 'I')
** UPLOAD_UPDATE
**   UPDATE contact_place SET Company = ? , name = ? , e_mail = ? , Company_phone = ? , Mobile_phone = ? , Address = ? , mTime = getdate(), dflag = 'U' WHERE ID = ? AND username = ?
** UPLOAD_DELETE
**   UPDATE contact_place SET dflag = 'D' WHERE ID = ? AND username = ?
**   DELETE FROM contact_place WHERE ID = ? AND username = ?
** DOWNLOAD_SELECT
**   SELECT ID, username, Company,name,e_mail,Company_phone,Mobile_phone,Address FROM contact_place where mTime >= ? AND dflag <> 'D'
** DOWNLOAD_DELETE
**   SELECT ID, username FROM contact_place where mTime >= ? AND dflag = 'D'
************************************************************************ */

extern void QuitLocalDB();
extern int InitLocalDB(char *dbname);
extern void CommitLocalDB(int isSuccess);
extern int SyncInsertLocalDB(S_Recoed * pRecord, int bCommit);
extern int GetMaxIDLocalDB();
extern int Show_LocalDB(int isAllshow);
extern int SelectLocalDB(char *pQuery);
extern int FetchLocalDB(S_Recoed * pRecord);
extern void QuitSelectLocalDB();
extern int QueryLocalDB(char *pQuery);

int
DownloadDelete_contact(char *Buffer)
{
    char tmp[64], username[32];
    int bCommit = 1;
    int nLength = strlen(Buffer);
    int nIndex = 0;
    int j = 0;
    int i;
    int nID, nRet;

    nID = -1;
    username[0] = '\0';
    for (i = 0; i < nLength; i++)
    {
        if (Buffer[i] == RECORD_DEL)
        {
            if (nIndex == 0)
            {
                strncpy(tmp, Buffer + j, i - j);
                tmp[i - j] = '\0';
                nID = atoi(tmp);
            }

            j = i;
            j++;

            sprintf(tmp,
                    "delete from contact where id = %d AND username = %s",
                    nID, username);
            nRet = QueryLocalDB(tmp);

            nIndex = 0;
            nID = -1;
            username[0] = '\0';
        }
        else if (Buffer[i] == FIELD_DEL)
        {
            strncpy(tmp, Buffer + j, i - j);
            tmp[i - j] = '\0';
            strcpy(username, tmp);

            j = i;
            j++;
            nIndex++;
        }
    }

    return 0;
}

int
DownloadSelect_contact(char *Buffer)
{
    char tmp[64];
    int bCommit = 1;
    int nLength = strlen(Buffer);
    int nIndex = 0;
    int j = 0;
    int i, nRet;
    S_Recoed Record;

    memset(&Record, 0x00, sizeof(S_Recoed));
    for (i = 0; i < nLength; i++)
    {
        if (Buffer[i] == RECORD_DEL)
        {
            strncpy(Record.Address, Buffer + j, i - j);
            j = i;
            j++;

#ifndef USE_MSYNC_TABLE
            Record.dflag = DFLAG_SYNCED;
#endif

            nRet = SyncInsertLocalDB(&Record, bCommit);
            if (nRet < 0)
                return -1;

            nIndex = 0;
            memset(&Record, 0x00, sizeof(S_Recoed));
        }
        else if (Buffer[i] == FIELD_DEL)
        {
            switch (nIndex)
            {
            case 0:
                strncpy(tmp, Buffer + j, i - j);
                tmp[i - j] = '\0';
                Record.ID = atoi(tmp);
                break;
            case 1:
                strncpy(Record.username, Buffer + j, i - j);
                break;
            case 2:
                strncpy(Record.Company, Buffer + j, i - j);
                break;
            case 3:
                strncpy(Record.Name, Buffer + j, i - j);
                break;
            case 4:
                strncpy(Record.e_mail, Buffer + j, i - j);
                break;
            case 5:
                strncpy(Record.Company_phone, Buffer + j, i - j);
                break;
            case 6:
                strncpy(Record.Mobile_phone, Buffer + j, i - j);
                break;
            default:
                return -1;      /* missmatch field count */
            }
            j = i;
            j++;
            nIndex++;
        }
    }

    return 0;
}


int
Upload_contact(char flag, char *Buffer)
{
    char tmp[8192];
    int i, nRows, nRet = 0;
    S_Recoed Record;

#ifdef USE_MSYNC_TABLE
    if (flag == INSERT_FLAG)
    {       // Upload Insert                
        sprintf(tmp,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mtime from contact INSERT_RECORD");
    }
    else if (flag == UPDATE_FLAG)
    {       // Upload Update
        sprintf(tmp,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mtime from contact UPDATE_RECORD");
    }
    else    // if( flag == DELETE_FLAG )
    {       // Upload Delete
        sprintf(tmp, "select id, username from contact  DELETE_RECORD");
    }

#else
    if (flag == INSERT_FLAG)
    {       // Upload Insert                
        sprintf(tmp,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mtime from contact where dFlag = %d",
                DFLAG_INSERTED);
    }
    else if (flag == UPDATE_FLAG)
    {       // Upload Update
        sprintf(tmp,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mtime from contact where dFlag = %d",
                DFLAG_UPDATED);
    }
    else    // if( flag == DELETE_FLAG )
    {       // Upload Delete
        sprintf(tmp, "select id, username from contact where dFlag = %d",
                DFLAG_DELETED);
    }
#endif

    nRows = SelectLocalDB(tmp);
    if (nRows < 0)
    {
        nRet = -1;
        goto return_pos;
    }
    else if (nRows == 0)
    {
        nRet = 0;
        goto return_pos;
    }

    memset(Buffer, 0x00, HUGE_STRING_LENGTH + 1);
    for (i = 0; i < nRows; i++)
    {
        memset(&Record, 0x00, sizeof(S_Recoed));
        if (FetchLocalDB(&Record) <= 0)
        {
            nRet = -2;
            goto return_pos;
        }

        //�ʵ屸����    FIELD_DEL
        //���ڵ� ������ RECORD_DEL
        memset(tmp, 0x00, sizeof(tmp));

        // id
        if (flag != UPDATE_FLAG)
        {
            sprintf(tmp, "%d%c", Record.ID, FIELD_DEL);
            //username
            sprintf(tmp + strlen(tmp), "%s%c", Record.username, FIELD_DEL);
        }
        else
            tmp[0] = '\0';

        if (flag != DELETE_FLAG)
        {
            //company
            sprintf(tmp + strlen(tmp), "%s%c", Record.Company, FIELD_DEL);
            //name
            sprintf(tmp + strlen(tmp), "%s%c", Record.Name, FIELD_DEL);
            //e_mail
            sprintf(tmp + strlen(tmp), "%s%c", Record.e_mail, FIELD_DEL);
            //company_phone
            sprintf(tmp + strlen(tmp), "%s%c", Record.Company_phone,
                    FIELD_DEL);
            //mobile_phone
            sprintf(tmp + strlen(tmp), "%s%c", Record.Mobile_phone, FIELD_DEL);
            //address
            sprintf(tmp + strlen(tmp), "%s%c", Record.Address, FIELD_DEL);
        }

        if (flag == UPDATE_FLAG)
        {
            sprintf(tmp + strlen(tmp), "%d%c", Record.ID, FIELD_DEL);
            sprintf(tmp + strlen(tmp), "%s%c", Record.username, FIELD_DEL);
        }


        tmp[strlen(tmp) - 1] = RECORD_DEL;
        tmp[strlen(tmp)] = '\0';

        if ((strlen(Buffer) + strlen(tmp)) > HUGE_STRING_LENGTH)
        {
            if (mSync_SendUploadData(flag, Buffer) < 0)
            {
                nRet = -3;
                goto return_pos;
            }
            memset(Buffer, 0x00, HUGE_STRING_LENGTH + 1);
        }
        sprintf(Buffer + strlen(Buffer), "%s", tmp);
    }

    if (strlen(Buffer) > 0)
    {
        if (mSync_SendUploadData(flag, Buffer) < 0)
        {
            nRet = -4;
            goto return_pos;
        }
    }

  return_pos:
    QuitSelectLocalDB();

#ifndef USE_MSYNC_TABLE
    if (nRet >= 0 && nRows > 0)
    {
        switch (flag)
        {
        case INSERT_FLAG:      // Upload Insert              
            sprintf(tmp, "update contact set mtime = now(), dflag = %d"
                    " where dFlag = %d", DFLAG_SYNCED, DFLAG_INSERTED);
            break;
        case UPDATE_FLAG:      // Upload Update
            sprintf(tmp, "update contact set mtime = now(), dflag = %d"
                    " where dFlag = %d", DFLAG_SYNCED, DFLAG_UPDATED);
            break;
        case DELETE_FLAG:      // Upload Delete
            sprintf(tmp,
                    "delete from contact where dFlag = %d", DFLAG_DELETED);
            break;
        }

        nRet = QueryLocalDB(tmp);
    }
#endif

    return nRet;
}

//***********************************************************************************//
//                      Client���� mSync ������ �����ϴ� ����
//***********************************************************************************//
//  1. mSync_InitializeClientSession(char *fileName, int timeout)
//              : Client ������ �ʱ�ȭ, �α�ȭ�ϼ���, ��Ʈ�� Ÿ�Ӿƿ� ����
//        �α�ȭ���� mSync_ClientLog(char *format,...)�Լ��� �̿��Ͽ� log ������ �����.
//  2. mSync_ConnectToSyncServer(char *serverAddr, int serverPort)
//              : ������ IP�� PORT�� �Է��Ͽ� mSync Server�� ����               
//  3. mSync_AuthRequest(char *uID, char *Password, char *application, int version)
//              : ID, PASSWORD, Admin Tool���� ������ Application name, version�� �Է��Ͽ� 
//                ����� ������ ��û
//                upgrade�� �ʿ��� ��� ��, ������ Application name�� ���� version�� ���� ��� 
//                UPGRADE_FLAG�� return�Ѵ�. 
//  4. mSync_SendDSN(char *dsn)
//              : Client���� �ν��ϴ�  dsn, Admin Tool���� DSN�� ����� �� �������� Client DSN.
//  5. mSync_SendTableName(int syncFlag, char *tableName)
//              : ����ȭ ���(��ü/�κе���ȭ)�� Server Table name.
//  6. mSync_SendUploadData(char flag, char *data)
//              : upload�� �������� ���� flag(insert, update, delete)�� upload data.
//                Admin Tool���� ������ script�� "?"�� upload data�� ������ ��ġ�ؾ� �Ѵ�.
//                �ʵ�   ������ : FIELD_DEL
//                ���ڵ� ������ : RECORD_DEL
//                data�� �ִ�ũ�� : 8192byte
//                Admin Toold�� script���� "?"�� �ϳ��� �ʵ尪�̴�. 
//  7. mSync_UploadOK(char *parameter)
//              : parameter�� mSync_ReceiveDownloadData�� ������ �� ���������� ������ parameter
//                ��, Admin Tool���� ������ Download_Select script�� �� parameter
//                ����, ������ ���� ������ 2�� �̻��̶�� aaaaa0x08bbbbb... �̷����·� ����(������ 0x08)
//  8. mSync_ReceiveDownloadData(char *data)
//              : Admin Tool�� Download script�� field ������ �����ϰ� ����
//                �ʵ�   ������ : 0x08
//                ���ڵ� ������ : 0X7F
// 10. mSync_Disconnect()
//              : mSync�� ���� ����
// ** �������� DSN�� �����ϱ� ���ؼ��� 4 - 9 ������ �ݺ��Ѵ�.
// ** mSync_ReceiveDownloadData(char *data)�� 2�� �����ϸ�
//    ù��°�� Download_Select�� �����ϰ� �ι�°�� Download_Delete�� �����Ѵ�.
//    mSync_UploadOK(char *parameter)�� �Էµ� parameter�� 
//    Download_Select�� Download_Delete���� �����ϰ� ����ȴ�.
//***********************************************************************************//
int
SYNC_TABLE(char *pClientDSN, char *pServerTable, int nSyncMode)
{
    int cnt = 0, i = 0;
    int nRet;
    int nlen = 0;
    char Data[HUGE_STRING_LENGTH + 4];

    /* DSN name ���� */
    if (mSync_SendDSN(pClientDSN) < 0)
    {
        mSync_ClientLog("SendDSN ==> %s\n", mSync_GetSyncError());
        return -1;
    }
    mSync_ClientLog("%s sync processing\n", pClientDSN);

    /* Table name ���� */
    if ((nRet = mSync_SendTableName(nSyncMode, pServerTable)) < 0)
    {
        mSync_ClientLog("SendTableName ==> %s\n", pServerTable);
        return -1;
    }

    /* Uploading */
    // insert upload                            
    if (Upload_contact(INSERT_FLAG, Data) < 0)
    {
        mSync_ClientLog("Insert MakeMessage error\n");
        return -1;
    }

    // update upload    
    if (Upload_contact(UPDATE_FLAG, Data) < 0)
    {
        mSync_ClientLog("Update MakeMessage error : addr\n");
        return -1;
    }

    // delete upload    
    if (Upload_contact(DELETE_FLAG, Data) < 0)
    {
        mSync_ClientLog("Delete MakeMessage error : addr\n");
        return -1;
    }

    // Upload �Ϸ�
    // download parameter ����
    // download parameter 2�� �̻��� ��� FIELD_DEL�� �����Ͽ� string�� �����Ѵ�.
    if (mSync_UploadOK("") < 0)
    {
        mSync_ClientLog("Upload ERROR\n");
        mSync_ClientLog("mSync_UploadOK ==> %s\n", mSync_GetSyncError());
        return -1;
    }

    /* Downloading */
    // Download_Select script ����      
    memset(Data, 0x00, sizeof(Data));
    while ((nRet = mSync_ReceiveDownloadData(Data)) > 0)
    {
        if (DownloadSelect_contact(Data) < 0)
            return -1;

        memset(Data, 0x00, sizeof(Data));
    }

    if (nRet < 0)
    {
        mSync_ClientLog("ERROR : %s\n", mSync_GetSyncError());
        return -1;
    }
    else    /* if(nRet == 0) */
    {
        mSync_ClientLog("DownLoad Success ----------\n");
    }

    // Download_Delete script ����
    memset(Data, 0x00, sizeof(Data));
    while ((nRet = mSync_ReceiveDownloadData(Data)) > 0)
    {
        if (DownloadDelete_contact(Data) < 0)
            return -1;

        memset(Data, 0x00, sizeof(Data));
    }

#ifdef USE_MSYNC_TABLE
    if (QueryLocalDB("alter table contact msync flush") < 0)
    {
        ;
    }
#endif

    if (nRet < 0)
    {
        mSync_ClientLog("ERROR : %s\n", mSync_GetSyncError());
        return -1;
    }
    else    /* if(nRet == 0) */
    {
        mSync_ClientLog("DownLoad Success ----------\n");
    }

    return 0;
}


int
DoSyncDB(char *user, char *pwd)
{
    int nRet;

    mSync_InitializeClientSession("==open_mSync.log", g_timeout);
    if (mSync_ConnectToSyncServer(g_SrvInfo._szSrvAddr, g_SrvInfo._nPort) < 0)
    {
        mSync_ClientLog("[ERROR] Connect To Sync Server : \n");
        printf("���� ���ῡ �����߽��ϴ�.\n");

        return -1;
    }

    /* ����� ���� */
    nRet = mSync_AuthRequest(user, pwd,
            g_SrvInfo._appName, g_SrvInfo._version);
    if (nRet < 0)
    {
        mSync_ClientLog("AuthRequest Error\n");
        nRet = -1;
    }
    else if (nRet == NACK_FLAG)
    {
        mSync_ClientLog("Auth Fail\n");
        nRet = -1;
    }
    else if (nRet == UPGRADE_FLAG)
    {
        mSync_ClientLog("Need Upgrade...\n");
        mSync_Disconnect(NORMAL);

        return 1;
    }
    else if (nRet != ACK_FLAG)
    {
        mSync_ClientLog("Auth Fail... returned %d \n", nRet);
        nRet = -1;
    }
    else
    {
        g_sync_mode = ALL_SYNC; /* ��ü����ȭ */
        //g_sync_mode = MOD_SYNC; /* �κе���ȭ */      

        nRet = SYNC_TABLE(g_clientDSN, g_serverTable, g_sync_mode);
        if (nRet < 0)
        {
            mSync_ClientLog("����ȭ �۾��� ����\n");
            printf("����ȭ �۾��� �����Ͽ����ϴ�.\n");
        }
        else
        {
            mSync_ClientLog("����ȭ �۾� ����\n");
            nRet = 0;
        }
    }

    if (nRet < 0)
    {
        if (mSync_Disconnect(ABNORMAL) < 0)
            mSync_ClientLog("mSync_Disconnect ==> %s\n", mSync_GetSyncError());
    }
    else
    {
        if (mSync_Disconnect(NORMAL) < 0)
        {
            mSync_ClientLog("mSync_Disconnect ==> %s\n", mSync_GetSyncError());
            return -2;
        }
    }

    return nRet;
}


void
get_inputstring(char *pBuf)
{
    int ch, i;

    i = 0;
    while ((ch = getchar()) != EOF)
    {
        if (ch == '\n')
            break;

        pBuf[i++] = ch;
    }
    pBuf[i] = '\0';
}

int
insert2LocalDB(int newid)
{
    S_Recoed sRec;
    char query[8192];

    if (newid < 0)
    {
        int i;
        char tmp[32];

        printf("\nEnter new contact....\n");
      retry_id:
        printf("%-7s: ", "ID");
        get_inputstring(tmp);
        //
        for (i = 0; tmp[i] != '\0'; i++)
        {
            if (('0' > tmp[i]) || ('9' < tmp[i]))
                break;
        }

        if (i == 0 || tmp[i] != '\0')
        {
            printf("\nRetry...\n");
            goto retry_id;
        }

        memset(&sRec, 0x00, sizeof(S_Recoed));
        sRec.ID = atoi(tmp);
    }
    else
    {
        printf("Enter new contact....\n");
        memset(&sRec, 0x00, sizeof(S_Recoed));
        sRec.ID = newid;
    }

    strcpy(sRec.username, g_pClientInfo->_userid);

    printf("%-7s: ", "Company");
    get_inputstring(sRec.Company);

    printf("%-7s: ", "Name");
    get_inputstring(sRec.Name);

    printf("%-7s: ", "E-Mail");
    get_inputstring(sRec.e_mail);

    printf("%-7s: ", "Phone");
    get_inputstring(sRec.Company_phone);

    printf("%-7s: ", "Mobile");
    get_inputstring(sRec.Mobile_phone);

    printf("%-7s: ", "Address");
    get_inputstring(sRec.Address);

#ifdef USE_MSYNC_TABLE
    sprintf(query,
            "insert into contact(id, username, company, name, e_mail, company_phone,"
            "mobile_phone, address, mtime) "
            "values(%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now())",
            sRec.ID, sRec.username, sRec.Company, sRec.Name, sRec.e_mail,
            sRec.Company_phone, sRec.Mobile_phone, sRec.Address);
#else
    sRec.dflag = DFLAG_INSERTED;        /* DFLAG_UPDATED */
    sprintf(query,
            "insert into contact(id, username, company, name, e_mail, company_phone,"
            "mobile_phone, address, mtime, dflag) "
            "values(%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now(), %d)",
            sRec.ID, sRec.username, sRec.Company, sRec.Name, sRec.e_mail,
            sRec.Company_phone, sRec.Mobile_phone, sRec.Address, sRec.dflag);
#endif

    return QueryLocalDB(query);
}

int
update2LocalDB(int update_id)
{
    char tmp[8192];
    int nRows, nRet = 0;
    S_Recoed oldRec, newRec;

#ifdef USE_MSYNC_TABLE
    sprintf(tmp, "select company, name, e_mail, company_phone, mobile_phone, "
            "address, mtime from contact where id = %d and username = '%s'",
            update_id, g_pClientInfo->_userid);
#else
    sprintf(tmp, "select company, name, e_mail, company_phone, mobile_phone, "
            "address, mtime, dflag from contact where dFlag <> %d "
            "and id = %d and username = '%s'",
            DFLAG_DELETED, update_id, g_pClientInfo->_userid);
#endif

    nRows = SelectLocalDB(tmp);
    if (nRows < 0)
    {
        nRet = -1;
        goto return_pos;
    }
    else if (nRows == 0)
    {
        nRet = 0;
        goto return_pos;
    }

    memset(&oldRec, 0x00, sizeof(S_Recoed));
    memset(&newRec, 0x00, sizeof(S_Recoed));
    if (FetchLocalDB(&oldRec) <= 0)
    {
        nRet = -2;
        goto return_pos;
    }

    newRec.ID = update_id;
    strcpy(newRec.username, g_pClientInfo->_userid);
    printf("\nEnter update contact....\n");

    printf("%-7s: %d\n", "ID", update_id);
    printf("----------------------------------------------------\n");
    printf("%-7s: %s ", "Company", oldRec.Company);
    printf("%-7s: %s ", "Name", oldRec.Name);
    printf("%-7s: %s ", "E-Mail", oldRec.e_mail);
    printf("%-7s: %s ", "Phone", oldRec.Company_phone);
    printf("%-7s: %s ", "Mobile", oldRec.Mobile_phone);
    printf("%-7s: %s ", "Address", oldRec.Address);
    printf("\n----------------------------------------------------\n");
    printf("%-7s: ", "Company");
    get_inputstring(newRec.Company);

    printf("%-7s: ", "Name");
    get_inputstring(newRec.Name);

    printf("%-7s: ", "E-Mail");
    get_inputstring(newRec.e_mail);

    printf("%-7s: ", "Phone");
    get_inputstring(newRec.Company_phone);

    printf("%-7s: ", "Mobile");
    get_inputstring(newRec.Mobile_phone);

    printf("%-7s: ", "Address");
    get_inputstring(newRec.Address);

#ifndef USE_MSYNC_TABLE
    if (oldRec.dflag == DFLAG_SYNCED)
        newRec.dflag = DFLAG_UPDATED;
    else
        newRec.dflag = oldRec.dflag;
#endif

    QuitSelectLocalDB();

#ifdef USE_MSYNC_TABLE
    sprintf(tmp,
            "update contact set company = '%s', name = '%s', e_mail = '%s',"
            "company_phone = '%s', mobile_phone = '%s', address = '%s', "
            "mtime = now() where id = %d and username = '%s'",
            newRec.Company, newRec.Name, newRec.e_mail, newRec.Company_phone,
            newRec.Mobile_phone, newRec.Address, newRec.ID, newRec.username);
#else
    sprintf(tmp,
            "update contact set company = '%s', name = '%s', e_mail = '%s',"
            "company_phone = '%s', mobile_phone = '%s', address = '%s', "
            "mtime = now(), dflag = %d where id = %d and username = '%s'",
            newRec.Company, newRec.Name, newRec.e_mail, newRec.Company_phone,
            newRec.Mobile_phone, newRec.Address, newRec.dflag, newRec.ID,
            newRec.username);
#endif

    return QueryLocalDB(tmp);
    //return UpdateLocalDB(&newRec, 1);         

  return_pos:
    QuitSelectLocalDB();
    return nRet;
}

int
delete2LocalDB(int delete_id)
{
    char tmp[256];

#ifdef USE_MSYNC_TABLE
    sprintf(tmp, "delete from contact where id = %d and username = '%s'",
            delete_id, g_pClientInfo->_userid);
    return QueryLocalDB(tmp);
#else
    int nRows, nRet = 0;
    S_Recoed oldRec;

    sprintf(tmp, "select dflag from contact where dFlag <> %d "
            "and id = %d and username = '%s'",
            DFLAG_DELETED, delete_id, g_pClientInfo->_userid);

    nRows = SelectLocalDB(tmp);
    if (nRows < 0)
    {
        QuitSelectLocalDB();
        return -1;
    }
    else if (nRows == 0)
    {
        QuitSelectLocalDB();
        return 0;
    }

    memset(&oldRec, 0x00, sizeof(S_Recoed));
    oldRec.ID = delete_id;
    if (FetchLocalDB(&oldRec) <= 0)
    {
        QuitSelectLocalDB();
        return -2;
    }

    QuitSelectLocalDB();

    if (oldRec.dflag == DFLAG_INSERTED)
    {       // delete
        sprintf(tmp, "delete from contact where id = %d and username = '%s' ",
                delete_id, szuser);
        nRet = QueryLocalDB(tmp);
        //nRet = SyncDeleteLocalDB(delete_id, 1); 
    }
    else if (oldRec.dflag == DFLAG_SYNCED || oldRec.dflag == DFLAG_UPDATED)
    {       // update dflag        

        sprintf(tmp, "update contact set mtime = now(), dflag = %d "
                "where id = %d and username = '%s'",
                DFLAG_DELETED, delete_id, szuser);
        QueryLocalDB(tmp);
    }

    return 0;
#endif
}

int
DoLocal(char *user, char *pwd)
{
    char tmp[64];
    int i, _id;                 // = GetMaxIDLocalDB();


  retry_pos:
    printf("\nEnter action code (i, ux, dx, l, q, s)...\n");
    printf("usage i / u3 / d2 / l / q / s\n");

    get_inputstring(tmp);
    if (stricmp(tmp, "i") == 0)
    {
        insert2LocalDB(-1);
    }
    else if (stricmp(tmp, "l") == 0)
    {
        Show_LocalDB(1);
    }
    else if (stricmp(tmp, "q") == 0)
    {
        return 0;
    }
    else if (stricmp(tmp, "s") == 0)
    {
        i = DoSyncDB(user, pwd);
        if (i == 1)
        {   /* for upgrade */
            return 0;
        }
        else if (i < 0)
        {
            return -1;
        }
    }
    else
    {
        for (i = 1; tmp[i] != '\0'; i++)
        {
            if (('0' > tmp[i]) || ('9' < tmp[i]))
                break;
        }

        if (i == 1 || tmp[i] != '\0')
        {
            goto retry_pos;
        }

        _id = atoi(tmp + 1);

        if (tmp[0] == 'u' || tmp[0] == 'U')
        {
            update2LocalDB(_id);
        }
        else if (tmp[0] == 'd' || tmp[0] == 'D')
        {
            delete2LocalDB(_id);
        }
    }

    goto retry_pos;
}

int
main(int argc, char *argv[])
{
    int nRet;
    int nlen = 0;

    if (argc < 2)
        nRet = 0;
    else
        nRet = atoi(argv[1]);

    g_pClientInfo = &(g_ClientInfo[nRet]);

    printf("DB= %s / USER= %s \n",
            g_pClientInfo->_db_name, g_pClientInfo->_userid);
    nRet = InitLocalDB(g_pClientInfo->_db_name);
    if (nRet < 0)
        return nRet;

    Show_LocalDB(1);

    nRet = DoSyncDB(g_pClientInfo->_userid, g_pClientInfo->_passwd);
    if (nRet == 1)
    {
        /* for upgrade */
        QuitLocalDB();
        return 1;
    }
    else if (nRet < 0)
    {
        QuitLocalDB();
        return -1;
    }

    Show_LocalDB(1);

    nRet = DoLocal(g_pClientInfo->_userid, g_pClientInfo->_passwd);
    if (nRet >= 0)
    {
        Show_LocalDB(1);
    }

    QuitLocalDB();
    return 0;
}
