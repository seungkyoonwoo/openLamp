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
#include "mdb_Mem_Mgr.h"
#include "mdb_inner_define.h"
#include "mdb_syslog.h"
#include "mdb_LogRec.h"

int CLR_WriteCLR(struct LogRec *logrec);
int UpdateTransTable(struct LogRec *logrec);

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
AfterLog_Undo(struct LogRec *logrec, int flag)
{
    if (logrec->header_.op_type_ == AUTOINCREMENT)
        return DB_SUCCESS;
    if (flag == CLR)
        CLR_WriteCLR(logrec);

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
int
AfterLog_Redo(struct LogRec *logrec)
{
    int issys;
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    UpdateTransTable(logrec);

    if (logrec->header_.op_type_ == RELATION_MSYNCSLOT)
    {
        OID_LightWrite_msync(logrec->header_.oid_, NULL,
                logrec->header_.recovery_leng_, Cont,
                (DB_UINT8) (*logrec->data1));
    }
    else
    {
        OID_LightWrite(logrec->header_.oid_,
                (const char *) logrec->data1, logrec->header_.recovery_leng_);
    }

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}
