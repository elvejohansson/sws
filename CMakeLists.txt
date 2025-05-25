cmake_minimum_required(VERSION 3.10)

project(sws C CXX)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES
    src/main.cpp
)

add_executable(sws ${SOURCES})

# set_source_files_properties(file.c PROPERTIES LANGUAGE C)
