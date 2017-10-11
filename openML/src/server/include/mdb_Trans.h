/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Less General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Less General Public License for more details.

   You should have received a copy of the GNU Less General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TRANS_H
#define TRANS_H

#include "mdb_config.h"
#include "mdb_LSN.h"
#include "mdb_inner_define.h"
#include "mdb_LockMgrPI.h"
#include "mdb_ppthread.h"
#include "mdb_UndoTransTable.h"

#define MAX_TRANS             21
#define MAX_TRANS_ID         (MAX_TRANS * 1000000)

#define TRANS_STATE_BEGIN   1
#define TRANS_STATE_COMMIT  2
#define TRANS_STATE_ABORT   3
#define TRANS_STATE_PARTIAL_ROLLBACK 4

/*
fReadOnly
    - logging ���� ����
        LogMgr_WriteLogRecord()
    - undo ��������
        LogMgr_PartailUndoTransaction()
        LogMgr_UndoTransaction()
    - ���� ���ڵ带 ���� �� Ʈ����� �� ���� cursor�� ���ؼ� insert/update�� 
      ����� ��� ���ڵ带 �о� utime�� qsid ���� ������ skip�� �ؾߵǴµ� 
      �� �� ���ڵ带 �д� �δ��� ���̱� ���ؼ� fReadOnly�� �̸� �Ǵ���.
      ��, fReadOnly == 1 �̸� insert/update�� ���ڵ尡 ���� ������ ���ڵ带
      �о utime�� qsid�� ���� �ʿ䰡 ����. ��, filter�� ���� ���
        Iter_FindNext()
    - commit/abort �� log buffer flush�� ���� ����
      üũ����Ʈ�� �ϱ� ���� Ʈ����� ���� ���Ե��� ����
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - �⺻���� 1�� �Ǿ� �ְ� insert/update/remove �� operation�� �߻��ϸ�
      0���� ������. ��, temp table�� ��� 0���� �������� ����
        dbi_Cursor_Insert(), dbi_Cursor_Distinct_Insert()
        dbi_Cursor_SetNullField()
        dbi_Cursor_Remove(), dbi_Cursor_Update(), dbi_Cursor_Update_Field()
        dbi_Direct_Remove2()
        dbi_Direct_Remove(), dbi_Direct_Update()
        dbi_Direct_Update_Field(), dbi_Cursor_Upsert()
      DDL�� temp table ������� 0���� ������.
        dbi_Relation_Create()
        dbi_Relation_Rename(), dbi_Relation_Drop(), dbi_Relation_Truncate()
        dbi_Index_Create(), dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Rename(), dbi_Index_Drop(), dbi_Index_Rebuild()
        dbi_Sequence_Create(), dbi_Sequence_Alter(), dbi_Sequence_Drop()
        dbi_Sequence_Read(), dbi_Sequence_Nextval(), dbi_Relation_Logging()
    - 0�� trans�� ���� Ʈ��������� 0���� �����Ǿ� ����.
        TransMgr_Init()
      
fLogging
    - commit/abort �ÿ� log buffer flush�� ������
      üũ����Ʈ�� �ϱ� ���� Ʈ����� �� ������ ���Ե�
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - �⺻���� 0�̰�, write operaton�� �߻��ϸ� 1�� ������
      ��, temp table�� nologging table�� �ƴ� ��� ������
        dbi_Cursor_Insert(), dbi_Cursor_Distinct_Insert(),
        dbi_Cursor_SetNullField(), dbi_Cursor_Remove(), dbi_Cursor_Update()
        dbi_Cursor_Update_Field(), dbi_Direct_Remove()
        dbi_Direct_Remove2(), dbi_Direct_Update(), dbi_Direct_Update_Field()
        dbi_Relation_Create(), dbi_Relation_Rename(), dbi_Relation_Drop()
        dbi_Relation_Truncate()
        dbi_Index_Create(), dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Rename(), dbi_Index_Drop(), dbi_Index_Rebuild()
        dbi_Sequence_Create(), dbi_Sequence_Alter(), dbi_Sequence_Drop()
        dbi_Sequence_Read(), dbi_Sequence_Nextval(), dbi_Relation_Logging()
        dbi_Cursor_Upsert()
        
fDeleted (����ϴ� �� ����)
    - �⺻���� 0�̰�, remove operaton �߻��� 1�� ������
        dbi_Cursor_Remove(), dbi_Direct_Remove(), dbi_Direct_Remove2()
        dbi_Relation_Sync_removeResidue()
    - 0�� trans�� ���� Ʈ��������� 1�� �����Ǿ� ����
        TransMgr_Init()

fBeginLogged
    - begin transacton log�� write �Ǿ������� ǥ����.
      read only�� transaction�� ��� �α׸� ������� �ʱ� ������ �ʿ�.
      �⺻���� 0�̰�, write �Ǿ��ٸ� 1�� ������.
        LogMgr_WriteLogRecord()
    - 0�� trans�� ���� Ʈ��������� 1�� �����Ǿ� ����
        TransMgr_Init()

fddl
    - �⺻���� 0�̰� DDL ����Ǹ� 1�� ������
    - �����Ǿ� ������ commit/abort �� üũ����Ʈ �߻� ��Ŵ        
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - DDL operaton �߿����� �ʿ��� ������ ���
      temp table�� �ƴ� �� ����. relation create�� ���� log�� ���ؼ�
      recovery�� ������ �������� fddl ������ ������.
        dbi_Relation_Drop(), dbi_Relation_Truncate(), dbi_Index_Create()
        dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Drop(), dbi_Index_Rebuild()

fRU
    - for READ UNCOMMITTED isolation transaction.
      default 0. 1 if TX is set to READ_UNCOMMITTED.

fUseResult
    - for READ UNCOMMITTED isolation transaction.
      default 0. 1 if fetch using USE RESULT method

      NOTE: Combination of USE RESULT and index scan can cause invalid TTree
            node access under READ UNCOMMITTED transaction. Therefore, we
            should be carefull when cursor open time is older than TTree
            modification time when fUseResult are set to 1.
*/

