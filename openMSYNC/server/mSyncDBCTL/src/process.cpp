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

#include	<windows.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<errno.h>
#include	<string.h>	

#include	<direct.h>
#include	<io.h>
#include	<time.h>
#include	<SYS\TIMEB.H>

#include	"Sync.h"

#define		LOGHOME	"ServerLog"

int				logInit = TRUE;
static FILE*	pErrorLog = NULL ;
static char		errorLogFileName[256] ;

char				mSyncPath[200];
CRITICAL_SECTION	crit;

int  Due_Year, Due_Mon;

/*****************************************************************************/
//! SetPath 
/*****************************************************************************/
/*! \breif  Setting the mSync's Path
 ************************************
 * \param	productInfo(in) : ��ǰ������ ��� �մ� ����ü 
 ************************************
 * \return	void
 ************************************
 * \note	mSync�� path�� setting�ϴ� �Լ��� ���� ����Ǵ� mSync�� path��\n
 *			�������� �� �� �ֵ��� �α׿� �����.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void SetPath(char *path)
{
	sprintf(mSyncPath, "%s\\", path);
}


/*****************************************************************************/
//! TimeGet
/*****************************************************************************/
/*! \breif  ���� �ð� ���
 ************************************
 * \param	CurrentTime(out) : MON DAY, Year at 24hh:mm:ss ���·� ���
 ************************************
 * \return  ��ǰ�� Due �ð��� ���ؼ� ���� �־��ش�\n
 *			TRUE	: ��ǰ ��� �ð��� ����\n
 *			FALSE	: ��ǰ ��� �ð��� ���� ���� 
 ************************************
 * \note	���� �ð��� Ư���� ������ ���ڿ��� ��ȭ������
 *****************************************************************************/
