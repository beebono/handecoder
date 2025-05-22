cmake_minimum_required (VERSION 3.5)
project(handecoder)

include(ExternalProject)

set(RKMPP_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/rkmpp-install")
ExternalProject_Add(
    rkmpp
    GIT_REPOSITORY https://github.com/rockchip-linux/mpp.git
    GIT_TAG develop
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${RKMPP_INSTALL_DIR} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${RKMPP_INSTALL_DIR}
)

ExternalProject_Add(
    ffmpeg
    GIT_REPOSITORY https://github.com/beebono/ffmpeg-type-c.git
    GIT_TAG custom
    PATCH_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg/configure --arch=aarch64 --target-os=linux --cross-prefix=aarch64-linux-gnu-
    --prefix=${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-install --enable-shared --disable-static --disable-doc --disable-programs --disable-avformat --disable-avdevice
    --disable-swscale --disable-swresample --disable-postproc --disable-avfilter --disable-network --disable-everything --enable-libudev --enable-libdrm
    --enable-v4l2-m2m --enable-v4l2-request --enable-rkmpp --enable-version3 --enable-decoder=h264,h264_v4l2m2m,h264_rkmpp --enable-hwaccel=h264_v4l2request
    #--sysroot=${CMAKE_SYSROOT} --enable-cross-compile
    BUILD_COMMAND make
    DEPENDS rkmpp
)

set(HDCD_SOURCES
    src/hdcd-init.c
    src/hdcd-hook.c
)

add_library(handecoder SHARED ${HDCD_SOURCES})

target_include_directories(handecoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-install/include
)

install(TARGETS handecoder LIBRARY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-install/lib/libavcodec.so.59.19.101 DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib RENAME libavcodec.so.59)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-install/lib/libavutil.so.57.39.100 DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib RENAME libavutil.so.57)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rkmpp-install/lib/librockchip_mpp.so.0 DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib RENAME librockchip_mpp.so.1)