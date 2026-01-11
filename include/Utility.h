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

typedef enum {
  INFO = 0, WARNING=1, ERROR=2, NONE=3
} LogLevel;

class Debug {
public:
  Debug() = delete;
  ~Debug() = delete;

  static bool showConnectionLifetime;
  static bool disableKeepAliveFix; // Disable keep-alive connection quick fix (for firefox)
  static bool disableKeepAlive; // disables keep-alive connection support
  static bool outputRequests; // outputs every request to stdout
private:
  static int sourcePathLevel; //use in case if you are running app in build folder, but editing source code folder (will step back <level> times to find resource folder)

  friend bool init(bool, unsigned int);
};

//---MIME TYPES---
namespace MIME {
const std::string octetStream = "application/octet-stream";
const std::string JSON = "application/json";
const std::string FORMURLENCODED = "application/x-www-form-urlencoded";

const std::string PLAINTEXT = "text/plain";
const std::string HTML = "text/html";
const std::string CSS = "text/css";
const std::string JAVASCRIPT = "text/javascript";
const std::string JS = "text/javascript";

const std::string APNG = "image/apng";
const std::string PNG = "image/png";
const std::string JPEG = "image/jpeg";
const std::string SVG_XML = "image/svg+xml";
const std::string SVG = "image/svg+xml";
const std::string BMP = "image/bmp";
const std::string GIF = "image/gif";
const std::string WEBP = "image/webp";
  }

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
const std::string HTTP_411 = "HTTP/1.1 411 Length Required\r\n";
const std::string HTTP_413 = "HTTP/1.1 413 Payload Too Large\r\n";
const std::string HTTP_429 = "HTTP/1.1 429 Too Many Requests\r\n";
const std::string HTTP_431 = "HTTP/1.1 431 Request Header Fields Too Large\r\n";

const std::string HTTP_500 = "HTTP/1.1 500 Internal Server Error\r\n"; 
const std::string HTTP_503 = "HTTP/1.1 503 Service Unavailable\r\n";

//returns string found in file. "" in case of an error.
//'filePath' is the target file path from resource folder.
const std::string getFileString(const std::string& filePath);
//remove leading and trailing spaces in 'str'.
std::string trim(const std::string& str);
//returns string with replaced 'from' with 'to'.
std::string replace(const std::string& s, const std::string& from, const std::string& to) noexcept;
//splits 's' by chars from 'seperators'.
//'maxsplit' is the maximum count of splits. -1 by default.
std::vector<std::string> split(const std::string& s, const std::string& seperators, int maxsplit = -1, bool trimNeeded=true);
//splits 's' by exact matches from 'seperator'.
//'maxsplit' is the maximum count of splits. -1 by default.
std::vector<std::string> splitByWord(const std::string& s, const std::string& seperator, int maxsplit = -1, bool trimNeeded=true);
//does math. Returns 0 on error. 'is_ok' is pointer to bool which is false on an error. if is set does not output errors
double calculate(const std::string& expression, bool* is_ok=nullptr);
//colorizes output. Usage: stream << colorize(color) << ... << colorize(NC) << "\n"; /*to clear color*/.
const char *colorize(int font = NC);
//decodes given URLEncoded string
std::string urlDecode(const std::string& str);
//makes given string URLEncoded
std::string urlEncode(const std::string &value);
//converts given string to upper case
std::string toUpper(const std::string& s);
//converts given string to lower case
std::string toLower(const std::string& s);

void setLogLevel(const LogLevel level);
LogLevel getLogLevel();
}
