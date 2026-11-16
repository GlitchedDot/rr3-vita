# TASK 11 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part20.c and source/rr3_part24.c exist
   (source/rr3_part11.c is also in the kit and is used below).
   If either is missing, write the single line "BLOCKED: <filename>
   not found" (naming the missing file) into a new file named
   standings-guard-inputs.md and stop. Do not search the rest of the
   disk, do not download anything.
2. Read these three functions in full, from opening line to closing
   brace: func_01EE98 in source/rr3_part20.c (lines 6867-7225),
   func_0142E4 in source/rr3_part11.c (lines 1442-1938), and
   func_023326 in source/rr3_part24.c (lines 6254-6542).

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- The results table builder func_014BDA has a tail fix-up that swaps
  the rider-name words of the top two entries. It runs only when a
  guard passes, and the guard (traced in an earlier task) tests four
  guest words: if the first-sorted rider's index has low nibble != 0,
  the chain tests word 0xFF05DA >= 0x20, falling back to word
  0xFF05C2 != 0; if the nibble == 0, it tests 0xFF05D8 >= 0x20,
  falling back to 0xFF05C0 != 0.
- A codebase-wide search (done by Candy) found every literal write
  to those four words. The value writes are: 0xFF05C0 and 0xFF05C2
  only in func_01EE98 (rr3_part20.c lines 7083 and 7193, each storing
  a fresh read of word 0xFF05BE); 0xFF05D8 and 0xFF05DA in
  func_0142E4 (rr3_part11.c lines 1608 and 1739, each storing a
  fresh read of word 0xFF05D6) and in func_023326 (rr3_part24.c
  lines 6490 and 6520, storing a read of 0xFF05D6 shifted right by
  1). The only other writes are zero-clears in an initializer
  (func_011D16) — out of scope.
- func_01EE98 and func_0142E4 are both called from func_013D76 in
  rr3_part10.c (call lines 6967 and 7011) — the same function that
  hands the player's race record to its post-race callees.
  func_023326 is called from rr3_part25.c lines 1238 and 1388
  (callers out of scope beyond this note).

TASK (single shot — three functions, one output file):
For each of the three functions, report:
1. Every write it makes to any of these six words: 0xFF05BE,
   0xFF05C0, 0xFF05C2, 0xFF05D6, 0xFF05D8, 0xFF05DA — line number,
   exact line quoted, and the stored value traced back within the
   same function to the read or computation that produced it
   (quote those lines too, including any shift, mask or arithmetic).
2. Every write it makes to 0xFF05BE or 0xFF05D6 specifically (the
   two source words): same treatment — these writes inside the
   function are what the guard words ultimately copy, so their
   origins matter most.
3. The calls the function makes (callee names and line numbers
   only), marking which calls fall between the source-word writes
   and the guard-word copies, if any do.
4. Any condition guarding the guard-word copies (quote the branch
   line if a copy is conditional).

Write your answer to a NEW file named standings-guard-inputs.md in
~/rr3-standings-mini-lane (do not edit any other file), containing
one section per function in the order func_01EE98, func_0142E4,
func_023326 (items 1-4 each), then a closing summary of at most
four sentences answering: at the moment func_014BDA's guard reads
them, what do the four guard words hold — copies of which source
values, transformed how — and does anything in these three writers
tie the source words 0xFF05BE / 0xFF05D6 to a race quantity (a
speed, a distance, a position, a count) whose normal finishing
values would clearly pass or clearly fail the guard's >= 0x20 and
!= 0 tests? If the writers do not establish what the sources
measure, say exactly that.

RULES:
- Use only the three named functions in their three files. Quote
  lines exactly, including variable names. If a value's origin
  cannot be sourced inside its function, write "origin not traced
  within this function" — do not guess and do not read other
  functions.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-guard-inputs.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: what the guard words hold, and whether their sources are
  identified as a race quantity or remain unidentified.

---
