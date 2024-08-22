cmake_minimum_required(VERSION 3.0)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
SetGeneralConfiguration()
	
if(QTVERSION EQUAL 6)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)	
else()
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

# 添加源文件
findSourceFile(
	FILEARRAYNAME fileList
	SUBDIRS /proxyStyle /dialog /utils /styleSheet
	)

# build project
buildProject(STATIC fileList)

# link Qt
linkQtModules(
	MODULES Core Widgets Test
		)

# 包含工程目录
INCLUDEDIRECTORY()

add_definitions(-DQT_SPY_LIB)
addPathToSysVar(${CMAKE_CURRENT_SOURCE_DIR} QT_SPY_INCLUDE)
#addPathToSysVar(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} Path)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

