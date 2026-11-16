# TASK 5 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part11.c exists.
   If it does not, write the single line "BLOCKED: rr3_part11.c not found"
   into a new file named standings-entry-build.md and stop. Do not search
   the rest of the disk, do not download anything.
2. Read lines 5000-5330 of source/rr3_part11.c. Stay inside that range.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- func_014BDA starts at line 5000. The standings table at guest 0xFF74E0
  holds 16 entries of 4 bytes: +0 word = time in tenths (0xFFFF prints
  "DNF"), +2 word = rider index.
- No code anywhere else in the game writes that table through a literal
  address (verified across all 34 generated parts). The table's contents
  are produced inside this function.
- The main sort (guest $014C08, lines 5091-5144) is verified separately
  and is NOT part of this task.
- At line 5212 guest $014C32 sets A[0] = 0x00FF74E0 and a loop at
  label_014C3E reads each existing entry and writes a transformed value
  back (the entry write is at guest $014C50). This task is about the
  phases that touch the entries BEFORE and INCLUDING that loop — what
  the entries contain when the loop starts, and exactly what the loop
  turns them into.

TASK (single shot — one line range, one output file):
Read lines 5000-5330 and report, in execution order, every phase in
that range that READS FROM or WRITES TO the table entries at 0xFF74E0
(skip the main sort's internals at lines 5091-5144 beyond noting where
it sits in the order). For each phase give: its line range, the guest
address of its first instruction, and what it does to the entries,
quoting the exact lines that read or write an entry (including the
line that sets the A-register base, and the line that advances it).
Then give a dedicated section for the label_014C3E loop answering:
(a) how many entries it processes and what decides the count (quote
    the counter's initialisation and its test);
(b) for one entry, in exact arithmetic steps, what the new +0 word and
    the new +2 word are as functions of the OLD entry's bytes — name
    every operation (shifts and their direction, the mask of the byte
    saved aside, the division and its divisor, the word swap, which
    byte or word is restored at the end) with its line number;
(c) what happens on the path where the shifted value is negative
    (which steps are skipped — quote the branch line);
(d) where the entries the loop transforms originally came from: the
    phase before it that last wrote them, per your phase list (if the
    range contains no earlier write that fills real values — only the
    0xFFFFFFFF initialisation — say exactly that).

Write your answer to a NEW file named standings-entry-build.md in
~/rr3-standings-mini-lane (do not edit any other file).

RULES:
- Use only lines 5000-5330 of source/rr3_part11.c. Quote lines exactly,
  including variable names. If a value's origin cannot be sourced
  inside the range, write "origin outside this range" — do not guess
  and do not read further.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-entry-build.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-sentence
  summary: what the entries contain when the transform loop starts,
  and the one-line arithmetic of what the loop does to each entry.

---
