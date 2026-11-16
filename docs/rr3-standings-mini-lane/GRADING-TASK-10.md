# Task 10 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-record-init.md, 20,942 B, DeepSeek V4.1 Flash Free, 2m57s)
Verdict: PASS. Candy verified: exactly 55 A[1]-relative writes in
rr3_part12.c lines 4596-6148 (the file's count), exactly the three
claimed calls (func_015AAE ×2, func_015AC8 ×1, none inside the
walks), and the quoted lines at 4882-4883 (walk-1 table load),
5464/5472 (+102 = sign-extended table byte × 100), 5677 (+104 zero
fill) and 6018 (walk-2 index starts at 0x10) are exact.

## Established by this task (now verified ground truth)
- func_015B40 is the record initializer. It walks two
  sentinel-terminated (0xFFFFFFFF) ROM pointer tables — 0x00034CF6
  assigning rider indices 0, 1, 2, … and 0x00034D3A assigning
  0x10, 0x11, … — reloading A[1] from the table each iteration.
- It writes 55 record fields via A[1] plus 8 literal fixed-record
  writes: a large zero fill (including +38 long and the +104 flag
  byte — bit 7 CLEARED, never set), data fields from ROM tables
  (+116 = walk index, +14 = same index, +102 = signed byte from
  the 0x00015F32 table × 100), and +92/+94/+96 from a source
  struct / globals.
- **Offset +98 is never written, in any form** — not via A[1], not
  by literal address, not by its only callees' position in the flow.
  Records leave initialization with +98 holding whatever it held
  before.

## Lane synthesis after Task 10 (Candy, same day)
Static picture is now nearly closed: the ONLY write32 to +98 in
the game is func_01E0AC's computed correction onto DNF-gated
records; +98 long reads exist only in the two results builders;
there are no variable-destination write32s to RAM anywhere (the
only non-register/literal destinations are VDP port writes).
A finisher's +98 is therefore produced either through a pointer
computed to record+98 and written at +0 (indistinguishable by
offset grep), or it is a leftover the game genuinely relies on —
distinguishing those needs runtime evidence (a watchpoint on
guest 0xFF0A14 during a race), which belongs to the main RR3
thread when the results screen is un-parked. The +104 flag byte
has 100+ write sites across the race code (guest 0x01Dxxx-0x028xxx)
and cannot be chased by offset grep alone.
