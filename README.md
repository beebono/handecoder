# HanDecoder Wrapper Library (libhandecoder)

A wrapper library that enables and forces the use of H264 hardware video decoding when using FFmpeg, specifically tailored for platforms that either support V4L2M2M natively or Allwinner/Rockchip SoCs. (Maybe more in the future!)

## Overview

This library wraps a precompiled program with an FFmpeg implementation that provides the following benefits:

- Simplified integration with certain pre-compiled programs that use FFmpeg for H264 software video decoding.
- Hardware-accelerated H264 video decoding on platforms that support V4L2 M2M natively.
- Integration of Rockchip SoC support with the MPP video engine via h264_rkmpp.
- Graceful fallback to software-based H264 video decoding, wrapping the frames in DRM_PRIME format for compatibility.

## Device Support

Currently intended for use with:
- Linux-based systems with V4L2 M2M support.
- Rockchip SoCs with the MPP video engine.
- Allwinner SoCs with the cedar video engine.

Currently tested and working with:
- Allwinner H700 devices running KNULLI

## Build dependencies

Ubuntu/Debian:
```bash
sudo apt-get install build-essential cmake libva-dev libdrm-dev libudev-dev
```

## Building

1. Clone the repository and its submodules:

```bash
git clone https://github.com/beebono/handecoder.git
cd handecoder
```

2. Run the CMake build system:

```bash
mkdir ./build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install
```

To Cross-Compile, add your CMake Toolchain file to the first cmake command. (-DCMAKE_TOOLCHAIN_FILE=[/path/to/your/toolchain.file])
    Also make sure to uncomment the the last line of CONFIGURE_COMMAND in CMakeLists.txt!

The resulting libraries will be in the specified build directory under `[build_directory]/lib`.

## Usage

This wrapper library is intended to be used with LD_PRELOAD to override the FFmpeg function calls. This is done by setting the LD_PRELOAD environment variable to the path of the wrapper library.
You are also REQUIRED to set the environment variable HDCD_RESOLUTION to your screen's resolution for proper compatibility. This can be automated with a shell script if you are so inclined.

```bash
export HDCD_RESOLUTION=1280x720
export LD_PRELOAD=/path/to/libhandecoder.so
./your_program
```
