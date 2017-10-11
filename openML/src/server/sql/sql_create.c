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
#include "mdb_compat.h"

#include "sql_datast.h"
#include "sql_main.h"

#include "sql_decimal.h"
#include "mdb_er.h"

int make_work_table(int *handle, T_SELECT * stmt);
int SQL_fetch(int *handle, T_SELECT * select, T_RESULT_INFO * result);

/*****************************************************************************/
//!  exec_createtable_elements

/*! \breif  table�� ���������� �����ϴ� �Լ�
 ************************************
 * \param handle(in)        :
 * \param tabledesc(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  step1 : create table
 *  step2 : create primary key
 *  step3 : create unique index
 *
 *  - CREATE TABLE�� UNIQUE KEYWORD�� �����Ѵ�
 *  - CREATE TABLE�� DEFAULT ������ SYSDATE�� ����
 *      PUT_DEFAULT_VALUE() ��ũ�θ� ������
 *  - COLLATION �߰�
 *****************************************************************************/
static int
exec_createtable_elements(int handle, T_CREATETABLE * tabledesc)
{
    FIELDDESC_T *fieldDesc;
    T_ATTRDESC *attr = NULL;
    char pIndex[INDEX_NAME_LENG] = "\0";
    char str_dec[MAX_DEFAULT_VALUE_LEN] = "\0";
    char *lowname = NULL;
    char *defval = NULL;
    int i, j, ret;

    fieldDesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * tabledesc->col_list.len);
    if (fieldDesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    for (i = 0; i < tabledesc->col_list.len; i++)
    {
        attr = &(tabledesc->col_list.list[i]);

        if (!attr->defvalue.defined || !attr->defvalue.not_null)
            defval = NULL;
        else
            PUT_DEFAULT_VALUE(defval, attr, str_dec);

        ret = dbi_FieldDesc(attr->attrname, attr->type, attr->len,
                (MDB_INT8) attr->dec, attr->flag, defval, attr->fixedlen,
                &(fieldDesc[i]), attr->collation);
        if (ret < 0)
        {
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
    }

    lowname = tabledesc->table.tablename;

    {
        MDB_UINT16 cont_type;   /* container type */
        int table_max_records;
        char *table_column_name;

        switch (tabledesc->table_type)
        {
        case 0:        /* normal table */
            cont_type = _CONT_TABLE;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        case 2:        /* nologging table */
            // Cont Type Error
            cont_type = _CONT_NOLOGGINGTABLE;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        case 6:
            cont_type = _CONT_RESIDENT_TABLE;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        case 7:
            cont_type = _CONT_MSYNC_TABLE;
            table_max_records = 0;
            table_column_name = NULL;
            break;
        default:
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        ret = dbi_Relation_Create(handle, lowname,
                tabledesc->col_list.len, fieldDesc,
                cont_type, _USER_USER, table_max_records, table_column_name);
        if (ret < 0)
        {
            goto RETURN_LABEL;
        }
        else
            ret = RET_SUCCESS;
    }

    // step 2 : make primary key
    if (tabledesc->fields.len > 0)
    {       // primary key ����
        for (i = 0; i < tabledesc->fields.len; i++)
        {
            for (j = 0; j < tabledesc->col_list.len; j++)
            {
                attr = &(tabledesc->col_list.list[j]);
                if (!mdb_strcmp(attr->attrname, tabledesc->fields.list[i]))
                    break;
            }
#if defined(MDB_DEBUG)
            if (j == tabledesc->col_list.len)
                sc_assert(0, __FILE__, __LINE__);

#endif
            ret = dbi_FieldDesc(tabledesc->fields.list[i], 0, 0, 0, 0, 0, -1,
                    &(fieldDesc[i]), attr->collation);
            if (ret < 0)
            {
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }
        }
        // index name�� ��ĥ �� �ִ�.
        // ��ġ�� �ʴ� index name�� ���ؼ��� db engine���� index name��
        // generation���־�߸� �Ѵ�.

        if (tabledesc->table.max_records > 0)
        {
            if (!mdb_strcmp(tabledesc->table.column_name,
                            tabledesc->fields.list[0]))
            {
                sc_snprintf(pIndex, INDEX_NAME_LENG, "%s%s", MAXREC_KEY_PREFIX,
                        lowname);

                ret = dbi_Index_Drop(handle, pIndex);
                if (ret < 0)
                {
                    goto RETURN_LABEL;
                }
            }
        }

        sc_snprintf(pIndex, INDEX_NAME_LENG, "%s%s", PRIMARY_KEY_PREFIX,
                lowname);

        ret = dbi_Index_Create(handle, tabledesc->table.tablename,
                pIndex, i, fieldDesc, UNIQUE_INDEX_BIT);
        if (ret < 0)
        {
            // ������� table�� ������.
            goto RETURN_LABEL;
        }
    }

  RETURN_LABEL:

    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        if (lowname)
            dbi_Relation_Drop(handle, lowname);
        ret = SQL_E_ERROR;
    }

    PMEM_FREENUL(fieldDesc);
    return ret;
}

/*****************************************************************************/
//! exec_createtable_query

/*! \breif  create as select ���� �̿��ؼ� table�� ���鶧 ���ȴ�
 ************************************
 * \param handle(in)            :
 * \param tabledesc :
 * \param traninfo() :
 * \param main_parsing_chunk(in):
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *  - parsing_chunk�� �Լ��� ���ڷ� �߰���
 *  - decimal type�� ��� attrdesc.len�� �����Ͽ� table�� ����
 *  - CREATE ���忡�� UNIQUE�� ������
 *  - COLLATION �߰�
 *
 *****************************************************************************/
static int
exec_createtable_query(int handle,
        T_CREATETABLE_QUERY * tabledesc,
        T_TRANSDESC * traninfo, T_PARSING_MEMORY * main_parsing_chunk)
{
    T_SELECT *select;
    T_QUERYDESC *qdesc;
    T_QUERYRESULT *qresult;
    T_LIST_SELECTION *selection;

    FIELDDESC_T *fielddesc = NULL;
    T_VALUEDESC *val;
    T_ATTRDESC *attr;
    OID inserted_rid;

    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    char *defval = NULL;
    char str_dec[MAX_DEFAULT_VALUE_LEN] = "\0";
    char lowname[REL_NAME_LENG + 1] = "\0";
    char *rec_buf = NULL;
    int Htable, rec_buf_len, nullflag_offset;
    int unknown_count = 0;
    int i, j, ret = RET_ERROR, ret_fetch;
    MDB_UINT8 flag;

    select = &tabledesc->query;
    select->main_parsing_chunk = main_parsing_chunk;

    qdesc = &select->planquerydesc.querydesc;

    if (qdesc->setlist.len > 0)
    {
        T_SELECT *tmp_select;

        tmp_select = qdesc->setlist.list[0]->u.subq;
        qresult = &tmp_select->queryresult;
        selection = &tmp_select->planquerydesc.querydesc.selection;
    }
    else
    {
        qresult = &select->queryresult;
        selection = &select->planquerydesc.querydesc.selection;
    }

    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * selection->len);
    if (!fielddesc)
    {
        ret = SQL_E_OUTOFMEMORY;
        goto RETURN_LABEL;
    }

    for (i = 0; i < selection->len; i++)
    {
        if (selection->list[i].expr.len == 1)
        {
            val = &selection->list[i].expr.list[0]->u.value;

            if (val->valueclass == SVC_VARIABLE)
            {
                if (!val->attrdesc.defvalue.defined ||
                        !val->attrdesc.defvalue.not_null)
                    defval = NULL;
                else
                {
                    attr = &val->attrdesc;
                    PUT_DEFAULT_VALUE(defval, attr, str_dec);
                }
                flag = val->attrdesc.flag & ~(PRI_KEY_BIT | UNIQUE_INDEX_BIT);
                if (!mdb_strcmp("rid", qresult->list[i].res_name))
                {
                    ret = SQL_E_INVALIDCOLUMN;
                    goto RETURN_LABEL;
                }

                for (j = 0; j < i; j++)
                {
                    if (!mdb_strcmp(fielddesc[j].fieldName,
                                    qresult->list[i].res_name))
                    {
                        ret = SQL_E_DUPLICATECOLUMNNAME;
                        goto RETURN_LABEL;
                    }
                }

                dbi_FieldDesc(qresult->list[i].res_name, val->attrdesc.type,
                        val->attrdesc.len, (MDB_INT8) val->attrdesc.dec, flag,
                        defval, val->attrdesc.fixedlen, &(fielddesc[i]),
                        val->attrdesc.collation);
            }
            else
            {
                for (j = 0; j < i; j++)
                {
                    if (!mdb_strcmp(fielddesc[j].fieldName,
                                    qresult->list[i].res_name))
                    {
                        ret = SQL_E_DUPLICATECOLUMNNAME;
                        goto RETURN_LABEL;
                    }
                }
                if (qresult->list[i].value.valuetype == DT_DECIMAL)
                {
                    dbi_FieldDesc(qresult->list[i].res_name,
                            qresult->list[i].value.valuetype,
                            qresult->list[i].value.attrdesc.len,
                            (MDB_INT8) qresult->list[i].value.attrdesc.dec,
                            NULL_BIT, NULL, -1, &(fielddesc[i]),
                            qresult->list[i].value.attrdesc.collation);
                }
                else
                {
                    dbi_FieldDesc(qresult->list[i].res_name,
                            qresult->list[i].value.valuetype,
                            qresult->list[i].len,
                            (MDB_INT8) qresult->list[i].value.attrdesc.dec,
                            NULL_BIT, NULL,
                            qresult->list[i].value.attrdesc.fixedlen,
                            &(fielddesc[i]),
                            qresult->list[i].value.attrdesc.collation);

                }
            }
        }
        else
        {
            if (mdb_strcmp(qresult->list[i].res_name, "?column?"))
            {
                mdb_lwrcpy(sizeof(lowname), lowname,
                        qresult->list[i].res_name);
            }
            else
            {
                sc_snprintf(lowname,
                        FIELD_NAME_LENG, "unknown_%d", unknown_count++);
            }

            for (j = 0; j < i; j++)
            {
                if (!mdb_strcmp(fielddesc[j].fieldName, lowname))
                {
                    ret = SQL_E_DUPLICATECOLUMNNAME;
                    goto RETURN_LABEL;
                }
            }
            if (qresult->list[i].value.valuetype == DT_DECIMAL)
            {
                dbi_FieldDesc(lowname, qresult->list[i].value.valuetype,
                        qresult->list[i].value.attrdesc.len,
                        (MDB_INT8) qresult->list[i].value.attrdesc.dec,
                        NULL_BIT, NULL, -1, &(fielddesc[i]),
                        qresult->list[i].value.attrdesc.collation);
            }
            else
                dbi_FieldDesc(lowname, qresult->list[i].value.valuetype,
                        qresult->list[i].len,
                        (MDB_INT8) qresult->list[i].value.attrdesc.dec,
                        NULL_BIT, NULL,
                        qresult->list[i].value.attrdesc.fixedlen,
                        &(fielddesc[i]),
                        qresult->list[i].value.attrdesc.collation);
        }
    }

    if (make_work_table(&handle, select) == RET_ERROR)
    {
        goto RETURN_LABEL;
    }

    qresult = &select->queryresult;
    selection = &select->planquerydesc.querydesc.selection;

    {
        MDB_UINT16 cont_type;   /* container type */
        int table_max_records;
        char *table_column_name;

        switch (tabledesc->table_type)
        {
        case 0:        /* normal table */
            cont_type = _CONT_TABLE;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        case 1:        /* temporary table */
            /* user temp table is not supported. hay, move it to check routine!!! */
            ret = DB_E_NOTSUPPORTED;
            goto RETURN_LABEL;
            break;
        case 2:        /* nologging table */
            cont_type = _CONT_NOLOGGING;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        case 6:
            cont_type = _CONT_RESIDENT_TABLE;
            table_max_records = tabledesc->table.max_records;
            table_column_name = tabledesc->table.column_name;
            break;
        default:
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        ret = dbi_Relation_Create(handle, tabledesc->table.tablename, i,
                fielddesc, cont_type, _USER_USER, table_max_records,
                table_column_name);

        if (ret < 0)
        {
            goto RETURN_LABEL;
        }
    }

    PMEM_FREENUL(fielddesc);

    ret = dbi_Schema_GetTableFieldsInfo(handle, tabledesc->table.tablename,
            &tableinfo, &fieldinfo);
    if (ret < 0)
    {
        dbi_Relation_Drop(handle, tabledesc->table.tablename);
        goto RETURN_LABEL;
    }

    rec_buf_len = get_recordlen(handle, NULL, &tableinfo);
    if (rec_buf_len < 0)
    {
        dbi_Relation_Drop(handle, tabledesc->table.tablename);
        ret = rec_buf_len;
        goto RETURN_LABEL;
    }

    rec_buf = PMEM_ALLOC(rec_buf_len);
    if (!rec_buf)
    {
        dbi_Relation_Drop(handle, tabledesc->table.tablename);
        ret = SQL_E_OUTOFMEMORY;
        goto RETURN_LABEL;
    }
    nullflag_offset = get_nullflag_offset(handle, NULL, &tableinfo);

    Htable = dbi_Cursor_Open(handle, tabledesc->table.tablename,
            NULL, NULL, NULL, NULL, LK_SCHEMA_EXCLUSIVE, 0);

    if (select->tmp_sub)
    {
        select = select->tmp_sub;
    }

    qresult = &select->queryresult;

    while (1)
    {
        ret_fetch = SQL_fetch(&handle, select, NULL);
        if (ret_fetch == RET_END)
        {
            ret = RET_SUCCESS;
            break;
        }
        else if (ret_fetch == RET_ERROR)
        {
            dbi_Relation_Drop(handle, tabledesc->table.tablename);
            ret = ret_fetch;
            break;
        }

        sc_memset(rec_buf, 0, rec_buf_len);

        for (i = 0; i < qresult->len; i++)
        {
            val = &qresult->list[i].value;
            if (!val->is_null)
            {
                ret = copy_value_into_record(-1, rec_buf + fieldinfo[i].offset,
                        val, &fieldinfo[i], 0);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                    ret = RET_ERROR;
                    goto RETURN_LABEL;

                }
            }
            else
            {
                DB_VALUE_SETNULL(rec_buf, i, nullflag_offset);
            }
        }

        ret = dbi_Cursor_Insert(handle, Htable, rec_buf, rec_buf_len,
                0, &inserted_rid);
        if (ret < 0)
        {
            dbi_Relation_Drop(handle, tabledesc->table.tablename);
            break;
        }
        if (ret_fetch == RET_END_USED_DIRECT_API)
        {
            ret = ret_fetch;
            break;
        }
    }
    dbi_Cursor_Close(handle, Htable);

  RETURN_LABEL:
    PMEM_FREENUL(fielddesc);
    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fieldinfo);

    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! exec_createindex

