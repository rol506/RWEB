#include <RWEB.h>

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <errno.h>
#include <thread>
#include <fstream>
#include <sstream>

#include "../include/Socket.h"
#include "HTMLTemplate.h"

#ifdef __linux__
#include <signal.h>
#endif

namespace rweb
{
  static std::string resourcePath = "";
  static std::unordered_map<std::string, HTTPCallback> serverPaths;
  static std::unordered_map<std::string, std::pair<std::string, std::string>> serverResources;
  static int serverPort = 4221;
  static bool serverDebugMode = false;
  static bool serverProfiling = false;
  static bool serverCompression = false; //not supported yet
  static bool shouldClose = false;
  //static int sockfd; //server socket file descriptor
  static bool initialized = false;
  static std::shared_ptr<Socket> serverSocket;

  //use in case if you are running app in build folder, but editing source code folder (will step back <level> times to find resource folder)
  static const int sourcePathLevel = 0; //set only at compile time for safety 

#ifdef _WIN32

  void activateVirtualTerminal()
  {       
      HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD consoleMode;
      GetConsoleMode( handleOut , &consoleMode);
      consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;            
      SetConsoleMode( handleOut , consoleMode );
  }
#endif

  static std::string getExecutablePath()
  {
#ifdef __linux__
    char* path = (char*)malloc(1024);
    size_t r;
    r = readlink("/proc/self/exe", path, 1023);
    std::string execPath = path;
    free(path);
    return execPath;
#elif _WIN32
    char* path = (char*)malloc(1024);
    GetModuleFileName(NULL, path, 1024);
    std::string execPath = path;
    free(path);
    return execPath;
#endif
  }

  //see sourcePathLevel docs
  static std::string calculateResourcePath(size_t level)
  {
    std::string resPath = getExecutablePath();
#ifdef _WIN32
    level++; //on windows we have "Debug" folder -> increment level by 1
#endif
    for (int i=0;i<=level;++i)
    {
      std::size_t pos = resPath.find_last_of("/\\");
      resPath = resPath.substr(0, pos);
    }
    resPath += "/res";
    return resPath;
  }

  std::string describeError()
  {
    switch (errno)
    {
      case 0:
        return "Success";
      case 9:
        return "Bad file descriptor";
      case 13:
        return "Permission denied";
      case 14:
        return "Bad address";

      case 32:
        return "Broken pipe";
      case 94:
        return "Socket type not supported";
      case 96:
        return "Protocol family not supported";
      case 98:
        return "Address already in use";
      case 100:
        return "Network is down";
      case 101:
        return "Network is unreachable";
      case 104:
        return "Connection reset by peer";
      case 110:
        return "Connection timed out";
      case 111:
        return "Connection refused";

      case ENOTSOCK:
        return "File descriptor is not a socket";
    }
    return std::to_string(errno);
  }

  static Request parseRequest(const std::string request)
  {
    Request r;
    std::string str = request;
    std::size_t pos;

    //method
    pos = str.find(" ");
    r.method = str.substr(0, pos);
    str = str.substr(pos+1);

    //path
    pos = str.find(" ");
    r.path = str.substr(0, pos);
    str = str.substr(pos+1);

    //protocol
    pos = str.find("\r\n");
    if (pos == std::string::npos)
    {
      r.isValid = false;
      return r;
    }
    r.protocol = str.substr(0, pos);
    str = str.substr(pos+1);

    r.isValid = true;
    return r;
  }

  std::string getResourcePath()
  {
    return resourcePath;
  }

  void setResourcePath(const std::string resPath)
  {
    resourcePath = resPath;
  }

  void addRoute(const std::string& path, const HTTPCallback callback)
  {
    serverPaths.emplace(path, callback);
  }

  void addResource(const std::string& URLpath, const std::string& resourcePath, const std::string& contentType)
  {
    serverResources.insert({URLpath, {resourcePath, contentType}});
  }

  HTMLTemplate redirect(const std::string& location, const std::string& statusResponce)
  {
    HTMLTemplate temp = createTemplate("", statusResponce);
    temp.m_location = location;
    return temp;
  }

  void setPort(const int port)
  {
    serverPort = port;
  }

  int getPort()
  {
    return serverPort;
  }

  bool getShouldClose()
  {
    return shouldClose;
  }

  void setShouldClose(const bool _shouldClose)
  {
    shouldClose = _shouldClose;
  }

  bool getDebugState()
  {
    return serverDebugMode;
  }

  void setProfilingMode(const bool enabled)
  {
    serverProfiling = enabled;
  }

  bool getProfilingMode()
  {
    return serverProfiling;
  }

