
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
	#�Զ�������Ŀ�е� .ui �ļ��������ɶ�Ӧ�� ui_*.h �ļ���Ҳ��ͨ�� uic ��������
	set(CMAKE_AUTOUIC ON)
	#�Զ������Ҫʹ��Ԫ����ϵͳ��Meta-Object System�����࣬������ MOC ���ߣ�Meta-Object Compiler��������Ӧ�� moc_*.cpp �ļ��������� Qt ��ʹ���ź���ۻ���ʱ���ɱ�Ҫ�ĸ������롣
	set(CMAKE_AUTOMOC ON)
	#�Զ�������Ŀ�е���Դ�ļ���.qrc �ļ�����ʹ�� rcc ���ߣ�Resource Compiler�������ɶ�Ӧ�� qrc_*.cpp �ļ�������Դ�ļ�����ɿ���Ӧ�ó�����ʹ�õ���Դ�ļ��� 
	set(CMAKE_AUTORCC ON)
endmacro()

macro(MULTI_PROCESS_COMPILE)
	add_definitions("/MP")
endmacro()

# ���ò�ʹ��c++�汾
function(setCXXVersion ver)
	set(CMAKE_CXX_STANDARD ver)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endfunction()


# ����Ŀ¼
function(createDir dirPath)
    if(NOT EXISTS ${dirPath})
        file(MAKE_DIRECTORY ${dirPath})
    endif()
endfunction()

macro(GENERAL_CONFIGURATION)
	# ������ǰĿ¼
	if(CMAKE_VERSION VERSION_LESS "3.7.0")
		set(CMAKE_INCLUDE_CURRENT_DIR ON)
	endif()

	if(PLATFORMTYPE STREQUAL "Windows")
		# �� Windows ƽ̨�ϵ��߼�
		set(BUILD_GENERATOR_NAME "Visual Studio 16 2019")
		set(CMAKE_BUILD_TYPE "Debug")
		set(CMAKE_INSTALL_CONFIG_NAME "Debug")
		set(QTCMAKE_PATH "E:/Develop/tookits/Qt/6.5.3/msvc2019_64")
		#set(QTCMAKE_PATH "D:/DevelopSoftware/Qt/Qt5.14.2/5.14.2/msvc2017/bin")
	elseif(PLATFORMTYPE STREQUAL "Linux")
		# �� Linux ƽ̨�ϵ��߼�
		set(BUILD_GENERATOR_NAME "Unix Makefiles")
		set(CMAKE_BUILD_TYPE "Debug")
		set(CMAKE_INSTALL_CONFIG_NAME "Debug")
		set(QTCMAKE_PATH "/opt/Qt/6.5.0/gcc_64")
	endif()
endmacro()

macro(INCLUDECURRENTDIR)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

set(appendSysValue "" "")

# ��·����ӵ�ϵͳ���������� û��ã���ͷ�ٸ�
macro(addPathToSysVar targetPath varName)
	#message("appendSysValue is ${appendSysValue}")
	#set(newValue "${appendSysValue};${targetPath}")
	#set(appendSysValue ${newValue} PARENT_SCOPE)
	#message("modvalue is ${appendSysValue}")
	set(value $ENV{${varName}})
	if("${value}" STREQUAL "")
		set(value ${targetPath})
	else()
		set(value "$ENV{${varName}};${targetPath}")
	endif()
	execute_process(COMMAND "setx" "${varName}" "${value}" /m)
endmacro()

# ���Qtģ��
function(linkQtModule moduleName)
	message("linkQtModule: Qt${QT_VERSION_MAJOR}::${moduleName}")
	find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${moduleName})
	target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::${moduleName})
	target_include_directories(${PROJECT_NAME} PUBLIC ${Qt${QT_VERSION_MAJOR}${moduleName}_INCLUDE_DIRS})	# ����ͷ�ļ�

	if(0)
	# private ͷ�ļ�
	foreach(curPath ${Qt${QT_VERSION_MAJOR}${moduleName}_INCLUDE_DIRS})
		set(headerpath "${curPath}/${QT_VERSION}")
		if(EXISTS ${headerpath})
		message("include Qt Private headerpath : ${headerpath}")
		target_include_directories(${PROJECT_NAME} PUBLIC ${headerpath})
		#addPathToSysVar(${headerpath} QT_PRIVATE_INCLUDE_PATH)
		endif()
	endforeach()
	endif()

endfunction()

# ����Ŀ����Ŀ
function(buildProject type)
	set(CodeDir ${CMAKE_CURRENT_SOURCE_DIR})
	
	message("addCodeFile: ${CodeDir}")
	
	# �ݹ��������cpp�ļ�
	aux_source_directory(${CodeDir} SourceList)
	#file(GLOB_RECURSE SourceList ${CodeDir}/*.cpp)
	
	# �ݹ��������CodeDir�����ļ����µ� .h��.hpp�ļ�
	file(GLOB_RECURSE HeaderList ${CodeDir}/*.h ${CodeDir}/*.hpp)
	
	if(type STREQUAL "STATIC")
		message("build static library ${PROJECT_NAME}")
		add_library(${PROJECT_NAME} STATIC ${SourceList} ${HeaderList})
	elseif(type STREQUAL "EXE")
	message("build executable ${PROJECT_NAME}")
		add_executable(${PROJECT_NAME} STATIC ${SourceList} ${HeaderList})
	elseif(type STREQUAL "SHARED")
	message("build shared library ${PROJECT_NAME}")
		add_library(${PROJECT_NAME} ${SourceList} ${HeaderList})
	endif()
	
endfunction()

function(setGeneralConfiguration)

	# ͨ�����ã�����·��
	GENERAL_CONFIGURATION()
	
	# C++�汾
	setCXXVersion(17)
	
	# Qt��Դ�ļ��Զ�����
	QT_AUTORESOURCE_COMPILE()
	
	# �ദ��������
	MULTI_PROCESS_COMPILE()
	
	# ����/utf-8 �������
	USE_EXECUTE_CHARSET_UTF8()
	
	# ���ù�������
	ENABLE_QTCREATOR_DEBUG()
	
	# Qt�汾
	set(QT_VERSION 5.14.2 PARENT_SCOPE)
	set(QT_VERSION_MAJOR 6 PARENT_SCOPE)
	message("Current Qt Version : ${QT_VERSION}")
	
endfunction()

#  cmake . -DPLATFORMTYPE:STRING=Windows -A Win32
