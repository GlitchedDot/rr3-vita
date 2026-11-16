# Road Rash 3 — Cosim Referee Report (2026-10-16)

**Question put to the referee:** the native (recompiled) and oracle (interpreter)
builds produce different framebuffer hashes on 57 frames in the attract-demo
region, and the demo shows a visible band wobbling at the horizon. Is the
recompiled code wrong somewhere — or is the difference only in presentation
timing?

**Instrument:** the engine's own differential co-simulation (pairing #1,
`GENESIS_COSIM`): recompiled 68K vs the clean-room Tier-3 interpreter, run in
lockstep on the dual-patched engine (fix #1 + fix #2), full machine state
hashed at every frame boundary across 10 subsystems — cpu68k, timing, ram,
z80, z80ram, handshake, vdp, fm, psg, evq.

## Method

- Cosim target built from the campaign configuration (597 functions, gate
  armed at 79,054) plus the cosim server TUs and the per-game cycle table
  (`rr3_cycles.c`, already emitted by the campaign regen).
- RR3 has no recompiler-generated WaitForVBla stub (no generated function
  calls the yield seam), so the coordinator ran with the convergence stub
  disabled: both backends execute the game's real frame-wait spin.
- Validation gates first, per the engine's COSIM.md: recomp-vs-recomp and
  interp-vs-interp must be bit-clean before any pairing result is trusted.

## Results

- **Gate 1 (recomp vs recomp):** 300/300 checkpoints, all subsystems, 0 divergence.
- **Gate 2 (interp vs interp):** 300/300 checkpoints, all subsystems, 0 divergence.
- **Pairing #1, frames 0–4600** (boot, title, the frame-1043 Z80 region): 4,600/4,600 frames, 0 divergence in every subsystem.
- **Pairing #1, frames 4600–10800** (HBlank engine arms at 4774, attract race, demo loop): 6,200/6,200 frames, 0 divergence in every subsystem.

**The full 10,800-frame arc: the recompiled code and the interpreter hold
bit-identical machine state at every single frame boundary.**

## Verdict

1. **There is no codegen bug.** The recompiled 68K reproduces the interpreter
   exactly — CPU, work RAM, Z80 and its RAM, FM, PSG, and VDP state included —
   through every region previously in question.
2. **The 57-frame framebuffer divergence is a sub-frame presentation effect.**
   Both machines end every frame in identical state; the rendered pictures
   differed only where the six-band raster-split engine reprograms the VDP
   *mid-frame*, where the image depends on the exact scanline at which each
   band's upload lands — information a frame-boundary state hash cannot see,
   and which re-synchronizes every frame (why the divergence never cascades).
3. **The band wobble has the same home.** Prime suspect remains fix #2's
   whole-phase raster advance per HV-counter read inside atomic HBlank
   delivery: the two backends execute the handler's spin at different read
   counts, so the simulated raster steps differently and the horizon seam
   lands a line early or late, frame to frame. It is an approximation error
   in the shared runner's timing layer — not in the game code, and not in
   the discovery campaign's output.

## What this referee cannot say

Pairing #1 shares one runner between both sides, so it cannot judge which
backend's seam placement is closer to real hardware, and it cannot make the
band disappear. It settles *where the difference lives* (sub-frame timing in
the shared runner) and *where it does not* (the recompiled game).

## Consequences for the road to PC 100%

- The native-vs-oracle divergence is **closed as an artifact**, not a bug.
- The band is a **cosmetic presentation-fidelity item**. If it is to be
  chased, the fix path is refining fix #2's advance granularity in the
  runner (proportional rather than whole-phase steps) — a contained
  experiment with Lola's eyes as the verdict, since state hashes cannot
  judge it. The same six-band engine drives the in-race horizon, so the
  interactive play test may show it too.
- Remaining queue unchanged: micro-campaign (599 functions + state-table
  conversion), interactive play test, then the PC-100% call is Lola's.
