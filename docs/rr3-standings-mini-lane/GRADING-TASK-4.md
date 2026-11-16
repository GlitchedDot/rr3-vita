# Task 4 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-writer-map.md, 3,127 B, DeepSeek V4.1 Flash Free, 3m11s)
Verdict: PASS, with one minor slip. The headline result is a ZERO: no
writes into guest 0xFF74A0-0xFF751F in part02, part03, part04 or part05,
and an empty shortlist. Candy verified the zero independently: a grep
for literal writes into that range across ALL 34 generated parts finds
exactly two hits in the whole codebase — the tail fix-up swap itself
(rr3_part11.c:6301 and 6309). The file's two register-base candidates
(part05:3026 in func_00B69C and part05:4428 in func_00BA72, both loading
A[0] = 0x00FF74A0) were checked line by line: the quoted lines,
function starts (2620 / 4092), the single indexed READ on each base,
the stack-pop discards (3068 / 4470) and the FF5306 reloads (3861 /
5469) are all exact. Both bases are read-only, as reported.
Minor slip: the boundary note pairs "part05:4858" with a write to
0xFF748E — line 4858 actually writes 0xFF747A; the FF748E writes exist
at lines 3424 and 4863. Non-load-bearing (both addresses are below the
range either way) but recorded.

## What the zero means (Candy's follow-up read, same day)
There is no external race-side writer filling the table through any
literal address, in any part file. The table is assembled INSIDE
func_014BDA itself: its build phase walks A[0] from 0xFF74E0 (set at
line 5212) and at guest $014C50 writes each entry as a full 32-bit D1
(lines 5299-5300 region), where D1's low word has just been loaded
from D2 (line 5292: D1 = (D1 & 0xFFFF0000) | (uint16)D2). So each entry
= (D1 high word) << 16 | D2 — the remaining question is what feeds
D1's high word and D2 inside that build loop (roughly lines
5212-5300), and from which per-rider source records. That trace
resolves both open puzzles at once: why entry 0's top bit can be set
(Task 3's guard) and why the time words arrive as multiples of 10
(if the high-word source is itself coarse). Task 5 = that build-loop
trace, inside part11 which is already in the kit.
