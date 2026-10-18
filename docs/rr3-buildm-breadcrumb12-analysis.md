# Build M — breadcrumb-12 analysis (prepared 2026-10-17 ~16:21 PDT)

Source: Lola's message 2026-10-17T23:16:56Z — breadcrumb-12.txt plus two
psp2core dumps (psp2core-1792278233-…, psp2core-1792278349-…).
Her report: "1: Attract mode. 5353 ticks, crashed / 2: Race mode. 1702 ticks, crashed"

breadcrumb-12.txt is cumulative (11 boot sections). The last two sections are
the Build M runs (the only ones carrying insn=/yld= counters, which Build M
added per the 2026-10-17 22:02 UTC tracking update):

- Run 10 (attract): last breadcrumb tick 5340 — matches her 5353.
- Run 11 (race): last breadcrumb tick 1680 — matches her 1702.

## The counters split into two regimes

Low regime (attract, ticks 960–4680):
- ~67k instructions per 60-tick interval, steady.
- yld essentially frozen: 69,359 at phase2-done, still only 69,412 at tick 4680
  (0–7 yields per interval).

High regime:
- ~693k instructions per 60-tick interval (~10.3× the low regime).
- ~13.8–14.1k yields per 60-tick interval.

Race mode enters the high regime almost immediately (transitional intervals
at ticks 960–1200, full rate from tick 1200) and dies at ~1702.
Attract stays in the low regime until the 4680→4740 interval (partial:
362,594 insn, 5,768 yld), is at full high rate by 4800, and dies at ~5353.

## Work done in the high regime before death is strikingly similar

- Attract: tick 4680 → 5340 = +7,079,302 insn, +139,093 yields, 660 ticks.
- Race: tick 900 → 1680 = +7,703,987 insn, +148,488 yields, 780 ticks
  (including its two transitional intervals).
- Total yields at last breadcrumb: attract 208,505 vs race 217,847 — within ~4.5%.
  Total insn differs much more (15.60M vs 11.91M) because attract banked
  ~3.8M extra low-regime instructions first — so the yield count (and
  high-regime instruction count) tracks the crash far tighter than total
  instructions or total ticks.

## Reading

Phase-3 itself is not the accumulator — the low-regime attract ran 3,720+
ticks with yields frozen and did not die. Whatever churns ~14k yields per
60 ticks in race / demo-race mode, and ~0 in attract-idle, is the resource
that accumulates to the crash after roughly 140–150k yields (≈7–7.7M
high-regime instructions). That is the metric Build M was built to find.

mark=0 / seen=0 on every line of both runs — stack canaries still clean.

## Not done in this pass

The two new psp2core dumps were not parsed here (a plain strings/constant
scan finds no THREAD_INFO/exception markers — they need the usual dump
parser). Whether crash #13/#14 carry the same cause constants
(0x1000d / 0x213a7, cf. crashes #11–12) is still unverified.
No visible/playable claim is made — ticks and counters are engine state
from the breadcrumb only; Lola's own report is that both runs crashed.
