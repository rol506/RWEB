#include <RWEB.h>

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <errno.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstdio>

#include "Socket.h"
#include "HTMLTemplate.h"
#include "Utility.h"

#ifdef __linux__
#include <signal.h>
#endif

namespace rweb
{
  static std::string resourcePath = "";
  static std::string execPath = "";
  static std::unordered_map<std::string, HTTPCallback> serverPaths;
  static std::unordered_map<std::string, std::pair<std::string, std::string>> serverResources;
  static std::unordered_map<int, HTTPCallback> errorHandlers;
  static std::unordered_map<std::string, std::pair<std::string, std::string>> serverDynamicResources;
  static std::map<unsigned long long, Session> sessions;
  static unsigned long long nextSessionID = 1; // 0 is invalid!
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

  static const std::string getExecutablePath()
  {
    if (!initialized)
    {
#ifdef __linux__
      char path[2048];
      size_t r;
      r = readlink("/proc/self/exe", path, sizeof(path)-1);
      if (r < 0)
      {
        std::cerr << "[RWEB] Failed to get executable path!\n";
        std::cerr << describeError() << "\n";
        throw std::runtime_error("Failed to get executable path");
      }
      execPath = path;
      execPath += "\0";
      std::cout << "PATH: " << execPath << "\n";
      return execPath;
#elif _WIN32
      char path[2048];
      GetModuleFileName(NULL, path, sizeof(path)-1);
      execPath = path;
      execPath += "\0";
      return execPath;
#endif
    }
    return execPath;
  }

  //see sourcePathLevel docs
  static const std::string calculateResourcePath(size_t level)
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

  static std::string urlDecode(const std::string& str) {
    std::string ret;
    char ch;
    size_t i, ii, len = str.length();

    for (i = 0; i < len; i++) {
      if (str[i] != '%') {
        if (str[i] == '+')
          ret += ' ';
        else
          ret += str[i];
      } else {
        sscanf(str.substr(i + 1, 2).c_str(), "%x", (unsigned int* )&ii);
        ch = static_cast<char>(ii);
        ret += ch;
        i = i + 2;
      }
    }
    return ret;
} 

  static const Request parseRequest(const std::string request)
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

    if (r.method == "GET")
    {
      r.isValid = true;
    } else if (r.method == "POST")
    {
      std::size_t pos1 = str.find("Content-Type:")+13;
      if (pos1 == std::string::npos)
      {
        r.isValid = false;
        return r;
      } else {
        std::size_t pos2 = str.find_first_of("\r\n", pos1);
        if (pos2 == std::string::npos)
        {
          r.isValid = false;
          return r;
        }

        r.contentType = trim(str.substr(pos1, pos2-pos1));

        pos1 = str.find_last_of("\n")+1;
        std::string body = str.substr(pos1);

        auto v = split(body, "&");
        for (auto it: v)
        {
          std::size_t pos = it.find_first_of("=");
          r.body.emplace(trim(urlDecode(it.substr(0, pos))), trim(urlDecode(it.substr(pos+1))));
        }

        if (r.contentType == "application/x-www-form-urlencoded")
        {
          r.isValid = true;
        } else {
          r.isValid = false;
          return r;
        }
      }

    } else {
      r.isValid = false;
      return r;
    }

    {
      size_t lastfnd = str.find("Cookie: ")+8;
      while (lastfnd != std::string::npos+8)
      {
        size_t end = str.find_first_of("\n", lastfnd);
        if (end == std::string::npos)
        {
          r.isValid = false;
          return r;
        }

        auto v = split(trim(str.substr(lastfnd, end-lastfnd)), ";");
        for (auto c: v)
        {
          size_t start = c.find_first_of("=");
          r.cookies.emplace(c.substr(0, start), c.substr(start+1));
        }
        lastfnd = str.find("Cookie: ", lastfnd)+8;
      }
    }

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
    std::string urlPath = path;

    if (urlPath.empty())
      urlPath = '/';

    if (urlPath[0] != '/')
      urlPath = '/' + urlPath;

