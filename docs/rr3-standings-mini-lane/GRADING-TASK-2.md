# Task 2 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-entry-format.md, 9,742 B, produced by DeepSeek V4.1
## Flash Free — the zero-retention Zen model — in ~5 minutes wall including
## its write/re-read passes, vs Qwen3 8B's 1h47m failed Task 1; model name
## confirmed from Lola's screenshot 2026-11-13, OpenCode since upgraded
## 1.19.5 -> OpenCode 2 on the mini)
Verdict: PASS. Every load-bearing line spot-checked against the real
rr3_part12.c matches exactly, including variable names:
1676, 2067, 2071, 2087, 2162, 2213, 2392, 2734, 2743, 2752, 2770, 2778,
2797, 3129, 3154 — all correct as quoted. No fabricated lines found.
The summary on screen matched the file (the Task 1 failure mode did not
recur). Minor caveat: the §3a header-digit index arithmetic is convoluted
and only partially traced — it is not load-bearing for the bug.

## Established entry format (now verified ground truth)
Each 4-byte entry of the order table at guest 0xFF74E0 is:
- +0 word (16-bit): the rider's TIME in tenths, fed to the formatter
  func_0158D4; the value 0xFFFF instead prints the literal "   DNF".
- +2 word (16-bit): the RIDER INDEX. The line builder (func_0155BE)
  doubles it, reads a word from the array A[3] points at (0xFF74A0, or
  0xFF74C0 when the word at 0xFF745E is zero), shifts that word left 4,
  and copies the 10-byte name record at ROM 0x15F32 + 16*value + 6
  into the row.
The main sort in func_014BDA compares the full 32-bit entry, so the
time word (the high half of the long) dominates and the rider index
acts as the low-half tiebreak; equal entries are not moved.

## Consequence for the standings bug
- func_014BDA's tail fix-up (guest $014E40) swaps ONLY the +2 words of
  entries 0 and 1 — i.e. it swaps which RIDER NAME is attached to the
  1st-place and 2nd-place times, while the times themselves stay put.
  Combined with the still-open questions (when its FF05C0 guard lets it
  run; what the race-side writer puts in the +0 time words — the ".0"
  gap), the standings fault tree is now:
  (a) tail fix-up running when it shouldn't / not running when it should,
  (b) sorted entries carrying wrong rider indices from the start
      (writer-side), (c) equal/uninitialised time words leaving roster
      order standing.
- Prize display remains downstream of position, as established before.

## Lane A/B result
Free hosted model: Task 2 PASS, ~2 minutes wall time.
Local Qwen3 8B: Task 1 FAIL, ~1h47m wall time.
Grading standard was identical. Recommendation recorded for Lola's call:
adopt the free model for this lane's remaining slices (public-repo code
only, zero-retention model preferred), keep local Qwen as fallback.
