# Road Rash 3 — Band Chase: Fix #2 Debt-Charge Experiment (2026-10-16)

**Outcome: hypothesis FALSIFIED, cleanly. Nothing ships. Fix #2 v1 stays.**

## The hypothesis

Cosim (see `rr3-cosim-report.md`) proved the two backends hold identical
machine state at every frame boundary, so the demo-region framebuffer
divergence (57 of 90 sampled frames) and the visible horizon band had to be
a *sub-frame timing* effect. Prime suspect: when RR3's HBlank raster-split
handler waits for its target line, fix #2 advances a private fake raster
one phase per HV read — but the wait itself costs the game almost no
cycles, while on hardware the handler *occupies the CPU for the whole
wait*. The engine repays handler time via the IRQ cycle debt
(`s_irq_cycle_debt`, drained against the main loop), but only for
*executed* cycles — a few hundred for a spin that represents ~2,400
cycles of beam time. Worse, the executed-cycle count is exactly where the
two backends' accounting can differ, so the main loop resumed at
backend-dependent raster points — a phase slip that fits the evidence.

## The experiment (engine-bandfix = engine-final + this change only)

- `machine_advance_raster_phase()` now returns the phase's beam time
  (M68K_PER_LINE/2 = 244 cycles; two phases sum to exactly one line).
- Both glue.c hook sites charge it: `s_irq_cycle_debt +=
  machine_advance_raster_phase();`
- The phase count to any target is fixed by the target, so the charge is
  identical on both backends by construction.

## Results

- **Safety: PASS.** 600 frames both modes, settled hash 0xBE99E04DC4DE31A9
  intact; 10,800 frames both modes [DONE]; 0 dispatch misses; 0 unique
  true-miss addresses. The charge visibly worked as designed — both
  backends' demo phases shifted *together* vs baseline (~30 sampled
  frames each over 4920–8400).
- **Verdict metric: FAIL.** Native-vs-oracle sample disagreement over
  10,800 frames: **56 of 90 (baseline: 57)**. Frame 4920 aligned; nothing
  else did. The backends still paint the demo differently.

## What this establishes

The divergence is **not** a main-loop resume-phase effect. With state
identical at boundaries (cosim) and the resume phase now provably
synchronized (this experiment), the remaining mechanism is the atomic
delivery itself: the handler's timed upload + register write **fire at
the delivery line** in the runner's render order, while on hardware they
fire at the **wait-target line** — and the exact sub-frame interplay of
the six chained deliveries per frame with the per-line render loop is
where the backends' pictures part ways. No accounting change can move a
fire that happens inside an atomic delivery.

The true fix would be deferred visibility: queue VDP register/data
writes made during atomic H-int service and apply them when the
scheduler reaches the handler's target line. That is engine-wide surgery
on the shared bus/VDP write path (every game on this engine), with a
regression surface far out of proportion to a cosmetic artifact in one
game's attract demo — and upstream's own HV-counter comment ("Refined
during raster-timing validation") marks this layer as known-approximate.

Interleaved H-int delivery was considered and rejected without a run:
the level-4 delivery branch has no interleave path in this build (the
interleave scaffold serves parked V-int delivery only), so it cannot
change H-int fire timing.

## Recommendation

- **Keep fix #2 v1.** The bandfix variant is safe but pointless; do not
  ship it. (Tree preserved at `~/workspace/rr3-runner-work/engine-bandfix`
  and logs at `hidden_files/bandfix/` for the record.)
- **Classify the band as a known cosmetic artifact** of the runner's
  atomic-delivery + per-line-render model. The game state is proven
  correct; the picture is correct except the seam placement inside
  raster-split frames.
- **Let the interactive play test arbitrate severity.** The raster-split
  engine also drives the in-race horizon, so gameplay may show the band
  too — if it genuinely bothers there, the deferred-apply project is
  the scoped fix, decided with full knowledge of its cost.
