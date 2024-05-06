cmake_minimum_required(VERSION 3.0)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
setGeneralConfiguration()
	
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "E:/code/choice/lib/windows")
	
# build project
buildProject(STATIC)

# link Qt
linkQtModule(Widgets)
linkQtModule(Test)
linkQtModule(Core)

# 包含工程目录
INCLUDECURRENTDIR()

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

