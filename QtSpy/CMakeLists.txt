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
linkQtModule(Core)
linkQtModule(Widgets)
linkQtModule(Test)

# 包含工程目录
INCLUDECURRENTDIR()

add_definitions(-DQT_SPY_LIB)
addPathToSysVar(${CMAKE_CURRENT_SOURCE_DIR} QT_SPY_INCLUDE)
#addPathToSysVar(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} Path)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

