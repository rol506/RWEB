#include "../include/HTMLTemplate.h"

#include "../include/Utility.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <vector>
#include <sstream>
#include <optional>
#include <variant>

namespace rweb
{ 
  typedef enum
  {
    VARIABLE,
    STRING,
    MATH,
    SUBSCRIPT,
    OPERATOR,
    COMPARISON_OPERATOR,
    IF,
    IF_BODY,
    ELSE,
    ELSE_BODY,
    ENDIF,
    FOR,
    FOR_BODY,
    ENDFOR,
    KEYWORD,
    ITERATOR,
    ARGUMENT,
    OPEN_BRACKET,
    CLOSE_BRACKET
  } TOKEN_TYPE;

  static inline const std::string typeToString(const TOKEN_TYPE in)
  {
    switch(in)
    {
      case STRING:
        return "STRING";
      case VARIABLE:
        return "VARIABLE";
      case OPERATOR:
        return "OPERATOR";
      case COMPARISON_OPERATOR:
        return "COMPARISON_OPERATOR";
      case MATH:
        return "MATH";
      case SUBSCRIPT:
        return "SUBSCRIPT";
      case IF:
        return "IF";
      case IF_BODY:
        return "IF_BODY";
      case ELSE:
        return "ELSE";
      case ELSE_BODY:
        return "ELSE_BODY";
      case ENDIF:
        return "ENDIF";
      case FOR:
        return "FOR";
      case FOR_BODY:
        return "FOR_BODY";
      case ENDFOR:
        return "ENDFOR";
      case KEYWORD:
        return "KEYWORD";
      case OPEN_BRACKET:
        return "OPEN_BRACKET";
      case CLOSE_BRACKET:
        return "CLOSE_BRACKET";
      case ITERATOR:
        return "ITERATOR";
      case ARGUMENT:
        return "ARGUMENT";
    }

    return "Unknown"; 
  }

