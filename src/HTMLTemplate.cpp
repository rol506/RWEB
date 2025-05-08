#include "../include/HTMLTemplate.h"

#include "../include/Utility.h"

#include <iostream>
#include <sstream>

namespace rweb
{ 

  //DEPRECATED!
  //get first found operator {% ... %} in 's'.
  //'pos_out' position of the found substring in 's'.
  static std::string getOperator(const std::string& s, std::size_t* pos_out = nullptr)
  {
    auto pos1 = s.find("{%")+2;
    auto pos2 = s.substr(pos1).find("%}");
    if (pos1 == std::string::npos || pos2 == std::string::npos)
    {
      if (pos_out)
        *pos_out = 0;
      return "";
    }
    if (pos_out)
      *pos_out = pos1;
    return s.substr(pos1, pos2);
  }

  //find {% 'target' %} in 's'. Returns true on success and false otherwise.
  //'s' is the input string.
  //'from' is position from where to start.
  //'target'. Function will find {% target %} in 's'.
  //'pos_out' will be position of found in target string.
  //'len_out' will be length of found substring.
  static bool findOperator(const std::string& s, const std::size_t from, const std::string& target, std::size_t* pos_out, std::size_t* len_out)
  {
    auto pos1 = s.find("{%", from);
    auto pos2 = s.find("%}", pos1)+2;
    std::string op = trim(s.substr(pos1+2, pos2-pos1-4));
    while ((trim(op) != target || op != "") || (pos1 == std::string::npos))
    {
      pos1 = s.find("{%", pos1+1);
      pos2 = s.find("%}", pos1)+2;
      if (pos1 == std::string::npos)
      {
        return false;
      }
      op = trim(s.substr(pos1+2, pos2-pos1-4));
      if (op == target)
      {
        *pos_out = pos1;
        *len_out = pos2-pos1;
        return true;
      }
    }

    if (op != "")
    {
      *pos_out = pos1;
      *len_out = pos2-pos1;
      return true;
    }

    return false;
  }

  //DEPRECATED!
  //Same as the getOperator, but searches from the end.
  static std::string getOperatorReverse(const std::string& s, std::size_t* pos_out = nullptr)
  {
    auto pos1 = s.rfind("{%")+2;
    auto pos2 = s.substr(pos1).rfind("%}");
    if (pos1 == std::string::npos || pos2 == std::string::npos)
    {
      if (pos_out)
        *pos_out = 0;
      return "";
    }
    if (pos_out)
      *pos_out = pos1;
    return s.substr(pos1, pos2);
  }

  //DEPRECATED!
  //"pos_out" - position of the found char
  //Returns first variable found in 's'.
  static std::string getVariable(const std::string& s, std::size_t start=0, std::size_t* pos_out=nullptr)
  {
    auto pos1 = s.find("{{", start)+2;
    if (pos1 == std::string::npos+2)
    {
      return "";
    }
    auto pos2 = s.find("}}", pos1-2);
    if (pos_out)
    {
      *pos_out = pos1-2;
    }
    return s.substr(pos1, pos2-pos1);
  }

  void HTMLTemplate::renderJSON(const nlohmann::json& json)
  {
    std::size_t start = 0;
    std::string var = "non-empty-string";
    while (var != "")
    {
      var = getVariable(m_html, start, &start);
      std::string name = trim(var);
      std::string body = name;
      auto vec = split(name, " +-/*");
      bool useMath = true;
      for (auto l: vec)
      {
        std::string res = "";
        auto value = json.find(l);
        if (value != json.end())
        {
          if (value->is_string())
          {
            res = *value;
            useMath = false;
          } else if (value->is_number_integer() || value->is_number_unsigned())
          {
            res = std::to_string((long long)*value);
          } else if (value->is_number_float())
          {
            std::stringstream ss;
            ss << (float)*value;
            res = ss.str();
          } else if (value->is_array())
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Failed to set array value for " << name << colorize(NC) << "\n";
            res = "";
            useMath = false;
            m_responce = HTTP_500;
            return;
          }
          else {
            std::cerr << colorize(RED) << "[TEMPLATE] Value of " << name << " is unsupported type!" << colorize(NC) << "\n";
            res = "";
            useMath = false;
            m_responce = HTTP_500;
            return;
          }

          body = replace(body, l, res);
        }
      }

      if (useMath)
      {
        if (auto f = std::find_if(body.begin(), body.end(), [](char c){return isalpha(c);}) == body.end())
        {
          //math does not have any variables -> ok
          double r = calculate(body);
          std::stringstream stream;
          //stream << std::fixed << std::setprecision(0) << r;
          stream << (float)r;
          body = stream.str();
        } else {
          std::cout << colorize(RED) << "[TEMPLATE] Cannot form a math statement. Found undeclared variable in " << '(' << body << ')' << colorize(NC) << "\n";
          //body = "";
          //m_responce = HTTP_500;
          start++;
          continue;
        }
      }

      m_html = replace(m_html, "{{" + var + "}}", body);
    }

