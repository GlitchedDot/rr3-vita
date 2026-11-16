# TASK 4 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm these four files exist in source/: rr3_part02.c, rr3_part03.c,
   rr3_part04.c, rr3_part05.c.
   If any is missing, write the single line "BLOCKED: <filename> not found"
   (naming the missing file) into a new file named
   standings-writer-map.md and stop. Do not search the rest of the disk,
   do not download anything, do not research fixes.
2. In each of the four files, run a text search for the string `0x00FF74`
   and note the hit count per file. If a file has zero hits, that file is
   done: record "no 0x00FF74 references" for it in the output and move on.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- The standings results table lives at guest 0xFF74E0: 16 entries of
  4 bytes (table spans 0xFF74E0-0xFF751F). Entry = +0 word TIME in tenths
  (0xFFFF prints "DNF"), +2 word RIDER INDEX.
- Two parallel word arrays sit just below it: 0xFF74A0 and 0xFF74C0
  (16 words each). The line builder reads a word from one of them,
  indexed by the rider index, to select the printed name record.
- func_014BDA (rr3_part11.c) initialises the table to 0xFFFFFFFF, sorts
  it, and func_0155BE (rr3_part12.c) renders it. The A/B evidence from
  the running port says the table's CONTENTS are already wrong before
  that render/sort chain runs, so the writer that fills the entries
  during the race is the suspect. It has never been located.
- These four files were chosen because they hold the most 0x00FF74
  references outside part11/part12.

TASK (single shot — four files, one output file):
In rr3_part02.c, rr3_part03.c, rr3_part04.c and rr3_part05.c, find every
WRITE whose destination is in the guest range 0xFF74A0-0xFF751F. Count:
(a) direct writes to a literal address in that range (any write size);
(b) writes through an address register whose value is set to an address
in that range earlier in the SAME function (quote the base assignment
line as well).
For each write found, report: the file, the line number, the exact line
quoted, the containing function's name and its line number, the write
size, the destination (literal address, or base+offset), and what value
is written — if the value comes from a register, trace that register
back within the same function to the read or computation that produced
it and quote that line too (say "origin not traced within this function"
if it arrives as a live-in value you cannot source in the function).

Write your answer to a NEW file named standings-writer-map.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. One section per input file, in order part02, part03, part04, part05.
   Each section: the 0x00FF74 hit count, then the writes in line order
   in the format above, or the single line "no writes into
   0xFF74A0-0xFF751F found".
2. A closing shortlist, one line per write that writes a TIME-sized
   value into the +0 word of a table entry or a value into the +2 word,
   of the form: "<file>:<line> — writes <what> to <destination> from
   <origin>". If no write qualifies, the shortlist is the single line
   "no candidate time/index writer in these four files".

RULES:
- Use only these four files. Quote lines exactly, including variable
  names. Do not open part11 or part12 for this task; the facts about
  them above are given.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-writer-map.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: total writes found per file, and how many made the shortlist.

---
