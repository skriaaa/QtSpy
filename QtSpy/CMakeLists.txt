cmake_minimum_required(VERSION 3.5)

# cmake custom utility function
include(utils.cmake)

# project name
project(QtSpy LANGUAGES CXX)

# set generalConfiguration
SetGeneralConfiguration()

# 宏定义
add_definitions(-DQTSPY_UTILS)

# 输出路径
set(targetPath ${PROJECT_SOURCE_DIR}/bin)
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
buildProject(SHARED fileList)

if(PLATFORMTYPE STREQUAL "Windows")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
		set(DETOURS_PATH "E:/develop/github/vcpkg/packages/detours_x64-windows")
		set(DBGHELPER_PATH "C:/Program Files (x86)/Windows Kits/10/Debuggers/lib/x64")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
		set(DETOURS_PATH "E:/develop/github/vcpkg/packages/detours_x86-windows")
		set(DBGHELPER_PATH "C:/Program Files (x86)/Windows Kits/10/Debuggers/lib/x86")
	endif()
	
	# add detours
	target_link_libraries(${PROJECT_NAME} PRIVATE "${DETOURS_PATH}/lib/detours.lib")
	target_include_directories(${PROJECT_NAME} PRIVATE "${DETOURS_PATH}/include")

	# add dbghelp
	target_link_libraries(${PROJECT_NAME} PRIVATE "${DBGHELPER_PATH}/dbghelp.lib")
endif()

# link Qt
linkQtModules(
	MODULES Core Widgets Test Concurrent
		)

# 包含工程目录
INCLUDEDIRECTORY()

# 添加宏定义
add_definitions(-DQT_SPY_LIB)