
macro(USE_NORMAL_WINDOWS)
    SET(CMAKE_MFC_FLAG 0)
endmacro()

macro(USE_STATIC_MFC)
    set(WITH_STATIC_MFC 1)
    SET(CMAKE_MFC_FLAG 1)
    target_link_libraries(${PROJECT_NAME} nafxcw$<$<CONFIG:Debug>:d> libcmt$<$<CONFIG:Debug>:d>)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endmacro()

macro(USE_DYNAMIC_MFC)
    set(WITH_STATIC_MFC 0)
    SET(CMAKE_MFC_FLAG 2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<NOT:$<BOOL:${WITH_STATIC_MFC}>>:_AFXDLL>)
    target_link_libraries(${PROJECT_NAME} mfcs140$<$<CONFIG:Debug>:d> MSVCRT$<$<CONFIG:Debug>:d>)
endmacro()

macro(ENABLE_VISUALSTUDIO_DEBUG)
    if(PLATFORMTYPE STREQUAL "Windows")
        set(CMAKE_BUILD_TYPE "DEBUG")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od")
    endif()
endmacro()

macro(ENABLE_QTCREATOR_DEBUG)
    if(PLATFORMTYPE STREQUAL "Windows")
        set(CMAKE_BUILD_TYPE "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od")
    endif()
endmacro()

macro(AS_SUBSYSTEM_WINDOWS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    #set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endmacro()

macro(USE_CHARSET_UNICODE)
    #if(PLATFORMTYPE STREQUAL "Windows")
        add_definitions(-DUNICODE -D_UNICODE)
    #endif()
endmacro()

macro(USE_CHARSET_MBCS)    
    if(PLATFORMTYPE STREQUAL "Windows")
        add_definitions(-D_MBCS -DMBCS)
    endif()
endmacro()

macro(USE_EXECUTE_CHARSET_UTF8)
    if(PLATFORMTYPE STREQUAL "Windows")
        add_definitions("/utf-8")
    endif()
endmacro()

# resource file auto create
macro(QT_AUTORESOURCE_COMPILE)
	#自动查找项目中的 .ui 文件，并生成对应的 ui_*.h 文件，也可通过 uic 命令生成
	set(CMAKE_AUTOUIC ON)
	#自动检测需要使用元对象系统（Meta-Object System）的类，并运行 MOC 工具（Meta-Object Compiler）生成相应的 moc_*.cpp 文件。这是在 Qt 中使用信号与槽机制时生成必要的辅助代码。
	set(CMAKE_AUTOMOC ON)
	#自动查找项目中的资源文件（.qrc 文件）并使用 rcc 工具（Resource Compiler）来生成对应的 qrc_*.cpp 文件，将资源文件编译成可在应用程序中使用的资源文件。 
	set(CMAKE_AUTORCC ON)
endmacro()

macro(MULTI_PROCESS_COMPILE)
    if(PLATFORMTYPE STREQUAL "Windows")
		add_definitions("/MP")
	endif()
endmacro()

# 设置并使用c++版本
macro(SetCXXVersion17)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro()


# 生成目录
function(createDir dirPath)
    if(NOT EXISTS ${dirPath})
        file(MAKE_DIRECTORY ${dirPath})
    endif()
endfunction()

macro(GENERAL_CONFIGURATION)
	# 包含当前目录
	if(CMAKE_VERSION VERSION_LESS "3.7.0")
		set(CMAKE_INCLUDE_CURRENT_DIR ON)
	endif()

	if(PLATFORMTYPE STREQUAL "Windows")
		# 在 Windows 平台上的逻辑
		set(BUILD_GENERATOR_NAME "Visual Studio 16 2019")
		set(CMAKE_BUILD_TYPE "Debug")
		set(CMAKE_INSTALL_CONFIG_NAME "Debug")
		
		# Qt版本
		if(QTVERSION EQUAL 6)
			set(QT_VERSION 6.5.3)
			#set(QT_VERSION 6.5.3 PARENT_SCOPE)
			set(QT_VERSION_MAJOR 6)
			#set(QT_VERSION_MAJOR 6 PARENT_SCOPE)
			set(QTCMAKE_PATH "E:/Develop/tookits/Qt/6.5.3/msvc2019_64")
		else()
			set(QT_VERSION 5.14.2)
			#set(QT_VERSION 5.14.2 PARENT_SCOPE)
			set(QT_VERSION_MAJOR 5)
			#set(QT_VERSION_MAJOR 5 PARENT_SCOPE)
			set(QTCMAKE_PATH "D:/DevelopSoftware/Qt/Qt5.14.2/5.14.2/msvc2017/bin")
		endif()
		message("Current Qt Version : ${QT_VERSION_MAJOR} ${QT_VERSION}")

		
	elseif(PLATFORMTYPE STREQUAL "Linux")
		# 在 Linux 平台上的逻辑
		set(BUILD_GENERATOR_NAME "Unix Makefiles")
		set(CMAKE_BUILD_TYPE "Debug")
		set(CMAKE_INSTALL_CONFIG_NAME "Debug")
		set(QT_VERSION 5.14.2)
		set(QTCMAKE_PATH "/opt/Qt${QT_VERSION}/${QT_VERSION}/gcc_64")
		set(QT_VERSION_MAJOR 5)
		set(QT5_DIR "/opt/Qt${QT_VERSION}/${QT_VERSION}/gcc_64")
		set(CMAKE_PREFIX_PATH ${QTCMAKE_PATH})
		message("Current Qt Path : ${CMAKE_PREFIX_PATH}")
		message("Current Qt Version : ${QT_VERSION_MAJOR} ${QT_VERSION}")
	endif()
endmacro()

macro(INCLUDEDIRECTORY)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

set(appendSysValue "" "")

# 将路径添加到系统环境变量中
macro(addPathToSysVar targetPath varName)
	set(value $ENV{${varName}})
	if("${value}" STREQUAL "")
		set(value ${targetPath})
	else()
		set(value "$ENV{${varName}};${targetPath}")
	endif()
	execute_process(COMMAND "setx" "${varName}" "${value}" /m)
endmacro()

# 添加Qt模块
function(linkQtModules)
	message("\n*****************linkQtModules begin*****************\n")
	message("Current Qt Version : ${QT_VERSION_MAJOR} ${QT_VERSION}")
	
	set(options)
	set(oneValueArgs)
	set(multiValueArgs MODULES)
	cmake_parse_arguments(MY_ARGS
		"${options}"   			# options
		"${oneValueArgs}"   	# one_value_keywords
		"${multiValueArgs}"     # multi_value_keywords
		${ARGV})
	
	foreach(moduleName ${MY_ARGS_MODULES})
		find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${moduleName})
		target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::${moduleName})
		target_include_directories(${PROJECT_NAME} PUBLIC ${Qt${QT_VERSION_MAJOR}${moduleName}_INCLUDE_DIRS})	# 常规头文件
		message("linkQtModule: Qt${QT_VERSION_MAJOR}::${moduleName}")
	endforeach()

	if(0)
	# private 头文件
	foreach(curPath ${Qt${QT_VERSION_MAJOR}${moduleName}_INCLUDE_DIRS})
		set(headerpath "${curPath}/${QT_VERSION}")
		if(EXISTS ${headerpath})
		message("include Qt Private headerpath : ${headerpath}")
		target_include_directories(${PROJECT_NAME} PUBLIC ${headerpath})
		#addPathToSysVar(${headerpath} QT_PRIVATE_INCLUDE_PATH)
		endif()
	endforeach()
	endif()
	message("\n*****************linkQtModules end*****************\n")
