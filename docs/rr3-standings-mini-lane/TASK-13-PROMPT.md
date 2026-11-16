# TASK 13 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part30.c, source/rr3_part03.c and
   source/rr3_part08.c exist. If any is missing, write the single
   line "BLOCKED: <filename> not found" (naming the missing file)
   into a new file named standings-which-builder.md and stop. Do
   not search the rest of the disk, do not download anything.
2. Read these windows in full:
   - source/rr3_part30.c lines 1900-2080 (inside func_185E3E)
   - source/rr3_part03.c lines 271-700 (inside func_004EF8)
   - source/rr3_part08.c lines 2446-2750 (inside func_0108EC) and
     lines 2916-3010 (inside func_010A1E)
   If a window ends in the middle of a function, note that in the
   report; do not read past the window to finish the function.

GIVEN FACTS (verified by Candy against the real source — build on these,
but trust the source over this prompt if they ever contradict, and flag it):
- The two results builders from Task 6, func_01A718 (rr3_part16.c)
  and func_01A86E (rr3_part17.c), are each called exactly once in
  the whole codebase, both inside func_185E3E in rr3_part30.c:
  func_01A86E at line 1983, func_01A718 at line 2012 (the second
  under the label label_1860C2). After the func_01A86E call,
  control jumps to label_1860C8, skipping the func_01A718 call —
  so at most one of the two builders runs per pass.
- Just before the calls, func_185E3E waits in a loop at guest
  $1860A0 until the 32-bit value at 0xFF0226 is non-zero, then at
  guest $1860AC computes word@0xFF022C - 1 and branches on the
  zero flag: non-zero result goes to label_1860C2 (func_01A718),
  zero result (word == 1) falls through to func_01A86E.
- Runtime evidence from the main thread's crash hunt (Oct 2026):
  in real race-end flow, func_01A718 is the builder that fired;
  func_01A86E has never been observed firing.
- A codebase-wide search (done by Candy) found every literal
  write to the flag word 0xFF022C. It is written with 0 in
  rr3_part02.c:3077, rr3_part03.c:314, rr3_part03.c:560,
  rr3_part08.c:2537, rr3_part29.c:7773; with 1 in rr3_part03.c:397,
  rr3_part03.c:642 (both inside func_004EF8) and rr3_part08.c:2669
  (inside func_0108EC); and with a computed value in
  rr3_part08.c:2957 (inside func_010A1E).

TASK (single shot — one call region, three flag-writer windows, one
output file):
PART A — the choice (source/rr3_part30.c window):
1. Confirm or correct the given branch structure from the lines
   themselves: quote the wait loop, the word@0xFF022C read, the
   subtraction, the branch, both calls, and the skip jump, with
   line numbers.
2. List every assignment to g_cpu.A[1] and g_cpu.A[2] inside the
   window (line + exact line), and state what A[1] and A[2] hold
   when each builder is called — or "not assigned in this window"
   for any register the window never sets.
3. Note any other guest word the window reads as a condition
   (address + line) between the wait loop and the two calls.

PART B — who sets the flag to 1 (the three windows in part03/part08):
4. For each write of literal 1 to 0xFF022C (rr3_part03.c:397,
   rr3_part03.c:642, rr3_part08.c:2669): quote the write line and
   the nearest branch or condition above it that decides whether
   it executes (quote that line too), and name in one sentence
   what else the surrounding code is doing (menus, setup, a mode
   select — only what the window's own lines show).
5. For the computed store at rr3_part08.c:2957: quote it and the
   lines that produce its value within the window, and state in
   one sentence what the value can be (a copied word, a literal
   in disguise, an arithmetic result — per the lines).

Write your answer to a NEW file named standings-which-builder.md
in ~/rr3-standings-mini-lane (do not edit any other file), with
Part A items 1-3 and Part B items 4-5 in order, then a closing
summary of at most four sentences answering: under what condition
does func_01A86E run instead of func_01A718, what sets that
condition (in the plain terms the windows support), and does
anything in these windows suggest the func_01A86E path belongs to
a mode a normal single race would ever enter — or is that not
determinable from these windows? Say which.

RULES:
- Use only the four named windows in their three files. Quote
  lines exactly, including variable names. If something cannot
  be sourced inside its window, write "not traced within this
  window" — do not guess and do not read outside the windows.
- The file on disk is the deliverable, not your chat summary:
  before you finish, re-read standings-which-builder.md once and
  confirm its content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: the condition that selects func_01A86E, and who sets
  it.

---
