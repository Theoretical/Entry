file(GLOB src *.cpp)

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  SET(CMAKE_CXX_FLAGS "-g -std=c++11 -pedantic")
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  SET(CMAKE_CXX_FLAGS "/EHsc") #So it doesn't constantly throw warnings
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  IF(WIN32)
    SET(CMAKE_CXX_FLAGS "/Qstd=c++11")
  ELSE()
    SET(CMAKE_CXX_FLAGS "-std=c++11")
  ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  SET(CMAKE_CXX_FLAGS "-std=c++11")
ENDIF()


include_directories(include ../dep/include ../dep/include/intlib)

add_executable(entry ${src})

target_link_libraries(entry enet intlib)