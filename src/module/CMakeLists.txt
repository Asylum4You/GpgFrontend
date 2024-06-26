# Copyright (C) 2021 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# The initial version of the source code is inherited from
# the gpg4usb project, which is under GPL-3.0-or-later.
#
# All the source code of GpgFrontend was modified and released by
# Saturneric <eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# define libgpgfrontend_module_sdk
aux_source_directory(sdk MODULE_SDK_SOURCE)

add_library(gpgfrontend_module_sdk SHARED ${MODULE_SDK_SOURCE})
set(_export_file_sdk "${CMAKE_CURRENT_SOURCE_DIR}/sdk/GFSDKExport.h")
generate_export_header(gpgfrontend_module_sdk EXPORT_FILE_NAME "${_export_file_sdk}")
target_include_directories(gpgfrontend_module_sdk PRIVATE 
  ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_module_sdk_autogen/include
  ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

target_include_directories(gpgfrontend_module_sdk PUBLIC sdk)

# link module system
target_link_libraries(gpgfrontend_module_sdk PRIVATE gpgfrontend_core)

if (XCODE_BUILD)
  set_target_properties(gpgfrontend_module_sdk
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif ()

# tracking modules
set(all_integrated_module_libraries "")
file(GLOB children LIST_DIRECTORIES true "mods/*")
foreach(child ${children})
    if(IS_DIRECTORY ${child})
        get_filename_component(dirName ${child} NAME)
        add_subdirectory("mods/${dirName}")
        
        list(APPEND all_integrated_module_libraries ${dirName})
    endif()
endforeach()