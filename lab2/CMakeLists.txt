cmake_minimum_required(VERSION 3.14)

project(WolfAndGoats VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Werror)

find_library (LIBRT rt)
find_package (Threads)

file(GLOB CONNS conn/conn_*.cpp)
file(GLOB CLIENT client/*.cpp client/*.h)
file(GLOB HOST host/*.cpp host/*.h)
file(GLOB INTERFACE interface/*.h)

foreach(PATH_TO_CONN ${CONNS})
    get_filename_component(FILE_NAME ${PATH_TO_CONN} NAME_WLE )
    string(REGEX REPLACE "conn_" "" CONN_NAME ${FILE_NAME})
    
    add_executable(host_${CONN_NAME} ${HOST} ${CLIENT} ${INTERFACE}
    conn/conn_${CONN_NAME}.cpp)
       
    target_link_libraries(host_${CONN_NAME} PUBLIC pthread rt)
endforeach()