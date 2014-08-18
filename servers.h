//--------------------------------------------------
// server class
//--------------------------------------------------
#ifndef CONN_MAN_H
#define CONN_MAN_H

//--------------------------------------------------
#include <vector>
#include <deque>

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

//--------------------------------------------------
using namespace::std;

//--------------------------------------------------
class KAbstractSocket/*{{{*/
{

public:
    KAbstractSocket();
    KAbstractSocket(const int &);
    virtual ~KAbstractSocket();

    virtual bool read(char *, int &) const;
    virtual bool write(const char *, int &) const;

    void setDescriptor(const int &);
    int  getDescriptor() const;

private:
    int s;

};
/*}}}*/
//--------------------------------------------------
class KTCPSocket : public KAbstractSocket
{

public:
    KTCPSocket();
    KTCPSocket(const int &);
    ~KTCPSocket();

    bool read(char *, int &) const;
    bool write(const char *, int &) const;
};

//--------------------------------------------------
class KUDPSocket : public KAbstractSocket
{

public:
    KUDPSocket();
    KUDPSocket(const int &);
    ~KUDPSocket();

    bool read(char *, int &) const;
    bool write(const char *, int &) const;

    void setPeer(const struct sockaddr_in &);

private:
    mutable bool peer_is_set;
    mutable struct sockaddr_in sa;
};

//--------------------------------------------------
class KAbstractServer/*{{{*/
{

public:
    KAbstractServer();
    virtual ~KAbstractServer();

    bool add_listening_socket(const KAbstractSocket &);
    bool remove_listening_socket(const KAbstractSocket &);

    bool add_udp_client(KUDPSocket *);
    bool process();

protected:
    virtual bool process_client(const KAbstractSocket *) = 0;

    void add_client(KAbstractSocket *);
    void remove_client(KAbstractSocket *);

private:
    bool   done;
    int    max_socket;
    int    threshold; // index of last udp client on the deque
    fd_set sockets;
    struct timeval to;
    vector<KAbstractSocket> *listeners;
    deque<KAbstractSocket *> *clients;

};
/*}}}*/
//--------------------------------------------------
class KEchoServer : public KAbstractServer
{
protected:
    bool process_client(const KAbstractSocket *);
};

//--------------------------------------------------
class KEchoSummingServer : public KAbstractServer
{
protected:
    bool process_client(const KAbstractSocket *);

private:
    vector<char> numbers;

};

#endif
//--------------------------------------------------
