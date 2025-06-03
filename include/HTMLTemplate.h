#pragma once

#include <string>
#include <map>
#include <optional>

#include "nlohmann/json.hpp"

namespace rweb
{ 

  class HTMLTemplate
  {
  public:
    HTMLTemplate(const char* html);
    HTMLTemplate(const std::string& html);
    const std::string& getHTML() const;
    const std::string& getFileName() const;
    const std::string& getStatusResponce() const;
    const std::string& getEncoding() const;
    const std::string& getContentType() const;
    const std::string& getRedirectLocation() const;

    const std::optional<std::string> getCookieValue(const std::string& name) const;
    const void setCookie(const std::string& name, const std::string& value, const unsigned int maxAgeSeconds=0, const bool httpOnly=false);

    //returns headers for setting cookies
    const std::string getAllCookieHeaders() const;

    //Renderes a template with specified json
    void renderJSON(const nlohmann::json& json);
  private:

    struct Cookie {
      std::string value;
      unsigned int maxAgeSeconds;
      bool httpOnly;
    };

    std::string m_html;
    std::string m_templateFileName;
    std::string m_responce;
    std::string m_encoding;
    std::string m_contentType;
    std::string m_location; //for redirects
    std::map<std::string, Cookie> m_cookies;

    friend HTMLTemplate createTemplate(const std::string&, const std::string&);
    friend HTMLTemplate redirect(const std::string&, const std::string&);
    friend HTMLTemplate abort(const std::string& statusResponce);
  };
}
