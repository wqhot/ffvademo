cmake_minimum_required(VERSION 3.5)
if (PLATFORM STREQUAL "ARM")
include(cross.cmake)
set(CMAKE_CROSSCOMPILING on)
else ()
SET(CMAKE_FIND_ROOT_PATH /usr)
endif ()
project(fastplayer_va)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -pthread -fopenmp -fPIC -s -Wl,-Bsymbolic")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -pthread -fopenmp -fPIC -s -Wl,-Bsymbolic")
else(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -g -Wall -pthread -fopenmp -pg -fPIC -Wl,-Bsymbolic")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall -pthread -fopenmp -pg -fPIC -Wl,-Bsymbolic")
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# set(ENV{PKG_CONFIG_PATH} /usr/lib/pkgconfig)
#find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Threads)

link_directories(${CMAKE_FIND_ROOT_PATH}/lib)

if (PLATFORM STREQUAL "ARM")
    link_directories("${CMAKE_SOURCE_DIR}/ext/ffmpeg_aarch64/lib")
    include_directories("${CMAKE_SOURCE_DIR}/ext/ffmpeg_aarch64/include")
    link_directories("${CMAKE_SOURCE_DIR}/ext/openssl_aarch64/lib")
    include_directories("${CMAKE_SOURCE_DIR}/ext/openssl_aarch64/include")
    link_directories("${CMAKE_SOURCE_DIR}/ext/gl_aarch64/lib")
endif ()

add_definitions(-DUSE_EGL)
add_definitions(-DUSE_X11)
add_definitions(-DUSE_DRM)
add_definitions(-DUSE_VA_X11)
add_definitions(-DUSE_VA_VPP)
# add_definitions(-DUSE_GLES_VERSION=1)

add_subdirectory(ext/live)

set(live555Includes
${live555_SOURCE_DIR}/BasicUsageEnvironment/include
${live555_SOURCE_DIR}/groupsock/include
${live555_SOURCE_DIR}/liveMedia/include
${live555_SOURCE_DIR}/UsageEnvironment/include
)


include_directories($ENV{INCLUDE_PATH})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpegInclude)
include_directories(
    ${CMAKE_FIND_ROOT_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${live555Includes}
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    ${CMAKE_FIND_ROOT_PATH}/include/freetype2
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_FIND_ROOT_PATH}/include/ffmpeg
)


file(GLOB Souce_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vafastplayer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffmpeg_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvadecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvadisplay.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvafilter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvarenderer_drm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvarenderer_egl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvarenderer_x11.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvarenderer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvasurface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvadecoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vaapi_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffvartsp.cpp
    )

message(STATUS "os is ${CMAKE_SYSTEM_NAME}")

set(target_name fastplayer_va)
add_library(${target_name} SHARED ${Souce_files})
# add_library(${target_name}  SHARED 
#          ${Souce_files})


target_link_libraries(${target_name}
#    ffvadecoder
   va
   avformat
   avfilter
   avcodec
   swscale
   swresample
   avutil
   va-x11
   X11
   EGL
   GLESv2
   liveMedia
   BasicUsageEnvironment
   groupsock
   UsageEnvironment
   ssl
   crypto
   freetype
)

if (PLATFORM STREQUAL "ARM")
target_link_libraries(${target_name}
${CMAKE_SOURCE_DIR}/ext/ffmpeg_aarch64/lib/libc.so.6
)
endif ()


target_compile_definitions(
	${target_name}
	PRIVATE
)

add_subdirectory(example)

SET(MAJOR_VERSION 1)
SET(MINOR_VERSION 0)
SET(PATCH_VERSION 0)

#说明要生成的是deb包
if (PLATFORM STREQUAL "ARM")
SET(CPACK_GENERATOR "DEB;RPM")
else()
SET(CPACK_GENERATOR "DEB")
endif()

##############设置debian/control文件中的内容###############

#设置版本信息
SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")

#设置安装包的包名，打好的包将会是packagename-version-linux.deb，如果不设置，默认是工程名
set(CPACK_PACKAGE_NAME "fastplayer_va")

#设置程序名，就是程序安装后的名字
set(CPACK_DEBIAN_PACKAGE_NAME "fastplayer_va")

#设置架构
if (PLATFORM STREQUAL "ARM")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    if (CPACK_GENERATOR MATCHES "DEB")
        set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/aarch64-linux-gnu)
    elseif (CPACK_GENERATOR MATCHES "RPM")
        set(CMAKE_INSTALL_LIBDIR /lib64)
    endif()
else ()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    if (CPACK_GENERATOR MATCHES "DEB")
        set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/x86_64-linux-gnu)
    elseif (CPACK_GENERATOR MATCHES "RPM")
        set(CMAKE_INSTALL_LIBDIR /lib64)
    endif()
endif ()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

#设置依赖
if (PLATFORM STREQUAL "ARM")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libx11-6, libavutil56, libavdevice58, libavcodec58, libavformat58, libxdmcp6, libbsd0")
else ()
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libx11-6, libavutil56, libavdevice58, libavcodec58, libavformat58, libxdmcp6, libbsd0")
endif ()


#设置description
SET(CPACK_PACKAGE_DESCRIPTION "fastplayer_va")

#设置联系方式
SET(CPACK_PACKAGE_CONTACT "wqhot@outlook.com")

#设置维护人
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "wq")

include(CPack)

SET(LIBS_DIR ${CMAKE_BINARY_DIR})
SET(BINARY_DIR ${CMAKE_BINARY_DIR}/example)
SET(INC_DIR ${CMAKE_SOURCE_DIR}/inc)

# 安装二进制可执行文件
install(TARGETS zntk_ar
        RUNTIME DESTINATION bin PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

# 安装共享库文件
install(TARGETS fastplayer_va
        LIBRARY DESTINATION lib)

# 安装头文件
install(FILES ${INC_DIR}/vafastplayer.h ${INC_DIR}/vafastplayer.h
        DESTINATION include)

# 安装资源文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/example/res/
        DESTINATION share/fastplayer)