# TASK 8 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part15.c and source/rr3_part27.c exist
   (source/rr3_part11.c is also in the kit and is used below).
   If either is missing, write the single line "BLOCKED: <filename>
   not found" (naming the missing file) into a new file named
   standings-time-store.md and stop. Do not search the rest of the
   disk, do not download anything.
2. Read these three functions in full, from opening line to closing
   brace: func_018C7C in source/rr3_part15.c (lines 2292-2979),
   func_0142A6 in source/rr3_part11.c (lines 1256-1441), and
   func_027D3E in source/rr3_part27.c (lines 2730-4186).

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- Record A is a per-rider race record with base guest 0xFF09B2. Its
  fields: +98 = 32-bit race time (copied verbatim into the results),
  +102 = word used as divisor/zero test, +104 = flag byte (bit 7 set
  means the rider's results entry becomes DNF), +116 = rider index
  byte.
- func_013D76 (rr3_part10.c) loads A[1] = 0x00FF09B2 twice and each
  time calls exactly one of your three functions next: func_018C7C
  (call at part10 line 7129), func_0142A6 (call at 7151), func_027D3E
  (call at 7181). So each of your three functions receives the record
  base in A[1] on those paths (a function may also be called from
  elsewhere with a different A[1] — report what you can show about
  A[1] inside the function itself, and flag any internal re-load of
  A[1] to a different base).
- Nothing in rr3_part10.c writes +98, +102 or +116 for this record,
  and the +104 bit 7 is never set or cleared in part10/part11's byte
  writes. The +98 store is expected inside one of your three
  functions, as a write at offset 98 from the A[1] base (any
  addressing form the generated code uses: A[1] + (int32_t)98, a
  brief-index form, or a copied pointer derived from A[1] by quoted
  arithmetic).

TASK (single shot — three functions, one output file):
In each of the three functions, find every WRITE whose destination is
offset +98, +102, +104 or +116 from the record base in A[1] (or from
a pointer you can show is derived from that A[1] value within the
function). For each write report: file, line number, exact line
quoted, write size, which offset, and the written value traced back
within the same function to the read or computation that produced it
(quote those lines too). For the +98 writes specifically, also state
the value's form: is it copied from another memory field (name the
address/offset), computed by arithmetic (name the operations and
operands), or built from parts (e.g. two words combined)? Also report
any write in these functions that sets or clears bit 7 of the +104
byte, in whatever form it appears. If a function contains no writes
to any of the four offsets, say so in one line for that function.

Write your answer to a NEW file named standings-time-store.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. One section per function, in the order func_018C7C, func_0142A6,
   func_027D3E: its writes in line order in the format above, or the
   no-writes line.
2. A closing summary of at most four sentences: which function writes
   the +98 race time and exactly what value it stores (source field
   or arithmetic); whether any of the three touches +104 bit 7; and
   whether anything you found would make the stored +98 value a
   whole number of seconds (a multiple of 60) or a multiple of 6.

RULES:
- Use only the three named functions in their three files. Quote
  lines exactly, including variable names. If a value's origin cannot
  be sourced inside its function, write "origin not traced within
  this function" — do not guess and do not read other functions.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-time-store.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: which function holds the +98 store, and what it stores.

---
