# Road Rash 3 — Z80 sound-driver handshake stall: root cause and fix

**Status: ROOT-CAUSED and FIXED (frame-1043 stall). One further stall of the same
engine family found at frame ~4774 — pinned down precisely, not fixed here (see
"What remains open").**

Engine: mstan/segagenesisrecomp @ f875aa8 (pristine clone
`~/workspace/rr3-vetting/segagenesisrecomp`, left untouched — verified clean with
`git status`). All work done in `~/workspace/rr3-runner-work/`; no generated C
edited; no git commits.

---

## 1. Symptom

Any run past ~17.5 s of game time dies on a runner watchdog, identically in
native (recompiled) and oracle (`GENESIS_FORCE_INTERP=1`) modes, on this VM and
on Lola's PC:

```
CRASH REPORT  frame=1043  reason: instruction watchdog: 20000000 native insns without yielding
[OWN-DIAG] cpuPC=$03AB6E z80pc=$099F z80_run=1 busreq=0 reset_off=1 bank=$000
           nz_z80ram=6282 ram_F00D=$A1 ram_F00A=$6010
[OWN-DIAG] z80_ram[0..0x1F]: C3 33 05 1B 25 1E FC 00 00 00 00 FF 01 E8 65 00 00
           E8 71 00 00 E8 71 00 00 …
```

The 68K is in a retry loop inside the sound **command-enqueue function at ROM
0x3AB40**, waiting for Z80 RAM byte **+0x0B** to become 0. The Z80 sits at
$099F, *inside* its queue-consume routine, with +0x0B = 0xFF (busy) and the
queue count already decremented to 1. Neither side can ever move again.

## 2. Reproduction (deterministic on this VM)

```
cd ~/workspace/rr3-runner-work/rr3/build
SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software LIBGL_ALWAYS_SOFTWARE=1 \
  xvfb-run -a -s "-screen 0 1024x768x24" ./RoadRash3Recomp ../rr3.bin \
  --no-launcher --turbo --max-frames 3600 --hash-frames 120
```

Dies at frame 1043 every time (spike-1 binaries reported 1052; same state).
Framebuffer hashes for frames 120–960 match the known-good values
(480/600 = `0xBE99E04DC4DE31A9`) — everything is correct until the stall.
Log: `hidden_files/z80-bug/repro-native-3600.log`.

## 3. The two sides of the handshake

### 68K side — enqueue (ROM 0x3AB40, from the Stage 0 Ghidra project)

```
0x03AB40: movem.l {A0,D1},-(SP)
0x03AB54: bsr.w  0x3B54A            ; bus-request helper
0x03AB58: lea    (0xA0000B).l,A0    ; queue busy flag
0x03AB5E: tst.b  (A0)
0x03AB60: beq.b  0x3AB76            ; flag == 0 → enqueue the command
0x03AB62: move.w #0,(0xA11100).l    ; release the Z80 bus
0x03AB6A: move.w #0x64,D1w          ; delay 100
0x03AB6E: subi.w #1,D1w             ; ← stall PC
0x03AB72: bne.b  0x3AB6E
0x03AB74: bra.b  0x3AB54            ; retry FOREVER — no timeout
```

Queue layout in Z80 RAM: +0x0B busy flag, +0x0C count (cap 0x10), +0x0D… =
16 entries × 4 bytes.

### Z80 side — driver (ROM 0x3B568–0x3D08E, MD5 b7b451ef47577d033a7c5b59b1178c93)

Full linear-sweep disassembly in `hidden_files/z80-bug/work/z80-sweep.txt`.
Only three writes to (0x000B) exist in the whole 6,951-byte driver:

- **0x096F (queue reset, called once at init):** +0x0B=0xFF, zero the queue,
  +0x0B=0x00.
- **0x0982 (consume/shift):** sets +0x0B=0xFF, decrements the count at +0x0C,
  shifts the remaining entries down one slot (copy loop 0x099C–0x09A4), then
  stores 0x00 to +0x0B and returns.

