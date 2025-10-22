cmake_minimum_required(VERSION 3.2...4.0)
project(DmxPlazer VERSION 1.0.0 LANGUAGES C CXX)

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
