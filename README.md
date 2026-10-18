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
- **Vita build: in active development.** The game boots and races on real
  hardware, but a crash (C2-12828-1) still ends runs during racing. The
  hunt is documented in `docs/`; the simulation itself is proven
  bit-faithful to the PC build frame-for-frame, and the Z80, the audio
  renderer, H-interrupt delivery, the display path, and the V-interrupt
  handler body have been bisected and cleared. The fault is isolated to
  the shared scheduling/fiber layer and is still being chased. Sound
  output on Vita is a later milestone. Expect a well-instrumented work in
  progress, not a finished game.

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
  crash hunt so far.
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
