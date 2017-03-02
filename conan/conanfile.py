import os

from conans import ConanFile, CMake
from conans.errors import ConanException
from conans.tools import download, unzip


class ZipkinConan(ConanFile):
    name = "zipkin"
    version = "0.2.0"
    description = "Zipkin tracing library for C/C++"
    license = "Apache-2.0"
    url = "https://github.com/flier/zipkin-cpp"
    settings = "os", "compiler", "build_type", "arch"
    requires = "glog/0.3.4@eliaskousk/stable", \
        "gflags/2.2.0@eliaskousk/stable", \
        "gtest/1.8.0@eliaskousk/stable", \
        "RapidJSON/1.0.2@SamuelMarks/stable"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "branch": "ANY",
    }
    default_options = 'shared=False', 'fPIC=False', 'branch=develop', 'glog:shared=False', 'gflags:shared=False', 'gtest:shared=False'
    zip_name = "v%s.tar.gz" % version
    unzipped_name = "zipkin-cpp-%s" % version
    exports_sources = "*"

    def config_options(self):
        if self.settings.compiler == "Visual Studio":
            self.options.remove("fPIC")

    def configure(self):
        if self.settings.compiler == "Visual Studio" and \
           self.options.shared and "MT" in str(self.settings.compiler.runtime):
            self.options.shared = False

    def source(self):
        try:
            download("%s/archive/%s" % (self.url, self.zip_name), self.zip_name)
            unzip(self.zip_name)
            os.unlink(self.zip_name)
        except ConanException:
            if not os.path.isdir(self.unzipped_name):
                self.run("git clone %s.git %s" % (self.url, self.unzipped_name))
            elif os.path.isdir(os.path.join(self.unzipped_name, '.git')):
                self.run("git pull", cwd=self.unzipped_name)

            if os.path.isdir(os.path.join(self.unzipped_name, '.git')):
                self.run("git checkout %s" % self.options.branch, cwd=self.unzipped_name)

    def build(self):
        self.run('conan install --build missing', cwd=self.unzipped_name)

        build_dir = os.path.join(self.unzipped_name, 'build')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        cmake = CMake(self.settings)
        self.run('cmake .. %s -DWITH_FPIC=%s -DSHARED_LIB=%s -DCMAKE_INSTALL_PREFIX=%s' % (
            cmake.command_line,
            'ON' if self.options.fPIC else 'OFF',
            'ON' if self.options.shared else 'OFF',
            os.path.join(self.conanfile_directory, 'dist')
        ), cwd=build_dir)
        self.run("cmake --build . --target install %s" % cmake.build_config, cwd=build_dir)

    def package(self):
        self.copy(pattern="*.h", dst="include", src='dist/include')
        self.copy(pattern="*.hpp", dst="include", src='dist/include')
        self.copy(pattern="*.lib", dst="lib", src='dist/lib')
        self.copy(pattern="*.a", dst="lib", src='dist/lib')
        self.copy(pattern="*.pc", dst="lib/pkgconfig", src='dist/lib/pkgconfig')

    def package_info(self):
        self.cpp_info.libs = ["zipkin"]
