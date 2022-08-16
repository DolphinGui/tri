cmake_minimum_required(VERSION 3.12)

project(tricolorasm CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include(FetchContent)

FetchContent_Declare(re2
GIT_REPOSITORY https://github.com/google/re2.git
GIT_TAG ba2e7c2f25047a6848f2baa9bc4cb700ea9dda84)
FetchContent_MakeAvailable(re2)

add_library(triasm ${CMAKE_SOURCE_DIR}/src/assembler.cpp)
target_include_directories(triasm 
  PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(triasm 
  PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(triasm PUBLIC cxx_std_20)
target_link_libraries(triasm PRIVATE re2::re2)

add_executable(asm-test ${CMAKE_SOURCE_DIR}/tests/asm-test.cpp)
target_link_libraries(asm-test PRIVATE triasm)