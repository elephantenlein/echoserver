//--------------------------------------------------
// main program
//--------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "servers.h"

//--------------------------------------------------
#define BACKLOG 4

//--------------------------------------------------
int main()
{
int tcp_s, udp_s;
struct sockaddr_in sa;

KAbstractSocket *incoming;
KUDPSocket *udp;
KAbstractServer *serv;

if( (tcp_s = socket(PF_INET, SOCK_STREAM, 0)) < 0)/*{{{*/
    {
    perror("socket");
    return 1;
    }

memset(&sa, 0, sizeof(sa));

sa.sin_family = AF_INET;
sa.sin_port   = htons(10000);

if(INADDR_ANY)
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

if(bind(tcp_s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
    perror("bind");
    return 2;
    }
/*}}}*/
if( (udp_s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)/*{{{*/
    {
    perror("socket");
    return 3;
    }

if(bind(udp_s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
    perror("bind");
    return 4;
    }
/*}}}*/
switch(fork())/*{{{*/
    {
    case 0:
	break;

    default:
	close(tcp_s);
	close(udp_s);
	return 0;
	break;

    case -1:
	perror("fork");
	return 5;
	break;
    };
/*}}}*/
listen(tcp_s, BACKLOG);

serv = new KEchoSummingServer;
incoming = new KAbstractSocket(tcp_s);
if(!serv->add_listening_socket(*incoming))
    {
    printf("echod: duplicate tcp socket\n");
    return 6;
    }

udp = new KUDPSocket(udp_s);
if(!serv->add_udp_client(udp))
    {
    printf("echod: duplicate udp socket\n");
    return 7;
    }

while(serv->process())
    { }

delete serv;
printf("good-bye!\n");
fflush(stdout);
return 0;
}

//--------------------------------------------------
