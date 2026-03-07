cmake_minimum_required(VERSION 3.10)
project(LibraryManagement C CXX)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard (Crow requires C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Find Threads (required by Crow)
find_package(Threads REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/Crow/include)

# C Source files (library core)
set(CORE_SOURCES
    src/book.c
    src/user.c
    src/borrow.c
    src/search.c
    src/error.c
    src/database.c
)

# ===== Console Application (C) =====
add_executable(library main.c ${CORE_SOURCES})
target_link_libraries(library SQLite::SQLite3)

# ===== Web Application (C++) =====
add_executable(library_web main_web.cpp ${CORE_SOURCES})

# Set C++ compilation for main_web.cpp only
set_source_files_properties(main_web.cpp PROPERTIES LANGUAGE CXX)

# Link libraries for web version
target_link_libraries(library_web 
    SQLite::SQLite3
    Threads::Threads
)

# Install targets
install(TARGETS library library_web DESTINATION bin)
