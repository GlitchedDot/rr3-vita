# GRADING — Task 13 (standings-which-builder.md, 13,070 B, DeepSeek V4.1 Flash Free)

**Verdict: PASS — and it caught a second Candy given-fact error.**

Verification performed by Candy against the real generated source:
- All sampled quotes check out exactly: part30 lines 1931 (the
  0x000022A0 arming store), 1940-1952 (the wait loop), 1960-1961
  (flag read and -1), 1971-1974 (the branch), 2003 (skip jump),
  2006 / 2029 (labels), 2035 (the merge into func_014E5C);
  part03 lines 273, 330-331, 397, 642; part08 lines 2485, 2494,
  2669, 2956-2957, 2965-2966.
- Independent check: zero assignments to g_cpu.A[1] / g_cpu.A[2]
  in part30 lines 1900-2080, matching the file's A.2 exactly.

## Candy's error #2, corrected by the model (recorded plainly)
The prompt's given facts said func_185E3E "waits in a loop until
the 32-bit value at 0xFF0226 is non-zero". The lines show the
opposite: func_185E3E itself stores 0x000022A0 into 0xFF0226 at
line 1931, then the loop at label_1860A0 spins WHILE the value is
non-zero and exits when it reaches zero — a countdown wait (the
value is decremented elsewhere, interrupt-side), not a
ready-flag wait. Candy paraphrased the loop from an old desk
note without re-reading its polarity — the same failure class as
Task 11's copy direction, one task after the lesson was written
down. Standing rule strengthened: EVERY behavioral claim in a
prompt's given facts gets re-read at the line level, no
exceptions for "simple" loops.

## What Task 13 establishes (verified)
- The builder choice is now settled statically AND matches the
  runtime record: func_01A86E runs iff word@0xFF022C == 1 at
  guest $1860AC in func_185E3E; any other value runs func_01A718.
  Both paths merge at label_1860C8 into func_014E5C (then
  func_014BDA), as the main thread's desk read had it.
- Neither builder receives A[1]/A[2] setup in the call window —
  the registers are live-in from earlier flow.
- Who sets the flag to 1: func_004EF8 (part03) at lines 397 and
  642 — two arms of an entry test on word@0xFF736C, each gated on
  word@0xFF736A - 2 being non-negative; the surrounding traffic
  is mode/state words (0xFF7530 mode word written 2, then 1 or
  the flag pair set). And func_0108EC (part08) at 2669, gated on
  bit 0 of a value derived from a func_027B84 return (*4, word
  swap, +1) — inside a save/convert/restore routine over the
  0xFF7460 block. The computed store (part08:2957, func_010A1E)
  is a RESTORE: it copies back the word that func_0108EC saved
  into 0xFF253C — and the file caught that the save slot is
  written twice in a row (2485 from 0xFF022A, then 2494 from
  0xFF022C), so the second wins and BOTH flags (0xFF022A and
  0xFF022C) are restored to the saved 0xFF022C value.
- Whether the 01A86E path is a mode a normal single race enters
  is NOT determinable from these windows — the setters are
  mode/state machinery, not race code — but combined with the
  main thread's runtime observation (01A718 fired in real
  race-end flow; 01A86E never observed), the fix only needs to
  match func_01A718's behavior unless a future mode test says
  otherwise.

Lane score: DeepSeek 12 passes in 13 tasks; local Qwen3 8B 0/1.
The model has now corrected the grader's given facts in two
consecutive tasks (11 and 13) and confirmed them accurate in one
(12, where they had been line-verified before inclusion).
