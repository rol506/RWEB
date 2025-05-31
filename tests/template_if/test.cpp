#include "Utility.h"
#include <iostream>

#include <RWEB.h>


int main()
{
  rweb::init(false);
  rweb::setResourcePath("/home/rol506/proj/C++/RWEB/tests/template_if/res/");
  rweb::HTMLTemplate temp = rweb::createTemplate("index.html", rweb::HTTP_200);

  nlohmann::json json = nlohmann::json::parse(rweb::getFileString("../menu.json"));

  temp.renderJSON(json);

  std::cout << "PATH: " << rweb::getResourcePath() << "\n";
  std::cout << (temp.getStatusResponce() == rweb::HTTP_500 ? rweb::colorize(rweb::RED) : "") << "RESULT HTML: " << rweb::colorize(rweb::NC) << temp.getHTML() << "\n";
  std::cout << "EXPECTED HTML: " << rweb::getFileString("result.html") << "\n";

  if (rweb::replace(temp.getHTML(), "\n", "") != rweb::replace(rweb::getFileString("result.html"), "\n", ""))
  {
    return -1;
  }

  return 0;
}
