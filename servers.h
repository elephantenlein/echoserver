//--------------------------------------------------
// server class
//--------------------------------------------------
#ifndef CONN_MAN_H
#define CONN_MAN_H

//--------------------------------------------------
#include <vector>

#include <stdbool.h>

//--------------------------------------------------
using namespace::std;

//--------------------------------------------------
class KAbstractServer
{

public:
    KAbstractServer();
    virtual ~KAbstractServer();

    bool add_listening_socket(int);
    bool remove_listening_socket(const int &);

    bool add_udp_client(int);
    bool process();

protected:
    virtual bool process_tcp_client(int) = 0;
    virtual bool process_udp_client(int) = 0;

    void add_client(int);
    void remove_tcp_client(vector<int>::iterator &);
    void remove_udp_client(vector<int>::iterator &);

private:
    int    max_socket;
    fd_set sockets;
    struct timeval to;
    vector<int> *listeners;
    vector<int> *tcp_clients;
    vector<int> *udp_clients;

};

//--------------------------------------------------
class KEchoServer : public KAbstractServer
{
protected:
    bool process_tcp_client(int);
    bool process_udp_client(int);
};

#endif
//--------------------------------------------------
