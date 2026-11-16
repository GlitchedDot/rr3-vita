# TASK 7 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part10.c exists (rr3_part11.c is also in source/
   and may be used for the two addresses listed for it below).
   If rr3_part10.c is missing, write the single line
   "BLOCKED: rr3_part10.c not found" into a new file named
   standings-fixed-records.md and stop. Do not search the rest of the
   disk, do not download anything.
2. In source/rr3_part10.c, run text searches for each of these strings:
   0x00FF0A14, 0x00FF0A18, 0x00FF0A1A, 0x00FF0A26, 0x00FF0E94,
   0x00FF0E98, 0x00FF0E9A, 0x00FF0EA6, 0x00FF09B2, 0x00FF0E32.
   Note the hit count for each string.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- Two per-rider race records have fixed bases: record A at guest
  0xFF09B2 and record B at guest 0xFF0E32. In both records the fields
  that feed the standings are at the same offsets:
  +98 = a 32-bit value (long at 0xFF0A14 for A, 0xFF0E94 for B) that
        the results builders copy verbatim as the rider's race time;
  +102 = a word (0xFF0A18 for A, 0xFF0E98 for B) used as a divisor /
        zero test by the results builders;
  +104 = a byte (0xFF0A1A for A, 0xFF0E9A for B) whose bit 7 decides
        whether the rider's results record becomes 0xFFFFFFFF (DNF);
  +116 = a byte (0xFF0A26 for A, 0xFF0EA6 for B) holding the rider
        index that decides the results slot.
- Record A's time goes to results slot 0 (before sorting). The
  standings bug under investigation: the results screen shows the
  player 1st with a wrong time when they did not finish 1st, and every
  displayed time ends in .0 after the frame-to-tenths conversion, so
  these fields' values are the prime suspects.
- There are NO writes to the literal addresses 0xFF0A14 or 0xFF0E94
  anywhere in the game (verified across all 34 generated parts), so
  the +98 longs are written through address registers based at
  0xFF09B2 / 0xFF0E32 or through computed addresses. Byte writes to
  the literal 0xFF0A1A and 0xFF0E9A DO exist in rr3_part10.c and in
  rr3_part11.c (guest 0x013xxx-0x014xxx functions — race logic).

TASK (single shot — one main file, one output file):
In source/rr3_part10.c, find:
(a) every WRITE to the byte addresses 0xFF0A1A and 0xFF0E9A — for each:
    file line, exact line quoted, containing function and its line,
    the value written (trace it to its producing line in the same
    function if it is not a literal), and specifically whether bit 7
    of the written value is set, clear, or data-dependent;
(b) every WRITE whose destination is record A +98 or record B +98
    (0xFF0A14 / 0xFF0E94) through an address register — i.e. any write
    through a register that the same function set to 0x00FF09B2 or
    0x00FF0E32 with a +98 offset, or any other addressing form you can
    show lands on those two addresses; quote the base assignment line
    and the write line, and trace the written value's origin within
    the function the same way;
(c) every WRITE to +102 (0xFF0A18 / 0xFF0E98) or +116 (0xFF0A26 /
    0xFF0EA6) in either form (literal or register-based), reported
    briefly: line, function, value origin in one phrase.
If a category has no finds in rr3_part10.c, say so in one line.
Additionally: repeat search (a)'s byte addresses only in
source/rr3_part11.c and list those writes compactly (line, function,
bit-7 verdict) — no other part11 analysis.

Write your answer to a NEW file named standings-fixed-records.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. The hit counts from setup step 2.
2. Sections (a), (b), (c) for rr3_part10.c in that order.
3. The compact rr3_part11.c byte-write list.
4. A closing summary of at most four sentences: which functions write
   the +104 flag bytes and what bit 7 ends up as on the paths you
   found; who (if anyone in this file) writes the +98 time longs and
   from what source values; anything you found that would make a
   +98 value a whole number of seconds (a multiple of 60).

RULES:
- Use only source/rr3_part10.c and, for the one listed purpose,
  source/rr3_part11.c. Quote lines exactly, including variable names.
  If a value's origin cannot be sourced in its function, write
  "origin not traced within this function" — do not guess.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-fixed-records.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: counts per category, and the summary's key point in one line.

---
