cmake_minimum_required(VERSION 3.31)

# Include version generation before project()
include(cmake/version.cmake)

project("SeaBattle" LANGUAGES CXX VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
qt_standard_project_setup()

add_subdirectory("src")

# CPack configuration for WiX installer
set(CPACK_PACKAGE_NAME "SeaBattle")
set(CPACK_PACKAGE_VENDOR "SeaBattle Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SeaBattle Game")
# Version is set by cmake/version.cmake - CPACK_PACKAGE_VERSION_* variables are already defined
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SeaBattle")

# WiX specific settings
set(CPACK_WIX_UPGRADE_GUID "12345678-1234-1234-1234-123456789ABC")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "SeaBattle")

# License file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/License.rtf")
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/License.rtf")
endif()

# Optional icon file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")
endif()

# Optional banner and dialog images
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/banner.bmp")
    set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/resources/banner.bmp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/dialog.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/resources/dialog.bmp")
endif()

# WiX fragment for shortcuts
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WixFragment.wxs")
    set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WixFragment.wxs")
endif()

# Product information
set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/ooo-me/SeaBattle")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/ooo-me/SeaBattle")

# Enable registry entry
set(CPACK_WIX_CMAKE_PACKAGE_REGISTRY "SeaBattle")

include(CPack)
