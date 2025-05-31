#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#endif

namespace rweb
{

  enum COLORS {
#ifdef __linux
    NC=-1,
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
#elif _WIN32

    RED = FOREGROUND_RED | FOREGROUND_INTENSITY,
    YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    BLUE = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    MAGENTA = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
    NC = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    GREY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    BLACK = FOREGROUND_INTENSITY,

#endif
  };

  //---HTTP RESPONCES---
  const std::string HTTP_200 = "HTTP/1.1 200 OK\r\n";

  const std::string HTTP_300 = "HTTP/1.1 300 Multiple Choice\r\n";
  const std::string HTTP_301 = "HTTP/1.1 301 Moved Permanently\r\n";
  const std::string HTTP_302 = "HTTP/1.1 302 Found\r\n";
  const std::string HTTP_303 = "HTTP/1.1 303 See Other\r\n";
  const std::string HTTP_304 = "HTTP/1.1 304 Not Modified\r\n";
  const std::string HTTP_307 = "HTTP/1.1 307 Temporary Redirect\r\n";
  const std::string HTTP_308 = "HTTP/1.1 308 Permanent Redirect\r\n";

  const std::string HTTP_400 = "HTTP/1.1 400 Bad Request\r\n";
  const std::string HTTP_401 = "HTTP/1.1 401 Unauthorized\r\n";
  const std::string HTTP_403 = "HTTP/1.1 403 Forbidden\r\n";
  const std::string HTTP_404 = "HTTP/1.1 404 Not Found\r\n";
  const std::string HTTP_405 = "HTTP/1.1 405 Method Not Allowed\r\n";

  const std::string HTTP_500 = "HTTP/1.1 500 Internal Server Error\r\n"; 

  //returns string found in file. "" in case of an error.
  //'filePath' is the target file path from resource folder.
  std::string getFileString(const std::string& filePath);
  //remove leading and trailing spaces in 'str'.
  std::string trim(const std::string& str);
  //returns string with replaced 'from' with 'to'.
  std::string replace(const std::string& s, const std::string& from, const std::string& to) noexcept;
  //splits 's' by chars from 'seperator'.
  //'maxsplit' is the maximum count of splits. -1 by default.
  std::vector<std::string> split(const std::string& s, const std::string& seperator, int maxsplit = -1);
  //does math. Returns 0 on error. 'is_ok' is pointer to bool which is false on an error. if is set does not output errors
  double calculate(const std::string& expression, bool* is_ok=nullptr);
  //colorizes output. Usage: stream << colorize(color) << ... << colorize(NC) << "\n"; /*to clear color*/.
  const char *colorize(int font = NC);
}
