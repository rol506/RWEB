#pragma once

#include <string>
#include <optional>

#ifdef _WIN32
#include <winsock2.h>
#elif __linux__
#include <netinet/in.h>
#include <netdb.h>
#endif

#define SERVER_BUFLEN 64512

namespace rweb {

//cross-platform wrapper for socket
struct SOCKFD
{
#ifdef __linux__
  int sockfd;
#elif _WIN32
  SOCKET sockfd;
#endif
};

class Socket
{
public: 
  Socket(int clientQueue, int timeoutSeconds=20);
  ~Socket();
  std::optional<SOCKFD> acceptClient();
  bool sendMessage(SOCKFD clientSocket, const std::string& message);
  std::string getMessage(SOCKFD clientSocket);
  static void closeSocket(SOCKFD socket);

  const int timeout; // connection timeout in seconds

private:

#ifdef __linux__
  int m_count;
  sockaddr_in m_serv_addr;
  hostent* m_server;
#elif _WIN32
  WSADATA m_wsaData;
  struct addrinfo* m_result, m_hints;

#endif
  bool m_debug;
  bool m_connected;
  SOCKFD m_socket;
};


}
