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

#include <string.h>
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#include <Nb30.h>
#include <direct.h>
#else
#include <inet/tcp.h>
#include <sys/socket.h>  
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  
#endif

#include "Sync.h"

struct timeval	tv;
static int		serverSocketFd;

/*****************************************************************************/
//! InitServerSocket
/*****************************************************************************/
/*! \breif  Server�� ���� �ʱ�ȭ �κ�
 ************************************
 * \param	text(in) : ������ port
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Socket�� �ʱ�ȭ�ϰ� �Ҵ� �޾� �̸� ������ port�� bind, listen��Ŵ\n
 *****************************************************************************/
int	InitServerSocket(int sport ) /* for server */
{
    struct	sockaddr_in	serv_addr;
#ifdef WIN32
    WSADATA wsaData;
#endif
    int		opton = 1;
    
    memset (&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons( sport );
    
#ifdef WIN32
    if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR) 
    {
        ErrLog("SYSTEM;There is some problems in Socket.;\n");
        WSACleanup();
        return (-1);
    }
#endif

    if (serverSocketFd != -1)
    {    
#ifdef WIN32
        closesocket(serverSocketFd);
#else
        close(serverSocketFd);
#endif
    }
    
    if ((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
    {
        ErrLog("SYSTEM;There is some problems in socket();\n");	
#ifdef WIN32        
        WSACleanup();
#endif
        return(-1);
	}

#ifndef WIN32   
    // Windeow������ SO_REUSEADDR ���� ���� port�� ���ؼ� bind error�� 
    // ��� ������� �ʾƼ� ������ ������ �ߴ� ��찡 �־� ������.
	setsockopt(serverSocketFd, 
               SOL_SOCKET, SO_REUSEADDR, (char *)&opton, sizeof(opton));
#endif

	setsockopt(serverSocketFd, 
               IPPROTO_TCP, TCP_NODELAY, (char *)&opton, sizeof(opton));
	{
        int bufsize = 1024 * 1024;
        // RCVBUF �������� local net������ upload�ð��� �پ��.
        setsockopt(serverSocketFd, 
                   SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize));        
	}

    if (bind(serverSocketFd, 
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) 
    {
#ifdef WIN32
        closesocket(serverSocketFd);
        WSACleanup();		
#else
        close(serverSocketFd);		
#endif
        ErrLog("SYSTEM;There is some problems in bind();\n");
        serverSocketFd = -1;
        return	(-1);
    }

    if (listen(serverSocketFd, 10000) == -1) 
    {    /* why 10 ? */
#ifdef WIN32
        closesocket(serverSocketFd);
        WSACleanup();		
#else
        close(serverSocketFd);		
#endif
        ErrLog("SYSTEM;There is some problems in listen();\n");
        serverSocketFd = -1;
        return	(-1);
    }
    
    return	(serverSocketFd);
}


/*****************************************************************************/
//! ProcessNetwork
/*****************************************************************************/
/*! \breif  ���ο� Ŀ��Ʈ�� ���� 
 ************************************
 * \param	clientAddr(out)	: Ŭ���̾�Ʈ�� �ּ�
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Client�κ��� connection�� �ξ����� accept�� �����ϸ� \n
 *			���ο� socket fd�� return�Ѵ�.
 *****************************************************************************/
int	ProcessNetwork(char *clientAddr)
{
    struct sockaddr_in	saddr ;
    int	   length, newSocketFd;
    
    
    length = sizeof(saddr);	
#ifdef WIN32
    newSocketFd = accept(serverSocketFd, (struct sockaddr *)&saddr, &length);
    if(newSocketFd == INVALID_SOCKET) 
#else
        newSocketFd = accept(serverSocketFd, 
        (struct sockaddr *)&saddr, (socklen_t *)&length);
    if(newSocketFd <0)
#endif
    {
#ifdef WIN32
        WSACleanup();
#endif
        return	-1;
    }
    
    memset(clientAddr, (char)0, sizeof(clientAddr));
    strcpy(clientAddr, (char *)inet_ntoa(saddr.sin_addr));
    
    return	newSocketFd;
}

/*****************************************************************************/
//! WriteToNet
/*****************************************************************************/
/*! \breif  ����Ÿ ���۽� ���
 ************************************
 * \param	fd(in)	: ���� ��ũ����
 * \param	buf(in) : ���� ������
 * \param	byte(in): ���� ����Ʈ�� ����
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	Socket�� data�� write�� �� ����ϴ� �Լ��� ���������δ� \n
 *			send()�� ȣ���ϸ� write ���н� -1�� return�Ѵ�
 *****************************************************************************/

int WriteToNet(int fd, char *buf, size_t byte)
{	
	int		ret, left, offset;

	left = byte;
	offset = 0;
	while(left > 0) 
    {
		ret = send(fd, (char* )(buf + offset), left, 0);
#ifdef WIN32
		if (ret == SOCKET_ERROR) {
#else
		if (ret < 0) {
#endif
			return -1;
		}
		
		offset += ret;
		left -= ret;
	}

	return(0);
}


/*****************************************************************************/
//! SetTimeout
/*****************************************************************************/
/*! \breif  Auth.ini�� ������ ���� timeout ���� setting �Ѵ�.
 ************************************
 * \param	timeout(in)	: 
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	Auth.ini�� ������ ���� timeout ������ ���õ� 
 *****************************************************************************/
void SetTimeout(int timeout) 
{
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
}

/*****************************************************************************/
//! ReadFrNet
/*****************************************************************************/
/*! \breif  ����Ÿ�� ���� ���� ����
 ************************************
 * \param	sock(in)	: ���� ��ũ����
 * \param	buf(out)	: ���� ����Ÿ�� ����
 * \param	byte(msgsz)	: ���� ����Ÿ�� ������
 ************************************
 * \return	0 : timeout : 1�ð� (3600��)
 *			>0 : bytes read
 *			<0 : error
 ************************************
 * \note	Socket���� data�� read�� �� ����ϴ� �Լ��� ���������δ� \n
 *			timeout�� üũ�ϱ� ���� select()�� recv()�� ȣ���ϸ� \n
 *			read ���� �� -1�� return �ϰ� ���� �ÿ��� ���� data length�� \n
 *			return �Ѵ�. ���� timeout �� �߻����� ������ 0�� return �Ѵ�. \n
 *			\n-- by jinki -- \n
 *			Ÿ�Ӿƿ��� �Ǹ� ������ �����̴�
 *****************************************************************************/
int	ReadFrNet(int sock, char *buf, size_t msgsz)
{
    int nleft, nread;
    char *ptr = (char *)buf;
    fd_set rfds;
    
    int retval;
    
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    
    retval = select(sock+1, &rfds, NULL, NULL, &tv);
    nleft = msgsz;
    *buf = '\0';
    if(retval>0)
    {	
        while (nleft > 0)
        {
            nread = recv(sock, ptr, nleft, 0);            
#ifdef WIN32
            if (nread == SOCKET_ERROR || nread <0){
#else
            if (nread <0){
#endif
                nleft = msgsz+1;
                goto ReadFrNet_END;	// return -1
            }
            else if (nread == 0){
                nleft = msgsz+1;
                goto ReadFrNet_END;	// return -1
            }
            nleft -= nread;
            ptr += nread;
        }
    }
    else if(retval<0){
        nleft = msgsz+1;
        goto ReadFrNet_END;	// return -1
    }
    else {
        goto ReadFrNet_END;	// return 0
    }
    
ReadFrNet_END:
    FD_CLR((u_int)sock, &rfds);
    return (msgsz - nleft); // ������ ��� nleft=0
}

