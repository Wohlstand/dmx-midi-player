include(ExternalProject)
include(GNUInstallDirs)

set_static_lib(SDL2_main_A_Lib "${DEPENDENCIES_INSTALL_DIR}/lib" SDL2main)
set_static_lib(SDL2_A_Lib "${DEPENDENCIES_INSTALL_DIR}/lib" SDL2${LIBRARY_STATIC_NAME_SUFFIX})

set(SDL2_INCLUDE_DIRS
    "${DEPENDENCIES_INSTALL_DIR}/include"
    "${DEPENDENCIES_INSTALL_DIR}/include/SDL2"
)

set(SDL2_LIBRARIES)

if(WIN32 AND MINGW)
    list(APPEND SDL2_LIBRARIES mingw32 "${SDL2_main_A_Lib}" "${SDL2_A_Lib}" )
elseif((WIN32 AND MSVC) OR NINTENDO_3DS OR VITA)
    list(APPEND SDL2_LIBRARIES "${SDL2_main_A_Lib}" "${SDL2_A_Lib}")
else()
    list(APPEND SDL2_LIBRARIES "${SDL2_A_Lib}")
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    list(APPEND SDL2_LIBRARIES
        "version" dbghelp advapi32 kernel32 winmm imm32 setupapi
    )
endif()

if(APPLE)
    macro(xtech_add_macos_library LIBRARY_NAME)
        find_library(MACOS_LIBRARY_${LIBRARY_NAME} ${LIBRARY_NAME})
        if(MACOS_LIBRARY_${LIBRARY_NAME})
            list(APPEND THEXTECH_SYSLIBS ${MACOS_LIBRARY_${LIBRARY_NAME}})
            message("-- Library ${LIBRARY_NAME} found")
        else()
            message("-- Library ${LIBRARY_NAME} NOT found")
        endif()
    endmacro()

    xtech_add_macos_library(CoreAudio)
    xtech_add_macos_library(AudioToolbox)
    xtech_add_macos_library(AudioUnit)
endif()

ExternalProject_Add(
    SDL2_Local
    PREFIX "${CMAKE_BINARY_DIR}/external/sdl2"
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/SDL2"
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}"
        "-DCMAKE_DEBUG_POSTFIX=${CMAKE_DEBUG_POSTFIX}"
        "-DSDL_SHARED=OFF"
        "-DSDL_STATIC=ON"
        "-DSDL_TESTS=OFF"
        "-DSDL_HAPTIC=OFF"
        "-DSDL_JOYSTICK=OFF"
        "-DSDL_VIDEO=OFF"
        "-DSDL_RENDER=OFF"
        "-DSDL_HIDAPI=OFF"
        "-DSDL_POWER=OFF"
        "-DSDL_SENSOR=OFF"
        "-DSDL_CPUINFO=OFF"
        "-DSDL_LOCALE=OFF"
        "-DSDL_MMX=OFF"
        "-DSDL_SSE2=OFF"
        "-DSDL_SSE3=OFF"
        $<$<BOOL:APPLE>:-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}>
        $<$<BOOL:APPLE>:-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}>
    BUILD_BYPRODUCTS
        "${SDL2_main_A_Lib}"
        "${SDL2_A_Lib}"
)
