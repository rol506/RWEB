#include <iostream>
#include <cstdlib>
#include <thread>

#include <RWEB.h>

#include "Socket.h"

static int reqNum = 0;

static rweb::HTMLTemplate homePage(const rweb::Request r);

void atexit_handler();

int main()
{
  if (!rweb::init(true, 1))
  {
    std::cout << "Failed to initialize RWEB!\n";
    return -1;
  }

  const int atex = std::atexit(atexit_handler);
  if (atex)
  {
    std::cout << rweb::colorize(rweb::RED) << "Failed to register atexit!\n" << rweb::colorize(rweb::NC);
    rweb::closeServer();
    return -1;
  }

  rweb::setPort(4221);
  rweb::setProfilingMode(true);
  rweb::Debug::disableKeepAliveFix = true;

  rweb::addRoute("/", [](const rweb::Request r){return rweb::redirect("/index");});
  rweb::addRoute("/index", [](const rweb::Request r){
    reqNum++;
    return (rweb::HTMLTemplate)("Index call: " + std::to_string(reqNum));
  });
  rweb::setErrorHandler(404, [](const rweb::Request r){return rweb::redirect("/index");});
  
  std::thread th([](){
    if (!rweb::startServer(2))
    {
      rweb::closeServer();
      std::cout << "Cannot start rweb!\n";
      return;
    }
  });
  th.detach();

  {
    System::Socket s(true);
    std::string request;
    std::string res;
    size_t pos;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!s.connect("127.0.0.1", 4221))
    {
      std::cerr << "Failed to connect to the test server!\n";
      goto err;
    }

    request = "GET /index HTTP/1.1\r\nConnection: keep-alive\r\n\r\n";
    if (!s.sendMessage(request))
    {
      std::cout << "Cannot send the first request!\n";
      goto err;
    }
    res = s.getMessage();

    request = "GET /index HTTP/1.1\r\nCookie: sessionID=1\r\nConnection: keep-alive\r\n\r\n";
    if (!s.sendMessage(request))
    {
      std::cout << "Cannot send the second request on the same connection!\n";
      goto err;
    }

    res = s.getMessage();

    request = "GET /index HTTP/1.1\r\nCookie: sessionID=1\r\nConnection: close\r\n\r\n";
    if (!s.sendMessage(request))
    {
      std::cout << "Cannot send to third request on the same connection!\n";
      goto err;
    }

    res = s.getMessage();

    request = "GET /index HTTP/1.1\r\nCookie: sessionID=1\r\n\r\n";
    s.sendMessage(request);

    res = s.getMessage();

    pos = res.find("Index call: ");
    if (pos == std::string::npos)
    {
      std::cout << "Server returned invalid responce!\n";
      goto err;
    }
    if (res.substr(pos+12, 1) != "2")
    {
      std::cout << "Server did not close the connection after 'Connection: close'\n";
      goto err;
    }

    rweb::closeServer();
    return 0;
  err:
    rweb::closeServer();
    return -1;
  }


  return 0;
}

void atexit_handler()
{
}
