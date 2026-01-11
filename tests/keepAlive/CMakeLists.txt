
project(RWEB)

add_executable(keepAliveTest
  server.cpp
  Socket.cpp
  Socket.h
)

target_link_libraries(keepAliveTest RWEB)

add_test(NAME keepAlive COMMAND keepAliveTest)
