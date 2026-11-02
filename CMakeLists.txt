# SPDX-FileCopyrightText: 2026 LingmoOS Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(lingmo-clock VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CTest)

find_package(LingmoSDK REQUIRED)

# The clock's arithmetic: formatting, the stopwatch, the timer and the world
# clock, with no UI so the tests can drive it directly.
add_library(clock-core STATIC
    src/model/clocktime.cpp
    src/model/countdown_timer.cpp
    src/model/stopwatch.cpp
    src/model/timeformat.cpp
    src/model/worldclock.cpp
)
target_include_directories(clock-core PUBLIC src)
target_link_libraries(clock-core PUBLIC Lingmo::SDKBase)

add_library(clock-ui STATIC
    src/clockwindow.cpp
    src/views/alarmpage.cpp
    src/views/analogclock.cpp
    src/views/clockpage.cpp
    src/views/stopwatchpage.cpp
    src/views/timerpage.cpp
    src/views/worldclockpage.cpp
    src/views/widgets.cpp
)
target_include_directories(clock-ui PUBLIC src)
target_link_libraries(clock-ui PUBLIC clock-core Lingmo::SDK)

add_executable(lingmo-clock src/main.cpp)
target_compile_definitions(lingmo-clock PRIVATE CLOCK_VERSION="${PROJECT_VERSION}")
target_link_libraries(lingmo-clock PRIVATE clock-ui)

install(TARGETS lingmo-clock RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES icons/lingmo-clock.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

lingmo_add_translations(lingmo-clock SOURCES src DESKTOP_FILES lingmo-clock.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lingmo-clock.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
