# TASK 10 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part12.c exists. If it is missing, write the
   single line "BLOCKED: rr3_part12.c not found" into a new file
   named standings-record-init.md and stop. Do not search the rest
   of the disk, do not download anything.
2. Read func_015B40 in source/rr3_part12.c (lines 4596-6148) in full,
   from its opening line to its closing brace.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- Per-rider race records have: a 32-bit race-time field at offset
  +98, a word at +102 (the results builders treat a zero +102 word
  or a set bit 7 in the +104 flag byte as DNF), the flag byte at
  +104, and a rider-index byte at +116. Two fixed records have bases
  0xFF09B2 and 0xFF0E32; other records are reached through pointer
  tables.
- A codebase-wide search (done by Candy) found that every write to
  offset +102 or +116 through A[1] in the entire generated source
  sits inside func_015B40: write16 to A[1]+102 at rr3_part12.c line
  5472, and write8 to A[1]+116 at lines 4906 and 6051.
- The same search found exactly ONE write32 to offset +98 anywhere
  (in func_01E0AC, already traced: it stores a computed correction
  onto already-DNF-gated records). If func_015B40 touches +98, it
  must be in another form: a different register, a word/byte pair, a
  brief-index form, a postincrement walk, or a helper call.
- func_015B40's only direct caller in the codebase is at
  rr3_part08.c line 1531 (that file is not in this kit; the caller
  itself is out of scope).

TASK (single shot — one function, one output file):
Map func_015B40's writes into per-rider records:
1. The record pointer(s): which address register(s) carry record
   pointers in this function, where each is loaded from (quote the
   lines: literal base, pointer-table read, caller live-in, loop
   advance), and whether the function walks multiple records in a
   loop (if so, quote the advance and the loop bound).
2. Every write whose destination is a record field (any offset,
   through the record pointer(s) from step 1, in any addressing
   form): line number, exact line quoted, offset, size, and the
   written value traced back within the function to the read or
   computation that produced it (quote those lines too).
3. The +102 write at line 5472 and the +116 writes at 4906 and 6051
   in their local context: what value each stores and where it came
   from (these are anchors — they must appear in your step-2 list;
   call out anything near them that writes neighbouring fields).
4. The +98 question, answered explicitly: does this function write
   offset +98 of any record, in ANY form? If yes, quote the line(s)
   and trace the value. If no, say so in one sentence and list which
   offsets adjacent to +98 (90-116) it does write, so the shape of
   the gap is visible.
5. Every call the function makes (callee names, line numbers) —
   names only, do not read the callees.

Write your answer to a NEW file named standings-record-init.md in
~/rr3-standings-mini-lane (do not edit any other file), containing
steps 1-5 in order, then a closing summary of at most four sentences:
what this function initializes in each record (the field list),
whether +98 is among the initialized fields, and — if +98 is written
— exactly what value it receives (a zero, a constant, a copy, or a
computation, naming the source).

RULES:
- Use only func_015B40 in source/rr3_part12.c. Quote lines exactly,
  including variable names. If a value's origin cannot be sourced
  inside this function, write "origin not traced within this
  function" — do not guess and do not read other functions.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-record-init.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: the initialized field list, and the +98 verdict.

---
