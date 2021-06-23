cmake_minimum_required(VERSION 3.10)

project(
    basicNesEmulator
    LANGUAGES C
)

# find SDL2
find_package(SDL2 REQUIRED)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(${PROJECT_NAME} main2.c ${SOURCES})

target_include_directories(
    ${PROJECT_NAME} 
        PRIVATE 
    "${PROJECT_SOURCE_DIR}/include" 
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})