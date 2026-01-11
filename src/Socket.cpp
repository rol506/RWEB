#include "../include/Socket.h"
#include "../include/Utility.h"

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

Socket::Socket(int clientQueue, int timeoutSeconds)
: m_debug(false), m_connected(false), timeout(timeoutSeconds)
{
#ifdef __linux__

  struct sockaddr_in serv_addr;

  m_socket = SOCKFD{ 0 };

  m_socket.sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket.sockfd < 0)
  {
    std::cerr << colorize(RED) << "[ERROR] Can't open socket!" << colorize(NC) << "\n";
    close(m_socket.sockfd);
    setShouldClose(true);
    return;
  }

  const int enable = 1;
  if (setsockopt(m_socket.sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
  {
    std::cerr << colorize(RED) << "[ERROR] setsockopt failed! (SO_REUSEADDR)" << colorize(NC) << "\n";
    close(m_socket.sockfd);
    setShouldClose(true);
    return;
  }

  struct timeval tv;
  tv.tv_sec = timeoutSeconds;
  tv.tv_usec = 0;
  if (setsockopt(m_socket.sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
  {
    std::cerr << colorize(RED) << "[ERROR] setsockopt failed! (SO_RCVTIMEO)" << colorize(NC) << "\n";
    close(m_socket.sockfd);
    setShouldClose(true);
    return;
  }

  bzero(&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(getPort());

  if (bind(m_socket.sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cerr << colorize(RED) << "[ERROR] Failed to bind socket: " << describeError() << colorize(NC) << "\n";
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
    std::cerr << colorize(RED) << "[ERROR] WSAStartup failed with error: " << iResult << "\n";
    std::cerr << "[ERROR] Can't open socket!" << colorize(NC) << "\n";
    setShouldClose(true);
  }

  ZeroMemory(&m_hints, sizeof(m_hints));
  m_hints.ai_family = AF_UNSPEC;
  m_hints.ai_socktype = SOCK_STREAM;
  m_hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  iResult = getaddrinfo(NULL, std::to_string(getPort()).c_str(), &m_hints, &m_result);
  if ( iResult != 0 ) {
    std::cerr << colorize(RED) << "[ERROR] getaddrinfo failed with error: " << iResult << colorize(NC) << "\n";
    WSACleanup();
    setShouldClose(true);
    return;
  }

  // Create a SOCKET for the server to listen for client connections.
  m_socket.sockfd = socket(m_result->ai_family, m_result->ai_socktype, m_result->ai_protocol);
  if (m_socket.sockfd == INVALID_SOCKET) {
    std::cerr << colorize(RED) << "[ERROR] socket failed with error: " << WSAGetLastError() << colorize(NC) << "\n";
    freeaddrinfo(m_result);
    WSACleanup();
    setShouldClose(true);
    return;
  }

  // Setup the TCP listening socket
  iResult = bind( m_socket.sockfd, m_result->ai_addr, (int)m_result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    std::cerr << colorize(RED) "[ERROR] bind failed with error: " << WSAGetLastError() << colorize(NC) << "\n";
    freeaddrinfo(m_result);
    closesocket(m_socket.sockfd);
    WSACleanup();
    setShouldClose(true);
    return;
  }

  freeaddrinfo(m_result);

  iResult = listen(m_socket.sockfd, clientQueue);
  if (iResult == SOCKET_ERROR) {
    std::cerr << colorize(RED) << "[ERROR] listen failed with error: " << WSAGetLastError() << colorize(NC) << "\n";
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

  if (Debug::showConnectionLifetime)
    std::cout << "[DEBUG] Closed connection: " << socket.sockfd << "\n";
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
    if (getLogLevel() <= ERROR)
      std::cerr << colorize(RED) << "[ERROR] Failed to shutdown server socket: " << describeError() << colorize(NC) << "\n";
    err = true;
  }
  if (::close(m_socket.sockfd))
  {
    if (getLogLevel() <= ERROR)
      std::cerr << colorize(RED) << "[ERROR] Failed to close server socket: " << describeError() << colorize(NC) << "\n";
    err = true;
  }

  if (!err)
  {
    if (getLogLevel() <= INFO)
      std::cout << "[SERVER] Socket was shut down successfully!\n";
  } else {
    if (getLogLevel() <= ERROR)
      std::cerr << colorize(RED) << "[ERROR] Failed to shutdown socket!" << colorize(NC) << "\n";
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
  m_count = 0;

  do
  {
    int n = write(clientSocket.sockfd, message.substr(m_count).c_str(), message.size() - m_count);
    if (n < 0)
    {
      if (getLogLevel() <= ERROR)
        std::cerr << colorize(RED) << "[ERROR] Failed to write to socket: " << describeError() << colorize(NC) << "\n";
      return false;
    }
    m_count += n;
  } while (m_count < message.size());
  //std::cout << "[DEBUG] Sent " << m_count << " bytes\n";

  return true;
#elif _WIN32

  int iResult = send(clientSocket.sockfd, message.c_str(), message.size(), 0);
  if (iResult == SOCKET_ERROR)
  {
    std::cerr << colorize(RED) << "[ERROR] send failed: " << WSAGetLastError() << colorize(NC) << "\n";
    return false;
  }

  return true;

#endif
}

// returns empty string on error
std::string Socket::getMessage(SOCKFD clientSocket)
{

#ifdef __linux__
  int received = 0;

  std::string request(SERVER_BUFLEN, '\0');
  do {
    int n = read(clientSocket.sockfd, &request[received], SERVER_BUFLEN-1);
    if (n < 0)
    {
      closeSocket(clientSocket);
      if (getShouldClose())
        return request;

      if (errno == 11)
      {
        if (getLogLevel() <= WARNING)
          std::cout << colorize(YELLOW) << "[WARNING] Connection timed out!" << colorize(NC) << "\n";
        return std::string{};
      }

      if (getLogLevel() <= ERROR)
        std::cerr << colorize(RED) << "[ERROR] Failed to read from client socket: " << describeError() << colorize(NC) << "\n";
      return std::string{};
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
      std::cerr << colorize(RED) << "[ERROR] recv failed: " << WSAGetLastError() << colorize(NC) << "\n";
    }
  } while (iResult > 0);

  free(buffer);
  return request;
#endif
}

std::optional<SOCKFD> Socket::acceptClient()
{
#ifdef __linux__

  struct sockaddr_in cli_addr;

  socklen_t cli_len = sizeof(cli_addr);
  int newsockfd = ::accept(m_socket.sockfd, (struct sockaddr*)&cli_addr, &cli_len);
  if (newsockfd < 0)
  {
    return std::nullopt;
  }

  if (Debug::showConnectionLifetime)
    std::cout << "[DEBUG] Opened connection: " << newsockfd << "\n";
  return SOCKFD{newsockfd};

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
