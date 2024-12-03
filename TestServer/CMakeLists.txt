cmake_minimum_required(VERSION 3.16)

project(TestServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

add_executable(TestServer
  main.cpp
  testserver.h testserver.cpp
  ../xml/1.1.1.1.xml ../xml/1.1.1.2.xml
  build/Desktop_Qt_6_8_1_MinGW_64_bit-Debug/server_data.db
)
target_link_libraries(TestServer PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

include(GNUInstallDirs)
install(TARGETS TestServer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
