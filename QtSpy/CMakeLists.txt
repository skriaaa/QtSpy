cmake_minimum_required(VERSION 3.0)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
SetGeneralConfiguration()

# 宏定义
add_definitions(-DQTSPY_UTILS)

# 输出路径
set(targetPath ${PROJECT_SOURCE_DIR})
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${targetPath})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${targetPath})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${targetPath})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${targetPath})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${targetPath})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${targetPath})

# 添加源文件
findSourceFile(
	FILEARRAYNAME fileList
	SUBDIRS /proxyStyle /dialog /utils /styleSheet
	)

# build project
buildProject(STATIC fileList)

# link Qt
linkQtModules(
	MODULES Core Widgets Test Concurrent
		)

# 包含工程目录
INCLUDEDIRECTORY()

# 添加宏定义
add_definitions(-DQT_SPY_LIB)

# 添加路径到系统环境变量
addPathToSysVar(${CMAKE_CURRENT_SOURCE_DIR} QT_SPY_INCLUDE)