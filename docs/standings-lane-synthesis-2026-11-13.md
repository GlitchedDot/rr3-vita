# Standings / results-screen bug — mini-lane synthesis for the main RR3 thread

Produced by the standings mini lane (DeepSeek V4.1 Flash Free on the
2018 Mac mini, graded line-by-line against the real generated source
by Candy Pebbles), 2026-11-11 to 2026-11-13, Tasks 1-12. Full
deliverables and grading sheets:
files/rr3-standings-mini-lane/ (standings-*.md + GRADING-TASK-*.md).
Nothing in the rr3-vita tree was edited; this is investigation only.
The results-screen fix remains parked behind the perf front per
Lola's call (33.4 ms cliff first) — this document is the starting
map for when it un-parks.

## The symptom
Results screen: first 3 positions wrong, PLAYER A shown 1st with
$1000 though Lola actually finished 6th, and every time ends ".0".

## The pipeline, verified end to end
1. func_015B40 (part12:4596) initializes every per-rider record
   from two sentinel-terminated ROM pointer tables (0x34CF6 walk
   assigns rider indices 0,1,2,...; 0x34D3A walk assigns
   0x10,0x11,...). Record layout: +98 = 32-bit race time, +102 =
   word (zero => DNF), +104 = flag byte (bit 7 => DNF), +116 =
   rider index. It zero-fills most fields (+104 cleared), sets
   +102 = signed ROM-table byte x 100, +116 = walk index — and
   NEVER writes +98 in any form. Records start each race with
   +98 = whatever RAM already held.
2. Results builders func_01A718 (part16) / func_01A86E (part17)
   copy each record's +98 verbatim into buffer slot 0xFF741E+4k,
   k = the record's own +116 byte (or 0xFFFFFFFF if bit 7 set or
   +102 zero). Names cannot desync from times at this stage.
   Which builder runs is SETTLED (Task 13): inside func_185E3E,
   word@0xFF022C == 1 selects func_01A86E, any other value runs
   func_01A718; both merge into func_014E5C. The flag is set to 1
   only by mode/state machinery (func_004EF8, func_0108EC, plus a
   save/restore in func_010A1E). Runtime has only ever observed
   func_01A718 in race-end flow — the fix needs to match
   func_01A718 unless a future mode test says otherwise.
3. func_014BDA (part11:5000) builds the order table: 15 passes
   with index sequence 0, 2, 3, ..., 15 — INDEX 1 IS SKIPPED, so
   one table slot keeps its 0xFFFFFFFF fill and becomes a phantom
   DNF row. Sort keys are (record << 8) | index, insertion sort
   with FULL 32-bit moves (the scout's partial-swap suspicion is
   cleared for the main sort); equal keys are not moved, so equal
   +98 values leave roster order standing — roster order starts
   with PLAYER A, matching the symptom. Transform: time word =
   record / 6 (records are 1/60s frames; /6 makes tenths), so
   ".0" on every line <=> every record value /6 is a multiple
   of 10. Tail fix-up (guest $014E40) swaps ONLY the +2 rider-
   index words of entries 0/1 (names swap, times stay), behind a
   verified guard chain (Tasks 3/5) — a live suspect for the
   top-2 names specifically.
4. func_0155BE (part12:1601) prints: +0 word = time in tenths
   (0xFFFF prints literal " DNF"), +2 word = rider index -> name
   record; prizes computed (base 100 x ROM multiplier), so the
   $1000 is downstream of the order, not a separate bug.

## Addendum — the tail fix-up's guard inputs (Tasks 11-12, graded)
The guard's four words are copies circulating around two staging
words, now traced to the end of the static trail:
- 0xFF05C0 / 0xFF05C2 are round-tripped by func_01EE98 through the
  shuttle word 0xFF05BE via func_01EF84. On every early-exit path
  the shuttle returns unchanged, so the guard words keep their own
  prior values. On the one heavily gated path the shuttle is
  replaced by ((word@0xFF7532 >> 1) * 5 + 1) & 0xFFFF. The gate:
  the 0xFF043C table contains the fixed pointer 0xFF0A32, its
  +104 bit 6 set, a magnitude test on +4 dwords vs the caller's
  record, caller +113 byte < 4, +18 word non-zero, and a bit
  pattern at 0xFF0212 — plus an interlock: func_01EE98's C0 block
  runs only when 0xFF05D8 < 0x20, its C2 block only when
  0xFF05DA < 0x20.
- 0xFF05D8 / 0xFF05DA are straight copies of 0xFF05D6
  (func_0142E4); func_023326 halves D8 (record A) or DA (other
  record) in place and pushes the halved value back into
  0xFF05D6.
- What remains unknown is pure data origin: who first loads
  0xFF7532, 0xFF05F0 and the 0xFF05D6 chain with real values
  during a race. Same class of question as the +98 hunt below —
  the un-parked watchpoint build can log these addresses in the
  same pass (add 0xFF7532 and 0xFF05D6 to the watch list).

## The one open question (needs runtime, not more reading)
Who writes a FINISHER's +98 during the race? Static hunt is
exhausted: the game's ONLY write32 to offset +98 is func_01E0AC's
computed correction, and it lands only on already-DNF-gated
records; +98 long reads exist only in the two builders; there are
no variable-destination write32s to RAM anywhere. So the stamp
either arrives through a pointer precomputed to record+98 and
written at offset +0, or the builders consume leftover state.

FIRST STEP WHEN UN-PARKED: an instrumented build logging every
write to guest 0xFF0A14 (record A +98) across one full race, with
the writer PC for each. That single log settles the fork and
points at the fix.

## Standing facts for the fix, whenever it lands
- Index 1's skip is IN THE ORIGINAL GAME's builder: whether
  rider 1 is a real finisher or an excluded duplicate is not
  decidable statically; the watchpoint build may show it.
- func_015B40's +104 is always cleared at init; bit 7 is set
  during the race (100+ write sites to +104 in race code —
  do not chase by grep).
- Do not re-derive anything above; every claim is graded against
  the source in the lane's GRADING-TASK-*.md sheets.
