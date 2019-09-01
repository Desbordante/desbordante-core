env = Environment(CPPPATH='/home/maxim/Study/Practice/MetanomeCoding/Software/boost_1_70_0/boost_output/include',
    CPPDEFINES=[],
    LIBS=[],
    SCONS_CXX_STANDARD="c++17")

env.Append(CPPFLAGS = [ '-O0', '-std=c++17',
        #'-fvar-tracking',
        '-ggdb', '-fstack-protector-all','-pedantic', '-Wall', '-Werror',
        '-Wextra', '-Wcast-align', '-Wcast-qual',
        #'-Wctor-dtor-privacy', '-Wswitch',
        '-Wdisabled-optimization', '-Wformat=2', '-Winit-self',
        #'-Wlogical-op',
        '-Wmissing-declarations',
        #'-Wnoexcept', '-Wold-style-cast', '-Woverloaded-virtual',
        '-Wredundant-decls', '-Wsign-promo',
        #'-Wstrict-null-sentinel', #'-Wsign-conversion',
        '-Wstrict-overflow=5', '-Wundef', '-Wno-unused', '-Wno-unused-parameter',
        #'-fsanitize=undefined', '-Wno-gnu-zero-variadic-macro-arguments'           # for clang
        ])

test_env = env.Clone()

SConscript('SConscript', variant_dir = 'build', exports=['env', 'test_env'], duplicate=0)
