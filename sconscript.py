Import('env')
env.Append(LIBS = ['stdc++fs'])
sources = Glob('src/Parser/*.cpp') + Glob('src/model/*.cpp')

metanome = env.Program(target = 'metanome', source = ['src/main.cpp', 'src/Parser/json/json.hpp'] + sources)
Default(metanome)

