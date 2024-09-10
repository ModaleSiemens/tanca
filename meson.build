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

deps = [
    dependency('libapp'),
    dependency('libnets')
]

subdir('src')

executable(
    'client',
    client_src,

    dependencies: deps  
)

executable(
    'server',
    server_src,

    dependencies: deps
)

executable(
    'server_manager',
    server_manager_src,

    dependencies: deps
)