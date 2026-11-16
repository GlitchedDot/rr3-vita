# RR3 Standings — Mini Qwen Lane

Purpose: productive parallel work on the RR3 standings bug, done by Qwen
(Qwen3 8B, OpenCode) on the 2018 Mac mini, in single-shot tasks, with
Candy Pebbles as the merge/review gate. Nothing from this lane edits the
real rr3-vita tree or any build. Output comes back as notes / a patch
diff, which Candy verifies against the real source before anything is used.

## Correction to the original pitch
Keep OpenCode on **Qwen3 8B** (Build + Plan, as already wired).
Do NOT switch this lane to qwen2.5-coder:7b — coder-tuned small models
can print tool calls as plain JSON text instead of executing them, and a
model that cannot call tools cannot drive OpenCode. (The coder model
works in decomp.wtf's own agent on the gaming PC; that is a different harness.)

## The bug (Lola-reported, her screen is the verdict)
- Standings show the first 3 positions wrong.
- PLAYER A reads 1st / $1000 even when Lola did not finish first.
- Times column shows ".0" on every line.

## What is already known (desk read / scout, 2026-11-10 — read-only, verified)
- Chain: func_014E5C (rr3_part11.c, orchestrator) calls func_014BDA
  (rr3_part11.c:5000, builds/sorts the results table), then func_0155BE
  (rr3_part12.c:1601, builds the standings text lines).
- Data: rider names at guest 0xFF7414 (16 x 10 bytes). Parallel per-rider
  word arrays 0xFF74A0 / 0xFF74C0 / 0xFF74E0 walked via A3.
  0xFF74E0 is the order/key table (init 0xFFFFFFFF x 16 longs, bubble-sorted).
- Prizes are COMPUTED, not a table: base 100 (guest 0xFF7470 = 0x0064)
  x ROM multiplier table at rr3.bin 0x15EDC = [1, 2, 4, 6, 10].
  So $1000 for 1st is downstream of the order. There is no prize ladder
  in the ROM — do not chase one.
- Bug 1 prime candidate: func_014BDA's sort. A SAMPLED read suggested the
  swap exchanges only the +2 words of adjacent 4-byte entries
  (0xFF74E2 <-> 0xFF74E6) — a partial swap would decouple names from keys.
  Swap completeness was sampled, NOT exhaustively verified. That is Task 1.
  Alternative in the same function: equal / uninitialised keys would leave
  roster order standing, and roster order starts with PLAYER A
  (matches the symptom exactly).
- Bug 2: the time formatter func_0158D4 (rr3_part12.c:3211, sole caller
  func_0155BE) renders MM:SS.T faithfully from a tenths word. ".0" on
  every line means every input arrives as a multiple of 10 — the fault is
  in the data fed to it, NOT the formatter. Do not "fix" the formatter.
- Known gap: the race-side writer of the per-rider time records was NOT
  located in the scout pass. That is a later slice, one part-file at a time.
- Dead end: the stride-10 run of 1000 words at ROM 0x1855C6 is frontend
  code/immediates, not a prize table. Do not chase it.

## Source in this kit
Verbatim copies from ~/workspace/rr3-vita/generated/ (verified 2026-11-11):
- source/rr3_part11.c — func_014BDA at line 5000
- source/rr3_part12.c — func_0155BE at line 1601, func_0158D4 at line 3211
- source/rr3_decls.h

## Lane rules
- Single-shot tasks only. One function, one file, one output file per task.
- Setup steps are named separately in each prompt, each with its own
  failure exit. A failed setup step is noted in one line and stops the
  task — no retries in a loop, no side quests.
- New files only. Never edit the .c copies. A patch, when its turn comes,
  is delivered as a separate diff file.
- Every claim must quote the real line and give its line number.
  A report naming functions/modules that are not in the kit is a failed task.
- Candy re-reads the cited lines in the real tree before any verdict
  from this lane is used.

## Slice queue (each is a separate single-shot prompt, in order)
1. TASK-1: exhaustively verify the func_014BDA swap (see TASK-1-PROMPT.md).
2. TASK-2 (after Candy grades Task 1): if the swap is partial, draft the
   corrected swap as a diff file only, with the original lines quoted.
   If the swap is complete, Task 2 becomes: list every comparison in
   func_014BDA and state exactly when two entries are NOT swapped
   (the equal-key path).
3. TASK-3+: locate the race-side writer of the per-rider time words,
   one generated part-file per task (search for writes to the
   0xFF74A0 / 0xFF74C0 arrays), until it is found.
