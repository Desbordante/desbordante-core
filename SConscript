def create_objs(srcs):
    return [env.Object(src) for src in srcs]

Import('env')

sources = Glob('src/parser/*.cpp') + Glob('src/model/*.cpp') + Glob('src/util/*.cpp')
objects = create_objs(sources)

metanome = env.Program(target = 'metanome', source = objects + ['src/main.cpp'], LIBS = ['stdc++fs'])
Command("config.json", "../config.json", Copy("$TARGET", "$SOURCE"))

#TESTS
libs = ['stdc++fs', 'gtest', 'pthread']

progr = env.Program('tests/tester', Glob('src/tests/*.cpp') + objects, LIBS = libs)

csv_tests = Glob("src/tests/*.csv")
for csv in csv_tests:
    Command('tests/' + csv.name, csv.rstr(), Copy("$TARGET", "$SOURCE"))

test_alias = Alias('test', [progr], progr[0].abspath)
AlwaysBuild(progr)