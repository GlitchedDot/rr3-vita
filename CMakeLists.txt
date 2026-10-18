cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
project(rr3vita C CXX)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(ENGINE "$ENV{HOME}/workspace/rr3-runner-work/engine-defer")
# Glue source selection (bisect builds):
#   default      -> engine glue (fix1+fix2+defer)
#   RR3_NODEFER  -> defer queue disabled (build H)
#   RR3_FORCE_INTERP -> hardcoded interp drive (build G)
#   RR3_NOINT    -> H-int delivery disabled (build I)
if(RR3_NODELIV)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_nodeliv.c)
elseif(RR3_NOVINT)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_novint.c)
elseif(RR3_NOINT)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_noint.c)
elseif(RR3_NODEFER)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_nodefer.c)
elseif(RR3_FORCE_INTERP)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_vita.c)
else()
  set(GLUE_SRC ${ENGINE}/runner/glue.c)
endif()


add_compile_definitions(VITA GENESIS_BATCHED_PLANES=0)
add_compile_options(-O2 -fno-strict-aliasing)

# Bisect option (build O): force signed plain char. ARM EABI defaults to
# UNSIGNED char; x86-64 (where every passing run lives) defaults to
# signed. Any engine/generated code that parks a signed value in a plain
# char behaves differently on the Vita than on the PC.
if(RR3_SIGNED_CHAR)
  add_compile_options(-fsigned-char)
endif()

file(GLOB GEN_SRC "${CMAKE_SOURCE_DIR}/generated/*.c")

# Bisect option (build L): interleaved interrupt delivery instead of the
# default atomic model (main.c sets GENESIS_INTERLEAVE_IRQ=1 at boot).
if(RR3_INTERLEAVE)
  add_compile_definitions(RR3_INTERLEAVE)
endif()

# Bisect option (build N): headless phase 3 -- no per-frame display calls,
# controller reads or vblank waits (main.c RR3_HEADLESS3 block).
if(RR3_HEADLESS3)
  add_compile_definitions(RR3_HEADLESS3)
endif()

# Bisect option (build P): whole-program interpreter drive via env
# (main.c sets GENESIS_FORCE_INTERP=1 before the sim thread starts).
if(RR3_FORCE_INTERP_ENV)
  add_compile_definitions(RR3_FORCE_INTERP_ENV)
endif()

# Bisect option (build R): the Z80 core never executes (patched copy of
# genesis_machine.c with step_z80 short-circuited).
if(RR3_NOZ80)
  set(MACHINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/genesis_machine_noz80.c)
else()
  set(MACHINE_SRC ${ENGINE}/runner/video/genesis_machine.c)
endif()

# Bisect option (build U): no display subsystem at all (no CDRAM alloc,
# no present). main.c guards display_init/display_present on it.
if(RR3_NODISPLAY)
  add_compile_definitions(RR3_NODISPLAY)
endif()

# Bisect option (build T): the audio mixer drain never runs (patched copy
# of sim_step.c with audio_mixer_drain removed).
if(RR3_NOAUDIO)
  set(SIMSTEP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/sim_step_noaudio.c)
else()
  set(SIMSTEP_SRC ${ENGINE}/runner/sim_step.c)
endif()

# Bisect option (build K): compile ONLY the generated 68K code at -O0 to
# test whether ARM -O2 codegen of the generated tree is crash-sensitive.
# Engine, glue and front-end stay at the global -O2.
if(RR3_GEN_O0)
  set_source_files_properties(${GEN_SRC} PROPERTIES COMPILE_OPTIONS "-O0")
endif()

add_executable(${PROJECT_NAME}
  src/main.c
  src/vita_compat.c
  src/rr3_spec.c
  src/rr3_extras.c
  src/fiber_compat.c
  ${GEN_SRC}
  ${SIMSTEP_SRC}
  ${GLUE_SRC}
  ${ENGINE}/runner/m68k_interp.c
  ${ENGINE}/runner/chip_trace.c
  ${ENGINE}/runner/crash_report.c
  ${ENGINE}/runner/runtime_evidence.c
  ${ENGINE}/runner/cmd_server_stub.c
  ${MACHINE_SRC}
  ${ENGINE}/runner/video/genesis_bus.c
  ${ENGINE}/runner/video/genesis_vdp.c
  ${ENGINE}/runner/video/color_lut.c
  ${ENGINE}/runner/audio/event_queue.c
  ${ENGINE}/runner/audio/sn76489.c
  ${ENGINE}/runner/audio/mixer.c
  ${ENGINE}/runner/audio/observability.c
  ${ENGINE}/runner/audio/audio_shadow.c
  ${ENGINE}/runner/audio/ym2612_ymfm.cpp
  ${ENGINE}/runner/audio/fm_shadow.cpp
  ${ENGINE}/runner/external/ymfm/src/ymfm_opn.cpp
  ${ENGINE}/runner/external/ymfm/src/ymfm_ssg.cpp
  ${ENGINE}/runner/external/ymfm/src/ymfm_adpcm.cpp
  ${ENGINE}/runner/external/ymfm/src/ymfm_pcm.cpp
  ${ENGINE}/runner/external/superzazu/z80.c
  ${ENGINE}/recompiler/src/m68k_decoder.c
  ${ENGINE}/recompiler/src/rom_parser.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
  generated
  ${ENGINE}/runner
  ${ENGINE}/runner/include
  ${ENGINE}/runner/video
  ${ENGINE}/runner/audio
  ${ENGINE}/runner/external
  ${ENGINE}/runner/external/clowncommon
  ${ENGINE}/runner/external/superzazu
  ${ENGINE}/runner/external/ymfm/src
  ${ENGINE}/recompiler/src
  ${ENGINE}
)

target_link_libraries(${PROJECT_NAME}
  SceCtrl_stub
  SceDisplay_stub
  SceSysmodule_stub
  SceLibKernel_stub
  SceAppMgr_stub
  SceCommonDialog_stub
  z
  m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk "RR3V00001" ${PROJECT_NAME}.self
  VERSION "01.00"
  NAME "Road Rash 3 Recomp"
  FILE ${CMAKE_SOURCE_DIR}/rr3.bin rr3.bin
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
)
