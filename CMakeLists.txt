cmake_minimum_required(VERSION 3.2)

project(QGLViewer)

find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED)


SET(SOURCES
  camera.cpp
  camera.h
  gldata.cpp
  gldata.h
  qglviewer.cpp
  qglviewer.h

  main.cpp
)


add_library(${PROJECT_NAME} ${SOURCES})


target_link_libraries(${PROJECT_NAME}
  Qt5::Core
  Qt5::Gui
  Qt5::OpenGL
  Qt5::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
    AUTOMOC ON
)

target_compile_features(${PROJECT_NAME} PUBLIC
  cxx_nonstatic_member_init
)