/*! \breif  SQL Engine���� index�� ������ ȣ��Ǵ� �Լ�
 ************************************
 * \param handle(in)        :
 * \param indexdesc(in/out) :
 * \param keyins_flag()     :
 * \param scandesc()        :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *  - COLLATION ����
 *      ������ ������ ���� ���ؼ�.. �ϴ� MDB_COL_CHAR_NOCASE�� �Ҵ�
 *  - GROUP INDEX�� ȣ���ϵ��� ����
 *
 *****************************************************************************/
static int
exec_createindex(int handle, T_CREATEINDEX * indexdesc,
        int keyins_flag, void *scandesc)
{
    FIELDDESC_T *fieldDesc;
    int i, ret;

    fieldDesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * indexdesc->fields.len);
    if (fieldDesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    for (i = 0; i < indexdesc->fields.len; i++)
    {
        MDB_UINT8 _type = 0;

        if (indexdesc->fields.list[i].ordertype == 'D')
            _type |= FD_DESC;
        // ������ ������ ���ϱ� ���ؼ�.. �ӽ÷� �Ҵ���
        // ���߿��� nchar�� Ÿ���� ��쵵 ������ ����� ��
        dbi_FieldDesc(indexdesc->fields.list[i].name, 0, 0, 0, _type, 0, -1,
                &(fieldDesc[i]), indexdesc->fields.list[i].collation);
    }

    ret = dbi_Index_Create_With_KeyInsCond(handle,
            indexdesc->table.tablename, indexdesc->indexname,
            indexdesc->fields.len, fieldDesc, indexdesc->uniqueness,
            keyins_flag, (SCANDESC_T *) scandesc);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        PMEM_FREENUL(fieldDesc);
        return SQL_E_ERROR;
    }

    PMEM_FREENUL(fieldDesc);
    return SQL_E_NOERROR;
}

