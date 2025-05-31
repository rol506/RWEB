
project(RWEB)

add_executable(templateTestVars
  test.cpp
)

target_link_libraries(templateTestVars RWEB)

add_test(NAME templateTestVars COMMAND templateTestVars)
