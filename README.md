# Road Rash 3 — native PS Vita port

A from-scratch native port of **Road Rash III** (Sega Genesis, 1995) to
the PlayStation Vita: the game's 68000 code statically recompiled to
native C with
[segagenesisrecomp](https://github.com/mstan/segagenesisrecomp), running
on a reimplemented Genesis machine layer (VDP, bus, Z80, YM2612/PSG) —
no emulation.

The recompilation record this port is built on (recon, cosim,
campaign reports, engine fixes) lives in the sister repo
**road-rash-trio**.

**You must supply your own ROM.** No game binary is included in this
repository, in any form. Developed against the USA/Europe ROM
(2 MiB, MD5 `dce9b99a3d692cb4463f9f35d278659b`).

## Status

- **PC build (in `engine/`): complete** — full speed with sound, races and
  two-player split-screen, verified over long play sessions.
- **Vita build: in active development, and the crash hunt has had a big
  week (updated 2026-10-21).** There turned out to be **two separate
  deaths**, not one:

  - **The interpreter death — SOLVED.** The engine's reference interpreter
    (used as a diagnostic backend) died inside the very first emulated
    tick on Vita, while the PC build ran it cleanly. Root cause: the
    game's code runs on a cooperative fiber (minicoro), and the engine's
    interpreter entry path executed an `fprintf` on that fiber. File
    IO/stdio from inside the fiber kills the process on this platform —
    the fiber tolerates at most one such write. (Several diagnostic
    builds then "re-poisoned" themselves by logging from the fiber; the
    ladder experiments that isolated this are written up in `docs/`.)
    **Fix, now in `engine/runner/glue.c`: the game fiber does zero
    stdio/file IO — the two fiber-side `fprintf`s are deleted.** With
    that, the interpreter runs indefinitely on the Vita and is
    **bit-exact against the PC interpreter** (identical instruction and
    yield counts over 600 frames; it ran past tick 10,020 on hardware),
    so there is now a trustworthy reference *on the device itself*.
  - **The native death — cornered, not yet fixed.** The recompiled build
    still crashes (C2-12828-1) in the attract-mode demo race, around tick
    5,353, with the simulation bit-identical to the PC reference at every
    checkpoint up to the edge. Using a dispatch mask that substitutes the
    interpreter for chosen guest functions (`src/rr3_dispatch_mask.c`,
    the `RR3_MASK_*` options in `CMakeLists.txt`), the fault has been
    bisected function-by-function:
    - **"Killer B" is named: guest function `func_024A4C`** — substituting
      it (which routes its subtree through the interpreter) eliminates a
      second, later death at demo tick ~5,579.
    - **"Killer A" is named: guest function `func_02823A`** — the last
      of the 40 candidates, convicted 2026-10-20. (An earlier revision
      of this README put A in a different set of candidates entirely —
      a flipped inference, caught and corrected the same day; see the
      correction note in `docs/rr3-crash-hunt-2026-10-19.md`.)
    - **The bisect tool then indicted itself.** Long runs that mix
      native and substituted functions die at ticks that move with the
      size of the substitution (7,279 / 7,456 / 7,956 / 14,481 across
      builds) — a cumulative "capsule drift" in the substitution path,
      deterministic per configuration and absent in both pure modes.
      It is a tooling artifact, not a game bug; the forensics are in
      the 2026-10-21 update in `docs/`. The fix is a **tick gate**
      (`RR3_MASK3_GATE_TICK`): a substitution range engages only just
      before the death under study, cutting exposure by >95% and
      making late-death bisections trustworthy again.
    - **"Killer C" (the third death, tick 18,797) is confined to 17
      candidate functions** (guest `0x20608`–`0x21748`); naming it is
      the current work.
    - **A fourth death, "Killer E", was found behind C** at demo tick
      ~19,985–20,027 (the exact tick shifts slightly with the build's
      substitution configuration), native in every configuration seen
      so far. It is queued behind C.
    - The earlier subsystem bisections stand: the Z80, audio renderer,
      DMA, H/V-interrupt delivery, display path, threading topology,
      stacks, and both compiler-optimization axes were all cleared
      before the fault was isolated to individual recompiled functions.

  Sound output on Vita remains a later milestone. Expect a
  well-instrumented work in progress, not a finished game.

## What's next

1. Finish naming Killer C (17 candidate functions left), then bisect
   Killer E the same way.
2. Read the convicted functions' generated code against the interpreter
   to identify the actual codegen defect(s); the prime suspects are the
   recomp's split-stack / skip-return bookkeeping paths.
3. Convert the mask workaround into a permanent, targeted fix (the
   convicted functions are small and called rarely; running them
   interpreted permanently costs nothing measurable).
4. Long soak test (well past 20k attract ticks) to prove no further
   deaths are hiding behind E.
5. Then the shipping list: performance pass, audio output, splitting
   the work across Vita cores 0/1/3 (sim / presenter / audio), the
   final presentation (pixel-perfect view with an ambient-glow fill),
   and a Select-button pause menu with save states (the Genesis pad
   has no Select, so the Vita's is free to claim).

## Building

You need VitaSDK and a legally obtained ROM placed as `rr3.bin` next to
`CMakeLists.txt`. Then:

```sh
cmake -S . -B build
cmake --build build -j4
```

produces `build/rr3vita.vpk`.

## Layout

- `src/` — the Vita front-end and every bisect glue variant used in the
  crash hunt so far (staged drivers, the no-DMA variant, the dispatch
  mask).
- `generated/` — the recompiled game code.
- `engine/` — the Genesis machine layer the port compiles against
  (vendored from the trio repo; its `LICENSE.md` / `LICENSE-recompiler`
  apply, PolyForm Noncommercial).
- `docs/` — the crash-hunt forensic analyses.
- `art/` — icon artwork (the in-race BETA icon that ships in test
  builds); the built icon is `sce_sys/icon0.png`.

## Working with an AI assistant

In the open: much of the porting, tooling, documentation, and crash-hunt
analysis here was done by an AI assistant (Muse) working with the
project owner, who directs the work, does all the testing on her real
Vita, and makes every judgment call.

## Why

Road Rash 3 is the owner's favorite Genesis game. This port is for her
Vita.
