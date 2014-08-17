//--------------------------------------------------
// server class
//--------------------------------------------------
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <algorithm>

#include "servers.h"

//--------------------------------------------------
#define BUFF_SIZE 50

//--------------------------------------------------
static int sum;
static void add_up(const int &);

// KAbstractSocket
//--------------------------------------------------
KAbstractSocket::KAbstractSocket() :/*{{{*/
	s(-1)
{
}
/*}}}*/
//--------------------------------------------------
KAbstractSocket::KAbstractSocket(const int &sock) :/*{{{*/
	s(sock)
{
}
/*}}}*/
//--------------------------------------------------
KAbstractSocket::~KAbstractSocket()/*{{{*/
{
}
/*}}}*/
//--------------------------------------------------
bool KAbstractSocket::read(char *, int &) const/*{{{*/
{
return false;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractSocket::write(const char *, int &) const/*{{{*/
{
return false;
}
/*}}}*/
//--------------------------------------------------
void KAbstractSocket::setDescriptor(const int &sock)/*{{{*/
{
s=sock;
}
/*}}}*/
//--------------------------------------------------
int KAbstractSocket::getDescriptor() const/*{{{*/
{
return s;
}
/*}}}*/
//--------------------------------------------------
// KTCPSocket
//--------------------------------------------------
KTCPSocket::KTCPSocket()/*{{{*/
{
}
/*}}}*/
//--------------------------------------------------
KTCPSocket::KTCPSocket(const int &sock) :/*{{{*/
	KAbstractSocket(sock)
{
}
/*}}}*/
//--------------------------------------------------
KTCPSocket::~KTCPSocket()/*{{{*/
{
}
/*}}}*/
//--------------------------------------------------
bool KTCPSocket::read(char *buff, int &sz) const/*{{{*/
{
int sock, size;
if( (sock = getDescriptor()) < 0)
    { return false; }

size=recv(sock, buff, sz, 0);
sz=size;
return size == 0 ? false : true;
}
/*}}}*/
//--------------------------------------------------
bool KTCPSocket::write(const char *buff, int &sz) const/*{{{*/
{
int size, sock;
if( (sock = getDescriptor()) < 0)
    { return false; }

size=send(sock, buff, sz, 0);
sz=size;
return true;
}
/*}}}*/
//--------------------------------------------------
// KUDPSocket
//--------------------------------------------------
KUDPSocket::KUDPSocket()/*{{{*/
{
}
/*}}}*/
//--------------------------------------------------
KUDPSocket::KUDPSocket(const int &sock) :/*{{{*/
	KAbstractSocket(sock),
	peer_is_set(false)
{
}
/*}}}*/
//--------------------------------------------------
KUDPSocket::~KUDPSocket()/*{{{*/
{
}
/*}}}*/
//--------------------------------------------------
bool KUDPSocket::read(char *buff, int &sz) const/*{{{*/
{
int sock, size;
socklen_t sa_size;
struct sockaddr_in peer;

if( (sock = getDescriptor()) < 0)
    { return false; }

memset(buff, 0, sz);
memset(&peer, 0, sizeof(peer));
sa_size=sizeof(peer);
size=recvfrom(sock, buff, sz, 0, (struct sockaddr *)&peer, &sa_size);

if(size < 0)
    {
    perror("KUDPSocket::read");
    return false;
    }

sz=size;
if(sa_size >= 0)
    {
    peer_is_set=true;
    sa=peer;
    }
else
    {
    peer_is_set=false;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    }

return true;
}
/*}}}*/
//--------------------------------------------------
bool KUDPSocket::write(const char *buff, int &sz) const/*{{{*/
{
int size, sock, sa_size;
if( (sock = getDescriptor()) < 0)
    { return false; }

if(!peer_is_set)
    return false;

sa_size=sizeof(struct sockaddr_in);
size=sendto(sock, buff, sz, 0, (struct sockaddr *)&sa, sa_size);

sz=size;
return true;
}
/*}}}*/
//--------------------------------------------------
void KUDPSocket::setPeer(const struct sockaddr_in &peer)/*{{{*/
{
sa=peer;
peer_is_set=true;
}
/*}}}*/
//--------------------------------------------------
// KAbstractServer
//--------------------------------------------------
KAbstractServer::KAbstractServer() :/*{{{*/
	max_socket(2),
	threshold(0),
	listeners(new vector<KAbstractSocket>),
	clients(new deque<KAbstractSocket *>)
{
FD_ZERO(&sockets);
to.tv_sec=0;
to.tv_usec=0;
}
/*}}}*/
//--------------------------------------------------
KAbstractServer::~KAbstractServer()/*{{{*/
{
vector<KAbstractSocket>::iterator iter;
deque<KAbstractSocket *>::iterator iter2;

for(iter=listeners->begin(); iter != listeners->end(); iter++)
    close((*iter).getDescriptor());

for(iter2=clients->begin(); iter2 != clients->end(); iter2++)
    {
    close((*iter2)->getDescriptor());
    delete (*iter2);
    }

delete listeners;
delete clients;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::add_listening_socket(const KAbstractSocket &s)/*{{{*/
{
bool duplicate;
int s_desc;
vector<KAbstractSocket>::iterator iter;

duplicate=false;
s_desc=s.getDescriptor();
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    if(s_desc == (*iter).getDescriptor())
	{
	duplicate=true;
	break;
	}
    }

if(duplicate)
    return false;

listeners->push_back(s);
if(s_desc > max_socket)
    max_socket=s_desc;

return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::remove_listening_socket(const KAbstractSocket &s)/*{{{*/
{
int s_desc;
vector<KAbstractSocket>::iterator iter;

s_desc=s.getDescriptor();
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    if(s_desc == (*iter).getDescriptor())
	break;
    }

if(iter == listeners->end())
    return false;

listeners->erase(iter);
close(s_desc);
return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::add_udp_client(KUDPSocket *s)/*{{{*/
{
bool duplicate;
int s_desc;
deque<KAbstractSocket *>::iterator iter;

duplicate=false;
s_desc=s->getDescriptor();
for(iter=clients->begin(); iter != clients->end(); iter++)
    {
    if(s_desc == (*iter)->getDescriptor())
	{
	duplicate=true;
	break;
	}
    }

if(duplicate)
    return false;

clients->push_front(s);
threshold++;

if(s_desc > max_socket)
    max_socket=s_desc;

return true;
}
/*}}}*/
//--------------------------------------------------
bool KAbstractServer::process()/*{{{*/
{
vector<KAbstractSocket>::iterator iter;
deque<KAbstractSocket *>::iterator iter2;

FD_ZERO(&sockets);/*{{{*/
for(iter=listeners->begin(); iter != listeners->end(); iter++)
    FD_SET((*iter).getDescriptor(), &sockets);

for(iter2=clients->begin(); iter2 != clients->end(); iter2++)
    FD_SET((*iter2)->getDescriptor(), &sockets);
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
// process clients first
bool retval=true;
for(iter2=clients->begin(); iter2 != clients->end(); iter2++)
    {
    if(!FD_ISSET((*iter2)->getDescriptor(), &sockets))
	continue;

    if(!process_client(*iter2))
	{
	remove_client(*iter2);
	retval = clients->size() == threshold ? false : true;
	break;
	}
    else
	retval=true;
    }

// process listeners
int c=-1;
int l_desc;
KAbstractSocket *tmp;

for(iter=listeners->begin(); iter != listeners->end(); iter++)
    {
    l_desc=(*iter).getDescriptor();
    if(!FD_ISSET(l_desc, &sockets))
	continue;

    if( (c = accept(l_desc, NULL, NULL)) < 0)
	{
	perror("echod accept");
	retval=false;
	break;
	}

    tmp = new KTCPSocket(c);
    add_client(tmp);
    retval=true;
    }

return retval;
}
/*}}}*/
//--------------------------------------------------
void KAbstractServer::add_client(KAbstractSocket *client)/*{{{*/
{
if(client->getDescriptor() > max_socket)
    max_socket = client->getDescriptor();

clients->push_back(client);
}
/*}}}*/
//--------------------------------------------------
void KAbstractServer::remove_client(KAbstractSocket *client)/*{{{*/
{
int th;
int c_desc;
KAbstractSocket *dummy;
deque<KAbstractSocket *>::iterator iter;

c_desc=client->getDescriptor();
for(th=0, iter=clients->begin(); iter != clients->end(); iter++, th++)
    {
    if((*iter)->getDescriptor() == c_desc)
	break;
    }

if(iter == clients->end())
    return;

if(th < threshold)
    threshold--;

threshold = threshold < 0 ? 0 : threshold;

dummy = *iter;
clients->erase(iter);
close(c_desc);
delete dummy;
}
/*}}}*/
//--------------------------------------------------
// KEchoServer
//--------------------------------------------------
bool KEchoServer::process_client(const KAbstractSocket *client)/*{{{*/
{
bool retval;
char buff[BUFF_SIZE];
int sz;

sz=BUFF_SIZE;
retval=client->read(buff, sz);

if(!retval)
    return false;

if(sz != 0)
    client->write(buff, sz);

return true;
}
/*}}}*/
//--------------------------------------------------
// KEchoSummingServer
//--------------------------------------------------
bool KEchoSummingServer::process_client(const KAbstractSocket *client)/*{{{*/
{
bool retval;
char buff[BUFF_SIZE];
int sz;

sz=BUFF_SIZE;
retval=client->read(buff, sz);

if(!retval)
    return false;

if(sz == 0)
    return true;

// echo back
client->write(buff, sz);

// do the stuff
char *nl, *cur;

cur=buff;
nl=find(buff, buff+sz, '\n');
while(cur != nl)
    {
    if(isdigit(*cur))
	numbers.push_back((*cur) - '0');
    ++cur;
    }

if(numbers.size() == 0)
    return true;

int sum;
vector<char>::iterator iter;

sum=0;
for(iter=numbers.begin(); iter != numbers.end(); iter++)
    sum += (*iter);

sort(numbers.begin(), numbers.end());
printf("received:\nsum: %d\nmin: %d\nmax: %d\n",
       sum,
       numbers.front(),
       numbers.back());

vector<char>::reverse_iterator riter;
for(riter=numbers.rbegin(); riter != numbers.rend(); riter++)
    printf("%d ", *riter);
printf("\n");
fflush(stdout);

numbers.clear();
cur=++nl;
while(*cur != buff[sz])
    {
    if(isdigit(*cur))
	numbers.push_back((*cur) - '0');
    ++cur;
    }

return true;
}
/*}}}*/
//--------------------------------------------------
// static functions
//--------------------------------------------------
static void add_up(const int &x)/*{{{*/
{
sum += x;
}
/*}}}*/
//--------------------------------------------------
