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
#include "mdb_KeyValue.h"
#include "mdb_KeyDesc.h"
#include "mdb_TTreeNode.h"
#include "mdb_TTree.h"
#include "mdb_syslog.h"

int TTree_Increment(struct TTreePosition *);
int Index_Col_Remove(struct Collection *, NodeID);
struct TTreeNode *TTree_GlbChild(struct TTreeNode *);
int KeyValue_Create(const char *record, struct KeyDesc *key_desc,
        int recSize, struct KeyValue *keyValue, int f_memory_record);
int KeyValue_Delete2(struct KeyValue *pKeyValue);
struct TTreeNode *TTree_SmallestChild(struct TTree *);
struct TTreeNode *TTree_NextNode(struct TTreeNode *);

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

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
TTree_FindPositionWithID_Exaustive(struct TTree *ttree,
        RecordID record_id, struct TTreePosition *p)
{
    struct TTreeNode *node;
    DB_INT32 n;

    node = (struct TTreeNode *) TTree_SmallestChild(ttree);

    while (node != NULL)
    {
        for (n = 0; n < node->item_count_; n++)
        {
            if (node->item_[n] == record_id)
            {
#ifdef INDEX_BUFFERING
                p->self_ = node->self_;
#endif
                p->node_ = (struct TTreeNode *) node;
                p->idx_ = n;

                return DB_SUCCESS;
            }
        }

        node = (struct TTreeNode *) TTree_NextNode(node);
    }

#ifdef INDEX_BUFFERING
    p->self_ = NULL_OID;
#endif
    p->node_ = NULL;

    return DB_FAIL;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

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
 * \note 
 *****************************************************************************/
int
TTree_FindPositionWithValueAndID(struct TTree *ttree, const char *record,
        RecordID record_id, struct TTreePosition *p)
{
    struct KeyValue keyValue;

#ifdef INDEX_BUFFERING
    struct TTreePosition p1;
#endif
    struct TTreePosition p2;
    int result = DB_FAIL;
    int rec_count, max_rec_count;

    result = KeyValue_Create(record, &ttree->key_desc_,
            ttree->collect_.record_size_, &keyValue, 0);
    if (result < 0)
    {
        return result;
    }

    result = TTree_Get_First_Eq(ttree, &keyValue, p);
    if (result < 0)
    {
        if (result == DB_E_NOMORERECORDS)
        {
            int trans_state = Trans_Get_State(-1);

            if (trans_state != TRANS_STATE_ABORT &&
                    trans_state != TRANS_STATE_PARTIAL_ROLLBACK)
            {
            }
        }
        else
        {
            MDB_SYSLOG(("Tree Find First Position FAIL: %d\n", result));
        }
        goto End_Of_Function;
    }

#ifdef INDEX_BUFFERING
    /* TTree_Get_First_Eq()�� ���� p->node_�� buffer fix�� �ȵǾ� �ִ� �����̰�,
     * �Ʒ� TTree_Get_Last_Eq(.., p2)�� ���� �� ���� ��ü�� p.node_ �� �����
     * �� �����Ƿ� p.self_ �� �����ϱ� ���ؼ� p1�� ���. */
    p1 = *p;
#endif

    if (p->node_->item_[p->idx_] == record_id)
    {
        result = DB_SUCCESS;
        goto End_Of_Function;
    }

    result = TTree_Get_Last_Eq(ttree, &keyValue, &p2);
    if (result < 0)
    {
        MDB_SYSLOG(("Tree Find Last Position FAIL: %d\n", result));
        goto End_Of_Function;
    }

    if (p2.node_->item_[p2.idx_] == record_id)
    {
#ifdef INDEX_BUFFERING
        p->self_ = p2.self_;
#endif
        p->node_ = p2.node_;
        p->idx_ = p2.idx_;
        result = DB_SUCCESS;
        goto End_Of_Function;
    }

    rec_count = 0;
    max_rec_count = ttree->collect_.item_count_ * TNODE_MAX_ITEM;

#ifdef INDEX_BUFFERING
    /* ������ ����� p1�� *p�� �ű��, p->self_�� �̿��Ͽ� �ش� node��
     * �ٽ� �޸𸮿� �ø���. */
    *p = p1;
    p->node_ = (struct TTreeNode *) Index_OID_Point(p->self_);
    while ((p->self_ != p2.self_) || (p->idx_ != p2.idx_))
#else
    while ((p->node_ != p2.node_) || (p->idx_ != p2.idx_))
#endif
    {
        if (TTree_Increment(p) == DB_SUCCESS)
        {
            if (p->node_->item_[p->idx_] == record_id)
            {
                result = DB_SUCCESS;
                goto End_Of_Function;
            }
        }
        else
        {
            MDB_SYSLOG(("TTree Increment FAIL: inconsistent.\n"));
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
#ifdef INDEX_BUFFERING
            p->self_ = NULL_OID;
#endif
            p->node_ = NULL;
            result = DB_FAIL;
            goto End_Of_Function;
        }

        if (rec_count > max_rec_count)
        {
#ifdef INDEX_BUFFERING
            p->self_ = NULL_OID;
#endif
            p->node_ = NULL;
            result = DB_E_INDEXNODELOOP;
            goto End_Of_Function;
        }
        ++rec_count;
    }

#ifdef INDEX_BUFFERING
    p->self_ = NULL_OID;
#endif
    p->node_ = NULL;
    result = DB_E_NOMORERECORDS;

  End_Of_Function:
    KeyValue_Delete2(&keyValue);
    return result;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

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
TTree_Remove(struct TTree *ttree, struct TTreeNode *node,
        DB_INT16 index_in_node, int bucketnum)
{
#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node = node->self_;

    SetBufIndexSegment(issys_node, oid_node);
#endif

    if (TTreeNode_DeleteKey(node, index_in_node, ttree) < 0)
    {
        MDB_SYSLOG(("TTree Delete iteem FAIL\n"));
#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif
        return DB_FAIL;
    }

    if (node->item_count_ >= (TNODE_MIN_ITEM >> 1))
    {
#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif
        return DB_SUCCESS;
    }

    if (node->left_ != NULL_OID && node->right_ != NULL_OID)
    {
        struct TTreeNode *glb_child;
        RecordID rid_of_glb;

#ifdef INDEX_BUFFERING
        int issys_glb;
        OID oid_glb;
#endif

        glb_child = (struct TTreeNode *) TTree_GlbChild(node);
        rid_of_glb = glb_child->item_[glb_child->item_count_ - 1];

#ifdef INDEX_BUFFERING
        oid_glb = glb_child->self_;
        SetBufIndexSegment(issys_glb, oid_glb);
#endif

        (void) TTreeNode_InsertKey(node, 0, rid_of_glb, ttree);

        node = (struct TTreeNode *) glb_child;

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
        oid_node = oid_glb;
        /* glb_child�� node�� �����ϴ� �������� glb�� buffer unfix ��Ű��,
         * node�� �ٽ� buffer fix�� ���Ѿ���.
         * �̹� glb_child�� buffer fix�� �� ���¿��� �� �ڵ�
         *     SetBufIndexSegment(issys_glb, oid_glb);
         * �� �ϴ� ��� issys_glb�� 0���� �����Ǳ� ������ �Ʒ� �ڵ�
         *     issys_node = issys_gbl;
         * �� �ϴ� ���� �ǹ̰� ����. */
        UnsetBufIndexSegment(issys_glb);
        SetBufIndexSegment(issys_node, oid_node);
#endif

        index_in_node = node->item_count_ - 1;

        if (TTreeNode_DeleteKey(node, index_in_node, ttree) < 0)
        {
            MDB_SYSLOG(("TTree Delete 1 FAIL\n"));
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_FAIL;
        }

        if (node->left_ == NULL_OID)
        {
            if (node->item_count_ > 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                return DB_SUCCESS;
            }
        }
        else if (node->item_count_ >= TNODE_MIN_ITEM)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_SUCCESS;
        }
    }

    if (node->left_ != NULL_OID && node->right_ == NULL_OID)
    {
        if (TTree_Merge_With_Left_Child(ttree, node, bucketnum) < 0)
        {
            MDB_SYSLOG(("TTree remove() FAIL after merging\n"));
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_FAIL;
        }
        else
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_SUCCESS;
        }
    }
    else if (node->left_ == NULL_OID && node->right_ != NULL_OID)
    {
        if (TTree_Merge_With_Right_Child(ttree, node, bucketnum) < 0)
        {
            MDB_SYSLOG(("TTree remove FAIL since merging.\n"));
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_FAIL;
        }
        else
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_SUCCESS;
        }
    }
    else
    {
        if (node->item_count_ > 0)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_SUCCESS;
        }

        if (node->parent_ != NULL_OID)
        {
            NodeID node_to_del = node->self_;

            node = (struct TTreeNode *) Index_OID_Point(node->parent_);
            if (node == NULL)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                return DB_E_OIDPOINTER;
            }

