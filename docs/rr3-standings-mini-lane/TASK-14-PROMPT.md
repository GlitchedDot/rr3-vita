# TASK 14 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part20.c and source/rr3_part12.c exist. If
   either is missing, write the single line "BLOCKED: <filename>
   not found" (naming the missing file) into a new file named
   standings-loose-ends.md and stop. Do not search the rest of
   the disk, do not download anything.
2. Read these in full, from opening line to closing brace:
   func_01E548 in source/rr3_part20.c (lines 2480-3795),
   func_015AAE in source/rr3_part12.c (lines 4248-4323), and
   func_015AC8 in source/rr3_part12.c (lines 4324-4595).

GIVEN FACTS (verified by Candy against the real source at line level —
build on these, but trust the source over this prompt if they ever
contradict, and flag it):
- func_01E0AC (traced in Task 9) is the game's only 32-bit writer
  of the race-record field at offset +98; it stores a computed
  correction. Its one previously traced caller, func_01E03A
  (rr3_part19.c), writes the record's +104 flag byte immediately
  before calling it, on a path where the record was already
  DNF-gated (bit 7 of +104 set).
- The only other call sites of func_01E0AC in the codebase are
  three in source/rr3_part20.c — lines 2904, 2990 and 3116 — all
  inside func_01E548, and each is immediately preceded by an
  8-bit write to (g_cpu.A[1] + 104): the write lines are 2896,
  2982 and 3108 respectively, storing computed bytes
  (_01E5F2r, _01E62Cr, _01E68Er).
- func_015B40 (the record initializer, traced in Task 10) makes
  exactly three calls: func_015AAE twice (rr3_part12.c lines 4631
  and 4666, both near the start of func_015B40) and func_015AC8
  once (line 5783). Whether those two helpers write any record
  fields was never checked — func_015B40's own write census in
  Task 10 did not include them.

TASK (single shot — three functions, one output file):
PART A — func_01E548 (source/rr3_part20.c):
1. For each of the three func_01E0AC call sites (2904, 2990,
   3116): quote the +104 write that precedes it and trace the
   stored byte back within the function to the read or
   computation that produced it (quote those lines too). State
   for each: which bit(s) of +104 the write sets or clears, or
   whether it copies the byte from somewhere else.
2. For each call site, state what g_cpu.A[1] points at: list
   every assignment to g_cpu.A[1] in the function (line + exact
   line), and say which assignment governs each call site — or
   "A[1] is live-in, never assigned in this function" if that is
   the case.
3. For each call site, quote the nearest branch above it that
   decides whether that call executes.

PART B — the two helpers (source/rr3_part12.c):
4. For func_015AAE and func_015AC8 separately: list EVERY
   guest-memory write in the function (m68k_write8/16/32) with
   line number, destination in the function's own terms (a fixed
   address quoted exactly, or a register-plus-offset with the
   register and offset), and width. If a function makes no
   guest-memory writes at all, say so explicitly.
5. State for each helper whether it writes, in any form: a
   record field at offset +98, +102, +104 or +116 from any base
   register; the fixed addresses 0xFF0A14 or 0xFF0E94; or a stack
   slot only. Also list each helper's calls (callee + line), or
   "no calls".

Write your answer to a NEW file named standings-loose-ends.md in
~/rr3-standings-mini-lane (do not edit any other file), with Part
A items 1-3 and Part B items 4-5 in order, then a closing summary
of at most four sentences answering: do the three part20 call
sites follow the same pattern as func_01E03A (a +104 flag write
immediately before the call — and is the record already
DNF-gated on those paths, per the traced byte values), and do
func_015B40's two helpers write any record fields the Task 10
census did not cover — and if so, which fields?

RULES:
- Use only the three named functions in their two files. Quote
  lines exactly, including variable names. If a value's origin
  cannot be sourced inside its function, write "origin not
  traced within this function" — do not guess and do not read
  other functions.
- The file on disk is the deliverable, not your chat summary:
  before you finish, re-read standings-loose-ends.md once and
  confirm its content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: the part20 call pattern verdict, and whether the
  helpers write any record fields.

---
