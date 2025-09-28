#include "../include/Socket.h"

#include <iostream>

#ifdef __linux__
#include <unistd.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>
#elif _WIN32
#include <ws2tcpip.h>
#endif

namespace rweb
{

int getPort();
bool getShouldClose();
void setShouldClose(bool _shouldClose);
std::string describeError();

Socket::Socket(int clientQueue)
: m_debug(false), m_connected(false)
{
#ifdef __linux__

  struct sockaddr_in serv_addr;

  m_socket = SOCKFD{ 0 };

  m_socket.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket.sockfd < 0)
  {
    std::cerr << "[ERROR] Can't open socket!\n";
    setShouldClose(true);
    return;
  }

  const int enable = 1;
  if (setsockopt(m_socket.sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
  {
    std::cerr << "[ERROR] setsockopt failed! (SO_REUSEADDR)\n";
    setShouldClose(true);
    return;
  }

  bzero(&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(getPort());

  if (bind(m_socket.sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cerr << "[ERROR] Failed to bind socket: " << describeError() << "\n";
    close(m_socket.sockfd);
    setShouldClose(true);
    return;
  }

  listen(m_socket.sockfd, clientQueue); 

#elif _WIN32

  m_result = NULL;

  m_socket = SOCKFD{INVALID_SOCKET};

  int iResult = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
  if (iResult != 0)
  {
    std::cerr << "[ERROR] WSAStartup failed with error: " << iResult << "\n";
    std::cerr << "[ERROR] Can't open socket!\n";
    setShouldClose(true);
  }

  ZeroMemory(&m_hints, sizeof(m_hints));
  m_hints.ai_family = AF_UNSPEC;
  m_hints.ai_socktype = SOCK_STREAM;
  m_hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  iResult = getaddrinfo(NULL, std::to_string(getPort()).c_str(), &m_hints, &m_result);
  if ( iResult != 0 ) {
    std::cerr << "[ERROR] getaddrinfo failed with error: " << iResult << "\n";
    WSACleanup();
    setShouldClose(true);
    return;
  }

  // Create a SOCKET for the server to listen for client connections.
  m_socket.sockfd = socket(m_result->ai_family, m_result->ai_socktype, m_result->ai_protocol);
  if (m_socket.sockfd == INVALID_SOCKET) {
    std::cerr << "[ERROR] socket failed with error: " << WSAGetLastError() << "\n";
    freeaddrinfo(m_result);
    WSACleanup();
    setShouldClose(true);
    return;
  }

  // Setup the TCP listening socket
  iResult = bind( m_socket.sockfd, m_result->ai_addr, (int)m_result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "[ERROR] bind failed with error: " << WSAGetLastError() << "\n";
    freeaddrinfo(m_result);
    closesocket(m_socket.sockfd);
    WSACleanup();
    setShouldClose(true);
    return;
  }

  freeaddrinfo(m_result);

  iResult = listen(m_socket.sockfd, clientQueue);
  if (iResult == SOCKET_ERROR) {
    std::cerr << "[ERROR] listen failed with error: " << WSAGetLastError() << "\n";
    closesocket(m_socket.sockfd);
    WSACleanup();
    setShouldClose(true);
    return;
  }
#endif
}

void Socket::closeSocket(SOCKFD socket)
{
#ifdef __linux__

  shutdown(socket.sockfd, SHUT_RDWR);
  close(socket.sockfd);

#elif _WIN32

  closesocket(socket.sockfd);

#endif
}

Socket::~Socket()
{

  m_connected = false;
  setShouldClose(true);

#ifdef __linux__

  bool err = false;

  if (shutdown(m_socket.sockfd, 0) && errno != ENOTCONN)
  {
    std::cerr << "[ERROR] Failed to shutdown server socket: " << describeError() << "\n";
    err = true;
  }
  if (::close(m_socket.sockfd))
  {
    std::cerr << "[ERROR] Failed to close server socket: " << describeError() << "\n";
    err = true;
  }

  if (!err)
  {
    std::cout << "[SERVER] Socket was shut down successfully!\n";
  } else {
    std::cerr << "[ERROR] Failed to shutdown socket!\n";
  }

#elif _WIN32
  closesocket(m_socket.sockfd);
  WSACleanup();

  std::cout << "[SERVER] Socket was shut down successfully!\n";
#endif
}

bool Socket::sendMessage(SOCKFD clientSocket, const std::string& message)
{
#ifdef __linux__
  m_count = write(clientSocket.sockfd, message.c_str(), message.size());
  if (m_count < 0)
  {
    if (m_debug)
      std::cerr << "[ERROR] Failed to write to socket!\n";
    return false;
  }

  return true;
#elif _WIN32

  int iResult = send(clientSocket.sockfd, message.c_str(), message.size(), 0);
  if (iResult == SOCKET_ERROR)
  {
    std::cerr << "[ERROR] send failed: " << WSAGetLastError() << "\n";
    return false;
  }

  return true;

#endif
}

std::string Socket::getMessage(SOCKFD clientSocket)
{

#ifdef __linux__
  int received = 0;

  std::string request(SERVER_BUFLEN, '\0');
  do {
    int n = read(clientSocket.sockfd, &request[0], SERVER_BUFLEN-1);
    if (n < 0)
    {
      shutdown(clientSocket.sockfd, SHUT_RDWR);
      close(clientSocket.sockfd);
      if (getShouldClose())
        return request;

      std::cerr << "[ERROR] Failed to read from client socket: " << describeError() << "\n";
      return request;
    }
    if (n == 0 || request.find("\r\n\r\n") != std::string::npos)
    {
      break;
    }
    received += n;
  } while (received < SERVER_BUFLEN-1);

  return request;

#elif _WIN32

  std::string request(SERVER_BUFLEN, '\0');
  char* buffer = (char*)malloc(SERVER_BUFLEN);
  int received = 0;

  int iResult = 0;

  do {
    iResult = recv(clientSocket.sockfd, buffer, SERVER_BUFLEN, 0);
    request += std::string(buffer);
    received += iResult;

    if (iResult == 0 || request.find("\r\n\r\n") != std::string::npos)
    {
      break;
    } else if (iResult < 0)
    {
      std::cerr << "[ERROR] recv failed: " << WSAGetLastError() << "\n";
    }
  } while (iResult > 0);

  free(buffer);
  return request;
#endif
}

SOCKFD Socket::acceptClient()
{
#ifdef __linux__

  struct sockaddr_in cli_addr;

  socklen_t cli_len = sizeof(cli_addr);
  int newsockfd = ::accept(m_socket.sockfd, (struct sockaddr*)&cli_addr, &cli_len);

  return {newsockfd};

#elif _WIN32

  SOCKET ClientSocket = ::accept(m_socket.sockfd, NULL, NULL);
  if (ClientSocket == INVALID_SOCKET && !getShouldClose()) {
    std::cerr << "[ERROR] accept failed with error: " << WSAGetLastError() << "\n";
    closesocket(m_socket.sockfd);
    WSACleanup();
  }

  return SOCKFD{ClientSocket};

#endif
}

}
