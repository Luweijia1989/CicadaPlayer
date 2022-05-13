cmake_minimum_required(VERSION 3.15)
project(YppPlayer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC TRUE)

message("QTDIR set by user, path is=${QTDIR}")
list(APPEND CMAKE_PREFIX_PATH "${QTDIR}")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickWidgets REQUIRED)

set(QRC
	player.qrc)
	
qt5_add_resources(player_QRC_SOURCES ${QRC})

list(APPEND SRC_FILE
        main.cpp
		glwidget.cpp
		glwidget.h
		qmlrender.cpp
		qmlrender.h
        )

add_executable(YppPlayer
        ${SRC_FILE}
		${player_QRC_SOURCES})
target_include_directories(YppPlayer PUBLIC ../mediaPlayer)

target_link_libraries(YppPlayer PRIVATE
        media_player
		Qt5::Quick
        Qt5::QuickWidgets
        Qt5::Widgets)