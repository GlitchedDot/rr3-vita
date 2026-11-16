# Task 1 grading — Candy Pebbles, 2026-11-12 (against the real rr3-vita source)

## The file Qwen left on the mini (standings-swap-verdict.md, 531 B)
Verdict: FAIL — do not use. It does not match Qwen's own on-screen summary
(which claimed 6 operations and a FULL verdict); the file on disk claims
3 operations and PARTIAL. The disk file is what counts.

Errors in the file:
- Line 5121 write32 is real, but its guest offset is NOT "0xFF74E0+2":
  A[1] has already advanced, so A[1]-4 is the entry itself (entry+0).
- The claimed write at line 5122 (`m68k_write32(g_cpu.A[1], g_cpu.D[1])`)
  does not exist — line 5122 is a flag-computation block.
- Missing reads that form the move: line 5093 (`m68k_read32(g_cpu.A[1])`,
  A[1] += 4) and line 5112 (`m68k_read32(g_cpu.A[1] - 4)` into D[3]).
- Missed the second swap site entirely (see below), although the
  follow-up instruction explicitly asked for every swap site including
  any word-sized swaps touching the order table.
- "PARTIAL" verdict is wrong for the main sort (see verified facts).
- One claim was right: equal entries are not moved; the comparison/carry
  logic is at lines 5093-5099 and the gate `if (!(g_cpu.SR & (1u<<0)))`
  is at line 5104, as cited.

## Verified ground truth (Candy, direct read of rr3_part11.c)
func_014BDA spans lines 5000-6324 and contains TWO distinct sites:

1. Main sort (guest $014C08-$014C18, lines 5091-5144): an insertion-style
   pass over the 0xFF74E0 table. read32 at 5093, compare at 5094-5100
   (carry set only when the table entry is greater than the carried
   value), gate at 5104, read32 of the entry into D[3] at 5112-5113,
   write32 of the carried value into the entry's slot at 5121, carried
   value replaced by the displaced entry at 5129. All moves are FULL
   32-bit. Equal entries are not moved.
   => The scout's "partial swap" suspicion is CLEARED for the main sort.

2. Tail fix-up (guest $014E40-$014E52, lines 6281-6315): read16 0xFF74E2
   into D[0] (6283-6284), read16 0xFF74E6 into D[1] (6292-6293),
   write16 D[1] to 0xFF74E2 (6301), write16 D[0] to 0xFF74E6 (6309).
   This swaps ONLY the +2 words of entries 0 and 1 — this is the site
   the scout sampled. It is guarded upstream (lines 6266-6277: a read16
   of 0xFF05C0 and a zero-flag check can skip it). What the +2 word of
   each entry means, and when this fix-up runs, are the open questions
   Task 2 starts on (via the line builder that consumes the table).

## Consequence for the bug hunt
- Main sort: full-width, equal keys stay put. Wrong order therefore
  points at the DATA the sort receives (keys equal/uninitialised would
  leave roster order standing, and roster order starts with PLAYER A),
  or at the tail fix-up desyncing entries 0/1, or upstream writers.
- The EL standings A/B (trio capsuled) remains untested — it died
  class-2 mid-race before the trio ever ran — so native-vs-interpreted
  divergence in this subtree is still an open possibility in the main
  RR3 thread, independent of this lane.
