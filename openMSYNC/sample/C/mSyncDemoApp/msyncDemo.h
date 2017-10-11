#ifndef _MSYNC_DEMO_H_
#define _MSYNC_DEMO_H_
typedef struct
{
    int ID;
    char username[32];
    char Company[256];
    char Name[256];
    char e_mail[256];
    char Company_phone[21];
    char Mobile_phone[21];
    char Address[256];
    char mTime[64];
#ifndef USE_MSYNC_TABLE
    int dflag;
    /*
       0: �ƹ��� �۾��� ����.

       1: local���� insert ��.
       2: local���� update ��.
       3: local���� delete ��.

       0: server���� �����ͼ� insert ��.
       0: server���� �����ͼ� update ��.
       6: server���� delete �Ǿ����� ������(?)
     */
#define DFLAG_INSERTED (1)
#define DFLAG_UPDATED  (2)
#define DFLAG_DELETED  (3)
#define DFLAG_SYNCED   (4)
#endif
} S_Recoed;





#endif // _MSYNC_DEMO_H_
