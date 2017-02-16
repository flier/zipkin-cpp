import os
import os.path
import itertools

from SCons.Script import SConscript, Environment, ARGUMENTS

debug_mode = int(ARGUMENTS.get('debug', 0))
build_dir = ARGUMENTS.get('build_dir', 'build')
gen_dir = ARGUMENTS.get('gen_dir', 'gen-cpp')

bin_dir = 'bin'
src_dir = 'src'
test_dir = 'test'
obj_dir = os.path.join(build_dir, 'obj')

env = Environment(CXXFLAGS=['-std=c++11', '-Wno-invalid-offsetof'],
                  ENV={'TERM': os.getenv('TERM', 'xterm-256color')})

env.VariantDir(build_dir, '.', duplicate=0)

conan_file = 'conanfile.txt'
conan_script = 'SConscript_conan'

conan_install = env.Command(target=conan_script,
                            source=conan_file,
                            action=['conan install .'])

env.Clean(conan_install, ['conaninfo.txt', 'conanbuildinfo.txt', 'conan_imports_manifest.txt'])

if not env.GetOption('clean') and not os.path.isfile(conan_script):
    env.Execute(action=['conan install .'])

    conan_libs = SConscript(conan_script)
else:
    conan_libs = {'conan': {'LIBS': ['']}}

if debug_mode:
    env.Append(CXXFLAGS=['-g'])
else:
    env.Append(CXXFLAGS=['-O2'])


def obj_files(source_files, base_dir, target_dir=obj_dir, env=env):
    for filename in source_files:
        source = os.path.join(base_dir, filename)
        target = os.path.join(target_dir, os.path.splitext(filename)[0])

        yield env.StaticObject(target=target, source=source)

zipkinCoreThrift = os.path.join(src_dir, 'zipkinCore.thrift')
zipkinCoreSources = ['zipkinCore_constants.cpp', 'zipkinCore_types.cpp']
zipkinCoreFiles = [os.path.join(gen_dir, name) for name in zipkinCoreSources]
zipkinCoreObjects = obj_files(zipkinCoreSources, base_dir=gen_dir)

zipkinLibSources = ['Span.cpp', 'Tracer.cpp', 'Collector.cpp']
zipkinLibObjects = obj_files(zipkinLibSources, base_dir=src_dir)

env.Command(target=zipkinCoreFiles,
            source=zipkinCoreThrift,
            action=['thrift -r --gen cpp ' + zipkinCoreThrift])

zipkinLib = env.StaticLibrary(target=os.path.join(build_dir, 'zipkin'),
                              source=list(itertools.chain(zipkinCoreObjects, zipkinLibObjects)))

test_env = env.Clone()
test_env.MergeFlags(conan_libs['conan'])
test_env.Replace(LIBS=[lib for lib in test_env['LIBS'] if not lib.endswith('_main')])

zipkinTestSources = ['TestMain.cpp', 'TestSpan.cpp', 'TestTracer.cpp', 'TestCollector.cpp']
zipkinTestObjects = obj_files(zipkinTestSources, base_dir=test_dir, env=test_env)

unittest = test_env.Program(target=os.path.join(bin_dir, 'unittest'),
                            source=list(zipkinTestObjects) + [zipkinLib])

runtest = test_env.Command(target='runtest',
                           source=unittest,
                           action=['unittest'],
                           DYLD_LIBRARY_PATH=bin_dir,
                           chdir=bin_dir)

env.Execute(runtest)
