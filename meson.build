project(
    'boinc-shmem-tool',
    'cpp',
    default_options: [
        'cpp_std=c++1z',
    ],
)

executable(
    'boinc-shmem-tool',
    [
        'src/main.cpp',
    ],
    dependencies: [
        dependency('boinc-app-api'),
        dependency(
            'boost',
            modules: [
                'filesystem',
                'format',
                'program_options',
                'system',
            ],
        ),
        dependency('jsoncpp'),
    ],
    install: true,
)
