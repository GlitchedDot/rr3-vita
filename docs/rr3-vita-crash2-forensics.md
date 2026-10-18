# RR3 Vita crash #2 forensics (Build B) — prepared 2026-10-16 evening (PDT)

Sources: `~/workspace/user/files/psp2core-1792203569-0x0003c532c1-eboot.bin.psp2dmp`,
converted core at `hidden_files/vita-crash2/core.elf`, and
`~/workspace/user/files/breadcrumb.txt`. Method validated against crash #1:
the same PROCESS_INFO/THREAD_INFO field and addr2line mapping reproduce
crash #1's known result (func_00AC48, rr3_part04.c:6005) exactly.

## Findings

- **Crash #2 fault PC: 0x81012d78** (PROCESS_INFO and THREAD_INFO both carry
  0x81012d79, thumb bit set; cause word 0x03c532c1, matching the dump filename).
  Build B's raw ELF (`~/workspace/rr3-vita/build-b/rr3vita`, with debug info)
  maps it to **func_000200 (ROM $000200), generated/rr3_part00.c:187** —
  a *different* site from crash #1 (func_00AC48, 0x81071d28). The Release
  config did not prevent the crash, so the build-A "no build type" mismatch
  is not the story.
- **The faulting instruction is the same species as crash #1:**
  `ldr.w r2, [sl]` at 0x81012d78, where sl is loaded once in func_000200's
  prologue (movw/movt) with the fixed address **0x813afaf8 =
  g_audio_cycle_counter**. The surrounding code is the per-instruction
  bookkeeping block (native_insn_count++, cycle += 8, compare against
  g_vblank_threshold). Crash #1 faulted on the analogous store to
  **g_cycle_accumulator at 0x813afb5c** — 0x64 bytes away, same BSS cluster.
  Both faults land on invariant, fixed-address cycle-counter accesses that
  execute millions of times per session. Working shape, strengthened:
  an environmental/page-level fault on that BSS page hitting whichever
  bookkeeping access was executing — not two separate game-code bugs.
- **Breadcrumb:** last line is tick=4920, so death came between tick 4920
  and the next scheduled write (~4980). Roll was ~33.36 ms with a constant
  a7 through tick 4140 (attract), then at tick 4200 a7 starts varying and
  roll jumps to ~50 ms with the frame hash changing every 60 ticks —
  consistent (inference) with the interactive race starting around tick
  4200, i.e. B died roughly 720 race ticks in, much earlier than A's
  ~4,546 play ticks. Race roll ~50.06 ms = same 3-vblank slot as build A;
  Release did not visibly move race speed.
- **Instrumentation gap for Build C:** the breadcrumb's 68K PC field reads
  `pc=000000` on every single line — it was never populated. The a7 and
  frame-hash fields work. Fix the PC capture before relying on it.

## Not established

- The actual faulting *data* address / kernel cause is still undecoded
  (same limitation as crash #1: this firmware's THREAD_REG_INFO does not
  hold exception-time user regs). The BSS-page theory is a shape supported
  by two fault sites, not a decoded cause.
- Whether Lola saw anything specific on screen for crash #2 — logs only;
  her report is the verdict on visible state.
