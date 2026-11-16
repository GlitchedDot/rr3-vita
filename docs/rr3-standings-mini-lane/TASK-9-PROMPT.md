# TASK 9 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part19.c and source/rr3_part14.c exist
   (source/rr3_part15.c is also in the kit and is used below).
   If either is missing, write the single line "BLOCKED: <filename>
   not found" (naming the missing file) into a new file named
   standings-elapsed-store.md and stop. Do not search the rest of the
   disk, do not download anything.
2. Read func_01E0AC in source/rr3_part19.c (lines 7678 to the end of
   the file) in full. Also read the tail of func_01E03A in the same
   file, lines 7430-7677, concentrating on how A[1] is set before its
   call to func_01E0AC at line 7560 and on the write to A[1]+104 just
   before that call.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- Per-rider race records have a 32-bit race-time field at offset +98,
  a word at +102, a flag byte at +104 (bit 7 = DNF gate for results),
  and a rider-index byte at +116. Two fixed records: base 0xFF09B2
  (results slot 0) and base 0xFF0E32.
- A codebase-wide search (done by Candy) found exactly ONE write32 to
  offset +98 anywhere: rr3_part19.c line 7848, inside func_01E0AC,
  at guest $01E0E6, storing D[2] into A[1]+98 immediately after
  D[2] = D[2] - D[3] at guest $01E0E4 (line ~7845).
- func_01E0AC is called from func_01E03A (rr3_part19.c line 7560,
  right after a bit-setting write to A[1]+104) and from three sites
  in rr3_part20.c (not in this kit — caller identity there is out of
  scope; note them as existing only).
- Two other +98 writes exist as word pairs: in func_0183FC
  (rr3_part14.c lines 5137 and 5145: write16 D[0] to A[0]+98 after
  adding 0xFF60 to D[0], and write16 literal 0x8730 to A[0]+100) and
  the same pair in func_018DC0 (rr3_part15.c lines 3768 and 3776).
  Whether those A[0] structs are the rider records or a different
  structure is part of this task.

TASK (single shot — one main function plus one classification, one
output file):
PART 1 — func_01E0AC (rr3_part19.c:7678-end):
Trace the store at line 7848 backwards and forwards:
(a) D[3]: where is it loaded in this function before the subtraction
    (quote the line(s)) — from which memory address or field, or from
    a caller's register (say which, if it is live-in)?
(b) D[2]: where does the value that enters the subtraction come from
    (quote the line(s)) — same treatment.
(c) A[1]: is it live-in (set by the caller), or loaded inside the
    function? From func_01E03A's tail (lines 7430-7677): how is A[1]
    set before the call at line 7560 (quote the line), and what is
    the write to A[1]+104 just before the call (quote it; which bit
    does it set)?
(d) Everything else the function writes to the same record (any
    offset), in line order, briefly.
(e) Any other call the function makes, and any condition guarding
    the +98 store (quote the branch line if the store is conditional).
PART 2 — classification of the word pairs:
For func_0183FC (rr3_part14.c, starts line 4620) and func_018DC0
(rr3_part15.c, starts line 2980): find where A[0] is set in each
function before the +98/+100 writes at part14:5137/5145 and
part15:3768/3776 (quote the base assignment), and state whether that
base can be one of the rider-record bases (0xFF09B2 / 0xFF0E32) or a
value read from a table (name it) — one short paragraph per function,
no full read of either function.

Write your answer to a NEW file named standings-elapsed-store.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. Part 1 sections (a)-(e), every claim backed by an exact quote with
   its file line number.
2. Part 2's two paragraphs with their quoted base assignments.
3. A closing summary of at most four sentences: what the +98 store in
   func_01E0AC actually computes (the two operands named by their
   source addresses/fields), whose record it lands in on the
   func_01E03A path, and whether the subtraction of those two sources
   could produce values that are multiples of 60 or of 6 — or whether
   nothing about the operands forces any such granularity.

RULES:
- Use only the named functions/ranges in rr3_part19.c, and for Part 2
  only the base-assignment regions of func_0183FC and func_018DC0.
  Quote lines exactly, including variable names. If a value's origin
  cannot be sourced inside the allowed ranges, write "origin outside
  the read range" — do not guess and do not read further.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-elapsed-store.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: what the store computes, and the granularity verdict.

---
