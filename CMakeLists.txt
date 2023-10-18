cmake_minimum_required(VERSION 3.15)

project(pdf2pcl)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE pdf2pcl.cpp)