struct Trans
{
    int id_;

    enum PRIORITY prio_;

    trans_lock_t trans_lock;

    int procId;                 // �� Ʈ������� �߻���Ų ���μ��� ��ȣ for shm connection
    pthread_t thrId;

    int clientType;
    int connid;                 /* connection ����� �� temp table �����ϱ� ���ؼ� */

    unsigned volatile int fRU:1,        // default 0. 1 if TX is set to READ_UNCOMMITTED.
      fUseResult:1,             // default 0. 1 if TX uses USE RESULT method
      fLogging:1,               //�ʱⰪ�� 0�̰� logging table ����� 1���� ����
      fReadOnly:1,              //�ʱⰪ�� 1�̰� update/insert/update �߻��� 0���� ����
      fDeleted:1,               //�ʱⰪ�� 0�̰� delete �߻��� 1�� ����
      fBeginLogged:1,           //�ʱⰪ�� 0�̰� begin log�� �������� 1�� ����
      fddl:1;                   //�ʱⰪ�� 0�̰� ddl�� ���������� 1�� ����
    unsigned volatile char fUsed;       //����������� ��Ÿ��

    DB_BOOL visited_;

    struct LSN First_LSN_;      // Ʈ������� ù��° LSN
    struct LSN Last_LSN_;       // ������ LSN
    struct LSN UndoNextLSN_;    // undo�� ���� ���� LSN, ������?

    struct LSN Implicit_SaveLSN_;       // implicit savepoint LSN for stmt atomicity 

    volatile DB_BOOL timeout_;
    volatile DB_BOOL aborted_;

    volatile DB_BOOL trans_lock_wait_;
    volatile DB_BOOL dbact_lock_wait_;

    unsigned char state;        /* TRANS_STATE_BEGIN, 
                                   TRANS_STATE_COMMIT, TRANS_STATE_ABORT */
    unsigned char interrupt;
    /* int num_operation; */

    int temp_serial_num;

    int tran_next;

    struct Cursor *cursor_list;

    pthread_mutex_t trans_mutex;        /* for sleeping (lock waiting) */
    pthread_cond_t wait_cond_;  /* for sleeping (lock waiting) */
};

struct Trans4Log
{
    int trans_id_;
    struct LSN Last_LSN_;
    struct LSN UndoNextLSN_;
};

struct TransMgr
{
    int NextTransID_;
    int Trans_Count_;
};

extern __DECL_PREFIX struct TransMgr *trans_mgr;
extern __DECL_PREFIX struct Trans *Trans_;

extern __DECL_PREFIX int my_trans_id;
extern __DECL_PREFIX struct Trans _trans_0;

int TransMgr_Init(void);
void Trans_Mgr_Free(void);
int *TransMgr_alloc_transid_key(void);
int TransMgr_InsTrans(int Trans_Num, int clientType, int connid);
int TransMgr_DelTrans(int);
void TransMgr_Set_NextTransID(int Trans_Num);
struct Trans *Trans_Search(int Trans_ID);
int Trans_Get_State(int Trans_ID);

int Trans_GetTransID_pid(int pid);

int TransMgr_trans_abort(int transid);

int TransMgr_Implicit_Savepoint(int transid);
int TransMgr_Implicit_Savepoint_Release(int transid);
int TransMgr_Implicit_Partial_Rollback(int transid);
int TransMgr_NextTempSerialNum(int *TransIdx, int *TempSerialNum);
int TransMgr_Find_By_ConnId(int connid);

int Trans_Mgr_AllTransGet(int *Count, struct Trans4Log **trans);
int TransTable_Insert(struct UndoTransTable *trans_table, struct LSN *lsn);
void TransTable_Remove_LSN(struct UndoTransTable *trans_table,
        struct LSN *lsn);
void TransTable_Replace_LSN(struct UndoTransTable *trans_table,
        struct LSN *lsn, struct LSN *return_lsn);
int Trans_Mgr_ALLTransDel(void);

#endif
