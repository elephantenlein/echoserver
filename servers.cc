//--------------------------------------------------
// server class
//--------------------------------------------------
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "servers.h"

//--------------------------------------------------
#define BUFF_SIZE 50

//--------------------------------------------------
KAbstractServer::KAbstractServer() :/*{{{*/
	listeners(new vector<int>),
	tcp_clients(new vector<int>),
	udp_clients(new vector<int>)
{
}
/*}}}*/
//--------------------------------------------------
KAbstractServer::~KAbstractServer()/*{{{*/
{
vector<int>::iterator iter;

for(iter=udp_clients->begin(); iter != udp_clients->end(); iter++)
    close(*iter);

for(iter=tcp_clients->begin(); iter != tcp_clients->end(); iter++)
    close(*iter);

for(iter=listeners->begin(); iter != listeners->end(); iter++)
    close(*iter);

delete udp_clients;
delete tcp_clients;
delete listeners;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::add_listening_socket(int s)/*{{{*/
{
vector<int>::iterator iter;
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    if(s == (*iter))
	{
	s=-1;
	break;
	}
    }

if(s == -1)
    return false;

listeners->push_back(s);
if(s > max_socket)
    max_socket=s;

return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::remove_listening_socket(const int &s)/*{{{*/
{
vector<int>::iterator iter;
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    if(s == (*iter))
	break;
    }

if(iter == listeners->end())
    return false;

listeners->erase(iter);
close(s);
return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::add_udp_client(int s)/*{{{*/
{
vector<int>::iterator iter;
for(iter=udp_clients->begin(); iter != udp_clients->end(); iter++)
    {
    if(s == (*iter))
	{
	s=-1;
	break;
	}
    }

if(s == -1)
    return false;

udp_clients->push_back(s);
if(s > max_socket)
    max_socket=s;

return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::process()/*{{{*/
{
vector<int>::iterator iter;

FD_ZERO(&sockets);/*{{{*/
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    FD_SET((*iter), &sockets);

for(iter=tcp_clients->begin(); iter != tcp_clients->end(); iter++)
    FD_SET((*iter), &sockets);

for(iter=udp_clients->begin(); iter != udp_clients->end(); iter++)
    FD_SET((*iter), &sockets);
/*}}}*/
int who;/*{{{*/

to.tv_sec=0;
to.tv_usec=500;
who=select(max_socket+1, &sockets, NULL, NULL, &to);

if(who < 0)
    {
    perror("echod select");
    return false;
    }

if(who == 0)
    return true; // just go on, nothing happened
/*}}}*/
// process udp clients first
bool retval=true;
for(iter=udp_clients->begin(); iter != udp_clients->end(); iter++)
    {
    if(!FD_ISSET((*iter), &sockets))
	continue;

    process_udp_client(*iter);
    }

// process tcp clients next
for(iter=tcp_clients->begin(); iter != tcp_clients->end(); iter++)
    {
    if(!FD_ISSET((*iter), &sockets))
	continue;
    if(!process_tcp_client(*iter))
	{
	remove_tcp_client(iter);
	retval = tcp_clients->size() == 0 ? false : true;
	break;
	}
    else
	retval=true;
    }

// process listeners
int c=-1;
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    if(!FD_ISSET((*iter), &sockets))
	continue;

    if( (c = accept((*iter), NULL, NULL)) < 0)
	{
	perror("echod accept");
	retval=false;
	break;
	}

    add_client(c);
    retval=true;
    }

return retval;
}
/*}}}*/
//--------------------------------------------------
void KAbstractServer::add_client(int c)/*{{{*/
{
if(c > max_socket)
    max_socket = c;

tcp_clients->push_back(c);
}
/*}}}*/
//--------------------------------------------------
void KAbstractServer::remove_tcp_client(vector<int>::iterator &c)/*{{{*/
{
close(*c);
tcp_clients->erase(c);
}
/*}}}*/
//--------------------------------------------------
void KAbstractServer::remove_udp_client(vector<int>::iterator &c)/*{{{*/
{
close(*c);
udp_clients->erase(c);
}
/*}}}*/
//--------------------------------------------------
// KEchoServer
//--------------------------------------------------
bool KEchoServer::process_tcp_client(int c)/*{{{*/
{
char buff[BUFF_SIZE];
int sz;

sz=recv(c, &buff, BUFF_SIZE, 0);
if(sz > 0)
    {
    send(c, &buff, sz, 0);
    return true;
    }

return false;
}
/*}}}*/
//--------------------------------------------------
bool KEchoServer::process_udp_client(int c)/*{{{*/
{
char buff[BUFF_SIZE];
int sz;
socklen_t sa_sz;
struct sockaddr sa;

sz=recvfrom(c, &buff, BUFF_SIZE, 0, &sa, &sa_sz);
if(sz > 0)
    {
    sendto(c, &buff, sz, 0, &sa, sa_sz);
    return true;
    }

return false;
}
/*}}}*/
//--------------------------------------------------
