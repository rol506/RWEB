#include <RWEB.h>

#include <iostream>
#include <chrono>

bool test(const std::string& expr, const double result)
{
  auto start = std::chrono::high_resolution_clock::now();
  auto res = rweb::calculate(expr);
  if (static_cast<int>(res) != result)
  {
    std::cout << "[RESULT] " << "" << expr << " = " << rweb::colorize(rweb::RED) <<  res << rweb::colorize(rweb::NC) << "\n";
    std::cout << rweb::colorize(rweb::RED) << "TEST FAILED!" << rweb::colorize(rweb::NC) << "\n";
    return false;
  }
  auto dur = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
  std::cout << "[RESULT] " << "" << expr << " = " << rweb::colorize(rweb::GREEN) <<  res << rweb::colorize(rweb::NC) << " [" << dur << "ms]\n";
  return true;
}

int main()
{
  rweb::init(false);

  if (!test("-1", -1))
    return -1;

  if (!test("3 ** 3 * 3 / 3", 27))
    return -1;

  if (!test("((1 + 2)) * (2 + 1) * 3 ** 9", 177147))
    return -1;

  if (!test("", 0))
    return -1;

  if (!test("ladno * (1 ** 22 + 21)", 0)) //error returns 0
    return -1;

  if (!test("-1", -1))
    return -1;

  return 0;
}