  const char *colorize(int color) {

    if (!initialized)
    {
      std::cerr << "[ERROR RWEB IS NOT INITIALIZED]\n";
      return "\033[0m";
    }
#ifdef __linux__
    static char code[20];   
    if (color >= 0)
      color += 30;
    else
      color = 0;
    sprintf(code, "\033[%dm", color);
    return code;
#elif _WIN32
    if (GetStdHandle(STD_OUTPUT_HANDLE) == INVALID_HANDLE_VALUE)
      return "";
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F & color | 0xf0 & console_info.wAttributes);
    return "";
#endif
  }

  //returns false on an error (step <level> times back to find resource folder. use only for dev purposes)
  bool init(bool debug, unsigned int level)
  {
    serverDebugMode = debug;

#ifdef __linux
    signal(SIGINT, closeServer);
    signal(SIGTERM, closeServer);
#elif _WIN32
    activateVirtualTerminal();
    if (!SetConsoleCtrlHandler(closeServer, TRUE))
    {
      std::cerr << "[ERROR] Failed to create Ctrl C handler!\n";
      return false;
    }
#endif
    //resource path
    setResourcePath(calculateResourcePath(getDebugState() ? level : 0));
    initialized = true;
    std::cout << colorize(NC);
    return true;
  }

  //returns false on an error
  bool startServer(const int clientQueue)
  {

    serverSocket = std::make_shared<Socket>(clientQueue);

    while (!getShouldClose())
    {
      SOCKFD newSock = serverSocket->acceptClient();

      if (getShouldClose())
      {
        break;
      }
      std::string request = serverSocket->getMessage(newSock);

      Request req = parseRequest(request);

      std::thread th(handleClient, req, newSock);
      th.detach();
    }

    return true;
  }

  static void handleClient(const Request r, const SOCKFD newsockfd)
  {
    const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now(); //for profiling
    std::cout << colorize(NC);
    std::string res;
    int n = 0;

    if (!r.isValid)
    {
      res = HTTP_400 + "\r\n";
      std::cout << "[RESPONCE] " << colorize(RED) << r.path << colorize(NC) << " -- " << HTTP_400.substr(9);
      serverSocket->sendMessage(newsockfd, res);
      Socket::closeSocket(newsockfd);
      return;
    }
  
    //process request
    auto it = serverPaths.find(r.path);
    if (it == serverPaths.end())
    {
      auto it2 = serverResources.find(r.path);
      if (it2 != serverResources.end())
      {
        res = sendFile(HTTP_200, it2->second.first, it2->second.second);
        std::cout << "[RESPONCE] " << colorize(CYAN) << r.path << colorize(NC) << " -- " << HTTP_200.substr(9);
      } else { 
        res = HTTP_404 + "\r\n";
        std::cout << "[RESPONCE] " << colorize(RED) << r.path << colorize(NC) << " -- " << HTTP_404.substr(9);
      }
    } else {
      HTMLTemplate temp = it->second(r);
      std::string code = temp.getStatusResponce().substr(9, 3);
      if (!temp.getFileName().empty()) //non-empty body
      {
        res = temp.getStatusResponce() + "Content-Type: " + temp.getContentType() + "\r\nContent-Length: " + std::to_string(temp.getHTML().size()) + "\r\n";
      } else {
        res = temp.getStatusResponce();
      }

      //---ADDITIONAL HEADERS---
      if (code[0] == '3') //redirect
      {
        res += "Location: " + temp.getRedirectLocation() + "\r\n";
      }

      //end of header section
      res += "\r\n";
      res += temp.getHTML(); //body

      std::cout << "[RESPONCE] ";
      if (code[0] == '2' || code[0] == '3')
      {
        std::cout << colorize(NC);
      } else {
        std::cout << colorize(RED);
        res = temp.getStatusResponce();
      }
      std::cout << r.path << colorize(NC) << " -- " << temp.getStatusResponce().substr(9);
    }

    if (getDebugState() && getProfilingMode())
    {
      const double timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - startTime).count();
      std::cout << colorize(NC) << "[PROFILING] Request processing time: " << timeDelta << "ms\n";
    }

    //send result
    serverSocket->sendMessage(newsockfd, res);
    Socket::closeSocket(newsockfd);
    return; 
  }

#ifdef __linux__
  void closeServer(int arg)
  {
    //linux handler
    std::cout << "\n";
    setShouldClose(true); 
    serverSocket = nullptr;
    std::cout << colorize(NC);
  }
#elif _WIN32

  BOOL WINAPI closeServer(DWORD signal)
  {
    //windows handler
    if (signal == CTRL_C_EVENT)
    {
      std::cout << "\n";
      setShouldClose(true);
      serverSocket = nullptr;
    }
    std::cout << colorize(NC);
    return TRUE;
  }

#endif

  std::string getFileString(const std::string& filePath)
  {
    std::ifstream f;
    f.open(resourcePath + "/" + filePath, std::ios::in | std::ios::binary);
    if (!f.is_open())
    {
      return std::string{};
    }

    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
  }

  std::string sendFile(const std::string& statusResponce, const std::string& filePath, const std::string& contentType)
  {
    std::string file = getFileString(filePath);
    if (file.empty())
    {
      return HTTP_500 + "\r\n";
    }
    return statusResponce + "Content-Type: " + contentType + "\r\nContent-Length: " + std::to_string(file.size()) + "\r\nContent-Encoding: utf-8\r\n"
      + "\r\n" + file;
  }

  HTMLTemplate createTemplate(const std::string& templatePath, const std::string& statusResponce)
  {
    std::string file;
    std::string resp;
    if (templatePath != "")
    {
      file = getFileString(templatePath);
      resp = statusResponce;
      if (file.empty())
      {
        std::cerr << "[TEMPLATE] File " << templatePath << " is empty!\n";
        resp = HTTP_500;
      }
    } else {
      file = "";
      resp = statusResponce;
    }

    HTMLTemplate temp(file);
    temp.m_contentType = templatePath == "" ? "" : "text/html";
    temp.m_encoding = templatePath == "" ? "" : "utf-8";
    temp.m_responce = resp;
    temp.m_templateFileName = templatePath;
    return temp;
  }

  HTMLTemplate::HTMLTemplate(const std::string& html)
    : m_html(html)
  {
  }
}
