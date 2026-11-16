# GRADING — Task 11 (standings-guard-inputs.md, 14,851 B, DeepSeek V4.1 Flash Free, 3m7s)

**Verdict: PASS — and it caught an error in the prompt's own given facts.**

Verification performed by Candy against the real generated source:
- All sampled quotes check out exactly: part20 lines 7051-52,
  7082-83, 7161-62, 7192-93 (the BE staging copies in both
  directions), 7032-33 and 7142-43 (the 0x20 subtractions on D8/DA),
  the three recomp_call_func(func_01EF84) sites at 6949/7064/7174;
  part11 lines 1607-08 and 1738-39; part24 lines 6481-84, 6490,
  6495-96, 6511-14, 6520, 6525-26.
- The file's reference census is exact: 23 references to the six
  words across the three function ranges (part20: BE 5, C0 2, C2 2,
  D8 1, DA 1; part11: D6 2, D8 1, DA 1; part24: D6 2, D8 3, DA 3) —
  re-counted independently, matches.
- Function extents (7224 / 1937 / 6541 closings) match.

## Candy's error, corrected by the model (recorded plainly)
The prompt's given facts said func_023326 stores "a read of
0xFF05D6 shifted right by 1" into D8/DA. That was Candy's misread
of scoping grep output. The truth, as the deliverable states and
the source confirms: func_023326 reads D8 (resp. DA), halves it in
place (>> 1 at lines 6484/6514), writes it back to D8 (resp. DA),
and only then copies the halved value INTO 0xFF05D6 (6496/6526).
The model flagged the discrepancy explicitly instead of parroting
the given fact. Future prompts: given-facts about direction of a
copy get re-read at the line level before inclusion.

## What Task 11 establishes (verified)
- The six words form a circulation system around two staging
  words, not six independent fields:
  - 0xFF05BE is a shuttle for the C0/C2 pair: func_01EE98 does
    C0 -> BE, CALL func_01EF84, BE -> C0, then C2 -> BE, CALL,
    BE -> C2. Whatever func_01EF84 does to BE is what lands back
    in the guard words; that callee is the un-traced link.
  - 0xFF05D6 and D8/DA circulate: func_0142E4 copies D6 -> D8 and
    D6 -> DA (straight, unconditional within blocks gated on
    bit 15 of words 0xFF05BA / 0xFF05BC); func_023326 halves D8 or
    DA in place — D8 when its record pointer A[1] equals the
    fixed record A base 0xFF09B2, DA otherwise — and pushes the
    halved value back into D6.
- func_01EE98's two blocks are per-fixed-record (A[1] + 104 flag
  tests; second block sets A[1] = 0xFF0E32 = record B) and are
  additionally gated on the OTHER pair being small: the block for
  C0 proceeds only when D8 - 0x20 is negative (D8 < 0x20), the C2
  block only when DA < 0x20 — plus the record's +104 bit 7 clear
  and words 0xFF05C6 / 0xFF05C8 zero.
- Bottom line for the fix question: at guard time the four words
  hold values whose ORIGIN is upstream of all three writers
  (live-in), circulated through staging words and one halving.
  The writers do not identify FF05BE/FF05D6 as any race quantity,
  so whether normal finishing values pass the >= 0x20 / != 0
  tests is NOT decidable from this cluster. Remaining un-traced
  links, in order: (1) func_01EF84 — what it does to the BE
  shuttle decides what C0/C2 actually hold; (2) whoever first
  loads D6/D8/DA with real values (candidate writers of D6 seen
  in scoping greps: rr3_part11.c 4694/4966, rr3_part20.c 6854,
  rr3_part23.c 7888, rr3_part24.c 1853/2264/2894/3448).

Lane score: DeepSeek 10 passes in 11 tasks (Task 4's minor slip
the only blemish); local Qwen3 8B 0 for 1.
