cmake_minimum_required(VERSION 3.21)

project(QGLViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED)


SET(SOURCES
  camera.cpp
  camera.h
  gldata.cpp
  gldata.h
  qglviewer.cpp
  qglviewer.h
)


add_executable(QGLViewerExample main.cpp)
target_link_libraries(QGLViewerExample ${PROJECT_NAME})


add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
PUBLIC
  Qt5::Core
  Qt5::Gui
  Qt5::OpenGL
  Qt5::Widgets
)

target_include_directories(${PROJECT_NAME}
PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_features(${PROJECT_NAME}
PUBLIC
  cxx_nonstatic_member_init
)