    serverPaths.emplace(urlPath, callback);
  }

  void addResource(const std::string& URLpath, const std::string& resourcePath, const std::string& contentType)
  {
    std::string urlPath = URLpath;
    if (urlPath[0] != '/')
      urlPath = '/' + urlPath;

    serverResources.insert({urlPath, {resourcePath, contentType}});
  }

  void addDynamicResource(const std::string& URLPrefix, const std::string& resourceFolderPrefix, const std::string& contentType)
  {
    std::string urlPrefix = URLPrefix;
    if (urlPrefix[0] != '/')
      urlPrefix = '/' + urlPrefix;

    if (urlPrefix.back() != '/')
      urlPrefix += '/';

    std::string resPrefix = resourceFolderPrefix;
    if (resPrefix.back() != '/')
    {
      resPrefix += '/';
    }

    if (resPrefix[0] != '/')
    {
      resPrefix = '/' + resPrefix;
    }

    serverDynamicResources.insert({urlPrefix, {resPrefix, contentType}});
  }

  void setErrorHandler(const int code, const HTTPCallback callback)
  {
    if (code / 100 == 1 || code / 100 == 2 || code / 100 == 3)
      return;

    auto it = errorHandlers.find(code);
    if (it != errorHandlers.end())
    {
      errorHandlers.erase(code);
      errorHandlers.emplace(code, callback);
    } else {
      errorHandlers.emplace(code, callback);
    }
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

  static const std::string sendFile(const std::string& statusResponce, const std::string& filePath, const std::string& contentType)
  {
    std::string file = getFileString(filePath);
    if (file.empty())
    {
      return HTTP_404 + "\r\n";
    }
    return statusResponce + "Content-Type: " + contentType + "\r\nContent-Length: " + std::to_string(file.size()) + "\r\nContent-Encoding: utf-8\r\n"
      + "\r\n" + file;
  }

  static const std::string sendData(const std::string& statusResponce, const std::string& data, const std::string contentType)
  {
    return statusResponce + "Content-Type: " + contentType + "\r\nContent-Length: " + std::to_string(data.size()) + "\r\nContent-Encoding: utf-8\r\n"
      + "\r\n" + data;
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

  static unsigned long long getEmptySessionID()
  {
    //allocate new session
    Session sess;
    sessions.emplace(nextSessionID, sess);
    nextSessionID++;
    return nextSessionID-1;
  }

  static const std::string handleRequest(const HTTPCallback callback, const Request r, const std::string& initialStatus=HTTP_200)
  {
    HTMLTemplate temp; 

    //check session
    {
      auto it = r.cookies.find("sessionID");
      if (it != r.cookies.end())
      {
        unsigned long long sessionID;
        try {
          sessionID = std::stoull(it->second);
          auto it2 = sessions.find(sessionID);
          if (it2 != sessions.end())
          {
            //session is valid -> can continue
            temp = callback(r);
          } else {
            temp = redirect(r.path, HTTP_303); //redirect
            temp.setCookie("sessionID", std::to_string(getEmptySessionID()), 0, true); //re-create session
          }
        } catch (std::invalid_argument& e)
        {
          temp = redirect(r.path, HTTP_303); //redirect
          temp.setCookie("sessionID", std::to_string(getEmptySessionID()), 0, true); //re-create session
        }
      } else {
        temp = redirect(r.path, HTTP_303); //redirect
        temp.setCookie("sessionID", std::to_string(getEmptySessionID()), 0, true); //re-create session
      }
    }

    const std::string code = temp.getStatusResponce().substr(9, 3);
    std::string res = "";

    if (!temp.getHTML().empty())
    {
      res = temp.getStatusResponce() + "Content-Type: " + temp.getContentType() + "\r\nContent-Length: " + std::to_string(temp.getHTML().size()) + "\r\n";
    } else {
      res = temp.getStatusResponce();
    }

    //---ADDITIONAL HEADERS---
    res += temp.getAllCookieHeaders(); // \r\n included

    if (code[0] == '3')
    {
      res += "Location: " + temp.getRedirectLocation() + "\r\n";
    }

    res += "\r\n";
    res += temp.getHTML();

    if (code[0] != '1' && code[0] != '2' && code[0] != '3')
    {
      auto it = errorHandlers.find(std::stoi(code));
      if (it != errorHandlers.end())
      {
        return handleRequest(it->second, r, temp.getStatusResponce());
      } else {
        std::cout << "[RESPONCE] " << r.method << " -- " << colorize(RED);
        res = temp.getStatusResponce();
        std::cout << r.path << colorize(NC) << " -- " << temp.getStatusResponce().substr(9, temp.getStatusResponce().size()-11);

        if (initialStatus != HTTP_200)
        {
          std::cout << " -- Handled " << initialStatus.substr(9, initialStatus.size()-11);
        }

        return res;
      }
    }

    std::cout << "[RESPONCE] " << r.method << " -- " << colorize(NC) << r.path << colorize(NC) << " -- " << 
      temp.getStatusResponce().substr(9, temp.getStatusResponce().size()-11);

    if (initialStatus != HTTP_200)
    {
      std::cout << " -- Handled " << initialStatus.substr(9, initialStatus.size()-11);
    }

    return res;
  }

  static void handleClient(const Request r, const SOCKFD newsockfd)
  {
    const auto startTime = std::chrono::high_resolution_clock::now(); //for profiling
    std::cout << colorize(NC);
    std::string res;
    int n = 0;

    if (!r.isValid)
    {
      //handle 400
      auto it = errorHandlers.find(400);
      if (it != errorHandlers.end())
      {
        res = handleRequest(it->second, r, HTTP_400); 
      } else {
        res = HTTP_400 + "\r\n";
        std::cout << "[RESPONCE] " << r.method << " -- " << colorize(RED) << r.path << colorize(NC) << " -- " << HTTP_400.substr(9, HTTP_400.size()-11);
      }
    } else {
      //process request
      auto it = serverPaths.find(r.path);
      if (it == serverPaths.end())
      {
        auto it2 = serverResources.find(r.path);
        if (it2 != serverResources.end())
        {
          res = sendFile(HTTP_200, it2->second.first, it2->second.second);
          std::cout << "[RESPONCE] " << r.method << " -- " << colorize(CYAN) << r.path << colorize(NC) << " -- " << HTTP_200.substr(9, HTTP_200.size()-11);
        } else { 
          
          std::string path = r.path;
          if (path.back() == '/')
            path = path.substr(0, path.size()-1);
          std::string currPrefix = "";
          std::string postfix = "";
          bool found = false;

          std::size_t pos = path.rfind("/");
          if (pos == std::string::npos)
          {
            found = false;
          } else {
            currPrefix = path.substr(0, pos);
            postfix = path.substr(pos+1, path.size()-pos-1);

            auto it3 = serverDynamicResources.find(currPrefix+"/");
            if (it3 != serverDynamicResources.end())
            {
              std::string filePath = it3->second.first + postfix; // '/' included
              std::string data = getFileString(filePath);
              if (data.empty())
              {
                found = false;
              } else {
                res = sendData(HTTP_200, data, it3->second.second);
                std::cout << "[RESPONCE] " << r.method << " -- " << colorize(NC) << r.path << colorize(NC) << " -- " << HTTP_200.substr(9, HTTP_200.size()-11);
                found = true;
              }
            }
          } 

          if (!found)
          {
            //handle 404
            auto it = errorHandlers.find(404);
            if (it != errorHandlers.end())
            {
              res = handleRequest(it->second, r, HTTP_404);
            } else { 
              res = HTTP_404 + "\r\n";
              std::cout << "[RESPONCE] " << r.method << " -- " << colorize(RED) << r.path << colorize(NC) << " -- " << HTTP_404.substr(9, HTTP_404.size()-11);
            }
          }
        }
      } else {
        res = handleRequest(it->second, r); 
      }
    }

    if (getDebugState() && getProfilingMode())
    {
      const double timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - startTime).count();
      std::cout << colorize(NC) << " -- " << timeDelta << "ms\n";
    } else {
      std::cout << "\n";
    }

    //send result
    serverSocket->sendMessage(newsockfd, res);
    Socket::closeSocket(newsockfd);
    return; 
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

  const std::string getFileString(const std::string& filePath)
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

  HTMLTemplate abort(const std::string& statusResponce)
  {
    HTMLTemplate temp = "";
    temp.m_responce = statusResponce;
    return temp;
  }

  Session* getSession(const Request& r)
  {
    //if this func is used -> all is checked
    auto it = r.cookies.find("sessionID");
    unsigned long long sessionID = std::stoull(it->second);
    auto it2 = sessions.find(sessionID);
    return &it2->second;
  }

  void clearAllSessions()
  {
    sessions.clear();
    std::cout << colorize(YELLOW) << "[SERVER] All sessions are cleared!" << colorize(NC) << "\n";
  }
}
