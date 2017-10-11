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

#include "mdb_config.h"
#include "dbport.h"

#include "mdb_Server.h"
#include "mdb_LogMgr.h"
#include "mdb_comm_stub.h"

int PhysicalLog_Make_BeforImage(struct PhysicalLog *physicallog, char *before);
int PhysicalLog_Make_AfterImage(struct PhysicalLog *physicallog, char *after);
int AfterLog_Init(struct AfterLog *afterlog);
int AfterLog_Make_AfterImage(struct AfterLog *afterlog, char *before);
int BeforeLog_Init(struct BeforeLog *beforelog);
int BeforeLog_Make_BeforeImage(struct BeforeLog *beforelog, char *before);
int MemAnchorLog_Init(struct MemAnchorLog *);

////////////////////////////////////////////////////////////
//
// Function Name : OID_HeavyWrite
// Call By : MemMgr_SetPrevPageID(), MemMgr_SetPageLink()
//  MemMgr_SetMemAnchor(), Collect_AllocateSlot(), Cont_CreateCont()
//  Col_Insert(), Col_Update(), Col_Remove()
//
////////////////////////////////////////////////////////////

/* LightWrite�� �α� record�� �ۼ����� �ʴ� ���̰�,
   HeavyWrite�� �α� record�� �ۼ���... */
/*****************************************************************************/
//! OID_LightWrite 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param curr :
 * \param data : 
 * \param size :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *  LightWrite�� �α� record�� �ۼ����� ����
 *****************************************************************************/
int
OID_LightWrite(OID curr, const char *data, DB_INT32 size)
{
    struct Page *page = (struct Page *) PageID_Pointer(curr);

    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer 2\n"));
        return DB_FAIL;
    }

    sc_memcpy((char *) page + OID_GetOffSet(curr), data, size);

    SetPageDirty(getSegmentNo(curr), getPageNo(curr));

    return DB_SUCCESS;
}

int
OID_LightWrite_msync(OID curr, const char *data, DB_INT32 size,
        struct Container *Cont, DB_UINT8 slot_flag)
{
    struct Page *page = (struct Page *) PageID_Pointer(curr);
    char *record;

    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer 2\n"));
        return DB_FAIL;
    }

    record = (char *) page + OID_GetOffSet(curr);

    if (data)
    {
        sc_memcpy(record, data, size);
    }

    if (slot_flag)
    {
        *(record + Cont->collect_.slot_size_ - 1) = slot_flag;
    }


    SetPageDirty(getSegmentNo(curr), getPageNo(curr));

    return DB_SUCCESS;
}

/*****************************************************************************/
//! OID_HeavyWrite 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param Op_Type :
 * \param Relation : 
 * \param curr :
 * \param data :
 * \param size : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *  HeavyWrite�� �α� record�� �ۼ���
 *****************************************************************************/
int
OID_HeavyWrite(short Op_Type, const void *Relation, OID curr,
        char *data, DB_INT32 size)
{
    struct Container *Cont = (struct Container *) Relation;
    struct Page *page;
    char *where;
    int *TxTrans_id;
    struct PhysicalLog physicallog;
    struct LogRec *logrec;

    page = (struct Page *) PageID_Pointer(curr);

    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer 3\n"));
        return DB_E_OIDPOINTER;
    }

    where = (char *) page + OID_GetOffSet(curr);

    /* temp table ó�� */
    if (isNoLogging(Cont))
    {
        sc_memcpy(where, data, size);
        SetDirtyFlag(curr);
        return DB_SUCCESS;
    }

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

    logrec = &(physicallog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    logrec->header_.op_type_ = Op_Type;
    logrec->header_.type_ = PHYSICAL_LOG;
    if (Cont == NULL)
    {
        logrec->header_.relation_pointer_ = NULL_OID;
        logrec->header_.tableid_ = 0;
    }
    else
    {
        logrec->header_.relation_pointer_ = Cont->collect_.cont_id_;
        logrec->header_.tableid_ = Cont->base_cont_.id_;
    }
    logrec->header_.collect_index_ = -1;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = curr;
    logrec->header_.recovery_leng_ = size * 2;
    logrec->header_.lh_length_ = size * 2;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    logrec->data1 = where;
    logrec->data2 = data;

    LogMgr_WriteLogRecord((struct LogRec *) &physicallog);

    sc_memcpy(where, data, size);

    SetPageDirty(getSegmentNo(curr), getPageNo(curr));

    return DB_SUCCESS;
}