    std::string html = m_html;
    size_t sz = html.size();

    int currLine = 1;
    int currChar = 1;

    for (int i=0;i<sz;++i)
    {
      char c = m_html[i];

      if (c == '\n')
      {
        currChar = 1;
        currLine++;
      } else {
        currChar++;
      }

      if (c == '{' && m_html[i+1] == '%') //if/for 
      {
        size_t startPos = i;
        std::string condition;
        std::string body;
        size_t len = 0;
        std::istringstream ss(m_html.substr(i+2));
        std::getline(ss, condition, '%');
        len = condition.size();
        condition = trim(condition);
        auto elements = split(condition, " ", 3);

        if (elements[0] == "for")
        {
          if (elements.size() == 4)
          {
            std::string endfor = getOperator(m_html.substr(m_html.find(condition)));
            while (trim(endfor) != "endfor" && trim(endfor) != "")
            {
              endfor = getOperator(m_html.substr(m_html.find(endfor)));
            }

            if (trim(endfor) == "endfor" && endfor != "")
            {
              size_t pos1 = m_html.find(condition)+len+1;
              size_t pos2 = m_html.find("{%" + endfor + "%}");
              body = trim(m_html.substr(pos1, pos2-pos1));
              std::string res = "";

              if (elements[2] != "in")
              {
                std::cerr << colorize(RED) <<  "[TEMPLATE] Bad for loop syntax!" << colorize(NC) << "\n";
                m_responce = HTTP_500;
                return;

                html.replace(pos2, endfor.size()+4, "");
                html.replace(pos1-len-2, len+4, "");

                i = 0;
                m_html = html;
                currLine = 1;
                currChar = 1;
                continue;
              }

              std::string result = "";
              if (elements[3].substr(0, 5) == "range")
              {
                std::string count = elements[3].substr(6, elements[3].size()-6-1);
                int cnt = atoi(count.c_str());
                if (!cnt)
                {
                  //count is a statement
                  auto els = split(count, "+-*/ ");
                  for (auto it: els)
                  {
                    std::string res = "";
                    auto value = json.find(it);
                    if (value != json.end())
                    {
                      if (value->is_string())
                      {
                        res = *value;
                      } else if (value->is_number_integer() || value->is_number_unsigned())
                      {
                        res = std::to_string((long long)*value);
                      } else if (value->is_number_float())
                      {
                        std::stringstream ss;
                        ss << (float)*value;
                        res = ss.str();
                      } else if (value->is_array())
                      {
                        std::cerr << colorize(RED) << "[TEMPLATE] Failed to set array value for " << it << colorize(NC) << "\n";
                        m_responce = HTTP_500;
                        return;
                        res = "";
                      }
                      else {
                        std::cerr << colorize(RED) << "[TEMPLATE] Value of " << it << " is unsupported type!" << colorize(NC) << "\n";
                        m_responce = HTTP_500;
                        return;
                        res = "";
                      }

                      count = replace(count, it, res);
                    }

                  }
                  if (auto f = std::find_if(count.begin(), count.end(), [](char c){return isalpha(c);}) == count.end())
                  {
                    //math does not have any variables -> ok
                    double r = calculate(count);
                    cnt = static_cast<int>(r); 
                  } else {
                    std::cout << colorize(RED) <<  "[TEMPLATE] Cannot form a math statement. Found undeclared variable at: " << f << colorize(NC) << "\n";
                    m_responce = HTTP_500;
                    return;
                    cnt = 0;
                  }
                }
                for (int k=0;k<cnt;++k)
                {
                  std::string tmp = body;
                  std::string var;
                  std::string var_start;
                  while ((var_start = getVariable(tmp)) != "")
                  {
                    var = var_start;
                    bool useMath = true;
                    //count is a statement
                    auto els = split(var, "+-*/ ");
                    for (auto it: els)
                    {
                      std::string res = "";
                      auto value = json.find(it);
                      if (value != json.end())
                      {

                        if (value->is_string())
                        {
                          res = *value;
                          useMath = false;
                        } else if (value->is_number_integer() || value->is_number_unsigned())
                        {
                          res = std::to_string((long long)*value);
                        } else if (value->is_number_float())
                        {
                          std::stringstream ss;
                          ss << (float)*value;
                          res = ss.str();
                        } else if (value->is_array())
                        {
                          std::cerr << colorize(RED) << "[TEMPLATE] Failed to set array value for " << it << colorize(NC) << "\n";
                          m_responce = HTTP_500;
                          return;
                          res = "";
                          useMath = false;
                        }
                        else {
                          std::cerr << colorize(RED) << "[TEMPLATE] Value of " << it << " is unsupported type!" << colorize(NC) << "\n";
                          m_responce = HTTP_500;
                          return;
                          res = "";
                          useMath = false;
                        }

                        var = replace(var, it, res);
                      } else {
                        if (trim(it) == elements[1])
                        {
                          var = replace(var, it, std::to_string(k));
                        }
                      }
                    }
                    if (auto f = std::find_if(var.begin(), var.end(), [](char c){return isalpha(c);}) == var.end())
                    {
                      //math does not contain any variables -> ok
                      double r = calculate(trim(var));
                      std::stringstream stream;
                      //stream << std::fixed << std::setprecision(0) << r;
                      stream << (float)r;
                      var = stream.str(); 
                    } else {
                      std::cout << colorize(RED) << "[TEMPLATE] Cannot form a math statement. Found undeclared variable at: " << f << colorize(NC) << "\n";
                      m_responce = HTTP_500;
                      return;
                      var = "";
                    }
                    tmp = replace(tmp, "{{"+var_start+"}}", var);
                  }
                  result += tmp;
                }

              } else {
                //iterating dict
                std::string& name = elements[3];
                
                auto value = json.find(name);
                if (value != json.end())
                {
                  if (value->is_array())
                  {
                    result = "";
                    for (auto it = value->begin(); it != value->end();++it)
                    { 
                      std::string tmp = body;
                      size_t pos1 = 0;
                      do {
                        pos1 = tmp.find("{{")+2;
                        size_t pos2 = tmp.find("}}");
                        if (pos1 == std::string::npos || pos2 == std::string::npos)
                        {
                          break;
                        }

                        std::string name = tmp.substr(pos1, pos2-pos1); 
                        std::vector<std::string> subscripts = split(name, ".");
                        std::string sRes = "";

                        auto val = it->find(subscripts[1]);
                        if (val == it->end())
                        {
                          auto els = split(subscripts[1], "+-*/ ");
                          std::string expr = subscripts[1];
                          for (auto i: els)
                          {
                            bool useMath = true;
                            std::string res = "";
                            auto l = it->find(i);
                            if (l != it->end())
                            {
                              if (l->is_string())
                              {
                                res = *l;
                                useMath = false;
                              } else if (l->is_number_integer() || l->is_number_unsigned())
                              {
                                res = std::to_string((long long)*l);
                              } else if (value->is_number_float())
                              {
                                std::stringstream ss;
                                ss << (float)*l;
                                res = ss.str();
                              } else if (l->is_array())
                              {
                                std::cerr << colorize(RED) <<  "[TEMPLATE] Failed to set value: " << '"' << i << '"' <<  " is an array!" << colorize(NC) << "\n";
                                m_responce = HTTP_500;
                                return;
                                res = "";
                                useMath = false;
                              }
                              else {
                                std::cerr << colorize(RED) << "[TEMPLATE] Value of " << i << " is unsupported type!" << colorize(NC) << "\n";
                                m_responce = HTTP_500;
                                return;
                                res = "";
                                useMath = false;
                              }

                              expr = replace(expr, i, res);
                            }

                            if (useMath)
                            {
                              if (auto f = std::find_if(expr.begin(), expr.end(), [](char c){return isalpha(c);}) == expr.end())
                              {
                                //math does not have any variables -> ok
                                double r = calculate(expr);
                                std::stringstream stream;
                                //stream << std::fixed << std::setprecision(0) << r;
                                stream << (float)r;
                                sRes = stream.str();
                              } else {
                                std::cerr << colorize(RED) << "[TEMPLATE] Cannot form a math statement: found undeclared variable: " << '"' << expr << '"' << 
                                  colorize(NC) << "\n";
                                m_responce = HTTP_500;
                                return;
                                sRes = "";
                              }
                            } else {
                              auto l = it->find(els[0]);
                              if (l != it->end())
                              {
                                if (l->is_string())
                                {
                                  sRes = *l;
                                } else {
                                  std::cerr << colorize(RED) << "[TEMPLATE] Value of the " << '"' << els[0] << '"' << " is not a string!" << colorize(NC) << "\n";
                                  m_responce = HTTP_500;
                                  return;
                                  sRes = "";
                                }
                              } else {
                                std::cerr << colorize(RED) << "[TEMPLATE] Cannot find value of the " << '"' << els[0] << '"' << colorize(NC) << "\n";
                                m_responce = HTTP_500;
                                return;
                                sRes = "";
                              }
                            }
                          }
                        }
                        auto res = val;
                        if (subscripts.size() > 2)
                        {
                          /*for (int i=1;i<subscripts.size();++i)
                          {
                            nlohmann::json::const_iterator tmp = val->find(subscripts[i]);
                            if (tmp != val->end())
                            {
                              val = tmp;
                            } else {
                              std::cout << "[TEMPLATE] Can't find value of " << subscripts[i] << "\n";
                              break;
                            }
                          }*/
                          std::cout << colorize(RED) << "[TEMPLATE] Too many subscripts in " << name << colorize(NC) << "\n";
                          m_responce = HTTP_500;
                          return;
                          break;
                        }

                        res = val;
                        //FOUND VALUE (res)
                        if (sRes == "")
                        {
                          sRes = *res;
                        }

                        tmp.replace(pos1-2, pos2-pos1+4, sRes);

                      } while (pos1 != std::string::npos);
                      result += tmp;
                    }
                  } else {
                    if (value->is_object())
                    {
                      std::cerr << colorize(RED) << "[TEMPLATE] Iterating dicts is an unsupported option! Value of " << '"' << name << '"' << " is a json object!"
                        << colorize(NC) << "\n";
                      m_responce = HTTP_500;
                      return;
                    } else {
                      std::cerr << colorize(RED) << "[TEMPLATE] Value of " << '"' << name << '"' << " is not a json dict or an array!" << colorize(NC) << "\n";
                      m_responce = HTTP_500;
                      return;
                    }
                  }
                } else {
                  std::cerr << colorize(RED) << "[TEMPLATE] Could not find value with key " << '"' << name << '"' << colorize(NC) << "\n";
                  m_responce = HTTP_500;
                  return;
                }
              }

              html.replace(startPos, m_html.find("{%" + endfor + "%}", startPos)-startPos+endfor.size()+4, result);

              i = 0;
              m_html = html;
              sz = m_html.size();
              currLine = 1;
              currChar = 1;
              continue;
            } else {
              std::cerr << colorize(RED) << "[TEMPLATE] Error: for loop must end with {% endfor %}!" << colorize(NC) << "\n";
              m_responce = HTTP_500;
              return;
              html.replace(startPos, m_html.find("{%" + endfor + "%}", startPos)-startPos+endfor.size()+4, "");
              i = 0;
              m_html = html;
              sz = m_html.size();
              currLine = 1;
              currChar = 1;
              continue;
            }
          }
          
          std::cerr << colorize(RED) << "[TEMPLATE] Error: invalid for loop syntax!" << colorize(NC) << "\n";
          //html.replace(i, len+4, ""); 
          m_responce = HTTP_500;
          return;
        } else if (elements[0] == "if")
        {
          // >0 -> true
          // string != "" -> true

          auto els = split(condition, " ", 1);

          std::string cond = replace(els[1], " ", "");

          size_t f;
          bool status; //is condition true or false
          std::string lv = "";
          std::string rv = "";
          std::string operation = "";
          bool useMath = true;
          double resL;
          double resR;

          if ((f=cond.find(">")) != std::string::npos) //greater
          {
            lv = cond.substr(0, f);
            rv = cond.substr(f+1, cond.size()-f-1);
            
            operation = ">";
          } else if ((f=cond.find("<")) != std::string::npos)
          {
            lv = cond.substr(0, f);
            rv = cond.substr(f+1, cond.size()-f-1);
            
            operation = "<";
          } else if ((f=cond.find("==")) != std::string::npos)
          {
            lv = cond.substr(0, f);
            rv = cond.substr(f+2, cond.size() - f - 2);

            operation = "==";
          } else if ((f=cond.find("!=")) != std::string::npos)
          {
            lv = cond.substr(0, f);
            rv = cond.substr(f+2, cond.size() - f - 2);

            operation = "!=";
          }

          //calculating values
          {
            resL = 0;
            resR = 0;

            //math for left part
            {
              auto elsL = split(lv, "+-*/ "); //math elements left
              for (auto e: elsL)
              {
                auto value = json.find(e);
                std::string res = "";
                if (value != json.end())
                {
                  if (value->is_string())
                  {
                    res = *value;
                    useMath = false;
                  } else if (value->is_number_integer() || value->is_number_unsigned())
                  {
                    res = std::to_string((long long)*value);
                  } else if (value->is_number_float())
                  {
                    std::stringstream ss;
                    ss << (float)*value;
                    res = ss.str();
                  } else if (value->is_array())
                  {
                    std::cerr << colorize(RED) << "[TEMPLATE] Failed to set array value for " << e << colorize(NC) << "\n";
                    res = "";
                    useMath = false;
                    m_responce = HTTP_500;
                    return;
                  }
                  else {
                    std::cerr << colorize(RED) << "[TEMPLATE] Value of " << e << " is unsupported type!" << colorize(NC) << "\n";
                    res = "";
                    useMath = false;
                    m_responce = HTTP_500;
                    return;
                  }

                  lv = replace(lv, e, res);
                }
              }

              {
                if (auto f = std::find_if(lv.begin(), lv.end(), [](char c){return isalpha(c);}) == lv.end())
                {
                  //math does not have any variables -> ok
                } else {
                  std::string fnd = lv.substr(f, lv.find(" ", f)-f);
                  std::stringstream ss;
                  ss << std::hash<std::string>{}(fnd);
                  lv = replace(lv, fnd, ss.str());
                }
                resL = calculate(lv);
              }
            }
            //math for right part
            {
              auto elsR = split(rv, "+-*/ "); //math elements right
              bool useMath = true;
              for (auto e: elsR)
              {
                auto value = json.find(e);
                std::string res = "";
                if (value != json.end())
                {
                  if (value->is_string())
                  {
                    res = *value;
                    useMath = false;
                  } else if (value->is_number_integer() || value->is_number_unsigned())
                  {
                    res = std::to_string((long long)*value);
                  } else if (value->is_number_float())
                  {
                    std::stringstream ss;
                    ss << (float)*value;
                    res = ss.str();
                  } else if (value->is_array())
                  {
                    std::cerr << colorize(RED) << "[TEMPLATE] Failed to set array value for " << e << colorize(NC) << "\n";
                    res = "";
                    useMath = false;
                    m_responce = HTTP_500;
                    return;
                  }
                  else {
                    std::cerr << colorize(RED) << "[TEMPLATE] Value of " << e << " is unsupported type!" << colorize(NC) << "\n";
                    res = "";
                    useMath = false;
                    m_responce = HTTP_500;
                    return;
                  }

                  rv = replace(rv, e, res);
                }
              }

              {
                if (auto f = std::find_if(rv.begin(), rv.end(), [](char c){return isalpha(c);}) == rv.end())
                {
                  //math does not have any variables -> ok
                } else {
                  std::string fnd = rv.substr(f, rv.find(" ", f)-f);
                  std::stringstream ss;
                  ss << std::hash<std::string>{}(fnd);
                  rv = replace(rv, fnd, ss.str());
                }
                resR = calculate(rv);
              }
            }
          }

          if (operation == ">")
            status = resL > resR;
          else if (operation == "<")
            status = resL < resR;
          else if (operation == "==")
            status = resL == resR;
          else if (operation == "!=")
            status = resL != resR;
          else
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Unknown operation! Something went wrong in the 'if' statement! Condition will be counted as false!" 
              << colorize(NC) << "\n";
            status = false;
            m_responce = HTTP_500;
            return;
          }

          std::string result = "";
          std::size_t else_pos = 0;
          std::size_t else_size = 0;
          std::size_t endif_pos = 0;
          std::size_t endif_size = 0; 

          findOperator(m_html, startPos, "else", &else_pos, &else_size);

          if (!findOperator(m_html, startPos, "endif", &endif_pos, &endif_size))
          {
            std::cerr << colorize(RED) <<  "[TEMPLATE] Failed to find endif!" << colorize(NC) << "\n";
            m_responce = HTTP_500;
            return;
          }

          if (endif_pos == 0) //no endif
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Cannot find endif!" << colorize(NC) << "\n";
            m_responce = HTTP_500;
            return;
            result = "";
          } else { //all ok
            if (status) //true section
            {
              if (else_size != 0) //with else
              {
                result = m_html.substr(startPos + len+4, else_pos-startPos-len-4);
              } else { //without else
                result = m_html.substr(startPos+len+4, endif_pos-startPos-len-4);
              }
            } else { //false section
              if (else_size != 0) //with else
              {
                result = m_html.substr(else_pos+else_size, endif_pos-else_pos-else_size);
              } else { //without else
                result = "";
              }
            }
          }

          std::string true_body = "";
          std::string false_body = "";

          if (else_size != 0)
          {
            true_body = m_html.substr(startPos+len+4, else_pos-startPos-len-4);
            false_body = m_html.substr(else_pos+else_size, endif_pos-else_pos-else_size);
          } else {
            true_body = m_html.substr(startPos+len+4, endif_pos-startPos-len-4);
          }

          result = trim(status ? true_body : false_body);

          html.replace(startPos, endif_pos+endif_size-startPos, result);
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Error: unknown operator " << '"' << elements[0] << '"' << colorize(NC) << "\n";
          std::cout << m_html << "\n";
          html.replace(i, len+4, "");
          m_responce = HTTP_500;
          return;
        }

        m_html = html;
        i = 0;
        sz = m_html.size();
        currLine = 1;
        currChar = 1;

      } else if (c == '{' && m_html[i+1] == '{') //variable
      {
      } 
    }
  }

  const std::string& HTMLTemplate::getHTML() const
  {
    return m_html;
  }

  const std::string& HTMLTemplate::getFileName() const
  {
    return m_templateFileName;
  }

  const std::string& HTMLTemplate::getStatusResponce() const
  {
    return m_responce;
  }

  const std::string& HTMLTemplate::getEncoding() const
  {
    return m_encoding;
  }

  const std::string& HTMLTemplate::getContentType() const
  {
    return m_contentType;
  } 

  const std::string& HTMLTemplate::getRedirectLocation() const
  {
    return m_location;
  }
}
