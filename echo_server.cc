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
register int s, c;
int b;
struct sockaddr_in sa;

KAbstractServer *serv=new KEchoServer;

if( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0)/*{{{*/
    {
    perror("socket");
    return 1;
    }

memset(&sa, 0, sizeof(sa));

sa.sin_family = AF_INET;
sa.sin_port   = htons(10000);

if(INADDR_ANY)
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

if(bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
    perror("bind");
    return 2;
    }
/*}}}*/
switch(fork())/*{{{*/
    {
    case 0:
	break;

    default:
	close(s);
	delete serv;
	return 0;
	break;

    case -1:
	perror("fork");
	return 3;
	break;
    };
/*}}}*/
listen(s, BACKLOG);
if(!serv->add_listening_socket(s))
    {
    printf("echod: duplicate socket\n");
    return 4;
    }

while(serv->process())
    { }

printf("good-bye!\n");
return 0;
}

//--------------------------------------------------
