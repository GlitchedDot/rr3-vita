/*
 * rr3_extras.c — per-game extras TU for Road Rash 3 (the sonic_extras.c
 * slot in the Sonic 1 layout).
 *
 * Currently carries exactly one function, and it is an ENGINE gap, not
 * game logic:
 *
 *   genesis_stop_until_interrupt() is declared in
 *   runner/include/genesis_runtime.h and called by generated code for the
 *   68K STOP instruction, but at engine HEAD f875aa8 no runner TU defines
 *   it (the pre-restructure runner/src/runtime.c did; the definition was
 *   lost when the production runner was promoted into the submodule —
 *   engine commit c1cca7a). The historical implementation "forwards to
 *   glue_yield_for_vblank" (engine commit c69ba49), which is what this
 *   definition restores. If the engine grows its own definition later,
 *   delete this file and its CMakeLists entry.
 */
#include "genesis_runtime.h"

#include <stdint.h>

void genesis_stop_until_interrupt(uint16_t sr_imm)
{
    /* The recompiled STOP emission has already written sr_imm into
     * g_cpu.SR before calling us, so the I-mask the next interrupt
     * service compares against is up to date. All that remains is to
     * park the game fiber until the next VBlank service raises an IRQ
     * of sufficient priority — the same yield the WaitForVBlank pattern
     * uses. */
    (void)sr_imm;
    glue_yield_for_vblank();
}
