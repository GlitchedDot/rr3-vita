# TASK 3 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm the file source/rr3_part11.c exists in this directory.
   If it does not, write the single line "BLOCKED: source/rr3_part11.c not found"
   into a new file named standings-fixup-guard.md and stop. Do not search
   the rest of the disk, do not download anything, do not research fixes.
2. Confirm line 6283 of source/rr3_part11.c is the read
   `uint16_t _ts014E40 = m68k_read16(0x00FF74E2);`.
   If it is not, write the single line "BLOCKED: tail fix-up not at line 6283"
   into standings-fixup-guard.md and stop.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- At guest $014E40 (lines 6281-6315 of rr3_part11.c), func_014BDA swaps
  ONLY the +2 words of order-table entries 0 and 1 (0xFF74E2 <-> 0xFF74E6).
- The +2 word of an order-table entry is the RIDER INDEX (it selects the
  printed name); the +0 word is the time. So this swap exchanges the
  names on the 1st- and 2nd-place rows and leaves both times in place.
- The swap block is reached via label_014E40, and skipped via
  label_014E58 (line 6316). The decision chain immediately before it
  starts at guest $014DFC (line 6169).

TASK (single shot — one range of one function, one file, one output file):
Read ONLY lines 6169-6281 of func_014BDA in source/rr3_part11.c.
Map every distinct path through this range: which conditions lead to
label_014E40 (the swap runs) and which lead to label_014E58 (the swap is
skipped). The range reads these guest addresses: 0xFF74E0 (line 6171),
0xFF05DA (line 6205), 0xFF05C2 (line 6224), 0xFF05D8 (line 6247),
0xFF05C0 (line 6266); it also masks a register value with 0xF (line 6188).

Write your answer to a NEW file named standings-fixup-guard.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. Every branch in the range, in execution order: the guest address
   (e.g. $014E10), the line number, the exact condition line quoted, and
   where each outcome goes (fall through / goto target line number).
2. A path table: one row per distinct combination of test outcomes that
   reaches label_014E40, and one row per combination that reaches
   label_014E58. Express each test in plain values, e.g.
   "word at 0xFF05DA >= 0x20 (sign flag clear)".
3. A closing summary of exactly two sentences: the first stating, in
   plain values, the complete condition under which the swap runs; the
   second stating what the four 0xFF05Cx/0xFF05Dx words appear to be,
   based ONLY on how this range uses them (say "unknown from this range"
   for anything this range does not show).

RULES:
- Use only lines 6169-6281 of this file. Quote condition lines exactly.
- The file on disk is the deliverable, not your chat summary: before you
  finish, re-read standings-fixup-guard.md once and confirm its content
  matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line summary:
  the complete swap-runs condition, and how many distinct paths you
  counted to the swap vs to the skip.

---
