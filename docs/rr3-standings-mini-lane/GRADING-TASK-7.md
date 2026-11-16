# Task 7 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-fixed-records.md, 10,119 B, DeepSeek V4.1 Flash Free, 2m16s)
Verdict: PASS. Candy re-ran the file's own setup searches: hit counts
for all ten strings in rr3_part10.c match exactly (0xFF0A1A: 18,
0xFF0E9A: 9, 0xFF09B2: 2, everything else 0), the file-wide write
count (239 m68k_write* calls) matches, and every sampled quote —
the flag-byte writes at 3213/3218, 4497, 5614, 5944, the func_013D76
base loads and calls at 7121, 7129, 7151, 7173, 7181, the function
starts (01335E at 2797, 013690 at 4152, 013968 at 5269, 013D76 at
6758), and both part11 writes at 305-310 and 333-338 — is exact.
Scope caveat (not an error): the bit-7 verdict covers writes that
name the flag byte's own address in part10/part11; a wider store from
another part file spanning the byte was outside this task's scope.

## Established by this task (now verified ground truth)
- The +104 flag bytes are only ever touched by read-modify-write bit
  operations on bits 1, 2 (part10: func_01335E, func_013690 for record
  A; func_013968 for record B) and bit 6 (part11: func_014108, both
  records). Bit 7 — the DNF gate the results builders test — is never
  set or cleared by any of these writes; it is carried through. Who
  sets bit 7 (initialisation, or a writer elsewhere) remains open.
- rr3_part10.c NEVER writes the +98 time longs, for either record:
  no literal writes, no register-plus-offset write form exists in the
  file at all, and record B's base 0xFF0E32 never appears in it.
  Record A's base is loaded exactly twice, both in func_013D76, and
  both times only handed to a callee: func_018C7C (call at line 7129),
  func_0142A6 (7151), func_027D3E (7181). The +98 writer for record A
  is inside one of those callees.
- Nothing in part10 writes +102 or +116 for either record either.

## Next hop (Task 8 material)
func_018C7C lives in rr3_part15.c, func_0142A6 in rr3_part11.c (line
1256, already in the kit), func_027D3E in rr3_part27.c. Task 8: read
those three callees for writes at offset +98 from the A[1] record
base they receive (and +102/+104/+116 while there), with value
origins — the finish-time store for the slot-0 record should be in
one of them.
