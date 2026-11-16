# Road Rash 3 — Stage 0 Recon Report

*Generated 2026-10-15 from Lola's supplied ROM, using Ghidra 12.1.4 headless
(68000:BE:32:default), seeded and unseeded passes, plus targeted disassembly.
All addresses are ROM offsets. This is a sizing/map pass, not a full disassembly.*

## ROM identity (verified)

| Item | Value |
|---|---|
| File | Road Rash III (USA, Europe).md |
| Size | 2,097,152 bytes (exactly 2 MiB) |
| MD5 | dce9b99a3d692cb4463f9f35d278659b |
| SHA1 | b2324833bf81223a9626a6060cafc203dd203468 |
| Header | "SEGA GENESIS", (C)T-50 1995.JAN, title ROAD RASH 3, serial GM T-50966-00 |
| Header checksum | 0xEAD2 — **recomputes to a match; the dump is intact** |
| Region | UEJ |
| Save RAM | **None declared** — RR3 is a password game. No SRAM emulation needed in the shell |
| Entry point | 0x000200 |
| Initial stack | 0x00FFFFF6 |

## Code vs data — the sizing answer

Two Ghidra passes bracket the truth:

- **Unseeded auto-analysis (floor):** 256 functions, ~52 KB of instructions.
  A known undercount — a raw binary gives the analyzer almost no entry points.
- **Pointer-seeded analysis (ceiling):** 6,765 functions, ~305 KB of instruction
  bytes. Seeded from every vector target plus every aligned longword in the ROM
  that points back into ROM space, so it includes false-positive "functions"
  inside graphics/table data.

**Read: at most ~300 KB of the 2 MB ROM is 68k code (≤15%). The other ≥85% is
data** — tiles, track/road data, music sequences. For the static-recompiler
route this is good news twice over: the code volume to cover is modest, and the
recompiler's interpreter fallback stands behind whatever its discovery misses.

## Architecture map

- **Boot (0x200):** standard Sega boot — TMSS/version checks, then register,
  VDP and Z80 setup driven from an init table stored at 0x28E. Falls into a
  region/version check at 0x2FA (reads a region byte table at 0x436) and then
  the main init function at **0x690**.
- **VBlank handler (0x21B6):** calls a per-frame callback whose pointer lives
  in RAM at 0xFF0226, checks the sound-alive magic (0x1234 at RAM 0xFF0000),
  and ticks the sound driver (command 1, below). HBlank handler at 0x24FA.
  All other exception vectors funnel to a shared stub at 0x24EC.
- **VDP access** is spread across many small helpers (control port referenced
  from ~70 functions in the seeded pass, data port from ~48), concentrated in
  the 0x2FA–0x11BA8 span plus per-screen upload code. No single "VDP engine"
  function — the shell just needs to honor the register/DMA protocol.

## Sound subsystem — fully sized

The 68k-side sound code occupies roughly **0x3A4A8–0x3B600 (~4.4 KB)**:

- **Dispatcher at 0x3A4A8:** takes a command index in D0 (0–0x13, 20 commands)
  and jumps through a branch table at 0x3A4CE. The VBlank handler calls it
  with command 1 every frame (the driver tick).
- **Command 0 = driver upload/init (0x3A53C, the only function that touches
  Z80 reset):** requests the Z80 bus (0xA11100), asserts reset (0xA11200),
  validates that the image starts with 0xC3 (Z80 JP), copies it into Z80 RAM
  at 0xA00000, releases the bus, then polls a handshake byte at Z80 RAM+0x4F
  until the driver answers 0xFF, and sets the 0x1234 alive-flag in 68k RAM.
  Bus helpers at 0x3B54A / 0x3B55E.
- **The Z80 driver image itself: ROM 0x3B568–0x3D08E, 6,951 bytes**
  (MD5 b7b451ef47577d033a7c5b59b1178c93), entered via `JP 0x0533`, filling
  **85% of the Z80's 8 KB RAM**. This is the complete music/SFX sequencer
  that drives the YM2612 and PSG — a self-contained second program, and the
  single most well-defined work item in the whole project.

## What Stage 1 needs from this

- Recompiler seeds: entry 0x200, VBlank 0x21B6, HBlank 0x24FA, the 20-entry
  sound branch table, and the harvested entry-point lists from this pass
  (saved with the raw outputs).
- The Z80 driver is a separate translation target (6,951 B) from the 68k
  code — how the chosen toolchain treats it is part of the vetting verdict.
- Shell scope confirmed: VDP, YM2612 + PSG (driven by the translated driver),
  joypads, 60 Hz timing. **No SRAM, no extra cart hardware.**

## Caveats

- The seeded function count includes false positives in data regions; treat
  ~300 KB as a ceiling, not a census. Numbers will tighten once the
  recompiler's own discovery runs.
- "Main init at 0x690" is a strong inference from the boot flow, not yet
  traced end-to-end.
- Ghidra projects, scripts and raw outputs are kept in this goal's
  hidden_files/ (ghidra-proj, ghidra-proj2, ghidra-out).
