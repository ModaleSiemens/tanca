project(
    'tanca',
    'cpp',

    version: '0.1',
    
    default_options: [
        'cpp_std=c++23',
        'buildtype=debug'
    ]
)

add_project_link_arguments(
    '-lstdc++exp',
    language: 'cpp'
)

libapp_proj  = subproject('app')
libnets_proj = subproject('nets')
lib_openssl = subproject('openssl')


deps = [
    dependency('libapp'),
    dependency('libnets'),
    dependency('libcrypto')
]

subdir('src')

install_subdir(
    'assets',
    install_dir: '.'
)

executable(
    'client',
    client_src,

    dependencies: deps,

    install: true  
)

executable(
    'server',
    server_src,

    dependencies: deps,

    install: true
)

executable(
    'server_manager',
    server_manager_src,

    dependencies: deps,

    install: true
)