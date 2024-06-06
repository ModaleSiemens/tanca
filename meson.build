project(
    'tanca',
    'cpp',

    version: '0.1',
    
    default_options: [
        'cpp_std=c++23',
        'buildtype=release'
    ]
)

libapp_proj  = subproject('app')
libnets_proj = subproject('nets')

deps = [
    dependency('libapp'),
    dependency('libnets')
]

subdir('src')

executable(
    'tanca_client',
    client_src,

    dependencies: deps,

    link_args: 
    [
        '-lstdc++exp' # Enable std::print, std::println
    ]    
)

executable(
    'tanca_server',
    server_src,

    dependencies: deps,

    link_args: 
    [
        '-lstdc++exp' # Enable std::print, std::println
    ]    
)

executable(
    'tanca_server_manager',
    server_manager_src,

    dependencies: deps,

    link_args: 
    [
        '-lstdc++exp' # Enable std::print, std::println
    ]    
)