  static const inline std::string stringifyJson(const nlohmann::json& json)
  {
    if (json.is_null())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify empty json!" << colorize(NC) << "\n";
      return "";
    } else if (json.is_array())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify json array!" << colorize(NC) << "\n";
      return "";
    } else if (json.is_object())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify json object!" << colorize(NC) << "\n";
      return "";
    } else if (json.is_string())
    {
      return trim((std::string)json);
    } else if (json.is_number())
    {
      std::stringstream ss;
      ss << json;
      return ss.str();
    } else {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify unsupported type json!" << colorize(NC) << "\n";
      return "";
    } 
  }

  static const inline std::string stringifyJson(const nlohmann::json::const_iterator& json)
  {
    if (json->is_null())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify empty json!" << colorize(NC) << "\n";
      return "";
    } else if (json->is_array())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify json array!" << colorize(NC) << "\n";
      return "";
    } else if (json->is_object())
    {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify json object!" << colorize(NC) << "\n";
      return "";
    } else if (json->is_string())
    {
      return trim((std::string)*json);
    } else if (json->is_number())
    {
      std::stringstream ss;
      ss << *json;
      return ss.str();
    } else {
      std::cerr << colorize(YELLOW) << "[TEMPLATE] Cannot stringify unsupported type json!" << colorize(NC) << "\n";
      return "";
    } 
  }

  static const std::optional<nlohmann::json> getJson(const nlohmann::json& root, const std::vector<std::string>& names)
  {
    const nlohmann::json* leaf = &root;
    for (const auto& name: names)
    {
      if (leaf->contains(name))
      {
        leaf = &leaf->at(name);
      } else {
        std::cerr << colorize(RED) << "[TEMPLATE] Cannot find the attribute \"" << name << "\"!" << colorize(NC) << "\n";
        return std::nullopt;
      }
    }
    return *leaf;
  }

  //false on an error
  static bool lexer_analyze(std::string& code, const nlohmann::json& json);

  static const std::optional<std::string> parser_eval(std::vector<std::pair<TOKEN_TYPE, std::string>>& input, const nlohmann::json& _json, bool* error = nullptr)
  {
    nlohmann::json json = _json; //working copy
    std::string result = "";
    std::string expression = "";

    /*for (auto it : input)
    {
      std::cout << "{ " << typeToString(it.first) << " - " << it.second << " }\n";
    }std::cout << "----TOKENS_END----\n";*/

    if (input.begin()->first == IF)
    {
      auto token = input.begin()+1;
      
      std::string lv = "";
      std::string op = "";
      std::string rv = "";
      std::string true_body = "";
      std::string else_body = ""; 

      //lv
      std::vector<std::string> attributes; //temporary
      while (true)
      {
        if (token == input.end())
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        if (token->first == MATH)
        {
          lv += token->second;
        } else if (token->first == OPERATOR)
        {
          lv += token->second;
        } else if (token->first == VARIABLE)
        {
          std::string name = token->second;

          token++;
          if (token == input.end())
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          if (token->first == MATH && token->second == ".")
          {
            attributes.push_back(name);
            while (true)
            {
              token++;
              if (token == input.end())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
                return std::nullopt;
              }

              if (token->first != MATH && token->first != VARIABLE)
                break;

              if (token->first == VARIABLE)
              {
                attributes.push_back(token->second);
              }

              token++;
              if (token == input.end())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
                return std::nullopt;
              } 

              if (token->first != MATH || token->second != ".")
              {
                break;
              }
            }

            auto r = getJson(json, attributes);
            if (!r)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << name;
              for (int it = 1; it < attributes.size(); ++it)
              {
                std::cerr << "." << attributes[it];
              }
              std::cerr << colorize(NC) << "\n";
              return std::nullopt;
            }

            nlohmann::json val = *r;
            lv += stringifyJson(val);
            attributes.clear();
            token--;
          } else {
            token--;
            attributes.clear();

            nlohmann::json val;
            auto ptr = json.find(token->second);
            if (ptr != json.end())
            {
              val = *ptr;
              lv += stringifyJson(val);
            } else {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << token->second << "\"!" << colorize(NC) << "\n";
              return std::nullopt;
            }
          }
          
        } else if (token->first == STRING)
        {
          lv += token->second;
        } else if (token->first == COMPARISON_OPERATOR)
        {
          op = token->second;
          break;
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Unexpected token " << typeToString(token->first) << " in the IF statement!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        token++;
      }

      //rv
      token++;
      while (true)
      {
        if (token == input.end())
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        if (token->first == MATH)
        {
          rv += token->second;
        } else if (token->first == OPERATOR)
        {
          rv += token->second;
        } else if (token->first == VARIABLE)
        {
          std::string name = token->second;

          token++;
          if (token == input.end())
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          if (token->first == MATH && token->second == ".")
          {
            attributes.push_back(name);
            while (true)
            {
              token++;
              if (token == input.end())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
                return std::nullopt;
              }

              if (token->first != MATH && token->first != VARIABLE)
                break;

              if (token->first == VARIABLE)
              {
                attributes.push_back(token->second);
              }

              token++;
              if (token == input.end())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Unexpected end of input in the IF statement!" << colorize(NC) << "\n";
                return std::nullopt;
              } 

              if (token->first != MATH || token->second != ".")
              {
                break;
              }
            }

            auto r = getJson(json, attributes);
            if (!r)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << name;
              for (int it = 1; it < attributes.size(); ++it)
              {
                std::cerr << "." << attributes[it];
              }
              std::cerr << colorize(NC) << "\n";
              return std::nullopt;
            }

            nlohmann::json val = *r;
            rv += stringifyJson(val);
            attributes.clear();
          } else {
            token--;
            attributes.clear();

            nlohmann::json val;
            auto ptr = json.find(token->second);
            if (ptr != json.end())
            {
              val = *ptr;
              rv += stringifyJson(val);
            } else {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << token->second << "\"!" << colorize(NC) << "\n";
              return std::nullopt;
            }
          }
          
        } else if (token->first == STRING)
        {
          rv += token->second;
        } else if (token->first == IF_BODY)
        {
          break;
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Unexpected token " << typeToString(token->first) << " in the IF statement!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        token++;
      }

      //if (token != input.end())
      {
        if (token->first != IF_BODY)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the IF_BODY token!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        true_body = trim(token->second);
        token++;

        if (token != input.end())
        {
          if (token->first != ELSE_BODY)
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the ELSE_BODY token!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          else_body = trim(token->second);
        }

      }/* else {
        std::cerr << colorize(RED) << "[TEMPLATE] Error! There is no data after IF conditons!" << colorize(NC) << "\n";
        return std::nullopt;
      }*/

      long long l_res = 0;
      long long r_res = 0;
      bool useStrings = false;

      {
        //left math
        {
          bool is_ok = true;
          double r = calculate(lv, &is_ok);
          if (is_ok)
          {
            std::stringstream ss;
            ss << r;
            lv = ss.str();
          }
        }

        //right math
        {
          bool is_ok = true;
          double r = calculate(rv, &is_ok);
          if (is_ok)
          {
            std::stringstream ss;
            ss << r;
            rv = ss.str();
          }
        }
      }

      try {
        l_res = std::stoi(lv);
      } catch (std::invalid_argument& e) {
        useStrings = true;
      }

      if (!useStrings) //not needed
      {
        try {
          r_res = std::stoi(rv);
        } catch (std::invalid_argument& e) {
          useStrings = true;
        }
      }

      bool result;

      if (useStrings)
      {
        if (op == ">") {
          result = lv.size() > rv.size();
        } else if (op == "<")
        {
          result = lv.size() < rv.size();
        } else if (op == "==")
        {
          result = lv == rv;
        } else if (op == "!=")
        {
          result = lv != rv;
        } else if (op.empty())
        {
          result = !lv.empty();
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Something went wrong! Unknown operator in IF statemnt: " << op << colorize(NC) << "\n";
          return std::nullopt;
        }
      } else {
        if (op == ">")
        {
          result = l_res > r_res;
        } else if (op == "<")
        {
          result = l_res < r_res;
        } else if (op == "==")
        {
          result = l_res == r_res;
        } else if (op == "!=")
        {
          result = l_res != r_res;
        } else if (op.empty())
        {
          result = l_res;
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Something went wrong! Unknown operator in IF statement: " << op << colorize(NC) << "\n";
          return std::nullopt;
        }
      }

      if (result)
      {
        return true_body;
      } else {
        return else_body.empty() ? "" : else_body;
      }

    } else if (input.begin()->first == ELSE)
    {
      std::cerr << colorize(RED) << "[TEMPLATE] Error! Unrecognized token ELSE before IF!" << colorize(NC) << "\n";
      return std::nullopt;
    } else if (input.begin()->first == ENDIF)
    {
      std::cerr << colorize(RED) << "[TEMPLATE] Error! Unrecognized token ENDIF before IF!" << colorize(NC) << "\n";
      return std::nullopt;
    } else if (input.begin()->first == FOR)
    {
      //FOR

      auto token = input.begin()+1;

      if (token->first != VARIABLE)
      {
        std::cerr << colorize(RED) << "[TEMPLATE] Error! For loop must start with variable declaration!" << colorize(NC) << "\n";
        return std::nullopt;
      }

      struct Variable {
        std::string name;
        std::variant<std::string, nlohmann::json> value;
      };

      std::vector<Variable> variables;

      while (token->first == VARIABLE)
      {
        variables.push_back({token->second, std::string("")});
        token++;
      }

      if (token->first != KEYWORD && token->second != "in")
      {
        std::cerr << colorize(RED) << "[TEMPLATE] Error! \"in\" keyword must separate variables and iterator!" << colorize(NC) << "\n";
        return std::nullopt;
      }
      token++;

      if (token->first != ITERATOR && token->first != VARIABLE)
      {
        std::cerr << colorize(RED) << "[TEMPLATE] Error! An iterator or a variable must be present after \"in\" keyword!" << colorize(NC) << "\n";
        return std::nullopt;
      }

      //handle iterator functions
      if (token->second == "enumerate")
      {
        if (token->first == VARIABLE)
        {
          std::cout << colorize(YELLOW) << "[TEMPLATE] Warning! Do not use 'enumerate' as a variable name, it is an iterator function name!"
            << colorize(NC) << "\n";
        }

        if (variables.size() != 2)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! \"enumerate\" has 2 return values! " << variables.size() << " provided!"
            << colorize(NC) << "\n";
          return std::nullopt;
        }

        token++;
        if (token->first != ARGUMENT)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! \"enumerate\" takes 1 positional argument! Less than 1 provided!"
            << colorize(NC) << "\n";
          return std::nullopt;
        }

        std::string argName = std::move(token->second);

        token++;
        if (token->first == ARGUMENT)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! \"enumerate\" takes only 1 positional argument! More than 1 provided!"
            << colorize(NC) << "\n";
          return std::nullopt;
        }

        if (token->first != FOR_BODY)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the FOR_BODY token!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        std::string temp = token->second; //template body

        nlohmann::json val;

        bool found = false;
        if (argName.find_first_of(".") != std::string::npos)
        {
          auto sp = split(argName, ".");
          auto r = getJson(json, sp);
          if (!r)
          {
            found = false;
          } else {
            val = *r;
            found = true;
          }
        } else {
          auto ptr = json.find(argName);
          found = ptr != json.end();
          val = *ptr;
        }

        if (found)
        {
          if (!val.is_array())
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Error! Iteration using \"enumerate\" supports only json arrays!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          int iter=0;
          for (auto it = val.begin(); it != val.end(); ++it)
          {
            //iteration

            //process
            std::string tmp = temp;
            {
              variables[0].value = std::to_string(iter);
              variables[1].value = *it;
            }

            nlohmann::json innerJson = json;
            try {
              innerJson[variables[0].name] = std::get<std::string>(variables[0].value);
            } catch (const std::bad_variant_access& e)
            {
              innerJson[variables[0].name] = std::get<nlohmann::json>(variables[0].value);
            }
            try {
              innerJson[variables[1].name] = std::get<std::string>(variables[1].value);
            } catch (const std::bad_variant_access& e)
            {
              innerJson[variables[1].name] = std::get<nlohmann::json>(variables[1].value);
            }
            
            if (!lexer_analyze(tmp, innerJson))
            {
              return std::nullopt;
            }

            //result
            result += trim(tmp);

            //after result
            ++iter;
          }

        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the json array \"" << argName << "\"!" << colorize(NC) << "\n";
          return std::nullopt;
        }

      } else {
        if (variables.size() > 1)
        {
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Array iteration uses exactly 1 argument! More than 1 provided!" << colorize(NC) << "\n";
          return std::nullopt;
        }

        nlohmann::json val;
        bool found = false;
        if (token->second.find_first_of(".") != std::string::npos)
        {
          auto sp = split(token->second, ".");
          auto r = getJson(json, sp);
          if (!r)
          {
            found = false;
          } else {
            val = *r; 
            found = true;
          }
        } else {
          auto ptr = json.find(token->second);
          found = ptr != json.end();
          if (found)
            val = *ptr;
        }

        if (found)
        {

          if (!val.is_array())
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Error! Iteration supports only json arrays! Provided object is not an array!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          ++token;
          if (token == input.end() || token->first != FOR_BODY)
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Cannot find the FOR_BODY token!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          const std::string temp = std::move(token->second);
          const std::string varName = variables[0].name;

          for (auto it: val)
          {
            std::string tmp = temp;

            nlohmann::json innerJson = json;
            innerJson[varName] = it;
            
            if (!lexer_analyze(tmp, innerJson))
            {
              return std::nullopt;
            }

            result += trim(tmp);
          }
        } else {
          std::cerr << colorize(RED) << "[TEMPLATE] Cannot find the json array \"" << token->second << "\"!" << colorize(NC) << "\n";
          return std::nullopt;
        }
      }

      return result;
    } else if (input.begin()->first == ENDFOR)
    {
      std::cerr << colorize(RED) << "[TEMPLATE] Error! Unrecognized token ENDFOR before FOR!" << colorize(NC) << "\n";
      return std::nullopt;
    } else {
      // VARIABLE 
      for (auto token = input.begin(); token != input.end(); ++token)
      {
        if (token->first == SUBSCRIPT){
          std::cerr << colorize(RED) << "[TEMPLATE] Error! Unrecognized subscript token in variable!" << colorize(NC) << "\n";
          return std::nullopt;
        } else if (token->first == VARIABLE)
        {
          if ((++token) != input.end() && token->first == MATH && token->second == ".")
          {
            token--;
            std::string lastName = token->second;
            std::string expr = token->second;
            bool fnd = true;

            std::vector<std::string> attributes = {token->second};
            while (token != input.end())
            {
              token++;
              if (token->first == MATH && token->second == ".")
              {
                token++;

                if (token->first != VARIABLE)
                {
                  break;
                }

                attributes.emplace_back(token->second);
              }
            }

            auto val = getJson(json, attributes);
            if (!val)
            {
              std::cout << colorize(RED) << "[TEMPLATE] Failed to process attributes: " << colorize(NC) << attributes[0];
              for (auto it = attributes.begin()+1; it!= attributes.end();++it)
              {
                std::cout << "." << *it;
              }std::cout << colorize(NC) << "\n";
              return std::nullopt;
            }

            token--;
            expression += stringifyJson(val);
          } else {
            token--;
            try {
              expression += stringifyJson(json.find(token->second));
            } catch (const nlohmann::detail::out_of_range& e)
            {
            }
          }
        } else if (token->first == VARIABLE)
        {
          if (!json.contains(token->second))
          {
            std::cerr << colorize(RED) << "[TEMPLATE] Cannot find the \"" << token->second << "\"!" << colorize(NC) << "\n";
            return std::nullopt;
          }

          expression += stringifyJson(json.at(token->second));
        }
        else if (token->first == MATH)
        {
          expression += token->second;
        }
        else if (token->first == OPERATOR)
        {
          expression += token->second;
        }
      }

      bool is_ok = true;
      double r = calculate(expression, &is_ok);
      if (!is_ok)
      {
        return expression;
      }

      std::stringstream ss;
      ss << r;
      result = ss.str();
    }

    return result;
  }  

  static inline bool isOperator(const char c)
  {
    return c == '*' || c == '-' || c == '+' || c == '/';
  }

  static inline bool isComparative(const char c)
  {
    return c == '=' || c == '!' || c == '>' || c == '<';
  }

  //false on an error
  static bool lexer_analyze(std::string& code, const nlohmann::json& json)
  {
    std::vector<std::pair<TOKEN_TYPE, std::string>> tokens;

    for (int i=0;i<code.size();++i)
    {

      if (i+1 < code.size() && code[i] == '{' && code[i+1] == '%') //statement
      {
        std::size_t start = i;
        std::size_t end = code.find("%}", start);
        std::string op = code.substr(start+2, end-start-2);
        if (trim(op) == "raw")
        {
          std::size_t pos = end+2;
          std::size_t pos2 = pos;

          int cnt = 0;
          while (true)
          {
            pos = code.find("{%", pos);
            if (pos == std::string::npos || pos >= code.size())
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find {% endraw %}!" << colorize(NC) << "\n";
              return false;
            }

            pos2 = code.find("%}", pos);
            if (pos2 == std::string::npos || pos >= code.size())
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find {% endraw %}!" << colorize(NC) << "\n";
              return false;
            }

            std::string found = trim(code.substr(pos+2, pos2-pos-2));

            if (found == "raw")
            {
              cnt++;
              pos++;
              continue;
            } else if (found == "endraw")
            { 
              if (cnt == 0)
              {
                break;
              } else {
                cnt--;
                pos++;
                continue;
              }
            } else {
              pos++;
              continue;
            }
          } //end while

          i += pos2+2 - end-2;
          code.replace(pos, pos2-pos+2, "");
          code.replace(start, end-start+2, "");
        } else {
          //NOT RAW

          i += end - start;

          std::string tmp; //first operator

          std::string cond = "";
          {
            std::string trimmed = trim(op);
            std::size_t pos = trimmed.find(" ");
            if (pos == std::string::npos)
            {
              tmp = "";
            } else {
              tmp = trimmed.substr(0, pos);
              cond = trim(trimmed.substr(pos+1));
            }
          }

          //parsing statement
          if (tmp == "if")
          {
            tokens.emplace_back(IF, "if");

            {
              int j = 0;
              std::string tmp = "";
              while (true)
              {
                if (j >= cond.size())
                {
                  char last = *tmp.rbegin();
                  if (isalpha(last) || last == '_')
                  {
                    tokens.emplace_back(VARIABLE, trim(tmp));
                  } else if (isdigit(last))
                  {
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (last == '.') // format like 3. = 3.0
                  {
                    tmp += "0";
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (isOperator(last))
                  {
                    tokens.emplace_back(OPERATOR, trim(tmp));
                  } else if (isComparative(last))
                  {
                    tokens.emplace_back(COMPARISON_OPERATOR, trim(tmp));
                  }
                  tmp = "";

                  break;
                }

                if (cond[j] == '"')
                {
                  char last = *tmp.rbegin();
                  if (isalpha(last) || last == '_')
                  {
                    tokens.emplace_back(VARIABLE, trim(tmp));
                  } else if (isdigit(last))
                  {
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (last == '.') // format like 3. = 3.0
                  {
                    tmp += "0";
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (isOperator(last))
                  {
                    tokens.emplace_back(OPERATOR, trim(tmp));
                  } else if (isComparative(last))
                  {
                    tokens.emplace_back(COMPARISON_OPERATOR, trim(tmp));
                  }
                  tmp = "";

                  std::size_t strStart = j;
                  std::size_t strEnd = cond.find('"', j+1);
                  if (strEnd == std::string::npos)
                  {
                    std::cerr << colorize(RED) << "[TEMPLATE] Error! Failed to parse string! Cannot find string end!" << colorize(NC) << "\n";
                    return false;
                  }

                  std::string str = cond.substr(strStart+1, strEnd-strStart-1);
                  tokens.emplace_back(STRING, str);
                  j = strEnd+1; //strEnd+1 - 1
                  continue;
                }

                if (tmp == "")
                {
                  tmp += cond[j];
                }
                else if ( (isalpha(*tmp.rbegin()) || *tmp.rbegin() == '_') && !(isalpha(cond[j]) || cond[j] == '_'))
                {
                  tokens.emplace_back(VARIABLE, trim(tmp));
                  tmp = cond[j];
                } else if ( (isdigit(*tmp.rbegin()) || *tmp.rbegin() == '.' || *tmp.rbegin() == '-') && !(isdigit(cond[j]) || cond[j] == '.' || cond[j] == '-'))
                {
                  tokens.emplace_back(MATH, trim(tmp));
                  tmp = cond[j];
                } else if (isOperator(*tmp.rbegin()) && !isOperator(cond[j]))
                {
                  tokens.emplace_back(OPERATOR, trim(tmp));
                  tmp = cond[j];
                } else if (isComparative(*tmp.rbegin()) && !isComparative(cond[j]))
                {
                  tokens.emplace_back(COMPARISON_OPERATOR, trim(tmp));
                  tmp = cond[j];
                } else {
                  tmp += cond[j];
                }

                j++;
              } //end while
            }

            std::size_t else_end = 0;
            std::size_t endif_end = 0;
            int cnt = 0;
            while (true)
            {

              if (i+1 == code.size())
              {
                break; //EOF
              }

              std::size_t pos1 = code.find("{%", i);
              if (pos1 == std::string::npos || i >= code.size())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find ENDIF!" << colorize(NC) << "\n";
                return false;
              }

              std::size_t pos2 = code.find("%}", pos1+2);
              if (pos1 == std::string::npos || i >= code.size())
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find ENDIF!" << colorize(NC) << "\n";
                return false;
              }

              i += pos2-pos1;

              std::string found = code.substr(pos1+2, pos2-pos1-2);
              std::string oper = "";
              {
                std::stringstream ss;
                ss << trim(found);
                std::getline(ss, oper, ' '); 
                oper = trim(oper);
              }

              if (oper == "if")
              {
                cnt++;
              } else if (oper == "endif")
              {
                if (cnt != 0)
                {
                  cnt--;
                } else {
                  if (else_end != 0)
                  {
                    std::string body = code.substr(else_end, pos1-else_end);
                    tokens.emplace_back(ELSE_BODY, trim(body));
                    tokens.emplace_back(ENDIF, "endif");
                    endif_end = pos2+2;
                    break;
                  } else {
                    std::string body = code.substr(end+2, pos1-end-2);
                    tokens.emplace_back(IF_BODY, trim(body));
                    tokens.emplace_back(ENDIF, "endif");
                    endif_end = pos2+2;
                    break;
                  }
                }
              } else if (oper == "else" && cnt == 0)
              {
                std::string body = code.substr(end+2, pos1-end-2);
                else_end = pos2+2;
                tokens.emplace_back(IF_BODY, trim(body));
                i = else_end;
              }

              i++;
            }

            auto res = parser_eval(tokens, json);
            if (!res)
            {
              return false;
            }
            std::string result = *res;
            tokens.clear();
            code.replace(start, endif_end-start, result);
            i = -1; //to the beginning
            continue;

          } else if (tmp == "for")
          {

            tokens.emplace_back(FOR, "for");

            //FOR
            //parse statement
            {
              bool variablesProcessed = false;
              bool argsProcessing = false;
              int j = 0;
              std::string tmp = "";
              while (true)
              {
                if (j >= cond.size())
                {
                  char last = *tmp.rbegin();
                  if (isalpha(last) || last == '_' || last == ')' || last == '(')
                  {
                    if (!variablesProcessed)
                    {
                      if (trim(tmp) == "in")
                      {
                        tokens.emplace_back(KEYWORD, "in");
                        variablesProcessed = true;
                      }
                      else
                        tokens.emplace_back(VARIABLE, trim(tmp));
                    } else {
                      std::string iterator = trim(tmp);
                      {
                        std::size_t pos = iterator.find("(");
                        bool found = pos != std::string::npos;
                        std::size_t pos2 = iterator.find(")");
                        if (!found && pos2 != std::string::npos)
                        {
                          std::cerr << colorize(RED) << "[TEMPLATE] Error! Argument list must be started with '('!\n" << "  " << iterator << colorize(NC) << "\n";
                          return false;
                        }

                        if (found)
                        {
                          tokens.emplace_back(ITERATOR, iterator.substr(0, pos));
                          auto args = split(iterator.substr(pos+1, pos2-pos-1), ",");
                          for (auto ite: args)
                          {
                            if (!trim(ite).empty())
                              tokens.emplace_back(ARGUMENT, trim(ite));
                          }
                        } else {
                          tokens.emplace_back(VARIABLE, iterator);
                        }
                      }
                    }
                  } else if (isdigit(last))
                  {
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (last == '.') // format like 3. = 3.0
                  {
                    tmp += "0";
                    tokens.emplace_back(MATH, trim(tmp));
                  } else if (isOperator(last))
                  {
                    tokens.emplace_back(OPERATOR, trim(tmp));
                  } else if (isComparative(last))
                  {
                    tokens.emplace_back(COMPARISON_OPERATOR, trim(tmp));
                  }
                  tmp = "";

                  break;
                }

                if (!variablesProcessed)
                {
                  if (tmp == "")
                  {
                    tmp += cond[j];
                  } else if (cond[j] == ',')
                  {
                    //Must be empty to not include commas in variable names 
                  } else if ( (isalpha(*tmp.rbegin()) || *tmp.rbegin() == '_') && !(isalpha(cond[j]) || cond[j] == '_'))
                  {
                    if (trim(tmp) == "in")
                    {
                      tokens.emplace_back(KEYWORD, "in");
                      variablesProcessed = true;
                    } else
                      tokens.emplace_back(VARIABLE, trim(tmp));
                    tmp = cond[j];
                  } else if ( (isdigit(*tmp.rbegin()) || *tmp.rbegin() == '.' || *tmp.rbegin() == '-') && !(isdigit(cond[j]) || cond[j] == '.' || cond[j] == '-'))
                  {
                    tokens.emplace_back(MATH, trim(tmp));
                    tmp = cond[j];
                  } else if (isOperator(*tmp.rbegin()) && !isOperator(cond[j]))
                  {
                    tokens.emplace_back(OPERATOR, trim(tmp));
                    tmp = cond[j];
                  } else if (isComparative(*tmp.rbegin()) && !isComparative(cond[j]))
                  {
                    tokens.emplace_back(COMPARISON_OPERATOR, trim(tmp));
                    tmp = cond[j];
                  } else {
                    tmp += cond[j];
                  }
                } else {
                  //parse iterator

                  if (tmp == "")
                  {
                    tmp += cond[j];
                  } else if ( ((isalpha(*tmp.rbegin()) || *tmp.rbegin() == '_' || *tmp.rbegin() == ')' || *tmp.rbegin() == '(') && 
                      !(isalpha(cond[j]) || cond[j] == '_' || cond[j] == ')' || cond[j] == '(' || cond[j] == ' ' || cond[j] == ',' || cond[j] == '.')))
                  {
                    std::string iterator = trim(tmp);
                    {
                      std::size_t pos = iterator.find("(");
                      bool found = pos != std::string::npos;
                      std::size_t pos2 = iterator.find(")");
                      if (!found && pos2 != std::string::npos)
                      {
                        std::cerr << colorize(RED) << "[TEMPLATE] Error! Argument list must be started with '('!\n" << "  " << iterator << colorize(NC) << "\n";
                        return false;
                      }

                      if (found)
                      {
                        tokens.emplace_back(ITERATOR, iterator.substr(0, pos));
                        auto args = split(iterator.substr(pos+1, pos2-pos-1), ",");
                        for (auto ite: args)
                        {
                          tokens.emplace_back(ARGUMENT, trim(ite));
                        }
                      } else {
                        tokens.emplace_back(VARIABLE, iterator);
                      }
                    }

                    //tmp = cond[j];
                  } else {
                    tmp += cond[j];
                  }
                }

                j++;
              } //end while

              //find body
              std::size_t endfor_end = 0;
              int cnt = 0;
              while (true)
              {

                if (i+1 == code.size())
                {
                  break; //EOF
                }

                std::size_t pos1 = code.find("{%", i);
                if (pos1 == std::string::npos || i >= code.size())
                {
                  std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find ENDFOR!" << colorize(NC) << "\n";
                  return false;
                }

                std::size_t pos2 = code.find("%}", pos1+2);
                if (pos1 == std::string::npos || i >= code.size())
                {
                  std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find ENDFOR!" << colorize(NC) << "\n";
                  return false;
                }

                i += pos2-pos1;

                std::string found = code.substr(pos1+2, pos2-pos1-2);
                std::string oper = "";
                {
                  std::stringstream ss;
                  ss << trim(found);
                  std::getline(ss, oper, ' '); 
                  oper = trim(oper);
                }

                if (oper == "for")
                {
                  cnt++;
                } else if (oper == "endfor")
                {
                  if (cnt != 0)
                  {
                    cnt--;
                  } else {
                    std::string body = code.substr(end+2, pos1-end-2);
                    tokens.emplace_back(FOR_BODY, trim(body));
                    tokens.emplace_back(ENDFOR, "endfor");
                    endfor_end = pos2+2;
                    break;
                  }
                }

                i++;
              }

              bool is_ok = true;
              auto res = parser_eval(tokens, json, &is_ok);
              tokens.clear();
              if (!is_ok || !res)
                return false;

              std::string result = *res;
              code.replace(start, endfor_end-start, result);
              i = -1;
              continue;
            }
          } else if (tmp == "block" || trim(op) == "endblock")
          {
            code.replace(start, end-start+2, "");
            i = -1;
            continue;
          } else if (trim(op).substr(0, 9) == "loadblock")
          {
            std::string trimmed = trim(op);
            std::size_t bracketStart = trimmed.find_first_of("(");
            if (bracketStart == std::string::npos)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Invalid \"loadblock\" syntax!" << colorize(NC) << "\n";
              return false;
            }

            std::size_t bracketEnd = trimmed.find_first_of(")", bracketStart+1);
            if (bracketEnd == std::string::npos)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Invalid \"loadblock\" syntax!" << colorize(NC) << "\n";
              return false;
            }

            std::string substr = trimmed.substr(bracketStart+1, bracketEnd-bracketStart-1);
            bracketStart = substr.find_first_of("\""); //reuse variable
            if (bracketStart == std::string::npos)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Invalid \"loadblock\" syntax!" << colorize(NC) << "\n";
              return false;
            }

            bracketEnd = substr.find_first_of("\"", bracketStart+1); //reuse variable
            if (bracketEnd == std::string::npos)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Invalid \"loadblock\" syntax!" << colorize(NC) << "\n";
              return false;
            }

            std::string filename = substr.substr(bracketStart+1, bracketEnd-bracketStart-1);

            std::size_t nameStart = substr.find_first_of(",", bracketEnd+1);
            if (nameStart == std::string::npos)
            {
              std::cerr << colorize(RED) << "[TEMPLATE] Error! Invalid \"loadblock\" syntax!" << colorize(NC) << "\n";
              return false;
            }

            std::string name = trim(substr.substr(nameStart+1));

            std::string file = getFileString(filename);
            std::size_t blockStart = -2;
            std::size_t blockStartEnd = -2;
            while (blockStart != std::string::npos && blockStartEnd != std::string::npos)
            {
              blockStart = file.find("{%", blockStartEnd+2);
              if (blockStart == std::string::npos)
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << name << "\" in the file \"" << filename << "\"!" << colorize(NC) << "\n";
                return false;
              }

              blockStartEnd = file.find("%}", blockStart+2);
              if (blockStartEnd == std::string::npos)
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find the \"" << name << "\" in the file \"" << filename << "\"!" << colorize(NC) << "\n";
                return false;
              }

              std::string fnd = trim(file.substr(blockStart+2, blockStartEnd-blockStart-2));
              if (fnd.substr(0, 5) != "block")
                continue;

              if (trim(fnd.substr(6)) != name)
              {
                continue;
              } else {
                break;
              }
            }

            std::size_t blockEnd = blockStartEnd;
            std::size_t blockEndEnd = blockStartEnd;
            int cnt = 0;
            while (blockEnd != std::string::npos && blockEndEnd != std::string::npos)
            {
              blockEnd = file.find("{%", blockEndEnd+2);
              if (blockEnd == std::string::npos)
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find \"endblock\" of the block \"" << name << "\"!" << colorize(NC) << "\n";
                return false;
              }

              blockEndEnd = file.find("%}", blockEnd);
              if (blockEndEnd == std::string::npos)
              {
                std::cerr << colorize(RED) << "[TEMPLATE] Error! Cannot find \"endblock\" of the block \"" << name << "\"!" << colorize(NC) << "\n";
                return false;
              }

              std::string fnd = trim(file.substr(blockEnd+2, blockEndEnd-blockEnd-2));
              if (fnd == "endblock")
              {
                if (cnt == 0)
                {
                  break;
                } else {
                  cnt--;
                }
              } else if (fnd.substr(0, 5) == "block") {
                cnt++;
              } else {
                continue;
              }
            }

            std::string blockPart = trim(file.substr(blockStartEnd+2, blockEnd-blockStartEnd-2));
            code.replace(start, end-start+2, blockPart);
            i = -1;
            continue;

          } else {
            std::cerr << colorize(RED) << "[TEMPLATE] Unrecognized token \"" << trim(op) << "\"!" << colorize(NC) << "\n";
            return false;
          }
        }
      }

      if (i+1 < code.size() && code[i] == '{' && code[i+1] == '{')
      {
        i += 2; //on position after '{{'

        std::string tmp = "";

        std::size_t start = i-2;
        std::size_t end = i-2;
        while (true)
        {

          if (i+1 == code.size()) //end of file
          {
            break;
          }

          if (i+1 < code.size() && code[i] == '}' && code[i+1] == '}')
          {

            char first = *trim(tmp).begin();
            char last = *trim(tmp).rbegin();
            if (first == '.' && isdigit(*(tmp.begin()+1)))
            {
              //.0 -> 0.0
              tokens.emplace_back(MATH, "0" + trim(tmp));
            } else if (isalpha(first) || first == '_')
            {
              tokens.emplace_back(VARIABLE, trim(tmp));
            } else if (isdigit(first))
            {
              tokens.emplace_back(MATH, trim(tmp));
            } else if (isOperator(first))
            {
              tokens.emplace_back(OPERATOR, trim(tmp));
            }

            tmp = "";

            i += 2;

            end = i;
            auto res = parser_eval(tokens, json);
            if (!res)
            {
              return false;
            }
            const std::string result = *res;
            tokens.clear();
            code.replace(start, end-start, result);
            i = -1; //to the beginning

            break;
          }

          /*if (tmp == "")
          {
            tmp += code[i];
          }
          else if ( (isalpha(*tmp.rbegin()) || *tmp.rbegin() == '_') && (!(isalpha(code[i]) || code[i] == '_')))
          {
            tokens.emplace_back(VARIABLE, tmp);
            tmp = code[i];
          } else if ( (isdigit(*tmp.rbegin()) || *tmp.rbegin() == '.') && !(isdigit(code[i]) || code[i] == '.'))
          {
            tokens.emplace_back(MATH, tmp);
            tmp = code[i];
          } else if (isOperator(*tmp.rbegin()) && !isOperator(code[i]))
          {
            tokens.emplace_back(OPERATOR, tmp);
            tmp = code[i];
          } else {
            tmp += code[i];
          }*/

          char first = *tmp.begin();
          char last = *tmp.rbegin();
          if (tmp == "")
          {
            tmp += code[i];
          } else if (isOperator(last) && !isOperator(last))
          {
            tokens.emplace_back(OPERATOR, trim(tmp));
            tmp = code[i];
          } else if ( (isdigit(first) || first == '.') && !(isdigit(code[i]) || code[i] == '.'))
          {
            tokens.emplace_back(MATH, trim(tmp));
            tmp = code[i];
          } else if ( (isalpha(first) || first == '_') && !(isalpha(code[i]) || isdigit(code[i]) || code[i] == '_'))
          {
            tokens.emplace_back(VARIABLE, tmp);
            tmp = code[i];
          } else {
            tmp += code[i];
          }

          i++;
        } //end while
      }
    }

    return true;
  }

  void HTMLTemplate::renderJSON(const nlohmann::json& json)
  {
    std::string reserve_copy = m_html;
    if (!lexer_analyze(reserve_copy, json))
    {
      std::cout << colorize(RED) << "[TEMPLATE] Rendering error detected! No changes have been made!" << colorize(NC) << "\n";
      m_responce = HTTP_500;
      return;
    }

    m_html = reserve_copy;
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