So +0x0B is a **Z80-busy mutex**: 0xFF while the Z80 is processing the queue,
0x00 when idle. The 68K enqueue spins until it reads 0. The init handshake at
+0x4F (set to 0xFF at Z80 0x0569) is a different byte and works fine.

The engine's `[SND-TRACE]` ring (GEN_DEV_TRACE build) shows the protocol
running healthily — e.g. frame 1035, VBlank enqueue at scanline 224, Z80
consume completing ~8 lines later (`pcz` = Z80 PC attribution):

```
f=1035 sl=224  68KW  z80[$000D] $91->$81          ; 68K enqueues command
f=1035 sl=224  68KW  z80[$000C] $00->$01          ; count 0→1
f=1035 sl=231  Z80W  pcz=$0987 z80[$000B]=$FF     ; consume: busy
f=1035 sl=232  Z80W  pcz=$0995 z80[$000C]=$00     ; count decremented
f=1035 sl=232  Z80W  pcz=$09AB z80[$000B]=$00     ; busy cleared
```

### Why frame 1043 — the title-screen burst

Frame 1042 (ring dump `snd-ring-dump1042.txt`) is where the title transition
starts the music: the queue goes **deep** (count reaches 9), the Z80 drains it
at a steady ~1 command per 35 scanlines, and the VBlank sound tick burst-
enqueues 7 more entries at sl=224:

```
f=1042 sl=205  Z80W  pcz=$0987 z80[$000B]=$FF     ; consume in progress…
f=1042 sl=206  Z80W  pcz=$0995 z80[$000C]=$02
f=1042 sl=208  Z80W  pcz=$09AB z80[$000B]=$00
f=1042 sl=224  68KW  z80[$000C] $02->$03          ; VBlank burst enqueue ×7
   …           68KW  z80[$000C] $08->$09
f=1042 sl=239  Z80W  pcz=$0987 z80[$000B]=$FF
f=1042 sl=248  Z80W  pcz=$09AB z80[$000B]=$00     ; still healthy at dump time
```

All game long the Z80 is idle at enqueue time, so the spin is never taken.
In frame 1043, for the first time, an enqueue races a consume: the Z80 has
just set +0x0B=0xFF and is ~50 cycles from finishing the shift — and the
68K's retry loop starts.

## 4. Root cause — the runner freezes the Z80 for the whole VBlank handler

The runner's own backend delivers VBlank/HBlank handlers **atomically**
(`own_deliver_vint` / `own_deliver_hint` in `runner/glue.c`): the handler runs
to completion on the main fiber with `s_in_vblank_service=1`, which gates
**every** mechanism that could advance the machine:

- `check_cycle_budget` — requires `!s_in_vblank_service` → no budget yields;
- `spin_check` (same-address spin detector) — first branch resets its streak
  and returns when `s_in_vblank_service`, so the poll loop is never yielded;
- `m68k_read8` Z80-poll yield — requires `s_interleave_active &&
  !s_in_vblank_service`; +0x0B isn't even in its special-cased address set;
- `glue_yield_for_vblank` / `glue_yield_for_interrupt_poll` — early-return.

The Z80 is stepped **only** by the scanline scheduler (`machine_run_frame`),
which cannot run until the handler returns. The handler cannot return until
the Z80 clears +0x0B. The Z80 cannot clear +0x0B until it executes ~50 more
cycles. Deadlock, by construction — ended only by the 20M-insn watchdog.

Measured at the stall (instrumented copy of the engine, `[Z80BUG-DIAG]`
printed from the watchdog path):

```
[Z80BUG-DIAG] cpuPC=$03AB6E in_vblank_service=1 interleave_active=0
  irq_in_progress=0 game_yielded_vblank=0 cycle_budget=458
  spin_addr=$000000 spin_count=0 z80poll_last=$000000 z80poll_streak=0
  frame=1043 spin_yields=0 z80poll_yields=63 fallback=0
```

Note `z80_run=1` in OWN-DIAG: bus free, reset off — the Z80 *could* run; the
scheduler simply never steps it. On hardware the Z80 executes concurrently
with the 68K handler, so the flag clears within ~100 cycles and the spin is
a non-event. This is a runtime/scheduling deficiency, not a game bug and not
a codegen bug — consistent with the stall being identical in both backends
(the interpreter's handler delivery is the same `own_deliver_vint`, and the
interp routes its bus accesses through the same glue functions).

**Why the engine's escape hatch doesn't cover this:**
`GENESIS_INTERLEAVE_IRQ=1` (STAGE-1 interleaved delivery) only engages when
the game fiber is *parked* at WaitForVBla when the V-int fires
(`if (interleave_irq_on() && s_game_yielded_vblank)`); the engine comments
call the other case "mid-run delivery" and keep it atomic. RR3's fatal
delivery is mid-run (title transition — the main line is busy when the
VBlank hits). Verified empirically: with `GENESIS_INTERLEAVE_IRQ=1` the run
still dies at frame 1043 with `in_vblank_service=1`
(`hidden_files/z80-bug/dev-interleave-native-3600.log`).

## 5. The fix

**Idea:** mirror hardware concurrency at exactly the point the game asks for
it. Inside atomic handler service, every time the handler **releases** the
Z80 bus (word write to $A11100 with the request bit clear), step the Z80 by
one scanline slice (228 cycles). RR3's enqueue loop releases the bus on every
retry, so the Z80 immediately gets the cycles it needs to finish the consume
and clear +0x0B — the same interleave hardware provides. `step_z80`
self-gates on bus-request/reset, so the step is a no-op whenever the Z80
would genuinely be halted. Outside atomic service nothing changes: the
scheduler already paces the Z80 there, and double-stepping would disturb
the engine's deliberate sound-event pacing.

**Patch:** `files/rr3-z80-fix.patch` (3 hunks, applies cleanly with
`git apply` / `patch -p1` from the engine root — verified against the
pristine clone):

1. `runner/glue.c`, end of `m68k_write16`:
   ```c
   if (s_in_vblank_service && byte_addr == 0xA11100u && !(val & 0x100u))
       machine_step_z80_scanline();
   ```
2. `runner/video/genesis_machine.c`: new exported helper
   ```c
   void machine_step_z80_scanline(void) { step_z80(&g_machine, Z80_PER_LINE); }
   ```
3. `runner/video/genesis_machine.h`: its declaration.

Because both backends (recompiled code and the Tier-3 interpreter) route all
writes through `m68k_write16`, one hook fixes native and oracle alike.
No generated C touched; the change lives entirely at the engine seam and is
proposed as an upstream change.

## 6. Proof

All runs headless (xvfb, dummy audio, software render), `--turbo`,
`--max-frames 3600 --hash-frames 120`, on a build of the **patch artifact
itself** (`files/rr3-z80-fix.patch` applied to a fresh copy of the pristine
engine; logs `hidden_files/z80-bug/patch-native-3600.log` /
`patch-oracle-3600.log`):

| run | before fix | after fix |
|---|---|---|
| native 3600 | **CRASH frame 1043**, instruction watchdog | **`[DONE] 3600 frames completed`** |
| oracle 3600 (`GENESIS_FORCE_INTERP=1`) | **CRASH frame ~1043** (spike 1) | **`[DONE] 3600 frames completed`** |

Framebuffer hashes (30 samples, every 120 frames):

- Settled frames agree exactly between modes, and with the spike-1
  known-good values from both machines: **frame 480 = frame 600 =
  `0xBE99E04DC4DE31A9`** in native *and* oracle.
