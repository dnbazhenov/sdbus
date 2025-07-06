project('sdbus',
    ['cpp'],
    default_options: [
        'werror=false',
        'warning_level=3',
        'cpp_std=c++23',
        'buildtype=minsize',
#        'buildtype=debug',
#        'buildtype=debugoptimized',
#        'buildtype=release',
#        'buildtype=plain',
        'b_ndebug=if-release',
        'b_lto=true',
    ])

systemd_dep = dependency(
    'libsystemd',
    include_type: 'system',
    required: true,
    )

boost_compile_args = [
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ASIO_NO_DEPRECATED',
    '-DBOOST_ASIO_HAS_BOOST_CONTEXT_FIBER',
    '-DBOOST_ASIO_DISABLE_BOOST_COROUTINE',
    ]

boost_dep = declare_dependency(
    dependencies: dependency(
        'boost',
        modules: ['context'],
        include_type: 'system',
        required: true,
        ),
    compile_args: boost_compile_args,
)

incdir = include_directories('.')

sdbus = library('sdbus',
    'sdbus/context.cpp',
    'sdbus/read.cpp',
    'sdbus/write.cpp',
    'sdbus/service.cpp',
    cpp_args: '-fconcepts-diagnostics-depth=2',
    include_directories: incdir,
    dependencies: [systemd_dep, boost_dep],
    )

sdbus_dep = declare_dependency(
    include_directories: incdir,
    link_with: sdbus,
    dependencies: [boost_dep, systemd_dep],
    )

subdir('test')