/*****************************************************************************/
//! exec_create_view

/*! \breif CREATE VIEW ���� �����Ѵ�(view�� �����)
 ************************************
 * \param handle(in)    :
 * \param view(in/out)  :
 ************************************
 * \return SQL_E_NOERROR or some error
 ************************************
 * \note
 *  - COLLATION ����
 *****************************************************************************/
static int
exec_create_view(int handle, T_CREATEVIEW * view)
{
    FIELDDESC_T *fieldDesc;
    T_ATTRDESC *attr;
    int i, ret;

    /* add view table to systables/sysfields */
    fieldDesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * view->columns.len);
    if (fieldDesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    for (i = 0; i < view->columns.len; i++)
    {
        attr = &view->columns.list[i];
        dbi_FieldDesc(attr->attrname, attr->type, attr->len,
                (MDB_INT8) attr->dec, attr->flag, NULL, attr->fixedlen,
                &(fieldDesc[i]), attr->collation);
    }
    ret = dbi_Relation_Create(handle, view->name, view->columns.len, fieldDesc,
            _CONT_VIEW, _USER_USER, 0, NULL);
    if (ret < 0)
    {
        PMEM_FREENUL(fieldDesc);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    /* add view definition to sysviews */
    ret = dbi_ViewDefinition_Create(handle, view->name, view->qstring);
    if (ret < 0)
    {
        PMEM_FREENUL(fieldDesc);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    PMEM_FREENUL(fieldDesc);
    return SQL_E_NOERROR;
}

/*****************************************************************************/
//! __set_sequence_default_value

/*! \breif SEQUENCE�� DEFAULT VALUE�� �����ϴ� �Լ�
 ************************************
 * \param out(in/out)   :
 * \param in(in)        :
 ************************************
 * \return SQL_E_NOERROR or some error
 ************************************
 * \note
 *  - ��å
 *  1. INCREMENT BY
 *      ���� �������� ���� ��� 1�̴�.
 *  2. MINVALUE
 *      Sequence�� ���� �����Ǵ� ��� 1�̸�,
 *      Sequence�� ���� ���ҵǴ� ��� MIN_INT_VALUE�̴�.
 *  3. MAXVALUE
 *      Sequence�� ���� �����Ǵ� ��� MAX_INT_VALUE�̸�,
 *      Sequence�� ���� ���ҵǴ� ��� -1�̴�.
 *  4. START WITH
 *      SEQUENCE�� ���� �����ϴ� ���, SEQUENCE�� MINVALUE�� �Ҵ�ȴ�.
 *      SEQUENCE�� ���� �����ϴ� ���, SEQUENCE�� MAXVALUE�� �Ҵ�ȴ�.
 *
 *****************************************************************************/
static int
__set_sequence_default_value(DB_SEQUENCE * out, T_SEQUENCE * in)
{
    DB_INT8 bDescend = DB_FALSE;

    sc_memset(out, 0x00, sizeof(DB_SEQUENCE));

    // 1. check sequence's name
    sc_strcpy(out->sequenceName, in->name);

    if (in->bIncrement && in->increment < 0)
        bDescend = DB_TRUE;

    // 3. increase by
    if (in->bIncrement)
    {
        if (in->increment == 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_SEQUENCE_INVALID_INCNUM, 0);
            return SQL_E_ERROR;
        }
        out->increaseNum = in->increment;
    }
    else
        out->increaseNum = 1;

    // 4. minvalue
    if (in->bMinValue)
        out->minNum = in->minValue;
    else
    {
        if (bDescend)
            out->minNum = MDB_INT_MIN;
        else
            out->minNum = 1;
    }

    // 5. maxvalue
    if (in->bMaxValue)
        out->maxNum = in->maxValue;
    else
    {
        if (bDescend)
            out->maxNum = -1;
        else
            out->maxNum = MDB_INT_MAX;
    }

    // 6. start number
    if (in->bStart)
    {
        out->startNum = in->start;
    }
    else
    {
        if (bDescend)
            out->startNum = out->maxNum;
        else
            out->startNum = out->minNum;
    }

    // 7. CYCLEd
    out->cycled = in->cycled;


    return SQL_E_NOERROR;
}

/*****************************************************************************/
//! exec_create_sequence

/*! \breif SEQUENCE�� ����
 ************************************
 * \param handle(in)    :
 * \param sequence(in/out)  :
 ************************************
 * \return SQL_E_NOERROR or some error
 ************************************
 * \note
 *
 *****************************************************************************/
static int
exec_create_sequence(int handle, T_SEQUENCE * sequence)
{
    int ret = -1;
    DB_SEQUENCE dbSequence;

// SQL���� üũ�ϰ�, DBI���� 2�� üũ��..
    ret = __set_sequence_default_value(&dbSequence, sequence);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }

    ret = dbi_Sequence_Create(handle, &dbSequence);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }

    return SQL_E_NOERROR;
}

