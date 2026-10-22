/* RR3 Vita front-end — build D (presenter bisect).
 *
 * Build D vs C: the vita2d/GXM presenter is GONE. Phase 3 draws by
 * CPU-blitting the framebuffer into a CDRAM display buffer and
 * flipping with sceDisplaySetFrameBuf — no GPU draw submission at
 * all. Everything else (engine, fibers, markers, breadcrumb) is
 * identical to build C. Purpose: builds A/B/C all died in racing at
 * an ordinary cycle-counter access; if D survives racing, the
 * GPU/vita2d path is indicted; if D dies the same way, it is cleared.
 *
 * Boots the segagenesisrecomp sim core (machine_init + glue_init, the
 * same sequence as the PC runner's main.c), then:
 *   phase 1: 600 headless ticks (no sink), timed — the pure-sim cost.
 *   phase 2: 300 ticks with the sink live, timed — sim + render cost.
 *   phase 3: interactive loop, vblank-paced, sceCtrl -> Genesis pads.
 * Audio chips are drained into scratch buffers (advanced, not played).
 */
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "font8x8.h"
#include "glue.h"
#include "genesis_runtime.h"
#include "sim_step.h"
#include "video/genesis_bus.h"
#include "video/genesis_machine.h"

#define FB_W 320
#define FB_H 224

#define SCR_W 960
#define SCR_H 544

static uint32_t s_fb[FB_H][FB_W];

static void sink_row(void *user, int line, const uint32_t *argb, int width)
{
    (void)user;
    if (line < 0 || line >= FB_H) return;
    if (width > FB_W) width = FB_W;
    /* Machine hands us 0xAARRGGBB words; the display buffer format
     * (A8B8G8R8) wants R in the low byte -> swap R and B. */
    uint32_t *dst = s_fb[line];
    for (int x = 0; x < width; x++) {
        uint32_t v = argb[x];
        dst[x] = (v & 0xFF00FF00u) | ((v >> 16) & 0xFFu) | ((v & 0xFFu) << 16);
    }
}

static int16_t s_fm[4096 * 2];
static int16_t s_psg[16384];
static size_t  s_fm_count, s_psg_count;

static GenesisSimAudio s_audio = { s_fm, 4096, &s_fm_count, s_psg, 16384, &s_psg_count };
static GenesisSimHooks s_hooks = { NULL, sink_row, NULL, NULL, NULL };

extern unsigned rr3_fiber_markers(void);
extern int glue_interp_seen_count(void);
/* Accumulators for the crash hunt (build M): if deaths cluster at a
 * similar instruction/yield count across activities, the killer is
 * something that accumulates with engine work. */
extern uint64_t g_native_insn_count;
extern uint64_t g_chunk_yield_count;
static void breadcrumb(const char *tag, double roll_ms)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < FB_H * FB_W; i += 97) { h ^= ((const uint32_t *)s_fb)[i]; h *= 16777619u; }
    FILE *b = fopen("ux0:/data/rr3vita/breadcrumb.txt", "a");
    if (!b) return;
    fprintf(b, "%s tick=%u pc=%06X a7=%06X fbh=%08X roll=%.2f mark=%X seen=%d insn=%llu yld=%llu\n",
            tag, (unsigned)genesis_sim_tick_count(), g_cpu.PC & 0xFFFFFFu,
            g_cpu.A[7] & 0xFFFFFFu, h, roll_ms,
            rr3_fiber_markers(), glue_interp_seen_count(),
            (unsigned long long)g_native_insn_count,
            (unsigned long long)g_chunk_yield_count);
    fclose(b);
}

static uint16_t read_pad(void)
{
    SceCtrlData c;
    memset(&c, 0, sizeof c);
    sceCtrlReadBufferPositive(0, &c, 1);
    uint16_t p = 0;
    if (c.buttons & SCE_CTRL_UP)       p |= GPAD_UP;
    if (c.buttons & SCE_CTRL_DOWN)     p |= GPAD_DOWN;
    if (c.buttons & SCE_CTRL_LEFT)     p |= GPAD_LEFT;
    if (c.buttons & SCE_CTRL_RIGHT)    p |= GPAD_RIGHT;
    if (c.buttons & SCE_CTRL_SQUARE)   p |= GPAD_A;
    if (c.buttons & SCE_CTRL_CROSS)    p |= GPAD_B;
    if (c.buttons & SCE_CTRL_CIRCLE)   p |= GPAD_C;
    if (c.buttons & SCE_CTRL_START)    p |= GPAD_START;
    return p;
}

static void boot_note(const char *tag, int code)
{
    FILE *b = fopen("ux0:/data/rr3vita/breadcrumb.txt", "a");
    if (!b) return;
    fprintf(b, "%s code=%d\n", tag, code);
    fclose(b);
}

/* ---- direct display (no GPU) ---- */
static uint32_t *s_disp[2];
static int s_disp_cur = 0;
/* Per-buffer stride size: the raw frame is 960*544*4 = 0x1FE000;
 * CDRAM memblocks want 256KB-aligned sizes -> round up to 0x200000. */
#define DISP_BUF_BYTES 0x200000

static int display_init(void)
{
    SceUID uid = sceKernelAllocMemBlock("rr3_disp",
                                        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                                        DISP_BUF_BYTES * 2, NULL);
    if (uid < 0) {
        boot_note("display-cdrm-failed", uid);
        uid = sceKernelAllocMemBlock("rr3_disp",
                                      SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
                                      DISP_BUF_BYTES * 2, NULL);
        if (uid < 0) { boot_note("display-rw-failed", uid); return uid; }
        boot_note("display-rw-fallback", 0);
    }
    unsigned char *base = NULL;
    int r = sceKernelGetMemBlockBase(uid, (void **)&base);
    if (r < 0 || !base) { boot_note("display-base-failed", r); return r; }
    s_disp[0] = (uint32_t *)base;
    s_disp[1] = (uint32_t *)(base + DISP_BUF_BYTES);
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        s_disp[0][i] = 0xFF201010u;
        s_disp[1][i] = 0xFF201010u;
    }
    boot_note("display-ok", 0);
    return 0;
}

/* ---- on-screen debug stats (Lola's request, build M onward) ----
 * Live TICK / SIM ms / ROLL ms drawn into the display buffer at
 * present time. The numbers mirror the breadcrumb fields so what
 * she watches on screen and what the file records stay the same. */
static int      s_stats_on = 0;
static unsigned s_stat_tick = 0;
static double   s_stat_sim_ms = 0.0, s_stat_roll_ms = 0.0;

static void draw_text(uint32_t *dst, int ox, int oy, const char *s)
{
    for (int ci = 0; s[ci]; ci++) {
        unsigned char c = (unsigned char)s[ci];
        if (c < 32 || c > 126) c = '?';
        const unsigned char *g = font8x8[c - 32];
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (!(g[y] & (0x80 >> x))) continue;
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++)
                        dst[(oy + y * 2 + dy) * SCR_W + ox + ci * 18 + x * 2 + dx] = 0xFFFFFFFFu;
            }
        }
    }
}

#ifdef RR3_BUILD_LABEL
#define RR3_STR_(x) #x
#define RR3_STR(x) RR3_STR_(x)
#define RR3_BUILD_LABEL_STR RR3_STR(RR3_BUILD_LABEL)
#endif

static void draw_stats(uint32_t *dst)
{
    if (!s_stats_on) return;
    /* dark backing strip so the text reads over any game content */
    for (int y = 6; y < 46; y++)
        for (int x = 6; x < 430; x++)
            dst[y * SCR_W + x] = 0xE8101010u;
    char line[64];
    snprintf(line, sizeof line, "TICK %u", s_stat_tick);
    draw_text(dst, 12, 10, line);
    snprintf(line, sizeof line, "SIM %.2f ROLL %.2f", s_stat_sim_ms, s_stat_roll_ms);
    draw_text(dst, 12, 28, line);
#ifdef RR3_BUILD_LABEL
    /* build identity, top-right (Lola's request, build CF onward):
     * which build this is, and how many builds the hunt has run */
    for (int y = 6; y < 46; y++)
        for (int x = SCR_W - 174; x < SCR_W - 6; x++)
            dst[y * SCR_W + x] = 0xE8101010u;
    snprintf(line, sizeof line, "BUILD %s", RR3_BUILD_LABEL_STR);
    draw_text(dst, SCR_W - 12 - (int)strlen(line) * 18, 10, line);
#ifdef RR3_BUILD_NUMBER
    snprintf(line, sizeof line, "#%d", RR3_BUILD_NUMBER);
    draw_text(dst, SCR_W - 12 - (int)strlen(line) * 18, 28, line);
#endif
#endif
}

static void display_present(void)
{
    if (!s_disp[0]) return;
    uint32_t *dst = s_disp[s_disp_cur];
    for (int y = 0; y < FB_H; y++) {
        const uint32_t *src = s_fb[y];
        uint32_t *r0 = dst + (48 + y * 2) * SCR_W + 160;
        uint32_t *r1 = r0 + SCR_W;
        for (int x = 0; x < FB_W; x++) {
            uint32_t v = src[x];
            r0[x * 2] = v; r0[x * 2 + 1] = v;
            r1[x * 2] = v; r1[x * 2 + 1] = v;
        }
    }
    draw_stats(dst);
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof fb);
    fb.size = sizeof fb;
    fb.base = dst;
    fb.pitch = SCR_W;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = SCR_W;
    fb.height = SCR_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    s_disp_cur ^= 1;
}

/* BUILD J (stack bisect): the ENTIRE sim (boot + all phases) runs on a
 * worker thread with a 16 MB stack. In every build so far the sim ran
 * on the main thread, whose stack is small — while the engine's atomic
 * V-int/H-int deliveries execute the game's handler call-graph on the
 * CALLING thread's stack. On the PC the main thread stack is ~8 MB and
 * the same code survives; if Vita deaths are main-stack overflow, J
 * survives racing. Engine, presenter, pacing: identical to build E. */
static int sim_thread(SceSize args, void *argp)
{
    (void)args; (void)argp;
    sceIoMkdir("ux0:/data/rr3vita", 0777);   /* exe_relative() target dir */
    boot_note("boot-start", 0);

    /* Display first this time: a dark-blue screen within the first
     * second proves the display path before the sim even boots, and
     * every startup stage leaves a breadcrumb note. */
#ifndef RR3_NODISPLAY
    display_init();
    display_present();
#endif
    /* Build U bisect: RR3_NODISPLAY skips the display subsystem entirely
     * (no CDRAM allocation, no fill, no present, no framebuffer ever
     * registered). display_present() already no-ops when s_disp[0] is
     * NULL, so phases 2/3 run the sim with the sink live but never touch
     * the display. If the interpreter still dies in phase 1, the front-
     * end's display surface is cleared along with everything else. */

    /* ---- boot the machine (same order as the PC runner) ---- */
    FILE *f = fopen("app0:rr3.bin", "rb");
    if (!f) { boot_note("rom-missing", 0); for (;;) sceDisplayWaitVblankStart(); }
    fseek(f, 0, SEEK_END);
    long rom_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *rom = (unsigned char *)malloc((size_t)rom_len);
    fread(rom, 1, (size_t)rom_len, f);
    fclose(f);
    boot_note("rom-loaded", (int)rom_len);

    machine_init();
    glue_init(rom, (unsigned int)rom_len);
    gbus_sram_setup(&g_machine.bus);
    free(rom);
    boot_note("machine-boots", 0);
#ifdef RR3_FORCE_INTERP_ENV
    /* Build Q instrumentation: prove whether the env switch actually
     * engages under this front-end (build P died in phase 1 with native
     * fault PCs, which should be impossible if the interp was driving). */
    boot_note("env-fi", getenv("GENESIS_FORCE_INTERP") ? 1 : 0);
    { extern int genesis_force_interp(void);
      boot_note("force-interp", genesis_force_interp()); }
#endif

    GenesisSimInput in;
    memset(&in, 0, sizeof in);
    in.human_mask = 1;

    /* ---- phase 1: headless sim cost ---- */
    GenesisSimHooks no_sink = s_hooks; no_sink.sink = NULL;
    uint64_t t0 = sceKernelGetProcessTimeWide();
#ifdef RR3_FINEGRAIN
    breadcrumb("p1-enter", 0.0);
#endif
    for (int i = 0; i < 600; i++) {
        genesis_sim_step(&in, &s_audio, &no_sink);
#ifdef RR3_FINEGRAIN
        breadcrumb("p1", 0.0);
#elif defined(RR3_FORCE_INTERP_ENV)
        if (((i + 1) % 60) == 0) breadcrumb("p1", 0.0);
#endif
    }
    uint64_t t1 = sceKernelGetProcessTimeWide();
    double sim_ms = (double)(t1 - t0) / 1000.0 / 600.0;
    breadcrumb("phase1-done", sim_ms);
#ifdef RR3_MASK_LO
    { extern uint32_t g_mask_tried, g_mask_hit;
      FILE *mf = fopen("ux0:/data/rr3vita/breadcrumb.txt", "a");
      if (mf) { fprintf(mf, "maskstat phase=1 tried=%u hit=%u\n",
                        g_mask_tried, g_mask_hit); fclose(mf); } }
#endif

    /* ---- phase 2: sink-live cost, unthrottled ---- */
    t0 = sceKernelGetProcessTimeWide();
    for (int i = 0; i < 300; i++) genesis_sim_step(&in, &s_audio, &s_hooks);
    t1 = sceKernelGetProcessTimeWide();
    double pres_ms = (double)(t1 - t0) / 1000.0 / 300.0;
    breadcrumb("phase2-done", pres_ms);

#ifdef RR3_HEADLESS3
    /* Build N bisect: phase 3 without the interactive OS surface. Same
     * sim, same sink (fbh stays meaningful), zero input, free-running
     * headless through the demo-race window and far beyond. A heartbeat
     * present every 600 ticks keeps the tick counter visible; there are
     * no per-frame display calls, no controller reads, no vblank waits.
     * Surviving 30,000 ticks convicts the phase-3 OS surface; dying here
     * convicts native engine execution itself. */
    breadcrumb("headless3-start", 0.0);
    s_stats_on = 1;
    for (unsigned t = 0; t < 30000; t++) {
        genesis_sim_step(&in, &s_audio, &s_hooks);
        if (((t + 1) % 60) == 0) breadcrumb("play", 0.0);
        if (((t + 1) % 600) == 0) {
            s_stat_tick = (unsigned)genesis_sim_tick_count();
            display_present();
        }
    }
    breadcrumb("survived-30000", 0.0);
    for (;;) sceKernelDelayThread(1000000);
#endif

    /* ---- phase 3: play ---- */
    uint64_t last = sceKernelGetProcessTimeWide();
    double roll_ms = 0.0, step_ms = 0.0;
    unsigned frames = 0;
    s_stats_on = 1;
    for (;;) {
        in.pad[0] = read_pad();
        uint64_t ts0 = sceKernelGetProcessTimeWide();
        genesis_sim_step(&in, &s_audio, &s_hooks);
        uint64_t ts1 = sceKernelGetProcessTimeWide();
        step_ms = step_ms * 0.9 + ((double)(ts1 - ts0) / 1000.0) * 0.1;
        s_stat_tick = (unsigned)genesis_sim_tick_count();
        s_stat_sim_ms = step_ms;
        s_stat_roll_ms = roll_ms;
        display_present();

        uint64_t now = sceKernelGetProcessTimeWide();
        roll_ms = roll_ms * 0.9 + ((double)(now - last) / 1000.0) * 0.1;
        last = now;
        frames++;
        if ((frames % 60) == 0) breadcrumb("play", roll_ms);
#ifdef RR3_MASK_LO
        if ((frames % 1200) == 0) {
            extern uint32_t g_mask_tried, g_mask_hit;
            FILE *mf = fopen("ux0:/data/rr3vita/breadcrumb.txt", "a");
            if (mf) { fprintf(mf, "maskstat phase=3 frames=%d tried=%u hit=%u\n",
                              frames, g_mask_tried, g_mask_hit); fclose(mf); }
        }
#endif

        sceDisplayWaitVblankStart();
    }
    return 0;
}

int main(void)
{
#ifdef RR3_INTERLEAVE
    /* Build L bisect: switch interrupt delivery from the default ATOMIC
     * model (whole handler runs on the scheduler fiber with a full g_cpu
     * save/swap) to INTERLEAVED (handler runs on the game fiber, budget
     * yields like hardware). glue.c reads this env lazily at first delivery. */
    setenv("GENESIS_INTERLEAVE_IRQ", "1", 1);
#endif
#ifdef RR3_FORCE_INTERP_ENV
    /* Build P bisect: drive the WHOLE program on the clean-room Tier-3
     * interpreter (m68k_interp) instead of the 637 recompiled functions.
     * glue's fiber entry checks genesis_force_interp() (a lazy getenv)
     * before calling the native entry point, so the env must be set
     * before the sim thread starts -- here in main() is bulletproof.
     * Builds F/G never achieved this drive (F's env never engaged under
     * sim_step; G only forced the handlers). Interp is much slower than
     * native; that is expected and fine for a bisect. */
    setenv("GENESIS_FORCE_INTERP", "1", 1);
#endif
    SceUID th = sceKernelCreateThread("rr3_sim", sim_thread, 0x10000100,
                                      0x1000000 /* 16 MB stack */, 0, 0, NULL);
    if (th < 0) return 1;
    sceKernelStartThread(th, 0, NULL);
    sceKernelWaitThreadEnd(th, NULL, NULL);
    return 0;
}
