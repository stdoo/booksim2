cmake_minimum_required(VERSION 3.8)
project(booksim2)
INCLUDE_DIRECTORIES(src/ src/allocators src/arbiters src/networks src/power src/routers)
set(CMAKE_CXX_STANDARD 11)

#bison
find_package(BISON)
message(${BISON_FOUND})
message(${CMAKE_CURRENT_SOURCE_DIR})
BISON_TARGET(MyPaser ${CMAKE_CURRENT_SOURCE_DIR}/src/config.y ${CMAKE_CURRENT_SOURCE_DIR}/src/y.tab.c
            #COMPILE_FLAGS -y -d
            DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/y.tab.h)

#flex
find_package(FLEX)
message(${FLEX_FOUND})
FLEX_TARGET(MyScanner ${CMAKE_CURRENT_SOURCE_DIR}/src/config.l ${CMAKE_CURRENT_SOURCE_DIR}/src/lex.yy.c)

#dependence
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyPaser)

#source files
file(GLOB source_files "src/*.cpp" "src/*/*.cpp")
set(SOURCE_FILES
        ${source_files}
        )

add_executable(booksim2 ${SOURCE_FILES} ${BISON_MyPaser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS})