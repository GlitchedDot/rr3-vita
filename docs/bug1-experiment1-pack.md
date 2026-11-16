# Road Rash 3 — Bug #1, Experiment 1 pack (ready to run on Lola's PC)

*Prepared 2026-10-15 evening (PDT) as the follow-on to `bug1-diagnosis.md`.
All commands are for the **Ubuntu terminal** (WSL) on the gaming PC — the
same terminal she built and booted the runner in on 2026-10-15.*

## Why this experiment, in one paragraph

The diagnosis (in `bug1-diagnosis.md`, root-cause report) found the stall
is scheduler starvation, not a driver bug: the 68K send-command function
at $03AB40 runs only on the interpreter floor, its poll loop has no yield
point, so the scanline loop never reaches `step_z80` again and the Z80
stays frozen mid-pop with the queue-busy byte $000B stuck at $FF. $03AB40
is one of 41 floor-coverage leads. Experiment 1 is the cheap falsifiable
test: regenerate with all 41 leads as native code, so the poll loop's bus
accesses go through `check_cycle_budget`, the chunk yields, and the Z80
gets its slices. If the theory is right, a 2000-frame run sails past the
~frame-1043/1052 crash point in both modes.

## Files

- `rr3-floor-leads.toml` (same folder as this pack) — the 41 addresses,
  extracted programmatically from the VM runner's
  `build/floor_coverage.toml` (engine git f875aa8eac), in the standard
  `[functions] extra = [...]` discovery-file format (same shape the
  engine's Sonic config uses for its `discovery_files`).

## Steps (Ubuntu terminal, gaming PC)

1. Copy `rr3-floor-leads.toml` into `~/rr3/` (next to `game.toml`) —
   via the NAS, her usual transfer route, so no extra copies pile up.
2. Edit `~/rr3/game.toml`: inside the existing `[game]` section, add
   one line:
   ```toml
   discovery_files = ["rr3-floor-leads.toml"]
   ```
3. Rebuild (regeneration is automatic when game.toml/discovery change,
   per the scaffold README):
   ```bash
   cd ~/rr3
   cmake --build build -j
   ```
   Expect the function count to rise above the first-pass 563.
4. The test — native mode, 2000 frames:
   ```bash
   cd ~/rr3/build
   ./RoadRash3Recomp ../rr3.bin --no-launcher --turbo --max-frames 2000 --hash-frames 120
   ```
5. Cross-check — oracle mode, same length:
   ```bash
   GENESIS_FORCE_INTERP=1 ./RoadRash3Recomp ../rr3.bin --no-launcher --turbo --max-frames 2000 --hash-frames 120
   ```

## How to read the result

- **Theory confirmed:** both runs end with `[DONE] 2000 frames completed`
  and no `instruction watchdog` crash around frame ~1043–1052. That also
  means her own eyes get the next verdict: a real-time run past the
  ~17-second mark, title music-start point, is hers to report — logs
  alone don't declare what she sees/hears.
- **Theory falsified:** the watchdog crash still lands at ~frame 1043.
  Then stop — do NOT patch game code and do NOT force $000B=0 (the guest
  behavior is hardware-correct; diagnosis §3). Next step is the
  engine-side fix direction (step the Z80 / yield from the bus-poll
  path), which is VM-side work, and the run log is the evidence to send.

## Status / uncertainty

- NOT yet run on Lola's PC, and deliberately NOT run on Candy's VM in
  this pass: the VM runner's `floor_coverage.toml` was still being
  updated by the live investigation (last_updated 2026-10-16T04:09:52Z,
  6 sessions), so this pack does not overwrite or race that work.
- The 41 addresses are the VM build's leads; they are ROM addresses, so
  they transfer to her PC's identical ROM (SHA1
  b2324833bf81223a9626a6060cafc203dd203468) unchanged.
- This pack builds on the pending Bug #1 diagnosis delivery; it does not
  replace it. If she has not seen the diagnosis yet, send that first.
