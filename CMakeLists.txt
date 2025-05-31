cmake_minimum_required (VERSION 3.5)
project(handecoder)

include(ExternalProject)

set(RKMPP_INSTALL_DIR "${CMAKE_BINARY_DIR}/rkmpp-install")
ExternalProject_Add(
    rkmpp
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/mpp"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${RKMPP_INSTALL_DIR} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${RKMPP_INSTALL_DIR}
)

set(CEDRUS_INSTALL_DIR "${CMAKE_BINARY_DIR}/cedrus-install")
ExternalProject_Add(
    cedrus
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/libcedrus-type-c"
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make clean && CC=aarch64-linux-gnu-gcc make
    INSTALL_COMMAND make install prefix=${CEDRUS_INSTALL_DIR}
)

set(FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg-install")
ExternalProject_Add(
    ffmpeg
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/ffmpeg-type-c"
    CONFIGURE_COMMAND env PKG_CONFIG_PATH=${RKMPP_INSTALL_DIR}/lib/pkgconfig:/usr/lib/aarch64-linux-gnu/pkgconfig 
    ${CMAKE_SOURCE_DIR}/external/ffmpeg-type-c/configure --arch=aarch64 --target-os=linux --pkg-config=/usr/bin/pkg-config --prefix=${FFMPEG_INSTALL_DIR}
    --enable-shared --disable-static --disable-doc --disable-programs --disable-avformat --disable-avdevice --disable-swscale --disable-swresample --disable-postproc
    --disable-avfilter --disable-network --disable-everything --enable-libudev --enable-libdrm --enable-v4l2-m2m --enable-v4l2-request --enable-rkmpp --enable-version3
    --enable-decoder=h264,h264_v4l2m2m,h264_rkmpp,h264_cedar --enable-hwaccel=h264_v4l2request --extra-cflags=-I${CEDRUS_INSTALL_DIR}/include
    --extra-ldflags=-L${CEDRUS_INSTALL_DIR}/lib --extra-libs=-lcedrus --enable-cross-compile --cross-prefix=aarch64-linux-gnu-
    BUILD_COMMAND make
    DEPENDS rkmpp cedrus
)

set(HDCD_SOURCES
    src/hdcd-init.c
    src/hdcd-hook.c
    src/hdcd-buffer.c
)

add_library(ffmpeglibs SHARED IMPORTED GLOBAL)
set_target_properties(ffmpeglibs PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavcodec.so
    IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavutil.so
)

add_library(handecoder SHARED ${HDCD_SOURCES})
add_dependencies(handecoder ffmpeg)
target_include_directories(handecoder PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${FFMPEG_INSTALL_DIR}/include
)
target_link_libraries(handecoder PRIVATE ffmpeglibs)

install(TARGETS handecoder LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib)
install(FILES ${FFMPEG_INSTALL_DIR}/lib/libavcodec.so.59.19.101 DESTINATION ${CMAKE_BINARY_DIR}/lib RENAME libavcodec.so.59)
install(FILES ${FFMPEG_INSTALL_DIR}/lib/libavutil.so.57.39.100 DESTINATION ${CMAKE_BINARY_DIR}/lib RENAME libavutil.so.57)
install(FILES ${RKMPP_INSTALL_DIR}/lib/librockchip_mpp.so.0 DESTINATION ${CMAKE_BINARY_DIR}/lib RENAME librockchip_mpp.so.1)
install(FILES ${CEDRUS_INSTALL_DIR}/lib/libcedrus.so.1 DESTINATION ${CMAKE_BINARY_DIR}/lib)