# TASK 12 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part20.c exists. If it is missing, write the
   single line "BLOCKED: rr3_part20.c not found" into a new file
   named standings-shuttle-callee.md and stop. Do not search the
   rest of the disk, do not download anything.
2. Read one function in full, from opening line to closing brace:
   func_01EF84 in source/rr3_part20.c (lines 7226-7573).

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- func_01EE98 (traced in Task 11) uses the word at 0xFF05BE as a
  shuttle: it copies guard word 0xFF05C0 into 0xFF05BE, calls
  func_01EF84, then copies 0xFF05BE back into 0xFF05C0; then it
  repeats the same round trip for 0xFF05C2. So whatever
  func_01EF84 leaves in 0xFF05BE is what the guard words
  0xFF05C0 / 0xFF05C2 ultimately hold when the results-screen
  guard reads them. (Note: Task 11's prompt contained one
  given-fact error about a copy direction, which the Task 11
  report caught and corrected. If anything in this prompt
  contradicts the source in front of you, trust the source and
  flag the contradiction in your report.)
- A codebase-wide search (done by Candy) found exactly two writes
  to 0xFF05BE inside func_01EF84: rr3_part20.c line 7509 (storing
  a value read into a temp) and line 7557 (storing
  (uint16_t)g_cpu.D[0]). No reads of 0xFF05BE and no references to
  0xFF05C0 / 0xFF05C2 / 0xFF05D6 / 0xFF05D8 / 0xFF05DA appear in
  the function.
- func_01EF84 opens by loading A[0] = 0x00FF043C and reading
  32-bit pointers from that table into A[2] in a loop.
- func_01EE98 calls func_01EF84 three times (rr3_part20.c lines
  6949, 7064, 7174); the calls at 7064 and 7174 are the two
  shuttle round trips. Before the 7064 call, the caller's A[1]
  points at a race record (its +104 flag byte is tested just
  before); before the 7174 call, the caller has set
  A[1] = 0x00FF0E32 (the second fixed record's base).

TASK (single shot — one function, one output file):
1. List every write func_01EF84 makes to 0xFF05BE (there should be
   exactly the two given above — confirm or correct): line
   number, exact line quoted, and the stored value traced back
   within the function to the read or computation that produced
   it (quote those lines too). For the line 7557 store, trace
   g_cpu.D[0] back to the last write to D[0] before it on that
   path.
2. State the conditions on each write: which branch or loop path
   reaches it (quote the branch lines), and whether a path
   through the function exists that writes NEITHER — i.e. leaves
   0xFF05BE holding exactly what the caller copied in.
3. List every read and write the function makes through A[1]:
   the offset and width of each, with line numbers (quote the
   load-bearing ones). If the function never uses A[1], say so
   explicitly.
4. List every call the function makes, in any form: direct
   recomp_call_func lines (callee + line), and any indirect or
   table-driven dispatch (describe the mechanism and quote the
   lines). Pay attention to the pointer-table loop at the top:
   state what the loop does with each pointer it loads into A[2],
   in as much detail as the function itself shows.
5. Note any other guest-memory writes the function makes to
   addresses in the range 0xFF0500-0xFF0600 (line + address only).

Write your answer to a NEW file named standings-shuttle-callee.md
in ~/rr3-standings-mini-lane (do not edit any other file),
containing sections for items 1-5 in order, then a closing
summary of at most four sentences answering: when func_01EF84
returns to func_01EE98, what does 0xFF05BE hold compared to what
the caller copied in — unchanged on all paths, changed only on
some paths (say which and to what), or always replaced (with
what) — for each of the function's exit paths if they differ?

RULES:
- Use only func_01EF84 in source/rr3_part20.c. Quote lines
  exactly, including variable names. If a value's origin cannot
  be sourced inside this function, write "origin not traced
  within this function" — do not guess and do not read other
  functions.
- The file on disk is the deliverable, not your chat summary:
  before you finish, re-read standings-shuttle-callee.md once
  and confirm its content matches the summary you are about to
  give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: whether the shuttle value survives func_01EF84
  unchanged, and on which paths it changes if it does.

---
