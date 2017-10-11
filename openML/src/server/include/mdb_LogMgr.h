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

#ifndef LOGMGR_H
#define LOGMGR_H

#include "mdb_config.h"
#include "mdb_LogAnchor.h"
#include "mdb_LogBuffer.h"
#include "mdb_LogFile.h"
#include "mdb_LogRec.h"
#include "mdb_LSN.h"
#include "mdb_ppthread.h"

/* connect ���� program���� log path�� ������ �� �ֵ��� �ϱ� ���� */
extern char log_path[];

struct LogMgr
{
    char log_path[MDB_FILE_NAME];       /* log ��ġ */
    struct LogAnchor Anchor_;   /* �α� ���ϰ� ���DB�� ���� ���� */
    struct LogBuffer Buffer_;   /* �޸� ���� �α� ���� */
    struct LogFile File_;       /* �α� ���� ���� */
    struct LSN Current_LSN_;    /* ���� �α׷��ڵ��� LSN */
    int reserved_File_No_;      /* ���� ���� ������ file ��ȣ */
};

extern __DECL_PREFIX struct LogMgr *Log_Mgr;
extern __DECL_PREFIX char *LogPage;

int LogMgr_init(char *Db_Name);
int LogMgr_Get_DB_Idx_Num(void);
int LogMgr_Restart(void);
void LogMgr_Free(void);
int LogMgr_WriteLogRecord(struct LogRec *logrec);
int LogMgr_PartailUndoTransaction(int Trans_ID, struct LSN *Rollback_LSN);
int LogMgr_UndoTransaction(int Trans_ID);
int LogMgr_buffer_flush(int f_sync);
int LogMgr_ReadLogRecord(struct LSN *lsn, struct LogRec *);
int LogMgr_Undo(void);
int LogMgr_Redo(struct LSN *Chkpt_Start_lsn);
int LogMgr_WriteBeginTransLogRecord(struct Trans *transaction);
int LogMgr_Check_LogPath(void);

#endif