int TimeGet(char *CurrentTime)
{
    char	*MON[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm	*tp;	
    time_t		tm;
    int			REMAIN_DAYS = TRUE;
    
    time(&tm);
    tp = localtime(&tm);
    memset(CurrentTime, 0x00, sizeof(CurrentTime));
    
    if(tp->tm_year + 1900 > Due_Year || 
        (tp->tm_year+1900 == Due_Year && tp->tm_mon >= Due_Mon )) 
    {
        REMAIN_DAYS = FALSE;
    }
    
    sprintf(CurrentTime, "%3s %02d, %04d at %02d:%02d:%02d", 
            MON[tp->tm_mon], tp->tm_mday,  tp->tm_year+1900, 
            tp->tm_hour, tp->tm_min, tp->tm_sec);
    
    return	REMAIN_DAYS;
}

/*****************************************************************************/
//! GetYYMMDD
/*****************************************************************************/
/*! \breif  ���� �ð��� Ư���� ������ ���ڿ��� ��ȭ������
 ************************************
 * \param	curTime(out) : MMDDHHMMSS ���·� ǥ�� 
 ************************************
 * \return  void
 ************************************
 * \note	���� �ð��� Ư���� ������ ���ڿ��� ��ȭ������
 *****************************************************************************/
void GetYYMMDD(char *curTime)
{
    struct _timeb tstruct;
    struct 	tm      *tp;	
    time_t tm;
    
    time(&tm);
    tp = localtime(&tm);
    _ftime( &tstruct );
    
    sprintf(curTime, "%02d%02d_%02d%02d%02d%03d", 
            tp->tm_mon+1, tp->tm_mday, 
            tp->tm_hour, tp->tm_min, tp->tm_sec, tstruct.millitm );
}

/*****************************************************************************/
//! ErrLog 
/*****************************************************************************/
/*! \breif  ���� �α׿� �Լ�
 ************************************
 * \param	char *format,... : �α׿� ����� �����
 ************************************
 * \return  void
 ************************************
 * \note	mSync���� �ʿ��� �α׸� log.txt�� �����ִ� �Լ��� LOG_FILE_SIZE��\n
 *			�ʰ��ϸ� ���� �α� ������ �ٸ� �̸����� move ���ְ� log.txt��\n
 *			����Ͽ� �α׸� �����.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void ErrLog(char *format,...)
{
    int     ret = 0;
    char	curTime[DATE_LEN + 1] ;	
    va_list	arg ;
    int		seek_file;
    char	newLogFileName[256] ;
    
    if(logInit)
    {
        memset(errorLogFileName, 0x00, sizeof(errorLogFileName)) ;
        sprintf(errorLogFileName, "%s%s", mSyncPath, LOGHOME);
        if (_access(errorLogFileName, 6) < 0)  // read & write permission
        {
            if (_mkdir(errorLogFileName) < 0) 
            {
                fprintf(stderr, "Log file open error[%s]\n", strerror(errno));
                exit(1) ;
            }
        }
        sprintf(errorLogFileName, "%s\\log.txt", errorLogFileName) ;
        
        GetYYMMDD(curTime);
        sprintf(newLogFileName, 
                "%s%s\\log_%s.txt", mSyncPath, LOGHOME, curTime);
        ret = rename(errorLogFileName, newLogFileName);
        pErrorLog = fopen(errorLogFileName, "w") ;
        
        if (pErrorLog == NULL)	
            exit(1);
        logInit = FALSE;
        fseek(pErrorLog, 0, SEEK_END);
        
        
        InitializeCriticalSection(&crit);
    }
    
    EnterCriticalSection(&crit);
    seek_file = ftell(pErrorLog);
    if(seek_file > LOG_FILE_SIZE)
    {	
        GetYYMMDD(curTime);
        sprintf(newLogFileName, 
                "%s%s\\log_%s.txt", mSyncPath, LOGHOME, curTime);
        fflush(pErrorLog);
        fclose(pErrorLog);
        ret = rename(errorLogFileName, newLogFileName);
        pErrorLog = fopen(errorLogFileName, "w") ;
    }
    
    TimeGet((char *)curTime);
    fprintf(pErrorLog,"%s;", curTime);
    
    va_start(arg, format) ;	
    vfprintf(pErrorLog, format, arg) ;
    
    if (fflush(pErrorLog) == EOF) 
    {
        fprintf(stderr, "fflush error[%s]\n", strerror(errno)) ;
        exit(1) ;
    }
    
    va_end(arg) ;
    
    LeaveCriticalSection(&crit);
}

/*****************************************************************************/
//! MYDeleteCriticalSection
/*****************************************************************************/
/*! \breif  ũ��ƼĮ �Ƽ�(���� ����)�� ����
 ************************************
 * \param	void	
  ************************************
 * \return  void
 ************************************
 * \note	Windows�뿡�� �α� ������ �����ϱ� ������ Critical section��\n
 *			����ϴµ� mSync ���� �ÿ� �� ���� �����Ѵ�.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void MYDeleteCriticalSection()
{
	DeleteCriticalSection(&crit);
}


/*****************************************************************************/
//! ParseData
/*****************************************************************************/
/*! \breif  ���� �ð� ���
 ************************************
 * \param	ptr(in)				: �Է¹��� ���ڿ� 
 * \param	uid(out)			: �Ľ̵� id
 * \param	passwd(out)			: �Ľ̵� pwd
 * \param	applicationID(out)	: �Ľ̵� application ID
 * \param	version(out)		: �Ľ̵� version(application)
 ************************************
 * \return  void
 ************************************
 * \note	Client�κ��� �Է¹��� ���۸� parsing �Ͽ� userid, passwd, \n
 *			application, version ������ return �Ѵ�. \n \n
 *			-- start jinki --\n
 *			������ �����Ѵٸ� pwd�� ��ȣȭ �����ְ� ��ȣȭ ������� �Ѵ�\n
 *			(���� ��Ŷ ����� Ŀ�� ���̴�)
 *****************************************************************************/
int	ParseData(char *ptr, char *uid, char *passwd, 
              char *applicationID, int *version)
{
    int		Idx = 0, s_Idx = 0, length;
    int		dataLength = strlen(ptr);
    char	tmpversion[VERSION_LEN+1];
    
    // UID �Ľ�
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > USER_ID_LEN) 
        return (-1);
    
    strncpy(uid, ptr+s_Idx, length);
    uid[length] = 0x00;
    if(ptr[Idx] == '|') Idx++;		// delimeter skip
    s_Idx = Idx;	
    
    // PWD �Ľ�
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > PASSWD_LEN) 
        return (-1);	
    
    strncpy(passwd, ptr+s_Idx, length);
    passwd[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;	
    
    // applicatonID �Ľ�
    while(Idx<dataLength && ptr[Idx] != '|')
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > APPLICATION_ID_LEN) 
        return (-1);	
    
    strncpy(applicationID, ptr+s_Idx, length);
    applicationID[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;	
    
    // version �Ľ�
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > VERSION_LEN) 
        return (-1);
    
    strncpy(tmpversion, ptr+s_Idx, length);
    tmpversion[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;
    *version = atoi(tmpversion);
    
    if(Idx == dataLength && ptr[Idx] != 0x00) 
        return (-1);
    
    return 0;
}

/*****************************************************************************/
//! DisplayError
/*****************************************************************************/
/*! \breif  due date�� setting
 ************************************
 * \param	hstmt(out)			:
 * \param	uid(in)				:
 * \param	order(in)			:
 * \param	returncoude(out)	:
 ************************************
 * \return	void
 ************************************
 * \note	Sync �߿� �߻��ϴ� DB ������ �޽����� ����ϴ� �Լ�.
 *****************************************************************************/
void DisplayError(SQLHSTMT hstmt, char *uid, int order, int returncode)
{
    SQLCHAR			SQLSTATE[6], Msg[256];
    SQLINTEGER		NativeError;
    SQLSMALLINT		MsgLen;
    
    Msg[0] = '\0';
    SQLSTATE[0] = '\0';
    SQLGetDiagRec(SQL_HANDLE_STMT, 
                  hstmt, 1, SQLSTATE, &NativeError, Msg, sizeof(Msg), &MsgLen);
    
    if (Msg[strlen((char *)Msg)-1]=='\n')
        Msg[strlen((char *)Msg)-1] = '\0';
    
    ErrLog("%d_%s;DBERROR[%d:%s]:%s;\n", order, uid, returncode, SQLSTATE, Msg);	
}
