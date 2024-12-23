cmake_minimum_required(VERSION 3.0)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
SetGeneralConfiguration()

# 宏定义
add_definitions(-DQTSPY_UTILS)

if(QTVERSION EQUAL 6)
	set(targetPath ${PROJECT_SOURCE_DIR}/../bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${targetPath})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${targetPath})
else()
	set(targetPath ${PROJECT_SOURCE_DIR})
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${targetPath})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${targetPath})
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
	MODULES Core Widgets Test Concurrent
		)

# 包含工程目录
INCLUDEDIRECTORY()

# 添加宏定义
add_definitions(-DQT_SPY_LIB)

# 添加路径到系统环境变量
set($ENV{QT_SPY_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})

## 使用该库，需要在qt工程中添加以下
#QT += testlib
#INCLUDEPATH += $(QT_SPY_INCLUDE)
#LIBS += -L$(QT_SPY_INCLUDE) -lQtSpy
