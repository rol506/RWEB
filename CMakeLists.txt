cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

#include(CTest)

project(RWEB)

#for clangd to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(RWEB

  include/RWEB.h
  include/Socket.h
  include/HTMLTemplate.h
  include/Utility.h

  include/nlohmann/json.hpp

  src/RWEB.cpp
  src/Socket.cpp
  src/HTMLTemplate.cpp
  src/Utility.cpp
)
target_include_directories(RWEB PUBLIC include)

# everything below can be deleted (it's for testing)
add_executable(app
  app.cpp
)

target_link_libraries(app RWEB)

#copy original /res directory to build path (build/res | build/Debug/res)
add_custom_command(TARGET app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:${PROJECT_NAME}>/res)

enable_testing()

add_subdirectory(tests/calc)
add_subdirectory(tests/templateVars)
add_subdirectory(tests/template_if)
add_subdirectory(tests/template)
add_subdirectory(tests/templateBlock)
