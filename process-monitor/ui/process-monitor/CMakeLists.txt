cmake_minimum_required(VERSION 3.5)

project(process-monitor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(process-monitor
  global.h
  main.cpp
  mainwindow.cpp
  mainwindow.h
  TaskInfo.h
  mainwindow.ui
  memdialog.cpp
  memdialog.h
  memdialog.ui
  killdialog.cpp
  killdialog.h
  killdialog.ui
  nicedialog.cpp
  nicedialog.h
  nicedialog.ui
  rundialog.cpp
  rundialog.h
  rundialog.ui
)

target_link_libraries(process-monitor PRIVATE Qt5::Widgets)
