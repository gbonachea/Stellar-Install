cmake_minimum_required(VERSION 3.16)

project(StellarInstall VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(STELLAR_PORTABLE_RPATH
       "Embed a relative RPATH so the binary can run from a portable folder" OFF)

# Qt 6 es la opción preferente; si no está disponible se usa Qt 5.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/DebParser.cpp
    src/DebParser.h
    src/PackageManager.cpp
    src/PackageManager.h
    src/InstalledPackageChecker.cpp
    src/InstalledPackageChecker.h
    src/DebPackageInfo.h
    resources/resources.qrc
)

add_executable(stellar-install ${PROJECT_SOURCES})

target_include_directories(stellar-install PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(stellar-install PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_compile_definitions(stellar-install PRIVATE APP_VERSION="${PROJECT_VERSION}")

if(STELLAR_PORTABLE_RPATH AND UNIX AND NOT APPLE)
    target_link_options(stellar-install PRIVATE
        "LINKER:--disable-new-dtags"
        "LINKER:-rpath,$ORIGIN/lib")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stellar-install PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)

install(TARGETS stellar-install RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/stellar-install.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
set(ICON_SIZES 16 24 32 48 64 128 256 512)
foreach(ICON_SIZE ${ICON_SIZES})
    install(FILES
            resources/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps/stellar-install.png
            DESTINATION
            ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps)
endforeach()
