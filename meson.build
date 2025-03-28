project('handecoder', 'c', 'cpp',
    meson_version: '>= 0.53.2',
    license: 'MIT',
    default_options: ['buildtype=minsize'],
	version: '0.1')

kernel_h_path = '/usr/include'
kernel_src_path = run_command(['sh', '-c', 'ls -d /usr/src/linux-headers-*']).stdout().split()
if kernel_src_path.length() > 0
    foreach path : kernel_src_path
        kernel_h_path += ':' + path + '/include'
    endforeach
endif

rkmpp_conf_cmd = ('cd ' + meson.current_build_dir() + ' && mkdir -p mpp-build && cd mpp-build' +
    ' && cmake ' + meson.current_source_dir() + '/subprojects/mpp' +
    ' -DCMAKE_INSTALL_PREFIX=' + meson.current_build_dir() + '/mpp-install' +
    ' -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_TEST=OFF')

rkmpp_pkgconf_path = 'PKG_CONFIG_PATH=' + meson.current_build_dir() + '/mpp-install/lib/pkgconfig:$PKG_CONFIG_PATH'

ff_conf_cmd = ('mkdir -p ffmpeg-install && '+ rkmpp_pkgconf_path + ' ' + meson.current_source_dir() + '/subprojects/ffmpeg/configure' +
    ' --prefix=' + meson.current_build_dir() + '/ffmpeg-install --enable-shared --disable-static --disable-doc --disable-programs --disable-avformat' +
    ' --disable-avdevice --disable-swscale --disable-swresample --disable-postproc --disable-avfilter --disable-network --disable-everything' +
    ' --enable-libudev --enable-libdrm --enable-v4l2-m2m --enable-vaapi --enable-v4l2-request --enable-rkmpp --enable-version3' +
    ' --enable-decoder=h264 --enable-decoder=h264_v4l2m2m --enable-hwaccel=h264_vaapi --enable-decoder=h264_v4l2request' +
    ' --enable-decoder=h264_rkmpp --enable-hwaccel=h264_v4l2m2m --enable-hwaccel=h264_vaapi --enable-hwaccel=h264_v4l2request' +
    ' --enable-hwaccel=h264_rkmpp')

if meson.is_cross_build()
    c_compiler_path = find_program('c').path()
    cpp_compiler_path = find_program('cpp').path()
    
    rkmpp_conf_cmd += (' -DCMAKE_TOOLCHAIN_FILE=' + meson.current_build_dir() + '/toolchain.cmake')
    ff_conf_cmd += (' --enable-cross-compile' +
        ' --target-os=' + host_machine.system().to_lower() +
        ' --arch=' + host_machine.cpu_family() +
        ' --cross-prefix=' + c_compiler_path.strip('/usr/bin/').strip('gcc'))
    toolchain_cmd = [meson.current_source_dir() + '/scripts/gen-toolchain.sh',
        host_machine.system(), host_machine.cpu_family(), c_compiler_path, cpp_compiler_path]

    sys_root = get_option('sys-root')
    if sys_root != ''
        new_kernel_h_path = sys_root + '/usr/include'
        new_kernel_src_path = run_command(['sh', '-c', 'ls -d ' + sys_root + '/usr/src/linux-headers-*']).stdout().split()
        if new_kernel_src_path.length() > 0
            foreach path : new_kernel_src_path
                new_kernel_h_path += ':' + path + '/include'
            endforeach
        endif
        kernel_h_path = new_kernel_h_path
        ff_conf_cmd += ' --sysroot=' + sys_root
        toolchain_cmd += sys_root
    endif
    run_command(['sh', toolchain_cmd], check: true)
endif

rkmpp_conf_cmd += ' -DCMAKE_INCLUDE_PATH=' + kernel_h_path
ff_conf_cmd += ' --extra-cflags="-I'
foreach path : kernel_h_path.split(':')
    ff_conf_cmd += path + ' -I'
endforeach
ff_conf_cmd = ff_conf_cmd.strip(' -I') + '"'

libva_proj = subproject('libva-v4l2-request')
libva_drv = libva_proj.get_variable('v4l2_request_drv_video')

run_command(['sh', '-c', rkmpp_conf_cmd], check: true)
rkmpp_build = custom_target('build-rkmpp',
    output: 'librockchip_mpp.so',
    command: ['sh', '-c', 'mkdir -p mpp-install && cd mpp-build && make -j$(nproc) && make install'],
    build_by_default: true,
    install: false
)

ffmpeg_patch = custom_target('patch-ffmpeg',
    output: 'ffmpeg-patched',
    command: ['sh', meson.current_source_dir() + '/scripts/patch-ffmpeg.sh'],
    build_by_default: true,
    install: false
)

ffmpeg_conf = custom_target('config-ffmpeg',
    output: 'config.h',
    command: ['sh', '-c', ff_conf_cmd],
    depends: [ffmpeg_patch, rkmpp_build],
    build_by_default: true,
    install: false
)

ffmpeg_build = custom_target('build-ffmpeg',
    output: ['libavcodec.so', 'libavutil.so'],
    command: ['sh', '-c', 'make -j$(nproc) && make install'],
    depends: ffmpeg_conf,
    build_by_default: true,
    install: true,
    install_dir: 'lib'
)

ffmpeg_dep = declare_dependency(sources: ffmpeg_build)

library('handecoder',
    sources: ['src/hdcd-init.c', 'src/hdcd-hook.c', 'src/ffmpeg-load.c'],
    include_directories: include_directories('src/include'),
    dependencies: ffmpeg_dep,
    build_by_default: true,
    install: true,
    install_dir: 'lib'
)
