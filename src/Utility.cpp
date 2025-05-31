#include "../include/Utility.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>

namespace rweb
{

  std::string trim(const std::string& str)
  {
    static const std::string ws = " \t\n";
    auto first = str.find_first_not_of(ws);
    auto last = str.find_last_not_of(ws);
    return first == std::string::npos ? "" : str.substr(first, last-first+1);
  }

  std::vector<std::string> split(const std::string& s, const std::string& seperator, int maxsplit)
  {
    int spt = maxsplit+1;
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    if (spt <= 0)
    {
      while ((pos = s.find_first_of(seperator, pos)) != std::string::npos)
      {
        std::string substring(trim(s.substr(prev_pos, pos-prev_pos)));
        if (substring.size() > 0)
        {
          output.push_back(substring);
        }
        prev_pos = ++pos;
      }
      output.push_back(trim(s.substr(prev_pos, pos-prev_pos))); //last word
      return output;
    } else {
      while ((pos = s.find_first_of(seperator, pos)) != std::string::npos && spt > 1)
      {
        std::string substring(trim(s.substr(prev_pos, pos-prev_pos)));
        if (substring.size() > 0)
        {
          output.push_back(substring);
        }
        prev_pos = ++pos;
        spt--;
      }
      output.push_back(trim(s.substr(prev_pos, s.size()-prev_pos)));
      return output;
    }
  }

  std::string replace(const std::string& s, const std::string& from, const std::string& to) noexcept
  {
    std::string cpy = s.substr();
    size_t pos = cpy.find(from);
    while (pos != std::string::npos)
    {
      cpy.replace(pos, from.size(), to);
      if (pos + from.size() >= cpy.size())
      {
        break;
      }

      pos = cpy.find(from, pos+from.size());
    }
    return cpy;
  }

  static bool isOperator(const char c) noexcept
  {
    return c == '/' || c == '*' || c == '+' || c == '-';
  }

  //int types:
  //1 - operator
  //2 - math
  //3 - '('
  //4 - ')'

  static const std::string typeToString(const int type) noexcept
  {
    switch(type){
      case 1:
        return "OPERATOR";
      case 2:
        return "MATH";
      case 3:
        return "OPEN";
      case 4:
        return "CLOSE";
    }

    return "UNKNOWN";
  }

  static float getOperatorValue(const std::string& oper) noexcept
  {
    if (oper == "+")
      return 1.1f;
    if (oper == "-")
      return 1.0f;
    if (oper == "*")
      return 2.1f;
    if (oper == "/")
      return 2.0f;
    if (oper == "**")
      return 3.1f;

    std::cerr << colorize(RED) << "[CALC] Error! Cannot get operator value: " << '"' << oper << '"' << colorize(NC) << "\n";
    return 0.0f;
  }

  static double process(const double left_operand, const double right_operand, const std::string& oper)
  {
    if (oper == "+")
      return left_operand + right_operand;
    if (oper == "-")
      return left_operand - right_operand;
    if (oper == "*")
      return left_operand * right_operand;
    if (oper == "/")
      return left_operand / right_operand;
    if (oper == "**")
      return pow(left_operand, right_operand);

    std::cerr << colorize(RED) << "[CALC] Failed to process operation! Unknown operator: " << '"' << oper << '"' << colorize(NC) << "\n";
    return 0;
  }

  static double calculate(std::vector<std::pair<short, std::string>>& tokens, bool* is_ok)
  {

    for (int i=0;i<tokens.size();++i)
    {
      if (tokens.size() == 1)
      {
        if (tokens[0].first != 2)
        {
          if (is_ok)
            *is_ok = false;
          else
            std::cerr << colorize(RED) << "[CALC] Error! Last token is not a math!" << colorize(NC) << "\n";
          return 0;
        }
        return std::stod(tokens.begin()->second);
      }

      if (tokens[i].first == 2)
        continue; 

      if (tokens[i].first == 1)
      {
        if (i == 0 || tokens[i-1].first != 2) //error
        {
          if (is_ok)
            *is_ok = false;
          else
            std::cerr << colorize(RED) << "[CALC] Error! Operator does not have left operand!" << colorize(NC) << "\n";

          return 0;
        }
        if (i == tokens.size()-1 || tokens[i+1].first != 2) //error
        {
          if (is_ok)
            *is_ok = false;
          else
            std::cerr << colorize(RED) << "[CALC] Error! Operator does not have a right operand!" << colorize(NC) << "\n";

          return 0;
        }

        if (i+2 < tokens.size() && getOperatorValue(tokens[i].second) < getOperatorValue(tokens[i+2].second))
        {
          i++;
          continue; //i += 2
        }

        double lv = stoi(tokens[i-1].second);
        double rv = stoi(tokens[i+1].second);
        double res = process(lv, rv, tokens[i].second);

        tokens[i].first = 2;
        std::stringstream ss;
        ss << res;
        tokens[i].second = ss.str();

        tokens.erase(tokens.begin() + i-1);
        tokens.erase(tokens.begin() + i);

        i = -1;
        continue;
      }
    }

    return 0;
  }

  double calculate(const std::string& expression, bool* is_ok)
  {
    if (trim(expression) == "")
    {
      std::cout << colorize(YELLOW) << "[MATH] WARNING! Calculating an empty expression! Result will be 0" << colorize(NC) << "\n";
      return 0;
    }

    std::string expr = expression;
    while (expr.find_first_of("()") != std::string::npos)
    {
      auto pos1 = expr.find_last_of("(");
      auto pos2 = expr.find_first_of(")", (pos1 == std::string::npos ? 0 : pos1));

      if (pos1 == std::string::npos)
      {
        std::cerr << "[CALC] ERROR FOUND UNOPENED BRACKET: " << pos2 << "\n";
        return 0;
      }

      if (pos2 == std::string::npos)
      {
        std::cerr << "[CALC] ERROR FOUND UNCLOSED BRACKET!\n";
        return 0;
      }

      std::string brackets = expr.substr(pos1+1, pos2-pos1-1);
      double res = calculate(brackets);
      expr.replace(pos1, pos2-pos1+1, std::to_string(res));
    }

    std::vector<std::pair<short, std::string>> tokens;
    std::string tmp = "";

    for (int i=0;i<=expr.size();++i)
    {
      char c = expr[i];
      if (c == ' ' || c == '\n')
        continue;

      if (!isdigit(c) && !isOperator(c) && (int)c != 0 && c != '\0' && c != '.')
      {
        if (is_ok)
          *is_ok = false;
        else
          std::cerr << colorize(RED) << "[MATH] Error! Unallowed character found in the math expression: " << expression << colorize(NC) << "\n";
        return 0;
      }

      if (tmp.empty() && tokens.size() == 0 && c == '-')
      {
        tmp = "";
        tokens.emplace_back(2, "0");
        tokens.emplace_back(1, "-");
      } else if (*tmp.rbegin() == '(')
      {
        tokens.emplace_back(3, "(");
        tmp = "";
      } else if (*tmp.rbegin() == ')')
      {
        tokens.emplace_back(4, ")");
        tmp = "";
      } else if ((isdigit(*tmp.rbegin()) || *tmp.rbegin() == '.') && !(isdigit(c) || c == '.'))
      {
        tokens.emplace_back(2, tmp);
        tmp = c;
      } else if (isOperator(*tmp.rbegin()) && !isOperator(c))
      {
        tokens.emplace_back(1, tmp);
        tmp = c;
      } else
        tmp += c;
    }

    return calculate(tokens, is_ok);
  }
}
