/* sim_step_stages.c -- patched copy of the engine's sim_step.c for
 * build AC (stage bisect). Identical logic; the only change is a
 * breadcrumb marker between every stage of the FIRST tick
 * (genesis_sim_tick_count()==0), so a death inside tick 1 names the
 * stage it died in: stg0 entry, stg1 frame-sync reset done,
 * stg2 game frame (68K execution) done, stg3 raster done,
 * stg4 audio drain done, stg5 vblank bookkeeping done. */
#include "sim_step.h"

#include <stdio.h>
#include <string.h>

#include "glue.h"
#include "genesis_runtime.h"
#include "audio/mixer.h"
#include "video/genesis_bus.h"
#include "video/genesis_machine.h"

extern uint64_t g_chunk_yield_count;
static void stg_note(const char *tag)
{
    if (genesis_sim_tick_count() != 0) return;
    FILE *b = fopen("ux0:/data/rr3vita/breadcrumb.txt", "a");
    if (!b) return;
    fprintf(b, "%s tick=%u pc=%06X a7=%06X insn=%llu yld=%llu\n",
            tag, (unsigned)genesis_sim_tick_count(),
            g_cpu.PC & 0xFFFFFFu, g_cpu.A[7] & 0xFFFFFFu,
            (unsigned long long)g_native_insn_count,
            (unsigned long long)g_chunk_yield_count);
    fclose(b);
}

/* One NTSC wall frame of master cycles: the audio chips advance this far every
 * tick whatever the 68K did (see main.c's comment history on why the drain is
 * the wall frame, not g_audio_cycle_counter). */
#define GENESIS_SIM_WALL_FRAME_MASTER_CYCLES 895780u

/* glue.c frame bookkeeping (not in glue.h; main.c historically declared
 * them at the call site). */
extern void glue_reset_frame_sync(void);
extern void glue_run_game_frame(void);
extern void glue_service_vblank(void);

static GenesisSimInput s_in;
static uint32_t        s_ticks;

/* Drain target when the caller supplies none: the chips must still advance
 * (a NULL mixer output skips ym2612_render/psg_render, i.e. the chip clock). */
#define SIM_SCRATCH_FM_FRAMES  4096
#define SIM_SCRATCH_PSG_FRAMES 16384
static int16_t s_scratch_fm[SIM_SCRATCH_FM_FRAMES * 2];
static int16_t s_scratch_psg[SIM_SCRATCH_PSG_FRAMES];

uint16_t genesis_sim_pad(int player)
{
    return (player >= 0 && player < GENESIS_SIM_MAX_PLAYERS) ? s_in.pad[player] : 0u;
}

uint32_t genesis_sim_human_mask(void) { return s_in.human_mask; }
const GenesisSimInput *genesis_sim_input(void) { return &s_in; }
void genesis_sim_input_restore(const GenesisSimInput *in) { if (in) s_in = *in; }
uint32_t genesis_sim_tick_count(void) { return s_ticks; }
void genesis_sim_set_tick_count(uint32_t n) { s_ticks = n; }

int genesis_sim_step(const GenesisSimInput *in, const GenesisSimAudio *audio,
                     const GenesisSimHooks *hooks)
{
    static const GenesisSimHooks no_hooks;
    if (!hooks) hooks = &no_hooks;
    if (in) s_in = *in;

    stg_note("stg0-entry");
    glue_reset_frame_sync();
    stg_note("stg1-sync");
    glue_run_game_frame();
    stg_note("stg2-gameframe");
    for (int port = 0; port < 2; port++) {
        machine_set_pad_type(port, s_in.pad_type[port]);
        machine_set_pad(port, s_in.pad[port]);
    }
    if (hooks->pre_raster) hooks->pre_raster(hooks->ctx);
    glue_sched_frame_begin();        /* scheduler rule: busy-wait streaks restart */
    stg_note("stg2b-sched");
    machine_run_frame(hooks->sink, hooks->sink_user);
    stg_note("stg3-raster");

    {
        GenesisSimAudio scratch = { s_scratch_fm, SIM_SCRATCH_FM_FRAMES, NULL,
                                    s_scratch_psg, SIM_SCRATCH_PSG_FRAMES, NULL };
        const GenesisSimAudio *a = (audio && audio->fm && audio->psg) ? audio : &scratch;
        size_t fm_n = 0, psg_n = 0;
        audio_mixer_drain(GENESIS_SIM_WALL_FRAME_MASTER_CYCLES,
                          a->fm,  a->fm_cap,  &fm_n,
                          a->psg, a->psg_cap, &psg_n);
        if (a->fm_count)  *a->fm_count  = fm_n;
        if (a->psg_count) *a->psg_count = psg_n;
    }
    stg_note("stg4-drain");
    if (hooks->post_drain && !hooks->post_drain(hooks->ctx))
        return 0;

    glue_service_vblank();
    glue_end_of_wall_frame();
    stg_note("stg5-vblank");
    s_ticks++;
    return 1;
}
