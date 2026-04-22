cmake_minimum_required(VERSION 3.16)

if(WIN32 AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
    file(GLOB _qt_mingw_gxx "C:/Qt/Tools/mingw*/bin/g++.exe")
    if(_qt_mingw_gxx)
        list(SORT _qt_mingw_gxx ORDER DESCENDING)
        list(GET _qt_mingw_gxx 0 _qt_gxx)
        string(REPLACE "g++.exe" "gcc.exe" _qt_gcc "${_qt_gxx}")
        if(EXISTS "${_qt_gcc}")
            set(CMAKE_C_COMPILER "${_qt_gcc}" CACHE FILEPATH "C compiler" FORCE)
            set(CMAKE_CXX_COMPILER "${_qt_gxx}" CACHE FILEPATH "C++ compiler" FORCE)
            message(STATUS "Using Qt MinGW toolchain: ${CMAKE_CXX_COMPILER}")
        endif()
    endif()
endif()

if(WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(_qt_prefix_candidates
        "C:/Qt/6.11.0/mingw_64"
        "C:/Qt/6.10.0/mingw_64"
        "C:/Qt/6.9.0/mingw_64"
        "C:/Qt/6.8.0/mingw_64"
        "C:/Qt/6.7.0/mingw_64"
        "C:/Qt/6.6.0/mingw_64"
        "C:/Qt/5.15.2/mingw81_64"
        "$ENV{QTDIR}"
    )

    foreach(_candidate IN LISTS _qt_prefix_candidates)
        if(_candidate AND EXISTS "${_candidate}/lib/cmake")
            set(CMAKE_PREFIX_PATH "${_candidate}" CACHE PATH "Qt install prefix" FORCE)
            message(STATUS "Auto-detected Qt at ${CMAKE_PREFIX_PATH}")
            break()
        endif()
    endforeach()
endif()

project(NimonspoliQtUi VERSION 0.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg SvgWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg SvgWidgets)

set(QT_UI_TARGET nimonspoli_qt_ui)
set(QT_CONF_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/qt.conf")

set(PROJECT_SOURCES
    main.cpp
    src/utils/UiCommon.cpp
    src/views/BoardWidget.cpp
    src/views/GameWindow.cpp
    src/views/PropertyCardWidget.cpp
    src/views/PropertyPortfolioWidget.cpp
    include/utils/UiCommon.hpp
    include/views/BoardWidget.hpp
    include/views/GameWindow.hpp
    include/views/PropertyCardWidget.hpp
    include/views/PropertyPortfolioWidget.hpp
)

set(GAME_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/ConfigLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/ConfigData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/PropertyConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/TaxConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/SpecialConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/MiscConfig.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${QT_UI_TARGET}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(${QT_UI_TARGET} SHARED ${PROJECT_SOURCES})
    else()
        add_executable(${QT_UI_TARGET} ${PROJECT_SOURCES})
    endif()
endif()

target_sources(${QT_UI_TARGET} PRIVATE ${GAME_CORE_SOURCES})
target_include_directories(${QT_UI_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(${QT_UI_TARGET} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::SvgWidgets
)

if(WIN32)
    if(CMAKE_PREFIX_PATH)
        set(_qt_prefix_candidates_from_cache ${CMAKE_PREFIX_PATH})
        list(GET _qt_prefix_candidates_from_cache 0 _qt_runtime_prefix)
        set(_qt_platform_plugin_dir "${_qt_runtime_prefix}/plugins/platforms")
    endif()

    get_filename_component(_compiler_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(_mingw_runtime_dlls
        "${_compiler_bin_dir}/libstdc++-6.dll"
        "${_compiler_bin_dir}/libgcc_s_seh-1.dll"
        "${_compiler_bin_dir}/libwinpthread-1.dll"
    )

    add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Core>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Gui>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Widgets>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Svg>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::SvgWidgets>
            $<TARGET_FILE_DIR:${QT_UI_TARGET}>
    )

    add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/images"
            "$<TARGET_FILE_DIR:${QT_UI_TARGET}>/images"
    )

    add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/pion"
            "$<TARGET_FILE_DIR:${QT_UI_TARGET}>/pion"
    )

    add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_CONF_SOURCE}"
            "$<TARGET_FILE_DIR:${QT_UI_TARGET}>/qt.conf"
    )

    foreach(_runtime_dll IN LISTS _mingw_runtime_dlls)
        if(EXISTS "${_runtime_dll}")
            add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_runtime_dll}"
                    $<TARGET_FILE_DIR:${QT_UI_TARGET}>
            )
        endif()
    endforeach()

    if(EXISTS "${_qt_platform_plugin_dir}/qwindows.dll")
        add_custom_command(TARGET ${QT_UI_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${QT_UI_TARGET}>/platforms
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_platform_plugin_dir}/qwindows.dll"
                $<TARGET_FILE_DIR:${QT_UI_TARGET}>/platforms
        )
    endif()
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.nimonspoliqtui)
endif()

set_target_properties(${QT_UI_TARGET} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE FALSE
)

include(GNUInstallDirs)
install(TARGETS ${QT_UI_TARGET}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${QT_UI_TARGET})
endif()
