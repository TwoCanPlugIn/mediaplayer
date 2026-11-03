# ~~~
# Summary:      Local, generic plugin setup
# Copyright (c) 2020-2021 Mike Rossiter
# License:      GPLv3+
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


# -------- Options ----------

set(OCPN_TEST_REPO
    "steven-adler/mediaplayer-alpha"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "steven-adler/mediaplayer-beta"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "steven-adler/mediaplayer-prod"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

#
#
# -------  Plugin setup --------
#
set(PKG_NAME media_player_pi) #or _pi
set(PKG_VERSION  2.0)
set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME Media Player)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME Media_Player) # As of GetCommonName() in plugin API
set(PKG_SUMMARY "Media Player")
set(PKG_DESCRIPTION "Remote Control for Fusion Media Players")
set(PKG_AUTHOR "twocanplugin@hotmail.com")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://https://github.com/twocanplugin/mediaplayer)
set(PKG_INFO_URL https://opencpn-manuals.github.io/main/engine-dash/0.1/index.html)

set(PKG_API_LIB api-20)   # Specify the plugin API level

add_definitions(-DocpnUSE_GL) # Specifiy the use of OpenGL

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC
${CMAKE_SOURCE_DIR}/src/mediaplayer_dialog.cpp
${CMAKE_SOURCE_DIR}/src/mediaplayer_dialogbase.cpp
${CMAKE_SOURCE_DIR}/src/mediaplayer_plugin.cpp
${CMAKE_SOURCE_DIR}/src/mediaplayer_options.cpp
${CMAKE_SOURCE_DIR}/src/mediaplayer_optionsbase.cpp
${CMAKE_SOURCE_DIR}/src/mediaplayer_optionpages.cpp
)
set (INC
${CMAKE_SOURCE_DIR}/inc/mediaplayer_dialog.h
${CMAKE_SOURCE_DIR}/inc/mediaplayer_dialogbase.h
${CMAKE_SOURCE_DIR}/inc/mediaplayer_plugin.h
${CMAKE_SOURCE_DIR}/inc/mediaplayer_options.h
${CMAKE_SOURCE_DIR}/inc/mediaplayer_optionsbase.h
${CMAKE_SOURCE_DIR}/inc/mediaplayer_optionpages.h
)

set (SOURCE_FILES ${SRC} ${INC})

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  if (APPLE)
    target_compile_definitions(${PACKAGE_NAME} PUBLIC OCPN_GHC_FILESYSTEM)
  endif ()
endmacro ()

macro(add_plugin_libraries)


endmacro ()
