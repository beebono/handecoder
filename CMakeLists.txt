cmake_minimum_required (VERSION 3.5)
project(handecoder)

include(ExternalProject)

set(RKMPP_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/rkmpp-install")
ExternalProject_Add(
    rkmpp
    GIT_REPOSITORY https://github.com/rockchip-linux/mpp.git
    GIT_TAG develop
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${RKMPP_INSTALL_DIR}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${RKMPP_INSTALL_DIR}
)

ExternalProject_Add(
    ffmpeg
    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
    GIT_TAG n7.1
    PATCH_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch-ffmpeg.sh ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg
    CONFIGURE_COMMAND env PKG_CONFIG_LIBDIR=${RKMPP_INSTALL_DIR}/lib/pkgconfig:${CMAKE_PKG_CONFIG_PC_LIB_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg/configure
    --arch=aarch64 --target-os=linux --cross-prefix=aarch64-linux-gnu- --prefix=${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-install --enable-shared --disable-static --disable-doc
    --disable-programs --disable-avformat --disable-avdevice --disable-swscale --disable-swresample --disable-postproc --disable-avfilter --disable-network --disable-everything
    --enable-libudev --enable-libdrm --enable-v4l2-m2m --enable-v4l2-request --enable-rkmpp --enable-version3 --enable-decoder=h264 --enable-decoder=h264_v4l2m2m
    --enable-decoder=h264_v4l2request --enable-decoder=h264_rkmpp --enable-hwaccel=h264_v4l2m2m --enable-hwaccel=h264_v4l2request --enable-hwaccel=h264_rkmpp
    DEPENDS rkmpp
)

set(HDCD_SOURCES
    src/hdcd-init.c
    src/hdcd-hook.c
    src/ffmpeg-load.c
)

add_library(handecoder ${HDCD_SOURCES})

target_include_directories(handecoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/ffmpeg
)

install(TARGETS handecoder
    LIBRARY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib
)