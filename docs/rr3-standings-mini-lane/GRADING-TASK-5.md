# Task 5 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-entry-build.md, 10,538 B, DeepSeek V4.1 Flash Free, 2m39s)
Verdict: PASS. Candy spot-checked 36 quoted lines across lines
5000-5330 of rr3_part11.c (5002, 5010, 5015, 5034, 5038, 5047, 5057,
5072, 5080, 5084, 5093, 5112, 5121, 5129, the Phase 3 lines, 5212,
5216, 5224, 5239, 5261, 5264, 5278, 5283, 5292, 5300, 5309, 5310,
5319, 5327) — every one exact, including the DIVU overflow line 5278
and the negative-path branch 5261/5264. One omission, not an error:
the file quotes the extra source-pointer advance at line 5189 but does
not note it is CONDITIONAL — it executes only on the pass where D[0]
goes 0→1→2 (the $014C22 zero-branch falls through to it; other passes
jump from $014C22 straight to label_014C2C). So the source records are
read at 0xFF741E, then 0xFF7426 (one 8-byte step), then every 4 bytes.
That stride detail matters for Task 6 and is recorded here.

## Established by this task (now verified ground truth)
- func_014BDA builds the table itself. Phase 1 fills all 16 entries
  with 0xFFFFFFFF. Phase 2 runs 15 outer passes (outer index sequence
  0, 2, 3, …, 15 — index 1 is SKIPPED): each pass reads one 32-bit
  record from the source buffer at 0xFF741E onward, forms the sort key
  (record << 8) | index, and insertion-sorts it into the table. The
  slot never written keeps 0xFFFFFFFF.
- Phase 5 (the transform) then rewrites each entry: new +0 word
  (time in tenths) = record / 6, new +2 word = the index byte,
  zero-extended. The record is therefore a race time in 1/60-second
  units (frames): /6 converts frames to tenths. The DIVU overflow
  path (record's top byte > 6 after the shift) and the negative path
  both leave a shifted, un-divided value in the +0 word instead.
- The leftover 0xFFFFFFFF slot transforms to 0xFFFF00FF: time 0xFFFF
  (prints "DNF"), index byte 0xFF.
- The guard puzzle from Task 3 dissolves: at $014DFC the "entries"
  are still sort keys of the form (record << 8) | index in the
  function's later phases — bit 31 of entry 0 is the top bit of the
  RECORD of whoever sorted first, not of a finished time word.

## What is still open
The records in the source buffer at 0xFF741E (15 records; first step
8 bytes, then 4-byte stride) are written before func_014BDA runs —
"origin outside this range" in the file's own words. Who writes them,
with what stride/layout, and whether the values arrive already coarse
(the ".0" question: displayed tenths are record/6, so a ".0" on every
line means every record/6 lands on a multiple of 10) is Task 6.