int
OID_SlotAllocLog(short Op_Type, const void *Relation, OID curr,
        char *data, DB_INT32 size)
{
    struct Container *Cont = (struct Container *) Relation;
    struct Page *page;
    char *where;
    int *TxTrans_id;
    struct PhysicalLog physicallog;
    struct LogRec *logrec;

    if (isNoLogging(Cont))      /* temp table ó�� */
    {
        return DB_SUCCESS;
    }

    page = (struct Page *) PageID_Pointer(curr);
    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer 3\n"));
        return DB_FAIL;
    }
    where = (char *) page + OID_GetOffSet(curr);

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

    logrec = &(physicallog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    logrec->header_.op_type_ = Op_Type;
    logrec->header_.type_ = PHYSICAL_LOG;
    logrec->header_.relation_pointer_ = Cont->collect_.cont_id_;
    logrec->header_.tableid_ = Cont->base_cont_.id_;
    logrec->header_.collect_index_ = -1;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = curr;
    logrec->header_.recovery_leng_ = size * 2;
    logrec->header_.lh_length_ = size * 2;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    logrec->data1 = where;
    logrec->data2 = data;

    LogMgr_WriteLogRecord((struct LogRec *) &physicallog);

    return DB_SUCCESS;
}

/* type: 0 - alloc, n - dealloc */
/*****************************************************************************/
//! OID_AnchorHeavyWrite 
/*! \breif  Page Allocation/Deallocation�� ���� log�� ����� function
 ************************************
 * \param oid : page�� ���ϴ� catalog �Ǵ� table�� oid (Cont)
 * \param page_id : page oid
 * \param type : 0 - allocation, 1 - deallocation
 * \param alloc_flag : 0 - truncate, 1 - allocation, 2 - deallocation 
 ************************************
 * \return  return_value
 ************************************
 * \note log record ���� \n
 *                      alloction                   deallocation \n
 * ---------------------------------------------------------------------------\n
 *  op_type          RELATION_CATALOG_PAGEALLOC RELATION_CATALOG_PAGEDEALLOC \n
                     INDEX_CATALOG_PAGEALLOC    INDEX_CATALOG_PAGEDEALLOC \n
                     RELATION_PAGEALLOC         RELATION_PAGEDEALLOC \n
 *  type             PAGEALLOC_LOG                PAGEALLOC_LOG \n
 *  tableid_         0                          0 \n
 *  relation_pointer NULL_OID                   NULL_OID \n
 *  trans_id_        txid                       txid \n
 *  oid_             NULL_OID                   # of pages to be deleted \n
 *  length_          sizeof(OID)*2              sizeof(OID)*2 \n
 *  record_lsn_      LSN                        LSN \n
 *  trans_prev_lsn_  LSN                        LSN \n
 *  data1             oid                        oid \n
 *  data2            pageid                     pageid \n
 * \n
 *  *** oid_: deallocaton�� ��� free�Ǵ� page��(list)�� ���� ��� \n
 *  *** relation_pointer_: allocation�� ��� free page list���� next page�� \n
 *      oid ���� ����. \n
 *      page allocation redo �ÿ� �� page�� oid(table)�� \n
 *      list�� �޾��ְ�, relation_pointer_�� �ִ� ������ mem_anchor�� \n
 *      first_free_page_id_�� ���� �����Ͽ� �ش�. \n
 *      page deallocation redo �ÿ� �� page�� mem anchor�� free list�� \n
 *      �޾��ְ�, relation_pointer_�� �ִ� ������ oid (table)�� \n
 *      page link�� ���� �����Ͽ� �ش�. 
 *****************************************************************************/
