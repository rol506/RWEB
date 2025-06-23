#include <iostream>

#include <RWEB.h>


int main()
{
  rweb::init(false);
  rweb::setResourcePath("/home/rol506/proj/C++/RWEB/tests/template/res/");
  rweb::HTMLTemplate temp = rweb::createTemplate("index.html", rweb::HTTP_200);

  temp.flash("ladno", "info");
  temp.flash("not ladno", "info");

  nlohmann::json json = nlohmann::json::parse(rweb::getFileString("../menu.json"));
  json["currentURL"] = "/ladno";

  temp.renderJSON(json);
  if (temp.getStatusResponce() == rweb::HTTP_500)
  {
    std::cout << rweb::colorize(rweb::RED) << "----RENDER_FAILURE----" << rweb::colorize() << "\n";
  } else {
    std::cout << rweb::colorize(rweb::GREEN) << "----RENDER_SUCCESS----" << rweb::colorize() << "\n";
    std::cout << "RESULT HTML: " << temp.getHTML() << "\n";
  }

  //std::cout << "EXPECTED HTML: " << rweb::getFileString("result.html") << "\n";

  if (rweb::replace(temp.getHTML(), "\n", "") != rweb::replace(rweb::getFileString("result.html"), "\n", ""))
  {
    return -1;
  }

  return 0;
}