endfunction()

function(findSourceFile)
	message("\n*****************collect files begin*****************\n")
	set(options)
	set(oneValueArgs FILEARRAYNAME)
	set(multiValueArgs SUBDIRS)
	cmake_parse_arguments(MY_ARGS
		"${options}"   			# options
		"${oneValueArgs}"   	# one_value_keywords
		"${multiValueArgs}"     # multi_value_keywords
		${ARGV})
	
	# 当前工程目录下的cpp文件
	aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" SourceList)
	list(LENGTH SourceList length) 
	message ("ProjectDir SourceFileCount : ${length}")
		
	# 递归子目录下的cpp文件
	foreach(dir ${MY_ARGS_SUBDIRS})
		file(GLOB_RECURSE arrSubFile ${CMAKE_CURRENT_SOURCE_DIR}${dir}/*.cpp)
		list(LENGTH arrSubFile length) 
		message ("SubDir[${dir}] SourceFileCount : ${length}")
		list(APPEND SourceList ${arrSubFile})
	endforeach()
	
	file(GLOB_RECURSE HeaderList ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp)
	list(LENGTH HeaderList length) 
	message("Project All HeaderFileCount : ${length}")	
	list(LENGTH SourceList length) 
	message("Project All SourceFileCount : ${length}")

	list(APPEND arrFile ${SourceList} ${HeaderList})
	set(${MY_ARGS_FILEARRAYNAME} ${arrFile} PARENT_SCOPE)
	
	message("\n*****************collect files end*****************\n")
endfunction()

# 构建目标项目
function(buildProject type arrFile)
	set(multiValueArgs SUB_VALUES DEFUALT_VALUES)
	if(type STREQUAL "STATIC")
		message("build static library ${PROJECT_NAME}")
		add_library(${PROJECT_NAME} STATIC ${${arrFile}})
	elseif(type STREQUAL "EXE")
	message("build executable ${PROJECT_NAME}")
		add_executable(${PROJECT_NAME} STATIC ${${arrFile}})
	elseif(type STREQUAL "SHARED")
	message("build shared library ${PROJECT_NAME}")
		add_library(${PROJECT_NAME} ${${arrFile}})
	endif()
	
endfunction()

macro(SetGeneralConfiguration)

	# 通用设置：查找路径
	GENERAL_CONFIGURATION()
	
	# C++版本
	SetCXXVersion17()
	
	# Qt资源文件自动编译
	QT_AUTORESOURCE_COMPILE()
	
	# 多处理器编译
	MULTI_PROCESS_COMPILE()
	
	# 按照/utf-8 编码编译
	USE_EXECUTE_CHARSET_UTF8()
	
	# 配置工程属性
	ENABLE_QTCREATOR_DEBUG()

endmacro()

#  cmake . -DPLATFORMTYPE:STRING=Windows -A Win32
