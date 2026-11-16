# Task 3 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-fixup-guard.md, 4,575 B, DeepSeek V4.1 Flash Free, ~40 s)
Verdict: PASS — clean. Candy independently traced lines 6169-6281 of
rr3_part11.c before reading the file; the file's branch list (B1-B7),
line numbers, goto targets, and path tables match that trace exactly:
4 distinct paths reach the swap (label_014E40), 3 reach the skip
(label_014E58). No fabricated lines, summary matches file.

## The guard, in plain values (now verified ground truth)
The 1st/2nd-place name swap runs ONLY IF:
- bit 31 of entry 0's 32-bit long at 0xFF74E0 is SET, AND
- one of two sub-chains passes, selected by the low nibble of entry 0's
  +2 word (the rider index):
  - nibble != 0: word at 0xFF05DA >= 0x20, OR (0xFF05DA < 0x20 AND
    word at 0xFF05C2 != 0)
  - nibble == 0: word at 0xFF05D8 >= 0x20, OR (0xFF05D8 < 0x20 AND
    word at 0xFF05C0 != 0)
If entry 0's long is non-negative, the swap is always skipped.

## The puzzle this raises
Entry 0's +0 word is the time in tenths (Task 2). Bit 31 of the long is
the top bit of that time word, so the swap requires a time >= 0x8000
tenths (~54m37s) — far beyond any normal race time. Either:
(a) the swap is effectively dead in normal finishes, and the wrong
    top-3 display comes from the data/writer branches of the fault
    tree, not from this fix-up running wrongly; or
(b) at the moment this section of func_014BDA runs, entry 0 does not
    yet hold a plain sorted time (different phase / different format),
    and the sign bit means something else.
Resolving (a) vs (b) means tracing what state the table is in when
guest $014DFC is reached, and what the 0xFF05C0/C2/D8/DA words are.
Writers exist (quick grep, Candy): each of the four words is cleared
to 0 in places and written from values read near guest 0x01EF20 /
0x01EF6C (for C0/C2) and 0x01434C / 0x0143B0, 0x0233A8 / 0x0233BC
(for D8/DA) — they look like fields of a per-rider record block at
0xFF05xx. That trace is Task 4 material; it needs more of the generated
source on the mini than the current 2-file kit.
