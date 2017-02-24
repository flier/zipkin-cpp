import os
import os.path
import itertools
from distutils.util import strtobool

from SCons.Script import SConscript, Environment, Glob, Configure, Depends, Exit, FindFile, ARGUMENTS

debug_mode = strtobool(ARGUMENTS.get('debug', 'true'))
release_mode = strtobool(ARGUMENTS.get('release', 'false'))
gen_doc = strtobool(ARGUMENTS.get('doc', 'false'))
build_dir = ARGUMENTS.get('build_dir', 'build')
gen_dir = ARGUMENTS.get('gen_dir', 'gen-cpp')

bin_dir = 'bin'
src_dir = 'src'
test_dir = 'test'
inc_dir = 'include'
example_dir = 'examples'
doc_dir = os.path.join(build_dir, 'docs')
obj_dir = os.path.join(build_dir, 'obj')

env = Environment(CXXFLAGS=['-std=c++14', '-Wno-invalid-offsetof'],
                  CPPPATH=[inc_dir],
                  ENV={'TERM': os.getenv('TERM', 'xterm-256color')})

env.VariantDir(build_dir, '.', duplicate=0)

for libname in ['thrift', 'rdkafka++']:
    env.ParseConfig('pkg-config --cflags --libs ' + libname)

conan_file = 'conanfile.txt'
conan_script = 'SConscript_conan'
conan_generated_files = [
    'conaninfo.txt',
    'conanbuildinfo.txt',
    'conan_imports_manifest.txt'
]

conan_install = env.Command(target=conan_script,
                            source=conan_file,
                            action=['conan install .'])

env.Clean(conan_install, conan_generated_files)

if not env.GetOption('clean'):
    env.Execute(conan_install)


def check_conf():
    check_env = env.Clone()
    check_env.MergeFlags(conan_libs['conan'])

    conf = Configure(check_env)

    if not conf.CheckLibWithHeader('glog', 'glog/logging.h', 'c++'):
        print 'Please install glog library!'
        Exit(1)

    if not conf.CheckCXXHeader('boost/smart_ptr.hpp'):
        print 'Please install boost library!'
        Exit(1)

    if not conf.CheckLibWithHeader('thrift', 'thrift/Thrift.h', 'c++'):
        print 'Please install thrift library!'
        Exit(1)

    if not conf.CheckCXXHeader('rapidjson/rapidjson.h'):
        print 'Please install rapidjson library!'
        Exit(1)

    if not conf.CheckLibWithHeader('rdkafka', 'librdkafka/rdkafkacpp.h', 'c++'):
        print 'Please install rdkafka library!'
        Exit(1)

    if not conf.CheckLibWithHeader('gtest', 'gtest/gtest.h', 'c++'):
        print 'Please install gtest library!'
        Exit(1)

    return conf.Finish()

if not env.GetOption('clean'):
    if not os.path.isfile(conan_script):
        env.Execute(action=['conan install .'])

    conan_libs = SConscript(conan_script)

    check_conf()

    env.MergeFlags(conan_libs['glog'])
    env.MergeFlags(conan_libs['gflags'])
    env.MergeFlags(conan_libs['RapidJSON'])
    env.Replace(LIBS=[lib for lib in env['LIBS'] if not lib.endswith('_main')])
else:
    conan_libs = {'conan': {'LIBS': ['']}}


if debug_mode:
    env.Append(CXXFLAGS=['-g'], CFLAGS=['-g'])
if release_mode:
    env.Append(CXXFLAGS=['-O2'], CFLAGS=['-O2'])


def obj_files(source_files, base_dir, target_dir=obj_dir, env=env):
    for filename in source_files:
        source = os.path.join(base_dir, filename)
        target = os.path.join(target_dir, os.path.splitext(filename)[0])

        yield env.StaticObject(target=target, source=source)

zipkinCoreThrift = os.path.join(src_dir, 'zipkinCore.thrift')
zipkinCoreSources = ['zipkinCore_constants.cpp', 'zipkinCore_types.cpp']
zipkinCoreFiles = [os.path.join(gen_dir, name) for name in zipkinCoreSources]
zipkinCoreObjects = obj_files(zipkinCoreSources, base_dir=gen_dir)

env.Command(target=zipkinCoreFiles,
            source=zipkinCoreThrift,
            action=['thrift -r --gen cpp ' + zipkinCoreThrift])

zipkinSources = ['Span.cpp', 'Tracer.cpp', 'Collector.cpp', 'CApi.cpp']
zipkinObjects = obj_files(zipkinSources, base_dir=src_dir)

zipkinLibObjects = itertools.chain(zipkinCoreObjects, zipkinObjects)

zipkinLib = env.StaticLibrary(target=os.path.join(build_dir, 'zipkin'),
                              source=list(zipkinLibObjects))

test_env = env.Clone()
test_env.MergeFlags(conan_libs['conan'])
test_env.Replace(LIBS=[lib for lib in test_env['LIBS'] if not lib.endswith('_main')])
test_env.Append(CPPPATH=[src_dir])

zipkinTestSources = [
    'TestMain.cpp',
    'TestSpan.cpp',
    'TestTracer.cpp',
    'TestCollector.cpp'
]
zipkinTestObjects = obj_files(source_files=zipkinTestSources,
                              base_dir=test_dir,
                              env=test_env)

unittest = test_env.Program(target=os.path.join(bin_dir, 'unittest'),
                            source=list(zipkinTestObjects) + [zipkinLib])

runtest = test_env.Command(target='runtest',
                           source=unittest,
                           action=['unittest'],
                           DYLD_LIBRARY_PATH=bin_dir,
                           chdir=bin_dir)

test_env.Execute(runtest)

zipkinSimpleProxySources = ['main.c', 'mongoose.c', 'zf_log.c']
zipkinSimpleProxyObjects = obj_files(source_files=zipkinSimpleProxySources,
                                     base_dir=os.path.join(example_dir, 'simple_proxy'),
                                     env=env)

env.Append(LIBS=['c++'])

env.Program(target=os.path.join(bin_dir, 'simple_proxy',),
            source=list(zipkinSimpleProxyObjects) + [zipkinLib])

header_files = list(itertools.chain([Glob(os.path.join(dir, '*.h'))
                                     for dir in [src_dir, inc_dir, gen_dir]]))

doxygen = env.Command(target=doc_dir,
                      source=['Doxyfile'] + header_files,
                      action=['doxygen'])

env.Clean(doxygen, doc_dir)

Depends(doxygen, zipkinLib)

if gen_doc:
    print "Generating API documents ..."
    env.AlwaysBuild(doxygen)