- 24/30 samples agree across modes. The 6 that differ (240, 360, 2160, 2520,
  2880, 3240) are animated transition frames; crucially, the **native values
  at those frames are bit-identical to the pre-fix native build** (checked
  frames 120–960 exhaustively: `NATIVE-IDENTICAL-THROUGH-960`), so the
  cross-mode differences at animated frames pre-date the fix and are
  unchanged by it. The fix alters no rendered output anywhere the unfixed
  build could reach.

## 7. What remains open

1. **Second stall, same family — frame 4774 (native) / 4784 (oracle), bus
   watchdog** (`watchdog: 10000000 bus accesses without yield`). Found by a
   10800-frame soak of the fixed build
   (`hidden_files/z80-bug/fixed-native-10800.log`,
   `fixed-oracle-10800.log`). This time the spinner is the **HBlank handler
   itself (ROM 0x24FA)**, delivered atomically (`in_vblank_service=1` at the
   stall). It busy-waits on the VDP HV counter ($C00008/$C00009, generated
   code at $00254A/$00256A: loop until the counter passes a threshold /
   until $C00009 bit 7 clears) before doing its raster-timed uploads. The
   runner's HV counter is line-granular —
   `gvdp_read_hv_counter()` returns `(scanline << 8) | (in_hblank ? 0x00 :
   0x80)` (`runner/video/genesis_vdp.c`) — and the scanline only advances
   in `machine_run_frame`, which is frozen during atomic delivery. Same
   root deficiency as §4 (atomic handler + wait-on-hardware-progress =
   deadlock), but the waited-on device is the raster, not the Z80, so the
   bus-release hook cannot cure it. The honest fix is the engine's own
   roadmap item: extend interleaved interrupt delivery to mid-run/latched
   V-ints and to H-ints (or give atomic service a bounded machine
   advance). Documented here precisely rather than hacked around; the game
   now runs ~80 s (boot → title → attract transition) instead of ~17.5 s.
2. The fix hooks **word** writes to $A11100. A game that releases the bus
   with a *byte* write inside an atomic handler would need the same
   one-line hook in `m68k_write8` (RR3's HBlank handler does use
   `write8 $A11100` at $002558, but not on a spin path). Trivial extension,
   left out to keep the proven patch minimal.
3. Not run: the cosim harness (`-DGENESIS_BUILD_COSIM=ON`) diff for the
   patch — the dual-mode 3600-frame proof with hash agreement above was
   the agreed bar; cosim is the natural extra validation before upstreaming.
4. Audio correctness past the title screen is asserted by construction
   (the Z80 now advances inside handlers exactly as hardware concurrency
   dictates, and the sound queue drains — visible in the snd-ring), not by
   listening. First on-hardware/listen check belongs with the next PC run.

## Evidence index (`hidden_files/z80-bug/`)

- `repro-native-3600.log` — unpatched stall, frame 1043.
- `snd-ring-dump1035.txt`, `snd-ring-dump1042.txt` (+ `chip-ring-*`,
  `z80-ram-dump*.bin`) — GEN_DEV_TRACE ring dumps: healthy queue protocol,
  and the frame-1042 burst that sets up the fatal race.
- `dev-native-3600.log`, `dev-interleave-native-3600.log` — `[Z80BUG-DIAG]`
  scheduler state at the stall, with and without `GENESIS_INTERLEAVE_IRQ=1`.
- `engine-dev-full-diff.txt` — the diagnostic instrumentation used (watchdog
  state dump), kept separate from the fix patch.
- `patch-native-3600.log`, `patch-oracle-3600.log` — proof runs of the
  patch artifact. `final-*.log`, `fixed-*.log` — same proof on the
  instrumented build; `fixed-*-10800.log` — the soak that found stall #2.
- `work/` — Z80 driver extraction + full sweep disassembly (`z80-sweep.txt`),
  68K poll-region listing (`m68k-poll-region.txt`), sampler script/output.
