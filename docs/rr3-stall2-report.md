# Road Rash 3 — Stall #2 (HBlank raster-split vs the frozen HV counter): root cause and fix

**Status: ROOT-CAUSED and FIXED. No third stall found out to 21600 frames
(6 minutes of game time) in either mode.**

Engine: mstan/segagenesisrecomp @ f875aa8 (pristine clone
`~/workspace/rr3-vetting/segagenesisrecomp`, left untouched — verified clean
with `git status`). All work done in `~/workspace/rr3-runner-work/`; no
generated C edited; no git commits. This stall is the sibling flagged in
§7.1 of `rr3-z80-bug-report.md`.

**Application order (important):** the shipped patch applies **on top of
fix #1**. Lola's clone already carries fix #1, so for her tree this is the
only patch to apply:

```
git apply rr3-z80-fix.patch      # (already applied on Lola's clone)
git apply rr3-stall2-fix.patch   # this fix
```

Both steps verified with `git apply --check` / `git apply` from a pristine
tree; the resulting three files are byte-identical to the tree the proof
build was made from.

---

## 1. Symptom

Any run of the fix-#1 build past ~80 s of game time dies on the bus
watchdog, on this VM and (by construction of the soak that found it)
anywhere else:

```
CRASH REPORT  frame=4774  reason: watchdog: 10000000 bus accesses without yield   (native)
CRASH REPORT  frame=4784  reason: watchdog: 10000000 bus accesses without yield   (oracle)
```

The crash ring is 64 consecutive `R16 $C00008` — a word-poll of the VDP HV
counter. Reproduced deterministically for this investigation
(`hidden_files/stall2/repro2-native-10800.log`,
`repro2-oracle-10800.log`): native frame 4774, oracle frame 4784, exactly
the soak's frames. (The 10-frame mode offset is the backends' known
phase difference around the mode transition that arms the effect; the
same class of offset as spike 1's 1043/1052.)

## 2. The wait — RR3's HBlank handler is a raster-split engine

The spinner is the game's own HBlank handler, `func_0024FA` (vector at
ROM 0x24FA; read from the generated C in the work tree — codegen was
never modified). Anatomy:

- **Entry:** writes VDP reg 0x0A (H-int rate) = 0xFF — one-shot: this
  arming cannot re-fire from the rate register alone.
- **Arm word:** if RAM `$FF2558` == −1 the handler RTEs immediately.
  The screen-setup routine at ROM **0x11D16** arms the effect: it points
  the long at `$FF276E` to a record table it builds in work RAM at
  **$FF62AC** and writes `$FF2558` = 0.
- **Timed path** (armed, and record key `$FF27B2` > 0 signed): the record
  is at `table + (int16)key`, 12 bytes:
  `+0` word → VDP control (address setup); **`+2` word → packed HV
  target** (V counter in the high byte, H position in the low byte);
  `+4` word → D1, a second control-port word written *after* the timed
  upload (the register change that takes effect from the split line on);
  `+6` long → D2, the data-port payload; `+10` word → next record key,
  stored back to `$FF27B2`. Before waiting, the handler also presets the
  VRAM write address with a constant long write (0x40000010) to the
  control port.
- **Loop 1 @ 0x254A:** `while (HV <= target & 0xFF00)` — spin until the
  beam passes the target **line**.
- Then it saves `$A11100` and byte-writes 0x01 → **asserts the Z80 bus
  request**, holding the Z80 off the bus for the critical window.
- **Loop 2 @ 0x2560:** reads `$C00009` (H byte), `while (bit 7 set)` —
  in the engine's HV model the H byte is 0x80 in active display and 0x00
  in hblank, so this waits for **hblank to begin**.
- **Loop 3 @ 0x256A:** `while (HV <= target)` — spin until the beam
  passes the **exact packed (line, H) point**.
- **Fire:** `write32 D2 → $C00000` (the timed VRAM upload), a NOP timing
  pad, write D1 → control port, restore `$A11100` from the stack
  (releasing the Z80 bus as found), decrement the chain counter at
  `$FF022A`, run the disarm/chain state machine (thresholds 0x90/0x130 on
  `$FF27B2`), pop regs, RTE.

