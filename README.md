# HanDecoder Wrapper Library (libhandecoder)

A wrapper library that enables and forces the use of H264 hardware video decoding when using FFmpeg, specifically tailored for platforms that either support it natively or Allwinner/Rockchip SoCs.

## Overview

This library wraps a precompiled program with an FFmpeg implementation that provides the following benefits:

- Simplified integration with pre-compiled programs that use FFmpeg for H264 software video decoding.
- Hardware-accelerated H264 video decoding on platforms that support V4L2 M2M natively.
- Integration of Allwinner SoC support with the cedar video engine via libva-v4l2-request.
- Integration of Rockchip SoC support with the rkmpp video engine via rkmpp.

## Device Support

Currently intended for use with:
- Linux-based systems with V4L2 M2M support.
- Allwinner SoCs with the cedar video engine.
- Rockchip SoCs with the rkmpp video engine.

Currently tested with:
- None yet...

## Dependencies

Ubuntu/Debian:
```bash
sudo apt-get install build-essential meson ninja-build cmake libva-dev libdrm-dev
```

It is also highly recommended to install the linux kernel headers for your target system.

## Building

1. Clone the repository and its submodules:

```bash
git clone --recursive https://github.com/yourusername/handecoder.git
cd handecoder
```
2. Setup the meson build system:

    2a. For native builds:
```bash
meson setup [build_directory]
```

    2b. For cross-compile builds:
```bash
meson setup [build_directory] --cross-file=[path_to_cross_file] (optional: -Dsys-root=[path_to_sysroot])
```

3. Build with ninja:

```bash
ninja -C [build_directory]
```

The resulting libraries will be in the specified build directory under `[build_directory]/lib`.

## Usage

This wrapper library is intended to be used with LD_PRELOAD to override the FFmpeg libraries being loaded and the related function calls. This is done by setting the LD_PRELOAD environment variable to the path of the wrapper library. For example:

```bash
export LD_PRELOAD=/path/to/libhandecoder.so
./your_program
```

## Licensing

This project uses multiple licenses:
- The wrapper code (this repository's code) is MIT licensed. See [LICENSE](LICENSE) file.
- The FFmpeg libraries (in `external/ffmpeg/`) are LGPL v2.1+ licensed. See [FFmpeg's license](https://github.com/FFmpeg/FFmpeg/blob/master/LICENSE.md) for details.
- The libva-v4l2-request library (in `external/libva-v4l2-request/`) is LGPL v2.1+ and MIT licensed. See [libva-v4l2-request's license](https://github.com/bootlin/libva-v4l2-request/blob/master/COPYING) for details.
- The Rockchip Media Process Platform (rkmpp) library (in `external/mpp/`) is Apache-2.0 and MIT licensed. See [rkmpp's license folder](https://github.com/rockchip-linux/mpp/blob/develop/LICENSES) for details.

**Important:** When using this library, you must comply with:
1. MIT license for the wrapper code
2. LGPL v2.1+ license for FFmpeg components
3. LGPL v2.1+ and MIT license for libva-v4l2-request
4. Apache-2.0 and MIT license for rkmpp

## Acknowledgments

- The [FFmpeg project](https://www.ffmpeg.org/) and its contributors
- [@bootlin](https://github.com/bootlin) for their work on libva-v4l2-request
- [@jernejsk](https://github.com/jernejsk) for their patches regarding v4l2-request usage in FFmpeg
- [@rockchip-linux](https://github.com/rockchip-linux) for their work on the Rockchip Media Process Platform (rkmpp)
- Those with the patience to wait for this project's continued development!