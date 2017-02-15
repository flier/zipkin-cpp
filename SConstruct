import os
import os.path
import itertools

from SCons.Script import SConscript, Environment, ARGUMENTS

debug_mode = int(ARGUMENTS.get('debug', 0))
build_dir = ARGUMENTS.get('build_dir', 'build')
gen_dir = ARGUMENTS.get('gen_dir', 'gen-cpp')

src_dir = 'src'
obj_dir = os.path.join(build_dir, 'obj')


def obj_files(source_files, base_dir, target_dir=obj_dir):
    for filename in source_files:
        source = os.path.join(base_dir, filename)
        target = os.path.join(target_dir, os.path.splitext(filename)[0])

        yield env.StaticObject(target=target, source=source)

env = Environment(CXXFLAGS=['-std=c++11'],
                  ENV={'TERM': os.getenv('TERM', 'xterm-256color')})

env.VariantDir(build_dir, '.', duplicate=0)

conan_file = 'conanfile.txt'
conan_script = 'SConscript_conan'

env.Command(target=conan_script,
            source=conan_file,
            action=['conan install .'])

env.MergeFlags(SConscript(conan_script)['conan'])

if debug_mode:
    env.Append(CXXFLAGS=['-g'])
else:
    env.Append(CXXFLAGS=['-O2'])

zipkinCoreThrift = os.path.join(src_dir, 'zipkinCore.thrift')
zipkinCoreSources = ['zipkinCore_constants.cpp', 'zipkinCore_types.cpp']
zipkinCoreFiles = [os.path.join(gen_dir, name) for name in zipkinCoreSources]
zipkinCoreObjects = list(obj_files(zipkinCoreSources, base_dir=gen_dir))

zipkinSources = ['Span.cpp', 'Tracer.cpp', 'Collector.cpp']
zipkinObjects = list(obj_files(zipkinSources, base_dir=src_dir))

env.Command(target=zipkinCoreFiles,
            source=zipkinCoreThrift,
            action=['thrift -r --gen cpp ' + zipkinCoreThrift])

env.StaticLibrary(target=os.path.join(build_dir, 'zipkin'),
                  source=zipkinCoreObjects + zipkinObjects)