Measured at runtime (instrumented dev build, `STALL2_DIAG`,
`hidden_files/stall2/diag2-native-4900.log`): H-ints are not delivered at
all before frame 4773 (the VDP H-int is disabled until this screen mode);
two *disarmed* deliveries occur at frame 4773 (lines 131/133 — instant
RTE, zero HV polls). From frame **4774** the effect is armed: **six
deliveries per frame** at scheduler lines **1, 3, 30, 42, 75, 152**,
consuming records at offsets 0x10, 0x20, 0x50, 0x60, 0x70, 0x80 — a
six-band raster split for the attract demo's race screen. Each invocation
waits for a split point ≈4–5 lines past its delivery line (measured beam
travel: 6–10 HV polls per invocation, ending at line delivery+3.5…+5 in
the hblank phase), i.e. **≈2,000–2,700 68K cycles of spin per invocation
on hardware**. This is the standard EA pattern: program the H-int a few
lines ahead of the split, set up, then spin on the HV counter to the
exact beam position so the upload lands mid-frame at the right line.

## 3. Root cause — the HV counter is frozen for the whole handler

Same engine deficiency as stall #1, different waited-on device:

- `gvdp_read_hv_counter()` (`runner/video/genesis_vdp.c`) derives the
  counter from two VDP fields: `(scanline << 8) | (in_hblank ? 0x00 :
  0x80)`. `scanline` is written **only** by `gvdp_begin_scanline()`,
  called from the scanline scheduler (`machine_run_frame`). `in_hblank`
  is initialized to 0 and **never set by the scheduler at all** — its
  only mutation is the phantom toggle inside `gvdp_read_control()`
  (a status-read side effect kept for Sonic 1's DAC feeder).
- H-int delivery (`glue_own_interrupt`, level 4) is **always atomic**:
  the handler runs to completion on the main fiber with
  `s_in_vblank_service=1`, which gates every yield mechanism (the same
  gate list as stall #1, §4 of the previous report). The scheduler
  cannot run until the handler returns; the handler cannot return until
  the HV counter moves; the HV counter cannot move until the scheduler
  runs. Deadlock by construction, ended only by the 10M-access
  watchdog. The observed spin is loop 1 (word reads at $C00008); loop
  2's exit condition is equally unreachable in the frozen model
  (`in_hblank` stuck at 0 ⇒ H byte stuck at 0x80).
- **Why `GENESIS_INTERLEAVE_IRQ=1` cannot cover this:** STAGE-1
  interleaved delivery is reachable only for level 6, only when the
  flag is set *and* the game fiber is parked at WaitForVBla
  (`s_pending_irq` is consumed solely in `glue_yield_for_vblank`).
  H-ints originate inside `machine_run_frame` — scheduler context,
  mid-frame — and the level-4 branch of `glue_own_interrupt` has no
  pending/interleaved path at all. Extending interleave to mid-run
  H-ints would require a pending-IRQ consumption point at arbitrary
  budget-yield resumes on the game fiber, and turning the flag on also
  changes V-int delivery globally: the engine's own roadmap item, but
  not the minimal seam.
- **Why fix #1's hook cannot cure it:** no Z80 traffic occurs inside
  the wait (the handler in fact holds the Z80 bus *requested* through
  loops 2–3); the waited-on device is the raster itself.

Not a game bug and not a codegen bug: on hardware the beam sweeps
concurrently with the handler and every loop resolves in ≤ ~5 lines.
Consistent with the stall being identical in both backends (the
interpreter delivers through the same atomic path and routes bus reads
through the same glue functions).

## 4. The fix

Mirror hardware concurrency at exactly the point the guest waits on it —
the fix-#1 pattern, applied to the raster:

**Patch:** `files/rr3-stall2-fix.patch` (3 files, applies on top of
fix #1 — see the header of this report):

1. `runner/video/genesis_machine.c`: new exported helper
   `machine_advance_raster_phase()` — advances the VDP raster position
   by one *observable phase*: active-display → hblank on the same line,
   then hblank → active on the next line. It touches **only** the two
   fields the HV counter is computed from (`scanline`, `in_hblank`).
   Advancing stops at the last raster line (261) instead of wrapping
   into the next frame.
2. `runner/video/genesis_machine.h`: its declaration.
3. `runner/glue.c`: in `m68k_read16` and `m68k_read8`, after the bus
   read completes —
   ```c
   if (s_in_vblank_service && byte_addr >= 0xC00008u && byte_addr <= 0xC0000Fu)
       machine_advance_raster_phase();
   ```
   The read returns the position as sampled (the first poll of a handler
   sees the true delivery-time beam); each subsequent poll sees the beam
   one phase further on — the same forward progress hardware gives a
   polling loop. Because both backends route all reads through
   `m68k_read16`/`m68k_read8`, one hook fixes native and oracle alike.

**Design choices, argued:**

- **Why not advance through `gvdp_begin_scanline()`?** It owns IRQ
  bookkeeping (hint_counter reload/decrement, `vint_pending` at the
  vblank boundary). Calling it from the read path would double-fire
  that bookkeeping when the scheduler later reaches the same line.
  The fix deliberately mutates only the two presentation fields;
  `gvdp_begin_scanline(line)` next scheduler iteration re-syncs
  `scanline` from the scheduler's own counter, so the advance is
  self-healing and cannot leak into the next line's state.
- **Why a phase (half-line) quantum?** It is the smallest step the
  engine's HV model can express (the model's H byte has exactly two
  phases). Loop 2 (wait-for-hblank) resolves in one step from any
  active phase; loops 1/3 resolve in ≤ 2 steps per line of wait.
- **Rendering safety:** `gvdp_render_scanline()` takes its line as a
  parameter and never reads `vdp->scanline`; sprite evaluation likewise
  works from the parameter. The handler still runs *before* the
  scheduler renders its delivery line, exactly as in the unfixed
  engine — the fix changes when the handler *returns*, not what the
  renderer sees.
- **Outside atomic service, nothing changes:** the gate is
  `s_in_vblank_service`, and the scheduler already paces the raster
  there; main-line HV polls (e.g. WaitForVBla-style loops) see the
  frozen-per-line values they always saw.

**Side-effect audit:**

- *IRQ bookkeeping:* untouched (no `hint_counter`, `vint_pending`,
  `in_vblank` mutation). A handler waiting across the vblank boundary
  line does not fabricate a V-int; RR3's targets are all in-frame
  (deepest split at line 157 of 224 active).
- *DMA:* the timed path uses immediate data-port writes, not DMA; no
  DMA state is advanced or skipped by the hook.
- *Z80/audio:* the hook does not step the Z80. On hardware the Z80 also
  runs during the wait — but through loops 2–3 the handler itself holds
  the Z80 bus requested, and the engine's handler-cycle debt mechanism
  (`s_irq_cycle_debt`) already charges the handler's executed cycles
  against subsequent main-loop chunks, preserving frame pacing in
  aggregate. Handler chip-write stamps use the delivery-raster rebase,
  unchanged.
- *Status port:* `gvdp_peek_status`/`gvdp_read_control` expose
  `in_hblank` as bit 2. A status read late in the same scheduler line,
  after an armed handler waited, can now observe the hblank bit set —
  which is the beam position the handler legitimately waited for; the
  pre-existing phantom-toggle in `gvdp_read_control` already makes this
  bit non-scheduler-owned. Noted, accepted.
- *HV-as-entropy:* a handler reading the HV counter for randomness
  during atomic service now sees slowly advancing values instead of a
  frozen one — strictly closer to hardware.

## 5. Proof

All runs headless (xvfb, dummy audio, software render), `--turbo`,
`--hash-frames 120`, on a build of the **shipped patch artifacts
themselves** (pristine → `rr3-z80-fix.patch` → `rr3-stall2-fix.patch`;
logs `hidden_files/stall2/final-*.log`):

| run | fix #1 only | fix #1 + fix #2 |
|---|---|---|
| native 10800 | **CRASH frame 4774**, bus watchdog | **`[DONE] 10800 frames completed`** |
| oracle 10800 (`GENESIS_FORCE_INTERP=1`) | **CRASH frame 4784**, bus watchdog | **`[DONE] 10800 frames completed`** |
| native 21600 soak | (dies at 4774) | **`[DONE] 21600 frames completed`** |
| oracle 21600 soak | (dies at 4784) | **`[DONE] 21600 frames completed`** |

Framebuffer hashes:

- **Settled frames unchanged:** frame 480 = frame 600 =
  `0xBE99E04DC4DE31A9` in both modes, both run lengths.
- **No rendering change anywhere the old build could reach:** the fixed
  native stream is bit-identical to the fix-#1 build for all 39 samples
  it produced (frames 120–4680); the fixed oracle stream is
  bit-identical to the fix-#1 oracle build for all 30 samples of the
  earlier 3600-frame proof. (The hook verifiably cannot fire earlier:
  the first HV poll inside atomic service in the entire run occurs at
  frame 4774 — diag log, §2.)
- **Determinism:** each mode's full 10800 hash stream is identical
  across repeated runs, and the 21600 soak's first 90 samples equal the
  10800 runs'. The attract loop is still cycling with varied content at
  frame 21600 (123 unique hashes among 180 samples per mode).
- **Stall #3:** none found out to 21600 frames (360 s) in either mode.

## 6. Residual risks — read before calling this closed

1. **The two backends diverge in the newly reached region.** From the
   first post-stall sample (frame 4800) the native and oracle hash
   streams are essentially disjoint (98 unique hashes each over
   4800–21600; exactly 1 shared), even though both are internally
   deterministic and agree everywhere before the stall (modulo the
   long-documented animated-transition samples). The attract demo is a
   long, raster-timing-sensitive gameplay sequence, and this engine
   pair has shown transition-phase sensitivity since spike 1 (frames
   240/360 diverged the same way, then converged on settled screens) —
   the demo plausibly amplifies a small phase difference into a
   different play-out. But there is **no ground-truth reference** for
   the demo region, so which backend (if either) matches hardware
   there is *not established by this proof*. The agreed bar — both
   modes complete, settled hashes intact, zero change in previously
   reachable territory — is met; a cosim run
   (`-DGENESIS_BUILD_COSIM=ON`) or a hardware/emulator reference
   capture of the attract loop is the natural next validation before
   upstreaming either fix.
2. **Waits resolve early in cycle terms.** One phase per poll is
   coarser than hardware (~26 cycles per poll iteration ≈ 1/19 line),
   so a handler's executed-cycle debt undercounts the true raster span
   of its wait (bounded: ≤ ~5 lines per invocation here). The
   observable consequence is at most a sub-frame timing shift of
   handler uploads, which this engine already renders at whole-line
   granularity. Same class of approximation as fix #1 (one Z80
   scanline slice per bus release).
3. **A target that never comes still hangs.** Advancement stops at
   line 261; a hypothetical handler polling for a next-frame line
   inside atomic service would still hit the watchdog — the pre-fix
   failure mode, not a new one. No such wait exists in RR3 (deepest
   split lands at line ~157).
4. **Cosim not run** for this patch (same status as fix #1); audio
   correctness past the title screen is asserted by construction and
   by the sound queue's continued operation, not by listening.
5. The hook covers word and byte HV reads (`$C00008–$C0000F`, matching
   the bus decode). A 32-bit HV read (`m68k_read32`) bypasses the glue
   read16 hook; RR3's handler uses only 16/8-bit polls, and no other
   atomic-service HV read exists in this ROM (the only two HV read
   sites in the generated tree are the handler's loops).

## Evidence index (`hidden_files/stall2/`)

- `repro2-native-10800.log`, `repro2-oracle-10800.log` — fix-#1 build
  reproducing the stall at frames 4774 / 4784.
- `diag2-native-4900.log` — instrumented run (`STALL2_DIAG`):
  `[HINT-DELIVER]` delivery lines + arm state per invocation,
  `[RASTER-ADV]` per-poll beam advances; first advance at frame 4774.
  (Diag instrumentation lives only in the throwaway `engine-dev2`
  tree; it is not in the shipped patch.)
- `dev2-*-10800.log`, `dev2-*-10800-run2.log` — dev-build proof +
  determinism repeats.
- `final-native-10800.log`, `final-oracle-10800.log` — proof runs of
  the shipped patch artifacts. (`final2-*.log`: re-proof after a
  comment-only wording correction in the patch — rebuilt from the
  final artifact, hash streams identical.)
- `final-native-21600.log`, `final-oracle-21600.log` — the 21600-frame
  soaks (no stall #3).
- `work/` — build/configure logs for the dev and final trees.
