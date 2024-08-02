cmake_minimum_required(VERSION 3.5)
project(TextEditorInC)
set(CMAKE_CXX_STANDARD 11) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# Define the source files

set(main_SOURCES main.c)

# Create the executable targets
add_executable(main.o ${main_SOURCES})
