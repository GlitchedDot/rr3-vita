# GRADING — Task 12 (standings-shuttle-callee.md, 14,869 B, DeepSeek V4.1 Flash Free)

**Verdict: PASS, clean.**

Verification performed by Candy against the real generated source:
- All sampled quotes check out exactly: part20 lines 7508-7509
  (0xFF05F0 read -> BE write), 7517-7518 (0xFF7532 read -> D[0]
  low word), 7527 (the >> 1), 7538 (the x5 full 32-bit replace),
  7546 / 7553 (the +1), 7557 (the D[0] store).
- Independent counts in rr3_part20.c 7226-7572: exactly 2
  m68k_write calls in the whole function (both to 0xFF05BE);
  exactly 0 recomp_call_func; g_cpu.A[1] appears exactly 3 times,
  all reads, at 7336 (+4 dword), 7413 (+113 byte), 7432 (+18
  word) — matching the file's table line for line.
- Table-scan constants confirmed at the lines quoted: A[0] =
  0x00FF043C (7228), sentinel 0xFFFFFFFF (7238), target record
  0x00FF0A32 (7257), +104 bit-6 read via A[2] (7277).
- The file explicitly reported NO contradiction with the prompt's
  given facts — and its check was real, since the given facts
  were all re-verified above.

## What Task 12 establishes (verified)
- The shuttle question is CLOSED. func_01EF84 never reads
  0xFF05BE and writes it on exactly one gated path. On every
  early exit (exits at 7321, 7389, 7408, 7463, 7502) BE returns
  to func_01EE98 holding exactly what the caller copied in — so
  on those paths guard words 0xFF05C0 / 0xFF05C2 keep their own
  prior values through the round trip.
- On the one path that reaches the write block (label_01F01A),
  BE is ALWAYS replaced by ((read16(0x00FF7532) >> 1) * 5 + 1)
  & 0xFFFF — computed entirely from the fixed word at 0xFF7532
  (in the neighborhood just past the results order table). The
  first write (7509, a verbatim copy of the word at 0xFF05F0) is
  a dead store: no branch intervenes, and 7557 overwrites it
  three statements later on the only path that reaches either.
- The path is heavily gated: the table at 0xFF043C must contain
  the fixed record pointer 0xFF0A32 (note: 0xFF0A32 is 0x80 past
  record A's base 0xFF09B2 — a specific offset into that record,
  or a distinct structure overlapping it; not resolved here);
  that record's +104 bit 6 set; a magnitude chain on the +4
  dwords of that record vs the caller's record (difference,
  absolutized, must survive subtractions of 0x10000 and
  0x60000); the caller's +113 byte < 4 and +18 word non-zero;
  and a bit pattern at 0xFF0212 (bit 1 clear, or bits 1+2 set).
  A fallback pass re-runs the record block against a second
  fixed record at 0xFF1B32.
- func_01EF84 makes NO calls of any form; the 0xFF043C loop is a
  record-pointer scan, not a dispatch table. It reads the
  caller's record (A[1]) at +4 / +18 / +113 only — the same
  record fields the results cluster keeps using (+4 = the value
  the builders' formulas consume; +18 = a count-like word).
- Net for the fix question: at guard time, C0/C2 hold either
  their own previous values or the 0xFF7532-derived value above.
  Combined with Task 11, the guard's C-side inputs are now fully
  characterized as a mechanism; the remaining unknowns are pure
  data origins (who writes 0xFF7532, 0xFF05F0, and the D6 chain's
  first real values during a race) — runtime territory, same as
  the +98 hunt's conclusion.

Lane score: DeepSeek 11 passes in 12 tasks (one minor slip,
Task 4); local Qwen3 8B 0 for 1.
