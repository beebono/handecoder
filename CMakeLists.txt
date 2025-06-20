cmake_minimum_required(VERSION 3.10)
project(handecoder)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

add_subdirectory(external/libtwig EXCLUDE_FROM_ALL)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(OLD_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    set(OLD_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
    set(OLD_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    set(OLD_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    
    string(REPLACE "-O3" "-O1" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "-O3" "-O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-O2" "-O1" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-O2" "-O1" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    
    add_subdirectory(external/mpp EXCLUDE_FROM_ALL)
    
    set(CMAKE_C_FLAGS_RELEASE ${OLD_C_FLAGS_RELEASE})
    set(CMAKE_CXX_FLAGS_RELEASE ${OLD_CXX_FLAGS_RELEASE})
    set(CMAKE_C_FLAGS_RELWITHDEBINFO ${OLD_C_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${OLD_C_FLAGS_RELWITHDEBINFO})
else()
    add_subdirectory(external/mpp EXCLUDE_FROM_ALL)
endif()

set(BUILD_TEST OFF)

set(TWIG_BUILD_PREFIX "${CMAKE_BINARY_DIR}/external/libtwig")
set(TWIG_BUILD_LIBDIR "${CMAKE_BINARY_DIR}/external/libtwig")
set(TWIG_BUILD_INCLUDEDIR "${CMAKE_SOURCE_DIR}/external/libtwig/include")
set(TWIG_VERSION "1.0.0")

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/twig-build.pc.in"
    "${CMAKE_BINARY_DIR}/twig.pc"
    @ONLY
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/mpp-headers/rockchip")
file(COPY "${CMAKE_SOURCE_DIR}/external/mpp/inc/" 
     DESTINATION "${CMAKE_BINARY_DIR}/mpp-headers/rockchip/")

set(MPP_BUILD_PREFIX "${CMAKE_BINARY_DIR}/external/mpp")
set(MPP_BUILD_LIBDIR "${CMAKE_BINARY_DIR}/external/mpp/mpp")  
set(MPP_BUILD_INCLUDEDIR "${CMAKE_BINARY_DIR}/mpp-headers")
set(MPP_VERSION "1.3.8")

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/rockchip_mpp-build.pc.in"
    "${CMAKE_BINARY_DIR}/rockchip_mpp.pc"
    @ONLY
)

include(ExternalProject)
set(FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg-install")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FFMPEG_DEBUG_FLAGS "--enable-debug=3")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(FFMPEG_DEBUG_FLAGS "--enable-debug=1")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(FFMPEG_DEBUG_FLAGS "--disable-debug --enable-small")
else()
    set(FFMPEG_DEBUG_FLAGS "--disable-debug")
endif()

set(PKG_CONFIG_PATH_STR "${CMAKE_BINARY_DIR}:/usr/lib/aarch64-linux-gnu/pkgconfig")
ExternalProject_Add(
    ffmpeg
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/ffmpeg-type-c"
    CONFIGURE_COMMAND env PKG_CONFIG_PATH=${PKG_CONFIG_PATH_STR}
    ${CMAKE_SOURCE_DIR}/external/ffmpeg-type-c/configure 
        --arch=aarch64 --target-os=linux --pkg-config=/usr/bin/pkg-config
        --enable-cross-compile --cross-prefix=aarch64-linux-gnu-
        --prefix=${FFMPEG_INSTALL_DIR} ${FFMPEG_DEBUG_FLAGS}
        --enable-shared --disable-static --disable-doc --disable-programs 
        --disable-avformat --disable-avdevice --enable-swscale --disable-swresample 
        --disable-postproc --disable-avfilter --disable-network --disable-everything 
        --enable-libudev --enable-libdrm --enable-v4l2-m2m --enable-v4l2-request 
        --enable-rkmpp --enable-version3
        --enable-decoder=h264,h264_v4l2m2m,h264_rkmpp
        --enable-hwaccel=h264_v4l2request
    BUILD_COMMAND make -j$(nproc)
    DEPENDS twig rockchip_mpp
)

set(HDCD_SOURCES
    src/hdcd.c
    src/hdcd-buffer.c
    src/hdcd-export.c
    src/hdcd-hooks.c
)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavcodec.so
)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavutil.so
)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libswscale.so
)

add_library(handecoder SHARED ${HDCD_SOURCES})
add_dependencies(handecoder ffmpeg)

target_include_directories(handecoder PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${FFMPEG_INSTALL_DIR}/include
    ${TWIG_BUILD_INCLUDEDIR}
)

target_link_libraries(handecoder PRIVATE 
    avcodec 
    avutil 
    swscale
    twig
)

install(TARGETS handecoder
    LIBRARY DESTINATION lib
)

install(FILES 
    $<TARGET_FILE:twig>
    DESTINATION lib 
    RENAME libtwig.so.1
)

install(FILES 
    $<TARGET_FILE:rockchip_mpp>
    DESTINATION lib 
    RENAME librockchip_mpp.so.1
)

install(FILES 
    ${FFMPEG_INSTALL_DIR}/lib/libavcodec.so.59.19.101 
    DESTINATION lib 
    RENAME libavcodec.so.59
)
install(FILES 
    ${FFMPEG_INSTALL_DIR}/lib/libavutil.so.57.39.100 
    DESTINATION lib 
    RENAME libavutil.so.57
)
install(FILES 
    ${FFMPEG_INSTALL_DIR}/lib/libswscale.so.8.3.100 
    DESTINATION lib 
    RENAME libswscale.so.8
)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    install(CODE "
        file(GLOB SO_FILES \"\${CMAKE_INSTALL_PREFIX}/lib/*.so*\")
        foreach(so_file \${SO_FILES})
            execute_process(COMMAND aarch64-linux-gnu-strip --strip-unneeded \${so_file})
        endforeach()
    ")
endif()