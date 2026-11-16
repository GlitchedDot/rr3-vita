# Road Rash 3 Recomp — Install & Get-Started Guide

*For Lola's gaming PC. Facts grounded in the segagenesisrecomp repo (HEAD
f875aa8) and its README, plus the Stage 0 recon of the RR3 ROM (2026-10-15).
Every command is labeled with the terminal it belongs in.*

## Where this happens

Everything in Phases 1–3 runs in the **Ubuntu (WSL)** terminal on the gaming
PC — the same Ubuntu as the decomp.wtf setup. The recompiler CLI is pure
C11 + CMake and needs no display. (The Windows MSVC variant of the build
commands is included for later, when the runner build wants a native
Windows home.)

End state of this guide: the recompiler built, its test suite passing, and
RR3's first generated C sitting in a folder. The runner build (actually
playing it, oracle boot first) is the following project step.

## Phase 0 — Prerequisites (Ubuntu terminal)

Install the build tools:

```bash
sudo apt update && sudo apt install -y git cmake ninja-build build-essential
```

(build-essential, not bare gcc: the linker needs libc6-dev's startup files —
Scrt1.o/crti.o. A gcc-only install compiles but cannot link; this bit the
first attempt on 2026-10-15 and is fixed by the line above.)

Verify:

```bash
git --version && cmake --version | head -1 && ninja --version && gcc --version | head -1
```

## Phase 1 — Clone and build the recompiler (Ubuntu terminal)

```bash
cd ~
git clone --recursive https://github.com/mstan/segagenesisrecomp.git
cd segagenesisrecomp/recompiler
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

The binary you care about lands at `~/segagenesisrecomp/recompiler/build/GenesisRecomp`.

Checkpoint — run the framework tests:

```bash
cd ~/segagenesisrecomp
cmake -S tests -B build/tests
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

All harnesses should pass (15/15 when this was vetted). If anything fails
here, stop and send me the output — everything after this depends on the
toolchain being healthy.

*Windows (MSVC) variant of the CLI build, for later — **PowerShell**:*

```powershell
cd recompiler
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# binary: recompiler\build\Release\GenesisRecomp.exe
```

## Phase 2 — The RR3 game folder (Ubuntu terminal)

```bash
mkdir -p ~/rr3
cd ~/rr3
```

Put two things in `~/rr3`:

1. **The ROM** — a copy of your Road Rash III (USA, Europe) dump, renamed
   to `rr3.bin`. (From Windows Explorer it's the file you sent me; copy it
   into the Ubuntu home via the `\\wsl$` share or `cp` from its /mnt path.)
2. **game.toml** — the one Candy prepared from the Stage 0 recon
   (attached to the chat message that delivered this guide). It sets
   `output_prefix = "rr3"`, turns on jump-table autodiscovery, seeds
   discovery with the entry point (0x200) and the VBlank/HBlank handlers
   (0x21B6 / 0x24FA), and sets the load-bearing stack fields to RR3's real
   reset stack pointer (0x00FFFFF6).

## Phase 3 — First code generation (Ubuntu terminal)

```bash
cd ~/rr3
~/segagenesisrecomp/recompiler/build/GenesisRecomp rr3.bin --game game.toml --output-dir generated
```

What success looks like: a `generated/` folder containing `rr3_partNN.c`
files (the recompiled functions), `rr3_dispatch.c` (the dispatch table),
and layout files. That is "started": the ROM has been translated to native
C for the first time. Discovery will be partial at this point — that's
expected and normal for a no-disassembly game; coverage grows through the
oracle loop in the next phase.

If the CLI errors on a missing or invalid game.toml key, the message names
the key — send me the exact output and I'll adjust the config.

## What comes after (not in this guide)

- **Runner build + oracle boot (spike 1):** the game needs a small
  per-game spec C file (the Sonic/RKA dirs carry theirs as templates) and a
  CMake glue file linking the generated sources against the runner. First
  boot is in interpreted-oracle mode (`GENESIS_FORCE_INTERP`), which both
  proves the runtime against the real ROM and harvests executed PCs.
- **Ghidra oracle export (spike 2):** the Stage 0 Ghidra project feeds a
  code-address oracle in the toolchain's formats — the regression gate
  that keeps discovery honest.
- **Discovery campaign:** iterate game.toml (jump tables, audited seeds)
  until coverage converges — the long, satisfying part, same shape as
  RKA's.
- **Vita front-end (spike 3 and the end goal):** a small new front-end
  around the dependency-free sim core; the desktop UI is discarded.
