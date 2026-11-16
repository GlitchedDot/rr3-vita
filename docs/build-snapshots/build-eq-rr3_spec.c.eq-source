/*
 * rr3_spec.c — GameSpec for Road Rash 3 (EA, 1995), USA/Europe.
 *
 * ROM layout: flat 2 MB at $000000-$1FFFFF, identity-mapped, no SRAM
 * (password saves). Boot/dispatch (from the 68K vector table — see
 * game.toml):
 *   EntryPoint        = $000200   (reset PC)
 *   VInt (VBlank ISR) = $0021B6
 *   HInt (HBlank ISR) = $0024FA
 *
 * Unlike RKA, both interrupt handlers are real ROM code (RKA's H-int is a
 * RAM-installed trampoline), so all three dispatchers are plain static
 * recomp_call_addr() targets.
 *
 * Minimal bring-up spec, modeled on rka/rka_spec.c (the toolchain's first
 * non-Sonic, no-disassembly bring-up): only the three mandatory
 * interrupt/entry dispatchers + ROM identity. No fill_frame_record /
 * per-game TCP commands yet (RR3's WRAM layout is only coarsely mapped, so
 * the Sonic-shaped tracking fields stay 0, exactly as RKA leaves its
 * unknown fields 0). Dispatchers use recomp_call_addr(), which routes
 * through the generated dispatch table (and the Tier-3 floor on a miss).
 */
#include "game_spec.h"
#include "genesis_runtime.h"

#include <stddef.h>
#include <stdint.h>

/* ---- Entry-point / interrupt dispatchers (canonical org-0 addresses) ---- */
static void rr3_call_entry_point(void) { recomp_call_addr(0x000200u); }
static void rr3_call_vblank(void)      { recomp_call_addr(0x0021B6u); }
static void rr3_call_hblank(void)      { recomp_call_addr(0x0024FAu); }

const GameSpec g_game_spec = {
    .display_name           = "Road Rash 3",
    .short_name             = "RR3",

    /* Road Rash III (USA, Europe), MD5 dce9b99a3d692cb4463f9f35d278659b.
     * Other regional dumps have different code/data layouts and cannot
     * share this recomp. */
    .expected_rom_crc32     = 0x15785956u,
    .expected_rom_size      = 0x200000u,   /* 2 MB */

    /* RR3 has no community disasm, so static discovery is heuristic and
     * WILL have holes (computed-dispatch targets the finder cannot see
     * statically). Run the tier-3 miss fallback always-on, same as RKA:
     * a miss executes correctly on the interpreter capsule and feeds
     * floor_coverage.toml for the next regen. */
    .tier3_floor_default    = 1,

    /* No battery SRAM (header carries no "RA"; password game), so
     * sram_start/end stay 0. */

    .call_entry_point       = rr3_call_entry_point,
    .call_vblank            = rr3_call_vblank,
    .call_hblank            = rr3_call_hblank,
    .resume_main_loop_pc    = 0u,    /* main-loop PC unknown — dispatcher re-entry */
    .save_resume_pc         = NULL,
    .dispatch_main_loop_pc  = 0u,
    .call_periodic          = NULL,

    .on_post_reset          = NULL,
    .on_frame_pre           = NULL,
    .on_frame_post          = NULL,
    .on_hblank              = NULL,

    .handle_arg             = NULL,
    .arg_usage              = NULL,
    .dispatch_override      = NULL,

    .fill_frame_record      = NULL,
    .frame_record_version   = 0u,

    .commands               = NULL,
    .command_count          = 0,
};
