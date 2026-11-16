# TASK 6 — paste this whole block into OpenCode on the mini (DeepSeek V4.1 Flash Free, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm source/rr3_part16.c and source/rr3_part17.c exist.
   If either is missing, write the single line "BLOCKED: <filename> not
   found" (naming the missing file) into a new file named
   standings-records-writer.md and stop. Do not search the rest of the
   disk, do not download anything.
2. Locate the two functions named in the task below in those files and
   read each one in full, from its opening line to its closing brace.

GIVEN FACTS (verified by Candy against the real source — do not re-derive
these, build on them):
- func_014BDA (rr3_part11.c) builds the standings table from 15 source
  records read out of a buffer starting at guest 0xFF741E. It reads the
  records at these addresses, in order: 0xFF741E, 0xFF7426, 0xFF742A,
  0xFF742E, and then every 4 bytes up to 0xFF745A (the first step is 8
  bytes, the rest are 4). Each record is a 32-bit race time in
  1/60-second units; the standings time shown is record / 6.
- func_014BDA assigns outer indices 0, 2, 3, ..., 15 to those 15 records
  (index 1 is skipped). The rider each record belongs to is therefore
  decided by the ORDER of the records in the buffer.
- A grep across all 34 generated parts (done by Candy) shows only three
  places outside part11 that load the exact base 0x00FF741E into an
  address register: rr3_part16.c line 7640 (A[2], inside func_01A718)
  and rr3_part17.c lines 14 and 65 (A[2], inside func_01A86E, which
  starts at line 4 of that file). func_01A718 is a known post-race
  results-table builder (it walks the pointer tables at 0x00FF0440,
  0x00FF03B4 and 0x00FF03F6). These two functions are the prime suspects
  for writing the records.

TASK (single shot — two functions, one output file):
Read func_01A718 in source/rr3_part16.c (it starts near line 7634; read
to its closing brace) and func_01A86E in source/rr3_part17.c (lines
4-1313). In each function, find every WRITE whose destination can be an
address in the guest range 0xFF7414-0xFF7462 — counting (a) writes to a
literal address in that range, and (b) writes through an address
register that the function sets to 0x00FF741E or 0x00FF7414, or to an
address derived from one of those bases by a quoted arithmetic line.
For each such write report: file, line number, the exact line quoted,
write size, destination (base + offset as computed in the code), and
what value is written — traced back within the same function to the
read or computation that produced it (quote that line too; if it
arrives from a caller or a table you do not read in this task, name
the source address or table and write "origin is that table/caller").
Also report, for each function, the STRIDE pattern of its in-range
writes in execution order (the sequence of destination offsets), and
which guest addresses of the 15 record slots (0xFF741E, 0xFF7426,
0xFF742A ... 0xFF745A) the function can write, if any.

Write your answer to a NEW file named standings-records-writer.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. One section per function: its writes in line order in the format
   above, its stride pattern, and its record-slot coverage — or the
   single line "no writes into 0xFF7414-0xFF7462" for that function.
2. A closing verdict, at most three sentences: which function (or
   neither, or both) writes the 15 records func_014BDA reads, and
   whether the write stride/layout you found matches the read pattern
   (first step 8 bytes, then every 4 bytes) or differs from it.

RULES:
- Use only those two functions in those two files. Quote lines exactly,
  including variable names. Do not open any other file for this task.
- The file on disk is the deliverable, not your chat summary: before
  you finish, re-read standings-records-writer.md once and confirm its
  content matches the summary you are about to give.
- Do not propose a fix or a patch. Do not end with a question.
- When the file is written and re-read, stop and give a two-line
  summary: writes found per function, and the verdict in one line.

---
