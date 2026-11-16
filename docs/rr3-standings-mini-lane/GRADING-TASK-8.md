# Task 8 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-time-store.md, 9,368 B, DeepSeek V4.1 Flash Free, 5m10s)
Verdict: PASS. Candy verified the load-bearing counts and quotes:
func_018C7C contains exactly 22 m68k_write* calls in lines 2292-2979
(file says 22: 9 stack pushes + 13 fixed-global stores), its quoted
reads at part15:2311 (+104 via A[1]) and 2601 (+116 via A[2]) are
exact; func_0142A6 contains zero m68k_write and zero g_cpu.A[1] in
lines 1256-1441, as claimed; func_027D3E contains exactly 12
A[1]-relative writes in lines 2730-4186, and the quoted lines at
part27:2822, 3199, 3315 and 3565 are exact. Headline result is a
verified negative: NONE of the three callees writes +98, +102 or
+116, and none sets or clears +104 bit 7.

## Established by this task (now verified ground truth)
- func_018C7C only reads the record (offsets +104, +4, +18 via A[1])
  and stores to fixed globals (0xFF7A64/68, 0xFF2568-6E, 0xFF05FE).
- func_0142A6 is a pure reader/dispatcher: no writes at all.
- func_027D3E writes only bytes at +105/+108/+110/+111 of the record
  it is handed, and dispatches on whether A[1] equals 0xFF09B2 or
  0xFF0E32 (comparisons at part27:3199 and 3315) — it knows the two
  fixed records by address but never stores their time fields.

## Candy's follow-up hunt (same day, after grading)
Broadening the search to every write form found exactly ONE
write32 to offset +98 in the entire generated source:
rr3_part19.c:7848, inside func_01E0AC (from line 7678), at guest
$01E0E6: it stores D[2] into A[1]+98 immediately after computing
D[2] = D[2] - D[3] (guest $01E0E4) — a difference of two values,
the shape of an elapsed-time store (finish clock minus start
clock). Two write16 pairs at +98/+100 exist in func_0183FC
(part14:5137/5145, +100 word = literal 0x8730) and func_018DC0
(part15:3768/3776, same constant) — a different, A[0]-based struct,
noted but not yet classified. Task 9 = func_01E0AC: what D[2] and
D[3] are, whose record A[1] points at when it runs, and what calls
it — plus a classification pass on the two +98/+100 word pairs.