#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node->self_;
            SetBufIndexSegment(issys_node, oid_node);
#endif

            if (Index_Col_Remove(&ttree->collect_, node_to_del) < 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                MDB_SYSLOG(("TTree remove item FAIL2\n"));
                return DB_FAIL;
            }

            if (node->left_ == node_to_del)
            {
                node->left_ = NULL_OID;
                if (node->right_ == NULL_OID)
                    node->height_ = 0;
                else
                {
                    struct TTreeNode *R;

                    R = (struct TTreeNode *) Index_OID_Point(node->right_);
                    if (R == NULL)
                    {
#ifdef INDEX_BUFFERING
                        UnsetBufIndexSegment(issys_node);
#endif
                        return DB_E_OIDPOINTER;
                    }
                    node->height_ = R->height_ + 1;
                }
                Index_SetDirtyFlag(node->self_);
            }
            else if (node->right_ == node_to_del)
            {
                node->right_ = NULL_OID;
                if (node->left_ == NULL_OID)
                    node->height_ = 0;
                else
                {
                    struct TTreeNode *L;

                    L = (struct TTreeNode *) Index_OID_Point(node->left_);
                    if (L == NULL)
                    {
#ifdef INDEX_BUFFERING
                        UnsetBufIndexSegment(issys_node);
#endif
                        return DB_E_OIDPOINTER;
                    }
                    node->height_ = L->height_ + 1;
                }
                Index_SetDirtyFlag(node->self_);
            }

            if (TTree_Rebalance_For_Delete(ttree, node, bucketnum) < 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                MDB_SYSLOG((" TTree remove FAIL 3\n"));
                return DB_FAIL;
            }
            else
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                return DB_SUCCESS;
            }
        }
        else
        {
            if (Index_Col_Remove(&ttree->collect_, node->self_) < 0)
            {
                MDB_SYSLOG(("TTree remove FAIL 4\n"));
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                return DB_FAIL;
            }

            ttree->root_[bucketnum] = NULL_OID;
            SetDirtyFlag(ttree->collect_.cont_id_);

#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_SUCCESS;
        }
    }
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

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
TTree_RemoveRecord(struct TTree *ttree, const char *record, RecordID record_id)
{
    struct TTreePosition p;
    int bucketnum = 0;
    int issys;
    int ret;

    SetBufSegment(issys, record_id);

    if (record == NULL)
        ret = TTree_FindPositionWithID_Exaustive(ttree, record_id, &p);
    else
        ret = TTree_FindPositionWithValueAndID(ttree, record, record_id, &p);
    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            int trans_state = Trans_Get_State(-1);

            if (trans_state != TRANS_STATE_ABORT &&
                    trans_state != TRANS_STATE_PARTIAL_ROLLBACK)
            {
                //MDB_SYSLOG(("TTree Find Position FAIL for Remove: NOREC\n"));
            }
        }
        else
        {
            //MDB_SYSLOG(("TTree Find Position FAIL for Remove: %d\n", ret));
        }
        goto end;
    }

#ifdef INDEX_BUFFERING
    p.node_ = (struct TTreeNode *) Index_OID_Point(p.self_);
    if (p.node_ == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto end;
    }
#endif
    ret = TTree_Remove(ttree, p.node_, p.idx_, bucketnum);

    ttree->modified_time_++;

  end:
    UnsetBufSegment(issys);

#ifdef CHECK_INDEX_VALIDATION
    {
        void TTree_Check_Root(struct TTree *ttree);

        TTree_Check_Root(ttree);
    }
#endif

    return ret;
}

int
TTree_FindRecord(struct TTree *ttree, char *record, RecordID record_id,
        int *found)
{
    struct TTreePosition p;
    int issys;
    int ret;

    SetBufSegment(issys, record_id);

    ret = TTree_FindPositionWithValueAndID(ttree, record, record_id, &p);
    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            *found = FALSE;
            ret = DB_SUCCESS;
        }
    }
    else
    {
        *found = TRUE;
    }

    UnsetBufSegment(issys);

    return ret;
}
