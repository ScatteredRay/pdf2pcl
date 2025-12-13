cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW) # enable MSVC_RUNTIME_LIBRARY

set(VCPKG_TARGET_TRIPLET x64-windows-static)

set(CMAKE_CXX_STANDARD 20)

project(pdf2pcl)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED IMPORTED_TARGET poppler)
pkg_check_modules(POPPLER_GLIB REQUIRED IMPORTED_TARGET poppler-glib)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

add_executable(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE /Gd)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_sources(${PROJECT_NAME} PRIVATE pdf2pcl.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  PkgConfig::POPPLER
  PkgConfig::POPPLER_GLIB
  PkgConfig::POPPLER_CPP
  PkgConfig::GLIB
  PkgConfig::GIO
)
