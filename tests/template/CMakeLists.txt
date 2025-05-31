
project(RWEB)

add_executable(templateTest
  test.cpp
)

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

target_link_libraries(templateTest RWEB)

add_test(NAME templates COMMAND templateTest)
