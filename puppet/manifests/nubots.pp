include apt

# http://www.puppetcookbook.com/posts/set-global-exec-path.html
Exec { path => [ "/bin/", "/sbin/" , "/usr/bin/", "/usr/sbin/" ] }

node default {

  # We need build tools to compile
  class {'build_tools': }

  # These user tools make the shell much easier
  class {'user_tools':
    user => 'vagrant',
  }

  # Get and install our toolchain
  $toolchain_version = '3.0.0'
  wget::fetch { 'nubots_deb':
    destination => "/root/nubots-toolchain-${toolchain_version}.deb",
    source      => "http://nubots.net/debs/nubots-toolchain-${toolchain_version}.deb",
    timeout     => 0,
  } ->
  package { 'nubots-toolchain':
    provider => 'dpkg',
    ensure   => 'latest',
    source   => "/root/nubots-toolchain-${toolchain_version}.deb",
  }
}

node nubotsvmbuild {
  $archs = {
    'native'    => {'flags'       => ['', ],
                    'params'      => ['-m64', ],
                    'environment' => {'TARGET' => 'GENERIC', 'USE_THREAD' => '1', 'BINARY' => '64', 'NUM_THREADS' => '2', 'AUDIO' => 'PORTAUDIO', 'LDFLAGS' => '-m64', 'PKG_CONFIG_PATH' => '/usr/lib/x86_64-linux-gnu/pkgconfig', 'CCAS' => 'gcc', 'CCASFLAGS' => '-m64', },
                   },
    'nuc7i7bnh' => {'flags'       => ['-m64', '-march=broadwell', '-mtune=broadwell', '-mmmx', '-mno-3dnow', '-msse', '-msse2', '-msse3', '-mssse3', '-mno-sse4a', '-mcx16', '-msahf', '-mmovbe', '-maes', '-mno-sha', '-mpclmul', '-mpopcnt', '-mabm', '-mno-lwp', '-mfma', '-mno-fma4', '-mno-xop', '-mbmi', '-mbmi2', '-mno-tbm', '-mavx', '-mavx2', '-msse4.2', '-msse4.1', '-mlzcnt', '-mno-rtm', '-mno-hle', '-mrdrnd', '-mf16c', '-mfsgsbase', '-mrdseed', '-mprfchw', '-madx', '-mfxsr', '-mxsave', '-mxsaveopt', '-mno-avx512f', '-mno-avx512er', '-mno-avx512cd', '-mno-avx512pf', '-mno-prefetchwt1', '-mclflushopt', '-mxsavec', '-mxsaves', '-mno-avx512dq', '-mno-avx512bw', '-mno-avx512vl', '-mno-avx512ifma', '-mno-avx512vbmi', '-mno-clwb', '-mno-mwaitx', ],
                    'params'      => ['-m64', '--param l1-cache-size=32', '--param l1-cache-line-size=64', '--param l2-cache-size=4096', ],
                    'environment' => {'TARGET' => 'HASWELL', 'USE_THREAD' => '1', 'BINARY' => '64', 'NUM_THREADS' => '2', 'AUDIO' => 'PORTAUDIO', 'LDFLAGS' => '-m64', 'PKG_CONFIG_PATH' => '/usr/lib/x86_64-linux-gnu/pkgconfig', 'CCAS' => 'gcc', 'CCASFLAGS' => '-m64', },
                   },
  }

  # Make sure the necessary installer prerequisites are satisfied.
  class { 'installer::prerequisites' :
    archs => $archs,
  }

  # We need build tools to compile and we need it done before the installer
  class {'build_tools': } -> Installer <| |>

  # These user tools make the shell much easier and these also should be done before installing
  class {'user_tools':
    user => 'vagrant',
  } -> Installer <| |>

  # List all of the archives that need to be downloaded along with any other associated parameters (creates, requires, etc).
  $archives = {
    'protobuf'     => {'url'         => 'https://github.com/google/protobuf/releases/download/v3.4.0/protobuf-cpp-3.4.0.tar.gz',
                       'args'        => { 'native'   => [ '--with-zlib', '--with-protoc=PROTOC_PATH', ],
                                          'nuc7i7bnh' => [ '--with-zlib', '--with-protoc=PROTOC_PATH', ], },
                       'require'     => [ Class['protobuf'], Installer['zlib'], ],
                       'prebuild'    => 'make distclean',
                       'postbuild'   => 'rm PREFIX/lib/libprotoc* && rm PREFIX/bin/protoc',
                       'method'      => 'autotools', },
    'zlib'         => {'url'         => 'http://www.zlib.net/zlib-1.2.11.tar.gz',
                       'creates'     => 'lib/libz.a',
                       'method'      => 'cmake',},
    'bzip2'        => {'url'         => 'https://github.com/Bidski/bzip2/archive/v1.0.6.1.tar.gz',
                       'args'        => { 'native'   => [ '', ],
                                          'nuc7i7bnh' => [ '', ], },
                       'creates'     => 'lib/libbz2.so',
                       'method'      => 'make',},
    'xml2'         => {'url'         => 'http://xmlsoft.org/sources/libxml2-2.9.3.tar.gz',
                       'args'        => { 'native'   => [ '--with-zlib=ZLIB_PATH', '--without-python', ],
                                          'nuc7i7bnh' => [ '--with-zlib=ZLIB_PATH', '--without-python', ], },
                       'method'      => 'autotools',},
    'nuclear'      => {'url'         => 'https://github.com/Fastcode/NUClear/archive/master.tar.gz',
                       'args'        => { 'native'   => [ '-DBUILD_TESTS=OFF', ],
                                          'nuc7i7bnh' => [ '-DBUILD_TESTS=OFF', ], },
                       'method'      => 'cmake',},
    # NOTE: OpenBLAS CMake support is experimental and only supports x86 at the moment.
    'openblas'     => {'url'         => 'https://github.com/xianyi/OpenBLAS/archive/v0.2.19.tar.gz',
                       'args'        => { 'native'   => [ '', ],
                                          'nuc7i7bnh' => [ 'CROSS=1', ], },
                       'method'      => 'make',
                       'creates'     => 'lib/libopenblas.a' },
    'libsvm'       => {'url'         => 'https://github.com/Bidski/libsvm/archive/v322.tar.gz',
                       'creates'     => 'lib/svm.o',
                       'method'      => 'make', },
    'armadillo'    => {'url'         => 'https://downloads.sourceforge.net/project/arma/armadillo-7.950.1.tar.xz',
                       'method'      => 'cmake',
                       'creates'     => 'lib/libarmadillo.so',
                       'require'     => [ Installer['openblas'], ],},
    'tcmalloc'     => {'url'         => 'https://github.com/gperftools/gperftools/releases/download/gperftools-2.5.93/gperftools-2.5.93.tar.gz',
                       'args'        => { 'native'   => [ '--with-tcmalloc-pagesize=64', '--enable-minimal', ],
                                          'nuc7i7bnh' => [ '--with-tcmalloc-pagesize=64', '--enable-minimal', ], },
                       'creates'     => 'lib/libtcmalloc_minimal.a',
                       'method'      => 'autotools',},
    'yaml-cpp'     => {'url'         => 'https://github.com/jbeder/yaml-cpp/archive/master.tar.gz',
                       'args'        => { 'native'   => [ '-DYAML_CPP_BUILD_CONTRIB=OFF', '-DYAML_CPP_BUILD_TOOLS=OFF', ],
                                          'nuc7i7bnh' => [ '-DYAML_CPP_BUILD_CONTRIB=OFF', '-DYAML_CPP_BUILD_TOOLS=OFF', ], },
                       'method'      => 'cmake',},
    'fftw3'        => {'url'         => 'http://www.fftw.org/fftw-3.3.6-pl2.tar.gz',
                       'args'        => { 'native'   => [ '--disable-fortran', '--enable-shared', ],
                                          'nuc7i7bnh' => [ '--disable-fortran', '--enable-shared', ], },
                       'method'      => 'autotools',},
    'jpeg'         => {'url'         => 'http://downloads.sourceforge.net/project/libjpeg-turbo/1.5.1/libjpeg-turbo-1.5.1.tar.gz',
                       'args'        => { 'native'   => [ 'CCASFLAGS="-f elf64"', ],
                                          'nuc7i7bnh' => [ 'CCASFLAGS="-f elf64"', ], },
                       'method'      => 'autotools',},
    'cppformat'    => {'url'         => 'https://github.com/fmtlib/fmt/archive/3.0.1.tar.gz',
                       'method'      => 'cmake',
                       'creates'     => 'lib/libfmt.a' },
    'portaudio'    => {'url'         => 'http://www.portaudio.com/archives/pa_stable_v19_20140130.tgz',
                       'args'        => { 'native'   => [ '', ],
                                          'nuc7i7bnh' => [ '', ], },
                       'method'      => 'autotools',},
    'eigen3'       => {'url'         => 'http://bitbucket.org/eigen/eigen/get/3.3.4.tar.bz2',
                       'creates'     => 'include/eigen3/Eigen/Eigen',
                       'method'      => 'cmake',},
    'boost'        => {'url'         => 'https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz',
                       'args'        => { 'native'   => [ 'address-model=64', 'architecture=x86', 'link=static', ],
                                          'nuc7i7bnh' => [ 'address-model=64', 'architecture=x86', 'link=static', ], },
                       'method'      => 'boost',
                       'creates'     => 'src/boost/build_complete',
                       'postbuild'   => 'touch build_complete',
                       'require'     => [ Installer['zlib'], Installer['bzip2'], ],},
    'espeak'       => {'url'         => 'https://github.com/Bidski/espeak/archive/v1.48.04.tar.gz',
                       'src_dir'     => 'src',
                       'prebuild'    => 'cp portaudio19.h portaudio.h',
                       'method'      => 'make',
                       'require'     => [ Installer['portaudio'], ],},
    'fswatch'      => {'url'         => 'https://github.com/emcrisostomo/fswatch/releases/download/1.9.3/fswatch-1.9.3.tar.gz',
                       'args'        => { 'native'   => [ '', ],
                                          'nuc7i7bnh' => [ '', ], },
                       'method'      => 'autotools', },
    'ffi'          => {'url'         => 'https://github.com/libffi/libffi/archive/v3.2.1.tar.gz',
                       'args'        => { 'native'   => [ '', ],
                                          'nuc7i7bnh' => [ '', ], },
                       'postbuild'   => 'if [ -e PREFIX/lib32/libffi.a ]; then cp PREFIX/lib32/libffi* PREFIX/lib/; fi',
                       'method'      => 'autotools', },
    'glib'         => {'url'         => 'ftp://ftp.gnome.org/pub/gnome/sources/glib/2.52/glib-2.52.3.tar.xz',
                       'args'        => { 'native'   => [ '--cache-file=PREFIX/src/glib.config', '--with-threads', '--with-pcre=internal', '--disable-gtk-doc', '--disable-man', ],
                                          # Technically we are cross compiling for the nuv7i7bnh, even though both the host and build systems are both x86_64-linux-gnu
                                          'nuc7i7bnh' => [ '--cache-file=PREFIX/src/glib.config', '--host=x86_64-linux-gnu', '--build=x86_64-unknown-linux-gnu', '--with-threads', '--with-pcre=internal', '--disable-gtk-doc', '--disable-man', ], },
                       'postbuild'   => 'cp glib/glibconfig.h PREFIX/include/glibconfig.h',
                       'require'     => [ Installer['ffi'], ],
                       'creates'     => 'lib/libglib-2.0.so',
                       'method'      => 'autotools', },
    'aravis'       => {'url'         => 'https://github.com/AravisProject/aravis/archive/ARAVIS_0_5_9.tar.gz',
                       'args'        => { 'native'   => [ '--cache-file=PREFIX/src/aravis.config', '--disable-viewer', '--disable-gst-plugin', '--disable-gst-0.10-plugin', '--disable-gtk-doc', '--disable-gtk-doc-html', '--disable-gtk-doc-pdf', '--enable-usb', '--disable-zlib-pc', ],
                                          # Technically we are cross compiling for the nuv7i7bnh, even though both the host and build systems are both x86_64-linux-gnu
                                          'nuc7i7bnh' => [ '--cache-file=PREFIX/src/aravis.config', '--host=x86_64-linux-gnu', '--build=x86_64-unknown-linux-gnu', '--disable-viewer', '--disable-gst-plugin', '--disable-gst-0.10-plugin', '--disable-gtk-doc', '--disable-gtk-doc-html', '--disable-gtk-doc-pdf', '--enable-usb', '--disable-zlib-pc', ], },
                       'require'     => [ Installer['xml2'], Installer['zlib'], Installer['glib'], ],
                       'creates'     => 'lib/libaravis-0.6.so',
                       'prebuild'    => 'sed "s/return\s(entry->schema\s>>\s10)\s\&\s0x0000001f;/return ((entry->schema >> 10) \& 0x0000001f) ? ARV_UVCP_SCHEMA_ZIP : ARV_UVCP_SCHEMA_RAW;/" -i src/arvuvcp.h',
                       'postbuild'   => 'cp src/arvconfig.h PREFIX/include/arvconfig.h',
                       'method'      => 'autotools', },
    'pybind11'     => {'url'         => 'https://github.com/pybind/pybind11/archive/v2.2.1.tar.gz',
                       'args'        => { 'native'   => [ '-DPYBIND11_TEST=OFF', ' -DPYBIND11_PYTHON_VERSION=3',  ],
                                          'nuc7i7bnh' => [ '-DPYBIND11_TEST=OFF', ' -DPYBIND11_PYTHON_VERSION=3', ], },
                       'creates'     => 'include/pybind11/pybind11.h',
                       'method'      => 'cmake', },

    # Mesa
    # http://www.linuxfromscratch.org/blfs/view/svn/x/mesa.html
    # 'pybeaker'     => {'url'         => 'https://files.pythonhosted.org/packages/source/B/Beaker/Beaker-1.9.0.tar.gz',
    #                    'args'        => { 'native'   => [ ' --optimize=1',  ],
    #                                       'nuc7i7bnh' => [ ' --optimize=1', ], },
    #                    'method'      => 'python', },
    # 'pymarkupsafe' => {'url'         => 'https://files.pythonhosted.org/packages/source/M/MarkupSafe/MarkupSafe-1.0.tar.gz',
    #                    'args'        => { 'native'   => [ ' --optimize=1',  ],
    #                                       'nuc7i7bnh' => [ ' --optimize=1', ], },
    #                    'method'      => 'python', },
    # 'pymako'       => {'url'         => 'https://files.pythonhosted.org/packages/source/M/Mako/Mako-1.0.4.tar.gz',
    #                    'args'        => { 'native'   => [ ' --optimize=1',  ],
    #                                       'nuc7i7bnh' => [ ' --optimize=1', ], },
    #                    'require'     => [ Installer['pybeaker'], Installer['pymarkupsafe'], ],
    #                    'method'      => 'python', },

    'freetype'     => {'url'         => 'https://downloads.sourceforge.net/freetype/freetype-2.8.1.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-static', ],
                                          'nuc7i7bnh' => [ '--disable-static', ], },
                       'prebuild'    => 'sed -ri "s:.*(AUX_MODULES.*valid):\1:" PREFIX/src/freetype/modules.cfg &&
                                         sed -r "s:.*(#.*SUBPIXEL_RENDERING) .*:\1:" -i PREFIX/src/freetype/include/freetype/config/ftoption.h',
                       'creates'     => 'lib/libfreetype.so',
                       'method'      => 'autotools', },
    'fontconfig'   => {'url'         => 'https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.12.6.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-docs', ],
                                          'nuc7i7bnh' => [ '--disable-docs', ], },
                       'prebuild'    => 'rm -f src/fcobjshash.h',
                       'require'     => [ Installer['freetype'], ],
                       'creates'     => 'lib/libfontconfig.so',
                       'method'      => 'autotools', },
    'util-macros'  => {'url'         => 'https://www.x.org/pub/individual/util/util-macros-1.19.1.tar.bz2',
                       'creates'     => 'share/aclocal/xorg-macros.m4',
                       'method'      => 'autotools', },
    'xcb-proto'    => {'url'         => 'https://xcb.freedesktop.org/dist/xcb-proto-1.12.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-static',  ],
                                          'nuc7i7bnh' => [ '--disable-static',  ], },
                       'prebuild'    => 'wget http://www.linuxfromscratch.org/patches/blfs/svn/xcb-proto-1.12-schema-1.patch -O - | patch -Np1 &&
                                         wget http://www.linuxfromscratch.org/patches/blfs/svn/xcb-proto-1.12-python3-1.patch -O - | patch -Np1',
                       'creates'     => 'lib/python3.5/site-packages/xcbgen/__init__.py',
                       'method'      => 'autotools', },
    'xorg-protocol-headers' => {'url' => '',
                                'prebuild'    => 'PREFIX/bin/xorg-protos.sh',
                                'require'     => [ Installer['util-macros'], ],
                                'creates'     => 'share/doc/xproto/x11protocol.xml',
                                'method'      => 'custom', },
    'Xdmcp'        => {'url'         => 'https://www.x.org/pub/individual/lib/libXdmcp-1.1.2.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-static',  ],
                                          'nuc7i7bnh' => [ '--disable-static',  ], },
                       'creates'     => 'lib/libXdmcp.so',
                       'require'     => [ Installer['xorg-protocol-headers'], ],
                       'method'      => 'autotools', },
    'Xau'          => {'url'         => 'https://www.x.org/pub/individual/lib/libXau-1.0.8.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-static',  ],
                                          'nuc7i7bnh' => [ '--disable-static',  ], },
                       'creates'     => 'lib/libXau.so',
                       'require'     => [ Installer['xorg-protocol-headers'], ],
                       'method'      => 'autotools', },
    'xcb'          => {'url'         => 'https://xcb.freedesktop.org/dist/libxcb-1.12.tar.bz2',
                       'args'        => { 'native'   => [ '--disable-static', '--enable-xinput', '--without-doxygen', ],
                                          'nuc7i7bnh' => [ '--disable-static', '--enable-xinput', '--without-doxygen', ], },
                       'prebuild'    => 'wget http://www.linuxfromscratch.org/patches/blfs/svn/libxcb-1.12-python3-1.patch -O - | patch -Np1 &&
                                         sed -i "s/pthread-stubs//" configure',
                       'creates'     => 'lib/libxcb.so',
                       'require'     => [ Installer['Xau'], Installer['xcb-proto'], ],
                       'method'      => 'autotools', },
    'xorg-libs'    => {'url' => '',
                       'prebuild'    => 'PREFIX/bin/xorg-libs.sh',
                       'creates'     => 'lib/libxshmfence.so',
                       'require'     => [ Installer['fontconfig'], Installer['xcb'], ],
                       'method'      => 'custom', },
    'drm'          => {'url'         => 'https://dri.freedesktop.org/libdrm/libdrm-2.4.85.tar.bz2',
                       'args'        => { 'native'   => [ '--enable-udev', ],
                                          'nuc7i7bnh' => [ '--enable-udev', ], },
                       'creates'     => 'lib/libdrm.so',
                       'require'     => [ Installer['xorg-libs'], ],
                       'method'      => 'autotools', },
    'mesa'         => {'url'         => 'https://mesa.freedesktop.org/archive/mesa-17.2.3.tar.xz',
                       'args'        => { 'native'   => [ '--enable-texture-float', '--enable-osmesa', '--enable-xa', '--enable-glx-tls', '--with-platforms="drm,x11"', '--enable-gles1', '--enable-gles2', '--enable-shared-glapi', '--enable-egl', '--with-dri-drivers="i965,i915"', '--with-gallium-drivers="swrast,svga"', '--with-vulkan-drivers=intel', '--enable-gbm', ],
                                          'nuc7i7bnh' => [ '--enable-texture-float', '--enable-osmesa', '--enable-xa', '--enable-glx-tls', '--with-platforms="drm,x11"', '--enable-gles1', '--enable-gles2', '--enable-shared-glapi', '--enable-egl', '--with-dri-drivers="i965,i915"', '--with-gallium-drivers="swrast,svga"', '--with-vulkan-drivers=intel', '--enable-gbm', ], },
                       'creates'     => 'lib/libEGL.so',
                       'require'     => [ Installer['xorg-libs'], Installer['drm'], ], #Installer['pymako'], ],
                       'method'      => 'autotools', },
    'glm'          => {'url'         => 'https://github.com/g-truc/glm/archive/0.9.8.5.tar.gz',
                       'args'        => { 'native'   => [ '-DGLM_TEST_ENABLE_CXX_14=ON', '-DGLM_TEST_ENABLE_LANG_EXTENSIONS=ON', '-DGLM_TEST_ENABLE_FAST_MATH=ON',  ],
                                          'nuc7i7bnh' => [ '-DGLM_TEST_ENABLE_FAST_MATH=ON', '-DGLM_TEST_ENABLE_FAST_MATH=ON', '-DGLM_TEST_ENABLE_FAST_MATH=ON',  ], },
                       'creates'     => 'include/glm/glm.h',
                       'method'      => 'cmake', },
  }


  # Download each archive and spawn Installers for each one.
  $archives.each |String $archive,
                  Struct[{'url' => String,
                          Optional['creates'] => String,
                          Optional['args'] => Hash,
                          Optional['require'] => Tuple[Any, 1, default],
                          'method' => String,
                          Optional['src_dir'] => String,
                          Optional['prebuild'] => String,
                          Optional['postbuild'] => String}] $params| {

        $extension = $params['url'] ? {
          /.*\.zip/       => 'zip',
          /.*\.tgz/       => 'tgz',
          /.*\.tar\.gz/   => 'tar.gz',
          /.*\.txz/       => 'txz',
          /.*\.tar\.xz/   => 'tar.xz',
          /.*\.tbz/       => 'tbz',
          /.*\.tbz2/      => 'tbz2',
          /.*\.tar\.bz2/  => 'tar.bz2',
          /.*\.h/         => 'h',
          /.*\.hpp/       => 'hpp',
          default         => 'UNKNOWN',
        }

        if $extension != 'UNKNOWN' {
          archive { "${archive}":
            url              => $params['url'],
            target           => "/nubots/toolchain/src/${archive}",
            src_target       => "/nubots/toolchain/src",
            purge_target     => true,
            checksum         => false,
            follow_redirects => true,
            timeout          => 0,
            extension        => $extension,
            strip_components => 1,
            root_dir         => '.',
            require          => [ Class['installer::prerequisites'], Class['build_tools'], ],
          }
        }
        installer { "${archive}":
          archs       => $archs,
          creates     => $params['creates'],
          require     => delete_undef_values(flatten([ $params['require'], Class['installer::prerequisites'], Class['build_tools'], ])),
          args        => $params['args'],
          src_dir     => $params['src_dir'],
          prebuild    => $params['prebuild'],
          postbuild   => $params['postbuild'],
          method      => $params['method'],
          extension   => $extension,
        }
  }

  # Install quex.
  class { 'quex': }

  # Install protobuf.
  class { 'protobuf': }

  # Install catch.
  installer { 'catch':
    url       => 'https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp',
    archs     => $archs,
    extension => 'hpp',
    method    => 'wget',
  }

  # Perform any complicated postbuild instructions here.
  $archs.each |String $arch, Hash $params| {
    # Update the armadillo config header file for all archs.
    file { "armadillo_${arch}_config":
      path    => "/nubots/toolchain/${arch}/include/armadillo_bits/config.hpp",
      source  => 'puppet:///modules/files/nubots/toolchain/include/armadillo_bits/config.hpp',
      ensure  => present,
      require => [ Installer['armadillo'], ],
    }
  }

  # After we have installed, create the CMake toolchain files and then build our deb.
  Installer <| |> ~> class { 'toolchain_deb': }

  # Patch some system utilities to make sure they ignore our preset LD_LIBRARY_PATH
  file { "/nubots/toolchain/bin/msgfmt.sh":
    content =>
"
#! /bin/bash
LD_LIBRARY_PATH= /usr/bin/msgfmt \"$@\"
",
    ensure  => present,
    path    => "/nubots/toolchain/bin/msgfmt.sh",
    mode    => "a+x",
  } -> Installer <| |>

  file { "/nubots/toolchain/bin/msgmerge.sh":
    content =>
"
#! /bin/bash
LD_LIBRARY_PATH= /usr/bin/msgmerge \"$@\"
",
    ensure  => present,
    path    => "/nubots/toolchain/bin/msgmerge.sh",
    mode    => "a+x",
  } -> Installer <| |>

  file { "/nubots/toolchain/bin/xgettext.sh":
    content =>
"
#! /bin/bash
LD_LIBRARY_PATH= /usr/bin/xgettext \"$@\"
",
    ensure  => present,
    path    => "/nubots/toolchain/bin/xgettext.sh",
    mode    => "a+x",
  } -> Installer <| |>

  file { "/nubots/toolchain/nuc7i7bnh/bin/pkg-config.sh":
    content =>
"
#! /bin/bash
LD_LIBRARY_PATH= /usr/bin/x86_64-linux-gnu-pkg-config \"$@\"
",
    ensure  => present,
    path    => "/nubots/toolchain/nuc7i7bnh/bin/pkg-config.sh",
    mode    => "a+x",
  } -> Installer <| |>

  file { "/nubots/toolchain/native/bin/pkg-config.sh":
    content =>
"
#! /bin/bash
LD_LIBRARY_PATH= /usr/bin/pkg-config \"$@\"
",
    ensure  => present,
    path    => "/nubots/toolchain/native/bin/pkg-config.sh",
    mode    => "a+x",
  } -> Installer <| |>

  $archs.each |String $arch, Hash $params| {
    $prefix = '/nubots/toolchain'

    # We need to prevent glib from trying to run tests when cross-compiling glib (to avoid SIGILL).
    file { "${arch}_glib.config":
      content =>
"glib_cv_stack_grows=no
glib_cv_uscore=no
",
      ensure  => present,
      path    => "${prefix}/${arch}/src/glib.config",
      mode    => "a-w",
      before  => [ Installer['glib'], ],
    }

    # Force paths to gettext bianries (to avoid SIGILL).
    file { "${arch}_aravis.config":
      content =>
"ac_cv_path_XGETTEXT=${prefix}/bin/xgettext.sh
ac_cv_path_MSGMERGE=${prefix}/bin/msgmerge.sh
ac_cv_path_MSGFMT=${prefix}/bin/msgfmt.sh
ac_cv_path_PKG_CONFIG=${prefix}/${arch}/bin/pkg-config.sh
",
      ensure  => present,
      path    => "${prefix}/${arch}/src/aravis.config",
      mode    => "a-w",
      before  => [ Installer['aravis'], ],
    }

    # Create CMake toolchain files.
    $compile_options = join(prefix(suffix($params['flags'], ')'), 'add_compile_options('), "\n")
    $compile_params  = join($params['params'], " ")

    file { "${arch}.cmake":
      content =>
"set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_FIND_ROOT_PATH \"${prefix}/${arch}\"
       \"${prefix}\"
       \"/usr/local\"
       \"/usr\")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

${compile_options}

include_directories(SYSTEM \"${prefix}/${arch}/include\")
include_directories(SYSTEM \"${prefix}/include\")

set(CMAKE_C_FLAGS \"\${CMAKE_C_FLAGS} ${compile_params}\" CACHE STRING \"\")
set(CMAKE_CXX_FLAGS \"\${CMAKE_CXX_FLAGS} ${compile_params}\" CACHE STRING \"\")

set(PLATFORM \"${arch}\" CACHE STRING \"The platform to build for.\" FORCE)
",
      ensure  => present,
      path    => "${prefix}/${arch}.cmake",
      before  => Class['toolchain_deb'],
    }

    file { "${prefix}/${arch}/bin/xorg-libs.sh":
      content => "#! /bin/bash

set -e

xorg_libs=(xtrans-1.3.5 libX11-1.6.5 libXext-1.3.3 libFS-1.0.7 libICE-1.0.9 libSM-1.2.2 libXScrnSaver-1.2.2 libXt-1.1.5 libXmu-1.1.2 libXpm-3.5.12 libXaw-1.0.13 libXfixes-5.0.3 libXcomposite-0.4.4 libXrender-0.9.10 libXcursor-1.1.14 libXdamage-1.1.4 libfontenc-1.1.3 libXfont2-2.0.2 libXft-2.3.2 libXi-1.7.9 libXinerama-1.1.3 libXrandr-1.5.1 libXres-1.2.0 libXtst-1.2.3 libXv-1.0.11 libXvMC-1.0.10 libXxf86dga-1.1.4 libXxf86vm-1.1.4 libdmx-1.1.3 libpciaccess-0.14 libxkbfile-1.0.9 libxshmfence-1.2)

for xorg_lib in \${xorg_libs[*]};
do
    wget https://www.x.org/pub/individual/lib/\"\${xorg_lib}.tar.bz2\" -O - | tar xjf -
    cd \"\${xorg_lib}\"
    case \${xorg_lib} in
        libICE* )
        ./configure --prefix=${prefix}/${arch} --disable-static ICE_LIBS=-lpthread
        ;;

        libXfont2-[0-9]* )
        ./configure --prefix=${prefix}/${arch} --disable-static --disable-devel-docs
        ;;

        libXt-[0-9]* )
        ./configure --prefix=${prefix}/${arch} --disable-static --with-appdefaultdir=${prefix}/${arch}/etc/X11/app-defaults
        ;;

        * )
        ./configure --prefix=${prefix}/${arch} --disable-static
        ;;
    esac
    make -j\$(nproc)
    make install
    cd ..
done
",
      ensure  => present,
      path    => "${prefix}/${arch}/bin/xorg-libs.sh",
      mode    => "a+x",
      before  => Installer['xorg-libs'],
    }

    file { "${prefix}/${arch}/bin/xorg-protos.sh":
      content => "#! /bin/bash

set -e

protos=(bigreqsproto-1.1.2 compositeproto-0.4.2 damageproto-1.2.1 dmxproto-2.3.1 dri2proto-2.8 dri3proto-1.0 fixesproto-5.0 fontsproto-2.1.3 glproto-1.4.17 inputproto-2.3.2 kbproto-1.0.7 presentproto-1.1 randrproto-1.5.0 recordproto-1.14.2 renderproto-0.11.1 resourceproto-1.2.0 scrnsaverproto-1.2.2 videoproto-2.3.3 xcmiscproto-1.2.2 xextproto-7.3.0 xf86bigfontproto-1.2.0 xf86dgaproto-2.1 xf86driproto-2.1.1 xf86vidmodeproto-2.3.1 xineramaproto-1.2.1 xproto-7.0.31)

for proto in \${protos[*]};
do
    wget https://www.x.org/pub/individual/proto/\"\${proto}.tar.bz2\" -O - | tar xjf -
    cd \"\${proto}\"
    ./configure --prefix=${prefix}/${arch} --disable-static
    make -j\$(nproc)
    make install
    cd ..
done
",
      ensure  => present,
      path    => "${prefix}/${arch}/bin/xorg-protos.sh",
      mode    => "a+x",
      before  => Installer['xorg-protocol-headers'],
    }
  }
}
