cmake_minimum_required(VERSION 3.0)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
SetGeneralConfiguration()
	
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

# 包含boost目录
set(boostPath "E:/迅雷下载/Code/boost_1_86_0")
target_include_directories(${PROJECT_NAME} PUBLIC ${boostPath})

# 添加宏定义
add_definitions(-DQT_SPY_LIB)
add_definitions(-D_SILENCE_CXX17_IS_LITERAL_TYPE_DEPRECATION_WARNING) # boost::hana c17编译警告

# 添加路径到系统环境变量
set($ENV{QT_SPY_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})

## 使用该库，需要在qt工程中添加以下
#QT += testlib
#INCLUDEPATH += $(QT_SPY_INCLUDE)
#LIBS += -L$(QT_SPY_INCLUDE)
