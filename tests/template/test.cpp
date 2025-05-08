#include <iostream>
#include <cstdlib>

#include <RWEB.h>


int main()
{
  rweb::init(false);
  rweb::setResourcePath("/home/rol506/proj/C++/RWEB/tests/template/res/");
  rweb::HTMLTemplate temp = rweb::createTemplate("index.html", rweb::HTTP_200);

  nlohmann::json json = nlohmann::json::parse(rweb::getFileString("../menu.json"));
  json["currentURL"] = "/ladno";

  temp.renderJSON(json);

  std::cout << "RESULT HTML: " << temp.getHTML() << "\n";
  std::cout << "EXPECTED HTML: " << rweb::getFileString("result.html") << "\n";

  if (rweb::replace(temp.getHTML(), "\n", "") != rweb::replace(rweb::getFileString("result.html"), "\n", ""))
  {
    return -1;
  }

  return 0;
}
