#pragma once

#include <string>

#ifdef __linux__
#include <netinet/in.h>
#include <netdb.h>
#elif _WIN32

#include <winsock2.h>

#endif

namespace System
{
  class Socket
  {
  public:
    
    Socket();
    ~Socket();

    bool connect(const std::string& hostname, const int port);
    bool sendMessage(const std::string& message);
    std::string getMessage();

  private:

#ifdef __linux__
    int m_sockfd, m_count;
    sockaddr_in m_serv_addr;
    hostent* m_server;
#elif _WIN32
    WSADATA m_wsaData;
    SOCKET m_connectSocket;
    struct addrinfo *m_result,
                           m_hints;
#endif

    //this version will use only 2KiB of memory for messages
    char m_buffer[2048];
    int m_port;
    bool m_connected;

  };
}