/*****************************************************************************/
//! create_temp_index

/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param handle(in) :
 * \param table(in/out) :
 * \param indexfields() :
 * \param indexname(in) :
 * \param keyins_flag :
 * \param scandesc :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *  - GROUP INDEX�� �����Ѵ�
 *****************************************************************************/
int
create_temp_index(int handle, T_TABLEDESC * table,
        T_IDX_LIST_FIELDS * indexfields,
        char *indexname, int keyins_flag, void *scandesc)
{
    T_CREATEINDEX indexdesc;

    indexdesc.table.tablename = table->tablename;
    indexdesc.table.aliasname = table->aliasname;
    indexdesc.table.correlated_tbl = NULL;
    indexdesc.indexname = indexname;
    indexdesc.uniqueness = 0;
    indexdesc.fields = *indexfields;

    return (exec_createindex(handle, &indexdesc, keyins_flag, scandesc));
}

/*****************************************************************************/
//! exec_create

/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param handle(in)     :
 * \param stmt(in/out)  :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *  - main stmt's parsing memory�� �Ѱ��־�� �Ѵ�
 *
 *****************************************************************************/
int
exec_create(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->u._create.type == SCT_TABLE_ELEMENTS)
    {
        ret = exec_createtable_elements(handle,
                &(stmt->u._create.u.table_elements));
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._create.type == SCT_TABLE_QUERY)
    {
        ret = exec_createtable_query(handle, &(stmt->u._create.u.table_query),
                stmt->trans_info, stmt->parsing_memory);
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._create.type == SCT_INDEX)
    {
        ret = exec_createindex(handle, &(stmt->u._create.u.index), 1, NULL);
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._create.type == SCT_VIEW)
    {
        ret = exec_create_view(handle, &(stmt->u._create.u.view));
        if (ret < 0)
            return RET_ERROR;
    }
    else if (stmt->u._create.type == SCT_SEQUENCE)
    {
        ret = exec_create_sequence(handle, &(stmt->u._create.u.sequence));
        if (ret < 0)
            return RET_ERROR;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOMMAND, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}
