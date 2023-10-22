cmake_minimum_required(VERSION 3.12)

set(VCPKG_TARGET_TRIPLET x64-windows)

project(pdf2pcl)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED IMPORTED_TARGET poppler)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE pdf2pcl.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${POPPLER_INCLUDE_DIRS}
  ${POPPLER_CPP_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${POPPLER_LIBRARIES}
  ${POPPLER_CPP_LIBRARIES}
  ${GLIB_LIBRARIES}
)

get_target_property(PROJ_INCLUDES ${PROJECT_NAME} INCLUDE_DIRECTORIES)


