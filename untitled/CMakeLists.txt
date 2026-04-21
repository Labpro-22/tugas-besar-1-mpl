cmake_minimum_required(VERSION 3.16)

# Use Qt's MinGW toolchain when no compiler is explicitly selected.
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

# Help terminal builds on Windows by detecting a common Qt installation prefix.
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

project(NimonspoliQtUi VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg SvgWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg SvgWidgets)

set(_qt_runtime_prefix "")
if(CMAKE_PREFIX_PATH)
    set(_qt_prefix_candidates_from_cache ${CMAKE_PREFIX_PATH})
    list(GET _qt_prefix_candidates_from_cache 0 _qt_runtime_prefix)
endif()
set(_qt_platform_plugin_dir "${_qt_runtime_prefix}/plugins/platforms")

set(PROJECT_SOURCES
    main.cpp
    src/BoardWidget.cpp
    src/GameWindow.cpp
    src/MonopolyUiShared.cpp
    src/PropertyCardWidget.cpp
    include/BoardWidget.hpp
    include/GameWindow.hpp
    include/MonopolyUiShared.hpp
    include/PropertyCardWidget.hpp
)

set(GAME_CONFIG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/ConfigLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/ConfigData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/PropertyConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/TaxConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/SpecialConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/config/MiscConfig.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(untitled
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET untitled APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(untitled SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(untitled
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_sources(untitled PRIVATE ${GAME_CONFIG_SOURCES})
target_include_directories(untitled PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(untitled PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::SvgWidgets)

if(WIN32)
    get_filename_component(_compiler_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(_mingw_runtime_dlls
        "${_compiler_bin_dir}/libstdc++-6.dll"
        "${_compiler_bin_dir}/libgcc_s_seh-1.dll"
        "${_compiler_bin_dir}/libwinpthread-1.dll"
    )

    add_custom_command(TARGET untitled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Core>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Gui>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Widgets>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Svg>
            $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::SvgWidgets>
            $<TARGET_FILE_DIR:untitled>
    )

    # Copy the images/ folder so icons load at runtime
    add_custom_command(TARGET untitled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/images"
            "$<TARGET_FILE_DIR:untitled>/images"
    )

    add_custom_command(TARGET untitled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/pion"
            "$<TARGET_FILE_DIR:untitled>/pion"
    )

    foreach(_runtime_dll IN LISTS _mingw_runtime_dlls)
        if(EXISTS "${_runtime_dll}")
            add_custom_command(TARGET untitled POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_runtime_dll}"
                    $<TARGET_FILE_DIR:untitled>
            )
        endif()
    endforeach()

    if(EXISTS "${_qt_platform_plugin_dir}/qwindows.dll")
        add_custom_command(TARGET untitled POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:untitled>/platforms
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_platform_plugin_dir}/qwindows.dll"
                $<TARGET_FILE_DIR:untitled>/platforms
        )
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

    add_executable(boardwidget_smoke_test
        tests/BoardWidgetSmokeTest.cpp
        src/BoardWidget.cpp
        src/GameWindow.cpp
        src/MonopolyUiShared.cpp
        src/PropertyCardWidget.cpp
        include/BoardWidget.hpp
        include/GameWindow.hpp
        include/MonopolyUiShared.hpp
        include/PropertyCardWidget.hpp
        ${GAME_CONFIG_SOURCES}
    )
    target_include_directories(boardwidget_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
    target_link_libraries(boardwidget_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::SvgWidgets
        Qt${QT_VERSION_MAJOR}::Test
    )

    if(WIN32)
        add_custom_command(TARGET boardwidget_smoke_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Core>
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Gui>
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Widgets>
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Svg>
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::SvgWidgets>
                $<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>
                $<TARGET_FILE_DIR:boardwidget_smoke_test>
        )

        foreach(_runtime_dll IN LISTS _mingw_runtime_dlls)
            if(EXISTS "${_runtime_dll}")
                add_custom_command(TARGET boardwidget_smoke_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_runtime_dll}"
                        $<TARGET_FILE_DIR:boardwidget_smoke_test>
                )
            endif()
        endforeach()

        if(EXISTS "${_qt_platform_plugin_dir}/qwindows.dll")
            add_custom_command(TARGET boardwidget_smoke_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:boardwidget_smoke_test>/platforms
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qt_platform_plugin_dir}/qwindows.dll"
                    $<TARGET_FILE_DIR:boardwidget_smoke_test>/platforms
            )
        endif()

        add_custom_command(TARGET boardwidget_smoke_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/pion"
                "$<TARGET_FILE_DIR:boardwidget_smoke_test>/pion"
        )
    endif()

    add_test(
        NAME boardwidget_smoke_test
        COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=windows"
            "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:boardwidget_smoke_test>/platforms"
            "PATH=$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>$<SEMICOLON>${_compiler_bin_dir}$<SEMICOLON>$ENV{PATH}"
            $<TARGET_FILE:boardwidget_smoke_test>
    )
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.untitled)
endif()
set_target_properties(untitled PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS untitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled)
endif()
