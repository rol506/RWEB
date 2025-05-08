
project(RWEB)

add_executable(templateBlock
  test.cpp
)

target_link_libraries(templateBlock RWEB)

add_test(NAME templateBlock COMMAND templateBlock)
