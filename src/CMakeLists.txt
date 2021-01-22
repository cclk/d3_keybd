CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

#设置工程名称（工程、解决方案）
PROJECT(d3_keybd)

SET(PROJECT_NAME "keybd")

#额外宏定义
#ADD_DEFINITIONS(-DJHTTP_EXPORT)
IF(WIN32)
    ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
    ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
    ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ENDIF()

#######################################################################################
#ADD_LIBRARY(PROJECT_NAME STATIC ${SRC_FILES})  #编译静态库
#ADD_LIBRARY(PROJECT_NAME SHARED ${SRC_FILES})  #编译动态库
#ADD_EXECUTABLE(PROJECT_NAME ${SRC_FILES})      #编译可执行文件
#######################################################################################

#SOURCE_DIR用来表示存放源码文件总目录
SET(SOURCE_DIR ${PROJECT_SOURCE_DIR})

IF(WIN32)
    SET(INCLUDE_LISTS
        ${SOURCE_DIR}
    )
    SET(LIB_LISTS
        ${SOURCE_DIR}/lib
        ${LIBRARY_OUTPUT_PATH}
    )
    SET(LINK_LISTS
    )
ELSEIF(UNIX)
    SET(INCLUDE_LISTS
        ${SOURCE_DIR}
    )      
    SET(LIB_LISTS
        ${LIBRARY_OUTPUT_PATH}
    )
    SET(LINK_LISTS
    )
ENDIF()

INCLUDE_DIRECTORIES(${INCLUDE_LISTS})   #头文件搜索目录
LINK_DIRECTORIES(${LIB_LISTS})          #链接库搜索路径
include (${SOURCE_DIR}/../nuget.cmake)

#当源码目录下有不需要编译的文件时可以指定后缀名
FILE(GLOB MAIN_SRC "${SOURCE_DIR}/*.c*" "${SOURCE_DIR}/*.h*")
SOURCE_GROUP("src" FILES ${MAIN_SRC})  

#设置自动生成moc文件，一定要设置
SET(CMAKE_AUTOMOC ON)
#设置工程包含当前目录，非必须
set(CMAKE_INCLUDE_CURRENT_DIR ON) 

#查找需要的Qt库文件，最好每一个库都要写，Qt也会根据依赖关系自动添加
find_package(Qt5Widgets)  
find_package(Qt5Core)
find_package(Qt5Gui)

#同理，查找当前文件夹中所有的ui文件
FILE(GLOB UI_FILES "${SOURCE_DIR}/*.ui") 

#通过Ui文件生成对应的头文件，一定要添加
QT5_WRAP_UI(WRAP_FILES ${UI_FILES})

#添加资源文件
SET(RCC_FILES keybd.qrc keybd.ico keybd.rc)

#将ui文件和生成文件整理在一个文件夹中，非必须
SOURCE_GROUP("ui" FILES ${UI_FILES} ${WRAP_FILES} ${RCC_FILES})

#整合要编译的源文件
SET(SOURCE_FILE ${MAIN_SRC} ${UI_FILES} ${WRAP_FILES} ${RCC_FILES})

#编译可执行文件
ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCE_FILE})

#执行文件链接属性
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LINK_LISTS})
#添加Qt5依赖项
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Qt5::Widgets Qt5::Core Qt5::Gui)

IF (WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} winmm.lib)
    INSTALL_NUGET(${PROJECT_NAME} Cvte.jsoncpp.native.static.md 1.7.4.2)
    INSTALL_NUGET(${PROJECT_NAME} Cvte.logger.native.dynamic.md 1.5.0.118)
ENDIF()

#pdb、c11支持
IF(MSVC)
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS "-std=c++11 -g -O2 -pthread -fopenmp")
ENDIF()

#禁用警告
#SET(CMAKE_CXX_FLAGS "/wd4275 ${CMAKE_CXX_FLAGS}")
ADD_DEFINITIONS(-DJSON_HAS_INT64)

