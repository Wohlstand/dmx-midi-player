cmake_minimum_required(VERSION 3.2...4.0)
project(DmxPlazer VERSION 1.0.0 LANGUAGES C CXX)

#===========================================================================================
# Strip garbage
if(APPLE)
    set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -dead_strip")
    set(LINK_FLAGS_MINSIZEREL  "${LINK_FLAGS_MINSIZEREL} -dead_strip")
elseif(NOT MSVC AND NOT MSDOS AND NOT OPENBSD_LOCALBASE)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fdata-sections -ffunction-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fdata-sections -ffunction-sections")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -fdata-sections -ffunction-sections")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fdata-sections -ffunction-sections")
    string(REGEX REPLACE "-O3" ""
            CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REGEX REPLACE "-O3" ""
            CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O2 -fdata-sections -ffunction-sections")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -fdata-sections -ffunction-sections")
    if(ANDROID)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -funwind-tables")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -funwind-tables")
    endif()
    if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -Wl,--gc-sections -Wl,-s")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -Wl,--gc-sections -Wl,-s")
        set(LINK_FLAGS_RELEASE  "${LINK_FLAGS_RELEASE} -Wl,--gc-sections -Wl,-s")
        set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -s -Wl,--gc-sections -Wl,-s")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s -Wl,--gc-sections -Wl,-s")
        set(LINK_FLAGS_MINSIZEREL  "${LINK_FLAGS_MINSIZEREL} -Wl,--gc-sections -Wl,-s")
    endif()
endif()

if(NOT MSVC AND NOT MSDOS)
# Global optimization flags
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fno-omit-frame-pointer")
# Turn on all warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
# Deny undefined symbols
    if(NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
    endif()
endif()
#===========================================================================================


if(DJGPP)
    set(MSDOS_BUILD TRUE)
endif()

if(NOT MSDOS_BUILD)
    option(ENABLE_ADDRESS_SANITIZER "Enable the Address Sanitizer GCC feature" OFF)
    set(CMAKE_CXX_STANDARD 14)
    set(COMPILER_SUPPORTS_CXX14 ON)
    include(src/fmopl3lib/chips/chipset.cmake)

    find_package(SDL2 REQUIRED)
endif()

add_executable(dmxplay
    src/main.cpp

    src/interface.h

    ${CHIPS_SOURCES}
    src/fmopl3lib/opl3class.h src/fmopl3lib/opl3class.cpp

    src/synthlib/i_oplmusic.h src/synthlib/i_oplmusic.cpp

    # Sequencer
    src/midi_seq.h
    src/midi_seq.cpp
    src/seq/cvt_mus2mid.hpp
    src/seq/cvt_xmi2mid.hpp
    src/seq/file_reader.hpp
    src/seq/fraction.hpp
    src/seq/midi_sequencer.h
    src/seq/midi_sequencer.hpp
    src/seq/midi_sequencer_impl.hpp
    src/emu_list.h
)

if(NOT MSDOS_BUILD AND ENABLE_ADDRESS_SANITIZER)
    target_compile_options(dmxplay PRIVATE -fsanitize=address)
    target_link_libraries(dmxplay PRIVATE -lasan)
endif()

if(MSDOS_BUILD)
    target_compile_definitions(dmxplay PRIVATE -DHW_DOS_BUILD)
else()
    target_include_directories(dmxplay PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(dmxplay PRIVATE ${SDL2_LIBRARIES})
endif()
