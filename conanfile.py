#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os.path
from conans import ConanFile, CMake
from conans.tools import Git
from conans.errors import ConanInvalidConfiguration

class ImguiSfmlConan(ConanFile):
    """ Imgui-SFML library conan file.

    Options:

    * shared: True or False
      Whether to build this library shared or static.

    * fPIC: True or False
      Whether or not to use -fPIC option while building.

    * imconfig: None or String
      Use 'None' if you want this library to provide default
      ImGui's imconfig.h file. Otherwise, provide a path to the
      imconfig.h file you want to include in the build.

    * imconfig_install_folder: None or String
      Use 'None' if you want this build to include custom config
      file, otherwise specify the directory where your imconfig
      will be installed.

    * imgui_revision: String
      Tag or branch of ImGui repository that should be used with
      ImGui-SFML.
    """

    name = 'imgui-sfml'
    version = '2.1'
    description = 'ImGui binding for use with SFML'
    topics = ('conan', 'sfml', 'gui', 'imgui')
    url = 'https://github.com/eliasdaler/imgui-sfml'
    homepage = 'https://github.com/eliasdaler/imgui-sfml'
    author = 'Elias Daler <eliasdaler@yandex.ru>'
    requires = 'sfml/2.5.1@bincrafters/stable'
    license = 'MIT'
    settings = 'os', 'compiler', 'build_type', 'arch'
    options = {
        'shared': [True, False],
        'fPIC': [True, False],
        'imconfig': 'ANY',
        'imconfig_install_folder': 'ANY',
        'imgui_revision': 'ANY'
    }
    default_options = {
        'shared': False,
        'fPIC': True,
        'imconfig': None,
        'imconfig_install_folder': None,
        'imgui_revision': 'v1.70'
    }
    exports_sources = [
        'cmake/FindImGui.cmake',
        'CMakeLists.txt',
        'imconfig-SFML.h',
        'imgui-SFML.cpp',
        'imgui-SFML_export.h',
        'imgui-SFML.h',
    ]
    exports = 'LICENSE'
    _imgui_dir = 'imgui'

    def source(self):
        git = Git(folder=self._imgui_dir)
        git.clone('https://github.com/ocornut/imgui.git', str(self.options.imgui_revision))

    def configure(self):
        imconfig = self.options.imconfig
        if imconfig:
            if not os.path.isfile(str(imconfig)):
                raise ConanInvalidConfiguration("Provided user config is not a file or doesn't exist")
            else:
                self._imconfig_path = os.path.abspath(str(self.options.imconfig))
        if not self.options.imgui_revision:
            raise ConanInvalidConfiguration("ImGui revision is empty. Try latest version tag or 'master'")

        self.options['sfml'].graphics = True
        self.options['sfml'].window = True
        self.options['sfml'].shared = self.options.shared

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions['IMGUI_DIR'] = os.path.join(self.source_folder, self._imgui_dir)
        cmake.definitions['SFML_DIR'] = os.path.join(self.deps_cpp_info['sfml'].lib_paths[0], 'cmake', 'SFML')
        cmake.definitions['IMGUI_SFML_BUILD_EXAMPLES'] = 'OFF'
        cmake.definitions['IMGUI_SFML_FIND_SFML'] = 'ON'
        if self.options.imconfig_install_folder:
            cmake.definitions['IMGUI_SFML_CONFIG_INSTALL_DIR'] = self.options.imconfig_install_folder
        if self.options.imconfig:
            cmake.definitions['IMGUI_SFML_USE_DEFAULT_CONFIG'] = 'OFF'
            cmake.definitions['IMGUI_SFML_CONFIG_NAME'] = os.path.basename(self._imconfig_path)
            cmake.definitions['IMGUI_SFML_CONFIG_DIR'] = os.path.dirname(self._imconfig_path)
        else:
            cmake.definitions['IMGUI_SFML_USE_DEFAULT_CONFIG'] = 'ON'
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ['ImGui-SFML']
