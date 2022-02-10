cmake_minimum_required(VERSION 3.15)
project(YppPlayer)

set(CMAKE_CXX_STANDARD 11)

if (QTDIR OR DEFINED ENV{QTDIR} OR DEFINED ENV{QTDIR32} OR DEFINED ENV{QTDIR64})
	message("QTDIR set by user, path is=${QTDIR}")
else()
	set(QTDIR "" CACHE PATH "Path to Qt (e.g. C:/Qt/5.7/msvc2015_64)")
	message(WARNING "QTDIR variable is missing.  Please set this variable to specify path to Qt (e.g. C:/Qt/5.7/msvc2015_64)")
endif()

if(DEFINED QTDIR${_lib_suffix})
	list(APPEND CMAKE_PREFIX_PATH "${QTDIR${_lib_suffix}}")
elseif(DEFINED QTDIR)
	list(APPEND CMAKE_PREFIX_PATH "${QTDIR}")
elseif(DEFINED ENV{QTDIR${_lib_suffix}})
	list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR${_lib_suffix}}")
elseif(DEFINED ENV{QTDIR})
	list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickWidgets REQUIRED)

list(APPEND SRC_FILE
        main.cpp
		glwidget.cpp
		glwidget.h
        )

add_executable(YppPlayer
        ${SRC_FILE})
target_include_directories(YppPlayer PUBLIC ../mediaPlayer)

target_link_libraries(YppPlayer PRIVATE
        media_player
		Qt5::Quick
        Qt5::QuickWidgets
        Qt5::Widgets)