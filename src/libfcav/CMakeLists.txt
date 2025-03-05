project(libfcav)

find_package(Threads)

file(GLOB FCAV_SRC_CPP *.cpp)
file(GLOB FCAV_SRC_C *.c)

add_library(libfcav STATIC ${FCAV_SRC_CPP} ${FCAV_SRC_C})

target_link_libraries(
    libfcav
    ${CMAKE_THREAD_LIBS_INIT}
)