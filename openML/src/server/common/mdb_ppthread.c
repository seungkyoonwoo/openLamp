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
#include "mdb_inner_define.h"
#include "mdb_ppthread.h"
#include "ErrorCode.h"

#define PR_ERROR(f)

#if defined(sun) && !defined(USE_PPTHREAD)
pthread_once_t __once_init__ = { PTHREAD_ONCE_INIT };
#else
pthread_once_t __once_init__ = PTHREAD_ONCE_INIT;
#endif

#ifdef USE_PPTHREAD

#if defined(WIN32)
#define LCH_Lock(latch)        { while(InterlockedExchange(latch, 1)) Sleep(0); }
#define LCH_Unlock(latch)    { *latch = 0; }

/* thread ���� ���� */
static int latchvalue_thread = 0;
typedef struct thread_node thread_node_t;
struct thread_node
{
    DWORD threadid;
    HANDLE hThread;
    thread_node_t *next;
};
static thread_node_t *thread_list = NULL;

/* mutex ������ ���� ���� */
#define NUM_MUTEXES    150 * 2
static int fMutexInit = 0;
static struct mutex_list
{
    HANDLE hMutex;
} mutexList[NUM_MUTEXES];

/* event ������ ���� ���� */
#define NUM_CONDS    150 * 2
static int fCondInit = 0;
static struct cond_list
{
    HANDLE hEvent;
} condList[NUM_CONDS];

/* key ������ ���� ����
   thread exit�� �� specific data�� ���ְų�,
   process exit�� �� release �ȵ� ������ destory �ϱ� ���ؼ� */
struct dest_addr
{
    void *addr_;
    struct dest_addr *next_;
};

static struct key_list
{
    pthread_key_t key;
    void (*destructor) (void *);
    struct dest_addr *d_addr;
    struct key_list *next;
} *pKeyListHead = NULL;

static struct key_once_list
{
    pthread_once_t *key_once;
    struct key_once_list *next;
} *pKeyOnceListHead = NULL;

static int latchvalue_keylist = 0;
static int latchvalue_keyonce = 0;
#endif

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
__DECL_PREFIX int
ppthread_create(pthread_t * tid,
        const pthread_attr_t * attr, void *(*start) (void *), void *arg)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_kill 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param threadid :
 * \param sig : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_kill(pthread_t threadid, int sig)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_self 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX pthread_t
ppthread_self(void)
{
    return 1;
}

/*****************************************************************************/
//! ppthread_exit 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param retval :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX void
ppthread_exit(void *retval)
{
    return;
}

/*****************************************************************************/
//! ppthread_attr_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_attr_init(pthread_attr_t * attr)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_attr_setstacksize 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 * \param stacksize : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/

/*****************************************************************************/
//! ppthread_attr_setscope 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 * \param scope : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_attr_setscope(pthread_attr_t * attr, int scope)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_attr_setdetachstate 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 * \param detachstate : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_setconcurrency 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param new_level :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_setconcurrency(int new_level)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_condattr_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_condattr_init(pthread_condattr_t * attr)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutexattr_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutexattr_init(pthread_mutexattr_t * attr)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutexattr_setpshared 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 * \param pshared : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutexattr_setpshared(pthread_mutexattr_t * attr, int pshared)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutex_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param mutex :
 * \param mutexattr : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutex_init(pthread_mutex_t * mutex,
        const pthread_mutexattr_t * mutexattr)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutex_destroy 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param mutex :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutex_destroy(pthread_mutex_t * mutex)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutex_lock 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param mutex :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutex_lock(pthread_mutex_t * mutex)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_mutex_unlock 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param mutex :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_mutex_unlock(pthread_mutex_t * mutex)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_cond_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 * \param attr : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_cond_destroy 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_destroy(pthread_cond_t * cond)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_condattr_setpshared 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param attr :
 * \param pshared : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_condattr_setpshared(pthread_condattr_t * attr, int pshared)
{
    return 0;
}

#define COND_TIMEOUT 20
                        /* ms */

/*****************************************************************************/
//! ppthread_cond_wait 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 * \param mutex : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_cond_timedwait 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 * \param mutex : 
 * \param abstime :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_timedwait(pthread_cond_t * cond, pthread_mutex_t * mutex,
        const struct db_timespec *abstime)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_cond_broadcast 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_broadcast(pthread_cond_t * cond)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_cond_signal 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param cond :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_cond_signal(pthread_cond_t * cond)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_key_create 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param key :
 * \param destructor : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_key_create(pthread_key_t * key, void (*destructor) (void *))
{
    return 0;
}

/*****************************************************************************/
//! ppthread_key_delete 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param key :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_key_delete(pthread_key_t key)
{
    return 0;
}

/*****************************************************************************/
//! ppthread_once 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param once_control :
 * \param init_routine : 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
__DECL_PREFIX int
ppthread_once(pthread_once_t * once_control, void (*init_routine) (void))
{
    if (*once_control != PTHREAD_ONCE_INIT)
        return 0;

    if (*once_control == PTHREAD_ONCE_INIT)
    {
        init_routine();

        *once_control = ~PTHREAD_ONCE_INIT;
    }

    return 0;
}

/*****************************************************************************/
//! pthread_key_init 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 * thread�� dll�� attach�� �ҷ����� ���� ��ϵ� specific data�� ���Ͽ� �ʱ�ȭ 
 * ǥ�ؿ��� ���³�, ��32������ ���
 *****************************************************************************/
#if defined(WIN32)
int
pthread_key_init(void)
{
    return 0;
}


/*****************************************************************************/
//! pthread_key_destruct 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
/* thread�� dll�� detach�� �� �ҷ�����
   ���� ��ϵ� specific data���� ������ */
int
pthread_key_destruct(void)
{
    return 0;
}

/*****************************************************************************/
//! pthread_mutex_destroy_all 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
/* process exit�� �� �ҷ����� ���� �ִ� mutex�� ��� destroy ��Ŵ */
int
pthread_mutex_destroy_all(void)
{
    return 0;
}

/*****************************************************************************/
//! pthread_key_delete_all 
/*! \breif  �����ϰ� � �Լ�������Ʈ�� \n
 *          ������ ������ �̷�������
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note ���� �˰���\n
 *      �� ��Ÿ ����
 *****************************************************************************/
/* process exit�� �� �ҷ����� ���� �ִ� specific key�� ��� delete ��Ŵ */
int
pthread_key_delete_all(void)
{
    return 0;
}
#endif

#endif /* USE_PPTHREAD */
