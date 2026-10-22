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
# Bisect option (build AE): first-tick ladder markers around the fiber
# switch and inside the interpreter drive loop (patched copy of glue.c).
if(RR3_GSTAGES)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_stages.c)
elseif(RR3_THROTTLE)
  set(GLUE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_throttle.c)
elseif(RR3_NODELIV)
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

# Bisect option (build AL+): native-death function mask. Guest functions
# at/above RR3_MASK_LO run through the interpreter capsule (patched copy
# of the generated dispatch). Pass -DRR3_MASK_LO=0x... to enable.
if(DEFINED RR3_MASK_LO)
  add_compile_definitions(RR3_MASK_LO=${RR3_MASK_LO})
  if(DEFINED RR3_MASK_HI)
    add_compile_definitions(RR3_MASK_HI=${RR3_MASK_HI})
  endif()
  if(DEFINED RR3_MASK2_LO)
    add_compile_definitions(RR3_MASK2_LO=${RR3_MASK2_LO})
  endif()
  if(DEFINED RR3_MASK2_HI)
    add_compile_definitions(RR3_MASK2_HI=${RR3_MASK2_HI})
  endif()
  if(DEFINED RR3_MASK3_LO)
    add_compile_definitions(RR3_MASK3_LO=${RR3_MASK3_LO})
  endif()
  if(DEFINED RR3_MASK3_HI)
    add_compile_definitions(RR3_MASK3_HI=${RR3_MASK3_HI})
  endif()
  if(DEFINED RR3_MASK3_GATE_TICK)
    add_compile_definitions(RR3_MASK3_GATE_TICK=${RR3_MASK3_GATE_TICK})
  endif()
  if(DEFINED RR3_MASK4_LO)
    add_compile_definitions(RR3_MASK4_LO=${RR3_MASK4_LO})
  endif()
  if(DEFINED RR3_MASK4_HI)
    add_compile_definitions(RR3_MASK4_HI=${RR3_MASK4_HI})
  endif()
  if(DEFINED RR3_MASK4_GATE_TICK)
    add_compile_definitions(RR3_MASK4_GATE_TICK=${RR3_MASK4_GATE_TICK})
  endif()
  if(DEFINED RR3_MASK5_LO)
    add_compile_definitions(RR3_MASK5_LO=${RR3_MASK5_LO})
  endif()
  if(DEFINED RR3_MASK5_HI)
    add_compile_definitions(RR3_MASK5_HI=${RR3_MASK5_HI})
  endif()
  if(DEFINED RR3_MASK5_GATE_TICK)
    add_compile_definitions(RR3_MASK5_GATE_TICK=${RR3_MASK5_GATE_TICK})
  endif()
  list(REMOVE_ITEM GEN_SRC ${CMAKE_SOURCE_DIR}/generated/rr3_dispatch.c)
  list(APPEND GEN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/rr3_dispatch_mask.c)
endif()

# Build identity stamp (Lola's request, build CF onward): the on-screen
# top-right readout shows BUILD <label> and the running build count.
if(DEFINED RR3_BUILD_LABEL)
  add_compile_definitions(RR3_BUILD_LABEL=${RR3_BUILD_LABEL})
endif()
if(DEFINED RR3_BUILD_NUMBER)
  add_compile_definitions(RR3_BUILD_NUMBER=${RR3_BUILD_NUMBER})
endif()

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

# Bisect option (build AB): fine-grain phase-1 breadcrumbs — a full
# telemetry line after EVERY phase-1 tick (plus a loop-entry marker),
# instead of every 60 ticks, to localize where in the first 60 ticks
# the interpreter death happens.
if(RR3_FINEGRAIN)
  add_compile_definitions(RR3_FINEGRAIN)
endif()

# Bisect option (build R): the Z80 core never executes (patched copy of
# genesis_machine.c with step_z80 short-circuited).
# Bisect option (build AD): first-tick markers inside machine_run_frame
# (patched copy of genesis_machine.c with per-stage/per-line notes).
if(RR3_NOZ80)
  set(MACHINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/genesis_machine_noz80.c)
elseif(RR3_MSTAGES)
  set(MACHINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/genesis_machine_stages.c)
else()
  set(MACHINE_SRC ${ENGINE}/runner/video/genesis_machine.c)
endif()

# Bisect option (build Z): VDP DMA never transfers (patched copy of
# genesis_vdp.c with all three DMA flavors stubbed at maybe_start_dma;
# busy flags still cycle synchronously, so game polls behave).
if(RR3_NODMA)
  set(VDP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/genesis_vdp_nodma.c)
else()
  set(VDP_SRC ${ENGINE}/runner/video/genesis_vdp.c)
endif()

# Bisect option (build U): no display subsystem at all (no CDRAM alloc,
# no present). main.c guards display_init/display_present on it.
if(RR3_NODISPLAY)
  add_compile_definitions(RR3_NODISPLAY)
endif()

# Bisect option (build AC): stage markers inside the first tick (patched
# copy of sim_step.c with a breadcrumb between every stage).
if(RR3_STAGES)
  set(SIMSTEP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/sim_step_stages.c)
elseif(RR3_NOAUDIO)
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

# Bisect option (build X): no worker thread — the sim runs inline on
# the main thread (main_mainthread.c calls sim_thread directly).
if(RR3_MAINTHREAD)
  set(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/main_mainthread.c)
else()
  set(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
endif()

add_executable(${PROJECT_NAME}
  ${MAIN_SRC}
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
  ${VDP_SRC}
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

# Bisect option (build AA): compile ONLY the engine core + front-end at
# -O0 (the generated 68K code stays at the global -O2). Build K tested
# the mirror image (generated at -O0, engine at -O2) and still crashed;
# the engine core has run at ARM -O2 in every build so far.
if(RR3_ENG_O0)
  get_target_property(RR3_ALL_SRCS ${PROJECT_NAME} SOURCES)
  set(RR3_ENG_SRCS ${RR3_ALL_SRCS})
  list(REMOVE_ITEM RR3_ENG_SRCS ${GEN_SRC})
  set_source_files_properties(${RR3_ENG_SRCS} PROPERTIES COMPILE_OPTIONS "-O0")
endif()

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
