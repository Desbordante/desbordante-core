Import('env')
env.Append(LIBS = ['stdc++fs'])
sources = Glob('src/parser/*.cpp') + Glob('src/model/*.cpp') + Glob('src/util/*.cpp')

metanome = env.Program(target = 'metanome', source = sources + ['src/main.cpp'])
Default(metanome)

