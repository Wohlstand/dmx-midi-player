set(CHIPS_SOURCES
    "src/fmopl3lib/chips/dosbox_opl3.cpp"
    "src/fmopl3lib/chips/dosbox_opl3.h"
    "src/fmopl3lib/chips/esfmu_opl3.cpp"
    "src/fmopl3lib/chips/esfmu_opl3.h"
    "src/fmopl3lib/chips/java_opl3.cpp"
    "src/fmopl3lib/chips/java_opl3.h"
    "src/fmopl3lib/chips/mame_opl2.h"
    "src/fmopl3lib/chips/mame_opl2.cpp"
    "src/fmopl3lib/chips/nuked_opl3.cpp"
    "src/fmopl3lib/chips/nuked_opl3.h"
    "src/fmopl3lib/chips/opal_opl3.cpp"
    "src/fmopl3lib/chips/opal_opl3.h"
    "src/fmopl3lib/chips/opal/opal.c"
    "src/fmopl3lib/chips/opal/opal.h"
    "src/fmopl3lib/chips/esfmu/esfm.c"
    "src/fmopl3lib/chips/esfmu/esfm.h"
    "src/fmopl3lib/chips/esfmu/esfm_registers.c"
    "src/fmopl3lib/chips/nuked/nukedopl3.c"
    "src/fmopl3lib/chips/nuked/nukedopl3.h"
    "src/fmopl3lib/chips/mame/opl.h"
    "src/fmopl3lib/chips/mame/mame_fmopl.cpp"
    "src/fmopl3lib/chips/dosbox/dbopl.cpp"
    "src/fmopl3lib/chips/dosbox/dbopl.h"
    "src/fmopl3lib/chips/nuked_opl3_v174.cpp"
    "src/fmopl3lib/chips/nuked_opl3_v174.h"
    "src/fmopl3lib/chips/nuked/nukedopl3_174.c"
    "src/fmopl3lib/chips/nuked/nukedopl3_174.h"
    "src/fmopl3lib/chips/ymf262_lle.cpp"
    "src/fmopl3lib/chips/ymf262_lle.h"
    "src/fmopl3lib/chips/ymf262_lle/nuked_fmopl3.c"
    "src/fmopl3lib/chips/ymf262_lle/nuked_fmopl3.h"
    "src/fmopl3lib/chips/ymf262_lle/nopl3.c"
    "src/fmopl3lib/chips/ymf262_lle/nopl3.h"
    "src/fmopl3lib/chips/ym3812_lle.cpp"
    "src/fmopl3lib/chips/ym3812_lle.h"
    "src/fmopl3lib/chips/ym3812_lle/nuked_fmopl2.c"
    "src/fmopl3lib/chips/ym3812_lle/nuked_fmopl2.h"
    "src/fmopl3lib/chips/ym3812_lle/nopl2.c"
    "src/fmopl3lib/chips/ym3812_lle/nopl2.h"
)

if(COMPILER_SUPPORTS_CXX14) # YMFM can be built in only condition when C++14 and newer were available
  set(YMFM_SOURCES
      "src/fmopl3lib/chips/ymfm_opl2.cpp"
      "src/fmopl3lib/chips/ymfm_opl2.h"
      "src/fmopl3lib/chips/ymfm_opl3.cpp"
      "src/fmopl3lib/chips/ymfm_opl3.h"
      "src/fmopl3lib/chips/ymfm/ymfm.h"
      "src/fmopl3lib/chips/ymfm/ymfm_opl.cpp"
      "src/fmopl3lib/chips/ymfm/ymfm_opl.h"
      "src/fmopl3lib/chips/ymfm/ymfm_misc.cpp"
      "src/fmopl3lib/chips/ymfm/ymfm_misc.h"
      "src/fmopl3lib/chips/ymfm/ymfm_pcm.cpp"
      "src/fmopl3lib/chips/ymfm/ymfm_pcm.h"
      "src/fmopl3lib/chips/ymfm/ymfm_adpcm.cpp"
      "src/fmopl3lib/chips/ymfm/ymfm_adpcm.h"
      "src/fmopl3lib/chips/ymfm/ymfm_ssg.cpp"
      "src/fmopl3lib/chips/ymfm/ymfm_ssg.h"
  )
  if(DEFINED FLAG_CPP14)
    set_source_files_properties(${YMFM_SOURCES} COMPILE_FLAGS ${FLAG_CPP14})
  endif()
  list(APPEND CHIPS_SOURCES ${YMFM_SOURCES})
endif()

#if(ENABLE_OPL3_PROXY)
#  list(APPEND CHIPS_SOURCES
#    "src/fmopl3lib/chips/win9x_opl_proxy.cpp"
#    "src/fmopl3lib/chips/win9x_opl_proxy.h"
#  )
#endif()
#
#if(ENABLE_SERIAL_PORT)
#  list(APPEND CHIPS_SOURCES
#    "src/fmopl3lib/chips/opl_serial_port.cpp"
#    "src/fmopl3lib/chips/opl_serial_port.h"
#  )
#endif()
#