int
OID_AnchorHeavyWrite(OID oid, int collect_index,
        int table_id, PageID page_id, PageID tail_id, PageID next_id,
        int free_count, int page_count, OID col_free_next, int alloc_flag)
{
    int *TxTrans_id;
    struct MemAnchorLog memanchorlog;
    struct LogRec *logrec;
    struct PageallocateLogData log_data;

#ifdef MDB_DEBUG
    sc_memset(&memanchorlog, 0, sizeof(memanchorlog));
#endif

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

    logrec = &(memanchorlog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    if (alloc_flag == 1)
    {
        if (oid == 1024)
            logrec->header_.op_type_ = RELATION_CATALOG_PAGEALLOC;
        else if (oid == 2048)
            logrec->header_.op_type_ = INDEX_CATALOG_PAGEALLOC;
        else
            logrec->header_.op_type_ = RELATION_PAGEALLOC;
    }
    else
    {
        if (oid == 1024)
            logrec->header_.op_type_ = RELATION_CATALOG_PAGEDEALLOC;
        else if (oid == 2048)
            logrec->header_.op_type_ = INDEX_CATALOG_PAGEDEALLOC;
        else
        {
            if (alloc_flag == 0)
                logrec->header_.op_type_ = RELATION_PAGETRUNCATE;
            else
                logrec->header_.op_type_ = RELATION_PAGEDEALLOC;
        }
    }

    logrec->header_.type_ = PAGEALLOC_LOG;
    logrec->header_.tableid_ = table_id;
    logrec->header_.collect_index_ = collect_index;
    logrec->header_.relation_pointer_ = oid;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = (PageID) page_id;
    logrec->header_.recovery_leng_ = sizeof(log_data);
    logrec->header_.lh_length_ = sizeof(log_data);
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    log_data.tail_id = tail_id;
    log_data.next_id = next_id;
    log_data.free_count = free_count;
    log_data.page_count = page_count;
    log_data.collect_free_page_list = col_free_next;
    logrec->data1 = (char *) &log_data;

#ifdef MDB_DEBUG
    sc_assert(OID_GetOffSet(next_id) == 0, __FILE__, __LINE__);
    sc_assert(free_count >= 0, __FILE__, __LINE__);
#endif

    LogMgr_WriteLogRecord(logrec);      /*(struct LogRec*) &memanchorlog); */

    SetPageDirty(getSegmentNo(oid), getPageNo(oid));


    if (*TxTrans_id >= 0)
    {
        Trans_[*TxTrans_id % MAX_TRANS].fLogging = 1;
    }

    return DB_SUCCESS;
}

/* pageid - last free page oid
   segment_id - new allocated segment id */
/*****************************************************************************/
//! function_name 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
int
OID_SegmentAllocLog(OID pageid, DB_UINT32 segment_id)
{
    int *TxTrans_id;
    OID sn = (OID) segment_id;
    struct MemAnchorLog memanchorlog;
    struct LogRec *logrec;

#ifdef MDB_DEBUG
    sc_memset(&memanchorlog, 0, sizeof(memanchorlog));
#endif

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

    logrec = &(memanchorlog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    logrec->header_.op_type_ = 0;
    logrec->header_.type_ = SEGMENTALLOC_LOG;
    logrec->header_.relation_pointer_ = NULL_OID;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = NULL_OID;
    logrec->header_.recovery_leng_ = sizeof(OID) * 2;
    logrec->header_.lh_length_ = sizeof(OID) * 2;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    logrec->data1 = (char *) &pageid;
    logrec->data2 = (char *) &sn;

    LogMgr_WriteLogRecord((struct LogRec *) &memanchorlog);

#ifdef MDB_DEBUG
    MDB_SYSLOG(("segment allocate log %ld, %ld\n", pageid, segment_id));
#endif

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
void
OID_SaveAsBefore(short Op_Type, const void *Relation, OID id_, int size)
{
    struct Page *page;
    char *where;
    struct Container *Cont = (struct Container *) Relation;
    int *TxTrans_id;
    struct BeforeLog beforelog;
    struct LogRec *logrec;

    /* temp table ó�� */
    if (isTempTable(Cont))
    {
        return;
    }

    page = (struct Page *) PageID_Pointer(id_);

    if (page == NULL)
    {
        MDB_SYSLOG(("Save After ERROR  since OID converte to Pointer \n"));
        return;
    }

    where = (char *) page;
    where = where + OID_GetOffSet(id_);
    if (Op_Type == RELATION_MSYNCSLOT)
    {
        where += Cont->collect_.slot_size_ - 1;
    }

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

#ifdef MDB_DEBUG
    sc_memset(&beforelog, 0, sizeof(beforelog));
#endif

    logrec = &(beforelog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    logrec->header_.op_type_ = Op_Type;
    logrec->header_.type_ = BEFORE_LOG;
    if (Cont == NULL)
        logrec->header_.relation_pointer_ = NULL_OID;
    else
    {
        logrec->header_.relation_pointer_ = Cont->collect_.cont_id_;
        logrec->header_.tableid_ = Cont->base_cont_.id_;
    }
    logrec->header_.collect_index_ = -1;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = id_;
    logrec->header_.recovery_leng_ = size;
    logrec->header_.lh_length_ = size;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    logrec->data1 = where;
    logrec->data2 = NULL;

    LogMgr_WriteLogRecord((struct LogRec *) &beforelog);
}

/*****************************************************************************/
//! function_name 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
void
OID_SaveAsAfter(short Op_Type, const void *Relation, OID id_, int size)
{
    struct Page *page;
    char *where;
    struct Container *Cont = (struct Container *) Relation;
    int *TxTrans_id;
    struct AfterLog afterlog;
    struct LogRec *logrec;

    /* temp table ó�� */
    if (isTempTable(Cont))
    {
        return;
    }

    page = (struct Page *) PageID_Pointer(id_);

    if (page == NULL)
    {
        MDB_SYSLOG(("Save After ERROR  since OID converte to Pointer \n"));
        return;
    }

    where = (char *) page;
    where = where + OID_GetOffSet(id_);
    if (Op_Type == RELATION_MSYNCSLOT)
    {
        where += Cont->collect_.slot_size_ - 1;
    }

    TxTrans_id = (int *) CS_getspecific(my_trans_id);

    logrec = &(afterlog.logrec);
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
    logrec->header_.op_type_ = Op_Type;
    logrec->header_.type_ = AFTER_LOG;
    if (Cont == NULL)
    {
        logrec->header_.relation_pointer_ = NULL_OID;
        logrec->header_.tableid_ = 0;
    }
    else
    {
        logrec->header_.relation_pointer_ = Cont->collect_.cont_id_;
        logrec->header_.tableid_ = Cont->base_cont_.id_;
    }
    logrec->header_.collect_index_ = -1;
    logrec->header_.trans_id_ = *TxTrans_id;
    logrec->header_.oid_ = id_;
    logrec->header_.recovery_leng_ = size;
    logrec->header_.lh_length_ = size;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));

    logrec->data1 = where;
    logrec->data2 = NULL;

    LogMgr_WriteLogRecord((struct LogRec *) &afterlog);

    SetPageDirty(getSegmentNo(id_), getPageNo(id_));

    return;
}

int
OID_InsertLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, char *item, int size, OID next_free_oid,
        DB_UINT8 slot_type)
{
    struct Container *Cont = (struct Container *) Relation;
    struct LogRec logrec;
    struct Collection *collect;
    struct Page *page;
    struct SlotUsedLogData s_log_data;

    if (isNoLogging(Cont))
        return DB_SUCCESS;

    collect = (collect_index == -1) ? &Cont->collect_ :
            &Cont->var_collects_[collect_index];
    page = (struct Page *) PageID_Pointer(id_); /* page != NULL */
    if (page == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    s_log_data.collect_new_item_count = collect->item_count_ + 1;
    s_log_data.page_new_record_count = page->header_.record_count_in_page_ + 1;
    s_log_data.page_new_free_slot_id = next_free_oid;
    s_log_data.page_cur_free_page_next = page->header_.free_page_next_;
    if (slot_type)
    {
        s_log_data.before_slot_type = FREE_SLOT;
        s_log_data.after_slot_type = slot_type;
    }
    else
    {
        s_log_data.before_slot_type = 0;
        s_log_data.after_slot_type = 0;
    }

#ifdef MDB_DEBUG
    if (getSegmentNo(s_log_data.page_new_free_slot_id) > SEGMENT_NO)
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    logrec.header_.check1 = logrec.header_.check2 = LOGREC_MAGIC_NUM;
    logrec.header_.op_type_ = Op_Type;
    logrec.header_.type_ = INSERT_LOG;
    logrec.header_.relation_pointer_ = Cont->collect_.cont_id_;
    logrec.header_.tableid_ = Cont->base_cont_.id_;
    logrec.header_.collect_index_ = collect_index;
    logrec.header_.trans_id_ = trans_id;
    logrec.header_.oid_ = id_;
    LogLSN_Init(&(logrec.header_.record_lsn_));
    LogLSN_Init(&(logrec.header_.trans_prev_lsn_));

    logrec.header_.recovery_leng_ = size + SLOTUSEDLOGDATA_SIZE;
    logrec.header_.lh_length_ = size + SLOTUSEDLOGDATA_SIZE;
    logrec.data1 = item;
    logrec.data2 = (char *) &s_log_data;
    logrec.data3 = NULL;

    LogMgr_WriteLogRecord(&logrec);

    return DB_SUCCESS;
}

int
OID_DeleteLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, int size, char *rep_data, int rep_size,
        DB_UINT8 slot_type)
{
    struct Container *Cont = (struct Container *) Relation;
    struct LogRec logrec;
    struct Collection *collect;
    struct Page *page;
    struct SlotFreeLogData s_log_data;

    if (isNoLogging(Cont))
        return DB_SUCCESS;

    collect = (collect_index == -1) ? &Cont->collect_ :
            &Cont->var_collects_[collect_index];
    page = (struct Page *) PageID_Pointer(id_); /* page != NULL */
    if (page == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    s_log_data.collect_new_item_count = collect->item_count_ - 1;
    s_log_data.page_new_record_count = page->header_.record_count_in_page_ - 1;
    s_log_data.page_cur_free_slot_id = page->header_.free_slot_id_in_page_;
    if (page->header_.free_page_next_ == FULL_PID)
        s_log_data.page_new_free_page_next = collect->free_page_list_;
    else
        s_log_data.page_new_free_page_next = NULL_PID;

    if (s_log_data.page_new_record_count < 0)
        s_log_data.page_new_record_count = 0;

    if (slot_type)
    {
        s_log_data.before_slot_type = slot_type;
        if (slot_type == DELETE_SLOT)
        {
            s_log_data.after_slot_type = FREE_SLOT;
        }
        else
        {
            s_log_data.after_slot_type = DELETE_SLOT;
        }
    }
    else
    {
        s_log_data.before_slot_type = 0;
        s_log_data.after_slot_type = 0;
    }

    logrec.header_.check1 = logrec.header_.check2 = LOGREC_MAGIC_NUM;
    logrec.header_.op_type_ = Op_Type;
    logrec.header_.type_ = DELETE_LOG;
    logrec.header_.relation_pointer_ = Cont->collect_.cont_id_;
    logrec.header_.tableid_ = Cont->base_cont_.id_;
    logrec.header_.collect_index_ = collect_index;
    logrec.header_.trans_id_ = trans_id;
    logrec.header_.oid_ = id_;
    LogLSN_Init(&(logrec.header_.record_lsn_));
    LogLSN_Init(&(logrec.header_.trans_prev_lsn_));

    logrec.header_.recovery_leng_ = size + SLOTFREELOGDATA_SIZE;
    logrec.header_.lh_length_ = size + SLOTFREELOGDATA_SIZE + rep_size;
    logrec.data1 = (char *) OID_Point(id_);
    if (logrec.data1 == NULL)
    {
        return DB_E_OIDPOINTER;
    }
    logrec.data2 = (char *) &s_log_data;
    logrec.data3 = rep_data;    /* primary key value */

    LogMgr_WriteLogRecord(&logrec);

    return DB_SUCCESS;
}

int
OID_UpdateLog(short Op_Type, int trans_id, const void *Relation, OID curr,
        char *data, DB_INT32 size, char *rep_data, int rep_size)
{
    struct Container *Cont = (struct Container *) Relation;
    struct LogRec logrec;

    if (isNoLogging(Cont))
        return DB_SUCCESS;

    logrec.header_.check1 = logrec.header_.check2 = LOGREC_MAGIC_NUM;
    logrec.header_.op_type_ = Op_Type;
    logrec.header_.type_ = UPDATE_LOG;
    logrec.header_.relation_pointer_ = Cont->collect_.cont_id_;
    logrec.header_.tableid_ = Cont->base_cont_.id_;
    logrec.header_.collect_index_ = -1;
    logrec.header_.trans_id_ = trans_id;
    logrec.header_.oid_ = curr;
    LogLSN_Init(&(logrec.header_.record_lsn_));
    LogLSN_Init(&(logrec.header_.trans_prev_lsn_));

    logrec.header_.recovery_leng_ = (size * 2);
    logrec.header_.lh_length_ = (size * 2) + rep_size;
    logrec.data1 = (char *) OID_Point(curr);
    if (logrec.data1 == NULL)
    {
        return DB_E_OIDPOINTER;
    }
    logrec.data2 = data;
    logrec.data3 = rep_data;    /* primary key value */

    LogMgr_WriteLogRecord(&logrec);

    if (isSequenceTable(logrec.header_.tableid_))
    {
        LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);
    }

    return DB_SUCCESS;
}

int
OID_UpdateFieldLog(short Op_Type, int trans_id, const void *Relation, OID curr,
        char *data, DB_INT32 size, char *rep_data, int rep_size)
{
    struct Container *Cont = (struct Container *) Relation;
    struct LogRec logrec;

    if (isNoLogging(Cont))
        return DB_SUCCESS;

    logrec.header_.check1 = logrec.header_.check2 = LOGREC_MAGIC_NUM;
    logrec.header_.op_type_ = Op_Type;
    logrec.header_.type_ = UPDATEFIELD_LOG;
    logrec.header_.relation_pointer_ = Cont->collect_.cont_id_;
    logrec.header_.tableid_ = Cont->base_cont_.id_;
    logrec.header_.collect_index_ = -1;
    logrec.header_.trans_id_ = trans_id;
    logrec.header_.oid_ = curr;
    LogLSN_Init(&(logrec.header_.record_lsn_));
    LogLSN_Init(&(logrec.header_.trans_prev_lsn_));

    logrec.header_.recovery_leng_ = size;
    logrec.header_.lh_length_ = size + rep_size;
    logrec.data1 = data;
    logrec.data2 = NULL;
    logrec.data3 = rep_data;    /* primary key value */

    LogMgr_WriteLogRecord(&logrec);

    if (isSequenceTable(logrec.header_.tableid_))
    {
        LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);
    }

    return DB_SUCCESS;
}
