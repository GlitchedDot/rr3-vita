# RR3 Vita — Next Build Spec: Engine Core at -O0 (NOT BUILT — awaiting Lola's word)

Prepared 2026-10-18 (Sun) as a recap-first resume card. Lola's instruction on
2026-10-18 with the Build Z test files stands: **do not build yet — she will
say when to build.** This file is the spec only; nothing has been compiled.

## Where the hunt stands (crash #28, Build Z)

Build Z (all three VDP DMA flavors stubbed at `maybe_start_dma` — 68K→VDP,
VRAM fill, VRAM copy; busy flags still cycling) died in the first ticks too.
DMA is cleared. The named-subsystem removal sweep is now complete:

- Z80 execution (R)
- Audio render (T)
- H-interrupt delivery (S)
- Display / front-end setup (U)
- V-blank handler body (V)
- All interrupt delivery (W)
- Worker-thread topology — sim on main thread (X)
- Fiber-switch dose at 1/16 rate (Y)
- DMA (Z)

Every one removed or neutralized; the interpreter still dies inside 60 ticks,
before the first phase-1 progress line, on both backends — while the PC runs
both backends cleanly, including under ASan/UBSan.

## The one axis never bisected

Build K dropped only the **generated game code** to -O0. The **engine core
itself — glue, machine, VDP, interpreter, chips — has run at ARM -O2 in every
build**. PC -O2 is x86; ARM -O2 codegen of the engine's own code has never
been tested.

## Next build (when Lola says "build")

- Engine core at **-O0**: glue + machine + VDP + interpreter + chips.
- Everything else as in the recent interpreter builds: interpreter engaged,
  debug stats (TICK/SIM/ROLL) on, breadcrumb trail kept, BETA icon
  (race-frame crop + angled red badge) wired in.
- Delivery per standing rule: bare `.vpk` file, attachment label is the
  plain filename itself, following the `rr3vita-<letter>.vpk` pattern.

## Verdict key

- **Survives past the ~60-tick / phase-1 threshold:** ARM -O2 codegen of the
  engine core is convicted — follow-up is narrowing which engine file(s)
  miscompile (per-file -O0/-O2 bisect).
- **Still dies inside 60 ticks:** engine codegen is exonerated too, and the
  fault is in the engine skeleton itself as it runs on Vita — the
  per-scanline machine advance (minus DMA), per-frame glue
  bookkeeping/telemetry, fiber parks/polls, or the CPU core — despite all of
  it being clean on PC.

## Desk-check note already done

`glue_service_vblank` / `glue_end_of_wall_frame` were read after Z: only
bookkeeping, telemetry snapshots, and a rate-bounded evidence tick — nothing
with teeth found there.

Sources: tracked RR3 updates for crashes #24–#28 (builds V–Z), 2026-10-18;
Lola's "DONT BUILD, yet. Ill tell you when you build." (2026-10-18T07:01Z).
