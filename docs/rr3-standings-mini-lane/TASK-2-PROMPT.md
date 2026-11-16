# TASK 2 — paste this whole block into OpenCode on the mini (Qwen3 8B, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm the file source/rr3_part12.c exists in this directory.
   If it does not, write the single line "BLOCKED: source/rr3_part12.c not found"
   into a new file named standings-entry-format.md and stop. Do not search
   the rest of the disk, do not download anything, do not research fixes.
2. Confirm line 1601 of source/rr3_part12.c begins the function
   func_0155BE (it should read `void func_0155BE(void) {`).
   If it does not, write the single line "BLOCKED: func_0155BE not at line 1601"
   into standings-entry-format.md and stop.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- func_014BDA's main sort moves FULL 32-bit entries of the table at guest
  0xFF74E0; equal entries are not moved.
- At the end of func_014BDA (guest $014E40), ONLY the +2 words of entries 0
  and 1 are swapped with each other (0xFF74E2 <-> 0xFF74E6, 16-bit writes).
- func_0155BE (the standings line builder) walks three parallel arrays via
  A[3]: starting at 0xFF74A0, then 0xFF74C0, then 0xFF74E0.

TASK (single shot — one function, one file, one output file):
Read func_0155BE in source/rr3_part12.c, from line 1601 to the end of the
function (the closing brace before `void func_0158D4` at line 3211).
For each of the three arrays (0xFF74A0, 0xFF74C0, 0xFF74E0), answer:
what does the line builder DO with the values it reads from that array?
Specifically, for the 0xFF74E0 order table: does it read each entry as a
32-bit long, as a low 16-bit word, as a high 16-bit word (entry+2), or
some combination — and does the value it reads decide (a) which rider
name is printed, (b) the prize amount, (c) the time shown, or (d)
something else (say what)?

Write your answer to a NEW file named standings-entry-format.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. For each array, in the order the function walks them: every read of
   that array you found, each with the line number in rr3_part12.c, the
   exact line of code quoted, the read size (8/16/32), and the offset
   used (entry+0, entry+2, and so on).
2. A closing summary of exactly three sentences, one per array, of the
   form: "0xFF74E0: read as <size/offset>; its value decides <a/b/c/d>."

RULES:
- Use only lines you actually read in this file. Quote them exactly,
  including the variable names on the left side of assignments.
- The file on disk is the deliverable, not your chat summary: before you
  finish, re-read standings-entry-format.md once and confirm its content
  matches the summary you are about to give.
- Do not open any other source file. Do not propose a fix or a patch.
- Do not end with a question. When the file is written and re-read, stop
  and give a two-line summary: the three array verdicts compressed into
  one line, and how many read lines you listed.

---
