# Vetting: mstan/segagenesisrecomp — Genesis static recompiler ecosystem

**Date:** 2026-10-15 · **Purpose:** toolchain candidate for the Road Rash 3 native PS Vita port (static-recomp route, decided by Lola 2026-10-15)
**Method:** recursive clone + code/docs read + build + test-suite run + synthetic-ROM end-to-end smoke test on this VM. Claims below are marked **[code]** (verified in source), **[built]** (verified by compiling/running here), or **[docs]** (project's own documentation only).

## Overall verdict: **ADOPT, with caveats**

No hard blockers found. The recompiler core is real, builds cleanly, and generates correct-looking C end-to-end. The runtime is a complete clean-room Genesis (VDP, bus, YM2612, PSG, Z80) in portable C with an interpreter correctness floor. The two honest caveats: (1) the fully-shipped games are all Sonic titles backed by famous community disassemblies — the no-disassembly bring-up path (Rocket Knight Adventures, Puyo) exists and is methodologically sound but is still in progress in-tree; (2) nothing has ever run on ARM32 or Vita — the desktop front-end must be replaced (a clean seam for that exists), and the coroutine backend's ARM32 path is present but engine-untested.

## Snapshot facts

| | |
|---|---|
| Clone | `~/workspace/rr3-vetting/segagenesisrecomp`, 181 MB with submodules |
| HEAD | `f875aa8` (2026-10-08, netplay merge PR #21) |
| History | 329 commits, 143 in the last 90 days — actively developed |
| Repo created | 2026-03-25 (~7 months old) |
| GitHub | 44 stars, 12 forks, 7 tags, 1 release |
| Submodules | `m68k-recomp-core` @ 921f67b · `z80-recomp-core` @ 0e6606a · `rbengine` (MIT) · `recomp-net` (MIT) · `s1disasm` |
| Maintainer shape | Effectively one primary author (Matthew Stan) + R.A.I.D. community; bus-factor risk is real |

## What I verified by building/running here [built]

- **Recompiler CLI builds first try** with the VM's stock cmake 3.28 / gcc 13 (`cmake -S recompiler -B build && cmake --build`), ~5 s, only snprintf-fortify warnings in the emitter.
- **Framework test suite: 15/15 CTest harnesses pass** — validator, decoder synthesis, codegen diagnostics, return-capture, effective-address semantics, ROM parser, plus runner-side tests including a fiber snapshot test (so `fiber_compat.c` compiles and works on Linux).
- **End-to-end smoke test:** I fabricated a minimal 512 KB Genesis ROM (SEGA header, reset vector, a 4-instruction loop + BSR subroutine) plus a minimal RKA-style `game.toml`. The CLI parsed the ROM, ran discovery (found both functions, reported coverage stats), and emitted the full output set: `<prefix>_decls.h`, 32 `<prefix>_partNN.c` TUs, `<prefix>_dispatch.c` (sorted table + binary search), `<prefix>_layout.c`, `<prefix>_cycles.c`. The generated C is legible register-machine C: explicit flag computation per instruction, per-instruction cycle/audio stamps, and a VBlank-threshold check — exactly the shape the headers promise.

Not run here: a real game (ROMs are deliberately not in the repo; users supply their own) and the full desktop runner executable (it is assembled by game repos around generated code).

## Architecture, as found in code [code]

**CLI flow.** `GenesisRecomp <rom> [--game game.toml] [--output-dir dir] [--fail-on-unsupported]`. Loads the ROM (header parse; checksum is computed/reported, not enforced), merges the game's evidence inputs, runs discovery in the shared core (`external/m68k-recomp-core`: `common/m68k_decoder.c` + validator; `profiles/genesis/function_finder.c` ~1,100 lines; `profiles/genesis/code_generator.c` ~5,500 lines), splits functions at boundaries, and emits the part/dispatch/layout files.

**Generated-code ABI.** One global `M68KState g_cpu` (D0–D7, A0–A7, SR, PC, USP shadow), a global 4 MB `g_rom[]` and 64 KB `g_ram[]`, and all memory traffic through runner-implemented `m68k_read8/16/32` / `m68k_write8/16/32` bus functions. Direct JSR/BSR become direct C calls; dynamic transfers go through `call_by_address` / tail-call trampolines that keep 68K tail-jump chains off the host stack; return-address-discarding idioms (`addq.l #4,sp; rts`) are propagated via a pending-return counter; TRAP/ILLEGAL/A-line/F-line, STOP, and RTE have real runtime semantics rather than comments.

**Interpreter fallback (the "Tier-3 floor").** `runner/m68k_interp.c` (~1,600 lines) executes any dispatch miss from its target PC until RTS, covers RAM-resident code (RAM trampoline resolver + RAM-stub dispatcher), and can drive the **whole program** (`GENESIS_FORCE_INTERP`) — that whole-program interpreted mode is what their "oracle" builds use to harvest executed-PC evidence and to co-simulate against the recompiled build. Unsupported execution halts loudly with PC + opcode instead of silently corrupting state. Codegen likewise records every unsupported path (`codegen_diag`) and can fail the build on them.

**Runtime — everything is in this repo, in `runner/`** [code]:

| Subsystem | Implementation | Notes |
|---|---|---|
| VDP | `video/genesis_vdp.c` (~1,000 lines), own code | Full register file, DMA (68K→VDP, fill, copy) with 68K stall cycles, window plane, sprite overflow/collision, interlace modes 1/2, H-int; per-scanline ARGB renderer (scalar is the default/qualified path; an experimental batched renderer is opt-in per title) |
| Bus | `video/genesis_bus.c` | Memory map, Z80 bank window, BUSREQ/reset handling, battery SRAM geometry parsed from the ROM header |
| Scheduler | `video/genesis_machine.c` + `glue.c` | Per-scanline: advances the 68K (game fiber), steps the Z80, ticks the VDP, delivers V/H interrupts, emits scanlines via a sink callback |
| YM2612 FM | `audio/ym2612_ymfm.cpp` | **ymfm** (Aaron Giles, BSD-3) behind a small interface — not hand-written, despite the README's "own YM2612" phrasing; the project's audio-fidelity doc shows measured A/B validation of the FM/PSG paths against a reference |
| PSG | `audio/sn76489.c` (~300 lines), own code | SN76489 |
| Z80 (sound CPU) | **superzazu/z80** core (MIT), embedded in the machine struct | **Interpreted by default.** A static Z80 recompiler exists but is optional/experimental (off unless `GENESIS_Z80_RECOMP=ON`, generator lives in the sibling smsggrecomp project, per-instruction fallback to the interpreter on any mismatch). Because the Z80 is interpreted, *any* uploaded sound driver — EA's included — runs unmodified. The SMPS-specific tracing in the machine file is Sega-driver telemetry tooling, not a runtime requirement |
| Audio delivery | `audio/mixer.c` + event queue | FM/PSG register writes are cycle-stamped onto an event queue; the mixer drains per wall-frame into caller buffers (see `sim_step.c`) |

**The simulation seam.** `runner/sim_step.c` exposes `genesis_sim_step(inputs, audio buffers, hooks)`: one NTSC tick — run the game frame, set pads, run the machine frame, drain audio, service VBlank. This is the exact interface a new platform front-end needs.

## Runner / platform layer — the Vita question [code]

**The core is platform-free.** Grep-verified: no SDL, pthread, Win32, sockets, or OS headers anywhere in `glue.c`, the machine/VDP/bus, the audio chips/mixer, the interpreter, or the Z80 glue. SDL appears only in `main.c`, `audio.c`, `gamepad.c`, `input_map.c`, `app_config.c` (front-end concerns) and netplay. The runner core's only OS touches are `stdio` and `getenv` feature knobs (all default-off; harmless if the Vita libc returns NULL).

**Do not port `main.c`** (3,892 lines: desktop windowing, save-state browser UI, rewind filmstrip, netplay lobby, debug TCP server). The Vita port is a *new thin front-end* around the core via `genesis_sim_step`: read Vita buttons → sim input; scanline sink → framebuffer → SceGxm or an SDL2 texture; mixer drain → `sceAudioOut` or SDL audio. The debug command server already has a stub variant selected by the runner's CMake (`TRACE=OFF`). Netplay, rewind (rbengine), and the save-state browser are droppable for v1 without touching the core; frame-boundary save states can be revisited later.

**Fibers — the one real porting item, and it's better than the README suggests.** The README says the scheduler is backed by Win32 Fibers / ucontext. The code says otherwise: `runner/fiber_compat.{h,c}` is a Win32-shaped fiber API implemented on **vendored minicoro in assembly mode**, with engine-owned stack mappings so a suspended fiber can be snapshotted/restored in place (that's for rollback netplay and rewind). Findings:

- The vendored minicoro **contains a complete 32-bit ARM EABI context-switch backend** (saves d8–d15 under hard-float ABIs, r4–r11, lr, sp) inherited from its LuaCoco lineage. Vita is ARMv7 hard-float EABI, so the backend's target triple matches.
- But the engine only *supports/tests* Win64, SysV x86-64, and aarch64 (their vendored note says so) — ARM32 is present-but-unproven in this codebase, and the inline asm block may need ARM/Thumb mode care under the VitaSDK toolchain. Budget a small spike to prove `fiber_compat` on Vita hardware early.
- Two Vita adaptations are needed regardless: fiber stacks are allocated with `VirtualAlloc`/`mmap` + a no-access guard region — on Vita this becomes `sceKernelAllocMemBlock` (fixed-address allocation is available; the guard region needs an unmapped-gap arrangement or can be dropped, it's a diagnostic safety net, not a functional requirement).
- The scheduler's shape is the simplest possible: one main fiber ⇄ one game fiber, strictly two-party ping-pong (`glue.c`). If minicoro fights back, a two-thread + semaphore emulation of the same API is a viable fallback. The snapshot/restore primitives are only needed by the features v1 drops.

**Other platform notes.** ROM is a fixed 4 MB global buffer (RR3's 2 MB fits; no banking/mapper on plain Genesis). Generated code and runtime use explicit-width types throughout; the bus functions centralize the 68K's big-endian accesses, so the Vita's little-endian ARM is fine. Env-var knobs and evidence-file writes (dispatch-miss TOMLs) are front-end/dev concerns to stub or redirect to `ux0:` paths.

**Performance — unknown, plausibly OK.** No ARM measurements exist anywhere in the project. Their own performance doc targets uncapped headroom on lower-power hosts (they name the original Xbox, a 733 MHz Pentium III) and tracks code size as a first-class metric, which is the right instinct for Vita-class hardware (444 MHz stock / ~500 MHz overclocked ARM). The generated C pays per-instruction cycle-stamp + VBlank-check overhead, and the scalar VDP renders per scanline in C — heavier per frame than a tight emulator core, but the same order of magnitude, and any path can stay on the interpreter floor if a hot spot misbehaves. **Recommendation: make a Vita perf spike (FORCE_INTERP + scalar VDP, attract mode) milestone zero of the Vita stage, before investing in full discovery.**

## Per-game workload — what a no-disassembly game actually costs

This is the part the Sonic materials understate and the RKA directory states honestly. Sonic's `game.toml` leans on the s1disasm: an annotations CSV (~1,800 labels), three auto-extracted discovery files (every static JSR/BSR target; 208 jump tables / 1,010 targets; named subroutines), and still carries ~70 hand-audited residual function entries and a data blacklist.

**Rocket Knight Adventures is the true RR3 precedent** [code/docs]: Konami 1993, 1 MB, *no* community disassembly, explicitly the project's first non-Sonic bring-up. Its `game.toml` + development log document the actual workflow:

1. **Pure-heuristic discovery** — vector-table scan + BSR/JSR/RTS walk from entry/V-int seeds, with `jump_table_autodiscovery = true` (validator-gated auto-walk of PC-indexed tables).
2. **`[ram_layout]` starts almost empty** — only the stack fields are load-bearing (initial SSP from the reset vector); game-mode/V-int-routine/player-object fields can be 0 and refined later. This is much less up-front RE than Sonic's file suggests.
3. **Runtime oracle loop** — run the whole game interpreted (attract mode + scripted inputs, thousands of frames), harvest the executed-PC set (`runtime_exec_file`), and use it as an additive evidence gate.
4. **A code-oracle gate is mandatory, not optional.** RKA's log records the failure mode in detail: aggressive discovery drove dispatch misses to zero *and garbled the title screen*, because without a `code_addrs_file` (instruction-start set) every speculative target — including data bytes and mid-instruction addresses — was admitted as a function. Their fix pattern is the workflow: Ghidra analysis supplies the code oracle; runtime misses are triaged one by one against the raw ROM bytes (their triage tooling classifies the producing dispatch shape) and admitted as audited `late_extra` entries (RKA's list is ~100 addresses), never blind-seeded.
5. Status honesty: RKA in-tree is *renders, gameplay bring-up ongoing* — backgrounds/palette correct first, object logic after the miss-triage loop. Puyo is a second in-tree game on the same pure-heuristic path. **No non-Sonic game has shipped end-to-end on this stack yet**; Sonic 1/2/3 (in their own repos) have.

**Stage 0 deliverables for Road Rash 3** (2 MB, EA code), derived from the above:

- ROM identity sheet: header fields, reset SSP/entry, V-int and H-int targets (H-int may be a RAM trampoline — RKA's was; the runtime handles RAM code via the floor).
- A minimal `game.toml`: output prefix, load-bearing `[ram_layout]` stack fields, `[functions].extra` = entry + interrupt entries, `jump_table_autodiscovery = true`.
- A Ghidra project over the ROM (auto-analysis with a correct Genesis memory map) exporting: the instruction-start set for `code_addrs_file`, plus JSR/BSR target seeds and jump-table candidates in their TOML/CSV formats. (The repo's own Ghidra tooling is Sonic-shaped but the formats are generic; an RR3 exporter is a Stage 0 work product.)
- An oracle run: PC runner build with `GENESIS_FORCE_INTERP`, attract + scripted gameplay inputs for several thousand frames → executed-PC file.
- Then the iterative loop: native build → `dispatch_misses.toml` → per-miss ROM triage → `late_extra` / `[[jump_table]]` additions (the four table formats: absolute-long, PC-relative word, bra.w/bra.s trampolines) → regenerate. Also identify RR3's VBlank-wait routine (the recompiler pattern-detects the spin loop and replaces it with a scheduler yield; the interpreter needs the same PC for convergence in co-sim).
- Expect RR3-specific idioms to surface in that loop: EA object-dispatch tables (large absolute-pointer tables are a known shape — the finder's two-step long-pointer walk exists because of RKA's 391-entry object table), function-pointer helpers (the config has a dedicated edge type for helpers that consume a code pointer loaded into A1), and possibly RAM-resident stubs. All have existing mechanisms; none are free.

Realistic sizing: RKA's documented campaign for a 1 MB ROM ran weeks of part-time iteration and is not finished. RR3 is twice the ROM with an unknown-but-probably-heavier dispatch-table density. Stage 0 (identity + minimal toml + Ghidra oracle + first interpreted boot) is days; full discovery convergence is the long pole of the whole project and should be planned as such.

## License [code]

- Engine, recompiler, `m68k-recomp-core`, `z80-recomp-core`: **PolyForm Noncommercial 1.0.0** (© 2026 Matthew Stan). Source-available, *not* OSI open source. Personal/hobby use is explicitly a permitted purpose; any commercial use is prohibited without a separate license (the recompiler license names a contact for commercial inquiries). Distribution must carry the license terms.
- Third parties are permissive and inventoried in `THIRD-PARTY-LICENSES.md`: ymfm BSD-3, superzazu/z80 MIT, minicoro Unlicense/MIT-0 (embedded LuaCoco-derived asm is MIT), SDL2 zlib (vendored 2.28.5 — a Vita build would instead use VitaSDK's SDL2 port), tomlc99 MIT, rbengine MIT, recomp-net MIT, shadow-enhancement code MIT/Apache-2.0. **No AGPL** in any current code path (the retired emulator oracles exist only in git history).
- Generated C is a machine translation of the game's ROM code; the project license can't and doesn't grant rights in EA's code. Their shipping model — binary loads a user-supplied ROM at runtime — maps cleanly onto a Vita VPK that reads the ROM from `ux0:`.

For Lola's purposes (noncommercial personal port, own ROM) the license is compatible. It would block any future commercial or paid distribution, and it means the toolchain can't be relicensed if the project is ever handed off commercially.

## Maturity [docs + code]

The engineering culture is unusually disciplined for a 7-month-old solo-led project: 25 written principles (discovery rules, a mandatory dispatch-miss loop, never editing generated C, disassembly as ground truth, user-verifies-end-to-end), recomp-vs-interpreter co-simulation as the semantic cross-check, boot-smoke baselines with framebuffer hashes, a measured audio-fidelity scorecard, and docs that record failures (the RKA over-discovery regression) as prominently as successes. Coverage honesty is explicit: their coverage doc lists known gaps (some dynamic JSR/JMP effective-address forms are diagnosed-unsupported; memory-predecrement ADDX/SUBX is a diagnosed codegen gap; test harnesses are synthetic, not an exhaustive opcode×EA proof). The interpreter floor converts most such gaps from "wrong behavior" into "slower, but correct — or a loud halt," which is the right failure shape for a port project.

## Verdicts by area

| Area | Verdict |
|---|---|
| Recompiler core | **Strong** — builds and generates correctly here; conservative, evidence-gated discovery; loud diagnostics |
| Runtime completeness | **Strong** — VDP/DMA/raster, bus+SRAM, ymfm FM, own PSG, interpreted Z80 all in-repo and code-verified; audio fidelity is measured, not asserted |
| Runner portability to Vita | **Good bones, real work** — platform-free core + `genesis_sim_step` seam; new thin front-end required; fiber backend needs an ARM32 proof spike + Vita memory allocation swap; drop netplay/rewind/save-browser for v1 |
| Per-game workload (no disassembly) | **Moderate-high, iterative** — RKA is the honest template; Ghidra code-oracle is mandatory; convergence is the project's long pole |
| License | **Compatible for this project** — PolyForm Noncommercial fits a personal hobby port; blocks commercial use |
| Maturity | **Young but disciplined** — 7 months, one maintainer, shipped Sonic titles, unfinished non-Sonic bring-ups |

**Bottom line for the RR3 port:** adopt it. The toolchain does not remove the fundamental cost of a no-disassembly bring-up — discovery convergence on 2 MB of EA code — but it converts that cost from "build a translator and a Genesis first" into "feed evidence to a working machine, one audited address at a time," with an interpreter floor that can run the game before discovery is complete. First three spikes, in order: (1) PC oracle boot of RR3 interpreted, (2) Ghidra code-oracle export, (3) Vita fiber + sim-step perf spike.
