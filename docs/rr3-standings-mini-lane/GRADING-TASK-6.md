# Task 6 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-records-writer.md, 13,975 B, DeepSeek V4.1 Flash Free, 2m31s)
Verdict: PASS. Candy spot-checked 37 quoted lines — 13 in rr3_part16.c
(7636, 7640, 7676, 7709, 7718, 7760, 7793, 7853, 7878, 8054, 8107,
8148, 8229) and 24 in rr3_part17.c (14, 37, 46, 65, 83, 84, 107, 140,
141, 157, 206, 207, 258, 289, 290, 466, 611, 636, 859, 860, 919, 960,
1041) plus both function starts (part16:7634, part17:4). Every one
exact, including the shared destination idiom
0x00FF741E + 4 x (int8)byte, the +98/+102/+116 field reads, and the
two division sites. No errors found.

## Established by this task (now verified ground truth)
- BOTH func_01A718 (5 write sites) and func_01A86E (11 write sites)
  write the record buffer. Every record write is a 32-bit store to
  0xFF741E + 4*k, where k is a signed byte taken from the source
  record itself (+116), i.e. the rider's own index. Values written:
  the source record's +98 long verbatim; or 0xFFFFFFFF (when the
  record's +104 bit 7 is set, or its +102 word is zero); or, on the
  second pointer-table walk (0xFF03B4), the divided value
  (word@0x00FF024C << 16) / (word@record+102), low 16 bits, stored
  only if smaller than the slot's current content.
- Slot 0 is special-cased in both functions from the record based at
  0xFF09B2 (value read at 0xFF0A14 — that record's +98 field), gated
  by bit 7 of 0xFF0A1A (that record's +104). func_01A86E additionally
  handles a second fixed record at 0xFF0E94, indexed by the byte at
  0xFF0EA6, and begins with a fixed ascending 0xFFFFFFFF fill of all
  16 slots (line 46).
- Layout verdict (the file's, confirmed by the quotes): the writers'
  layout MATCHES the reader's — one 4-byte record per rider-indexed
  slot at the same base. func_014BDA's 8-byte first step is a SKIP of
  slot 1 (0xFF7422), not a writer stride property.

## What this narrows the bug to
- Names and times cannot be desynced by these writers: a record lands
  in the slot named by its own rider-index byte, and the reader labels
  each read slot with that same slot number. A wrong pairing would
  require a wrong +116 byte in the source records themselves.
- Rider index 1 is written (both writers can write slot 1) but never
  read by func_014BDA: that rider vanishes from the table and the
  never-overwritten 16th table slot becomes the phantom 0xFFFF DNF row.
  Whether index 1 is a real finisher or a deliberately excluded slot
  (e.g. a duplicate player record) is not decidable from these files.
- The ".0" question moves one step upstream again: the writers copy
  record+98 verbatim, so any coarseness is already in the per-rider
  race records (the ones reached through the 0xFF0440 / 0xFF03B4 /
  0xFF03F6 pointer tables, plus the fixed records at 0xFF09B2 and
  0xFF0E94). Who fills +98 (and +102/+104/+116) during the race is
  the remaining trace.
- Open runtime question for the main thread, not this lane: 01A718 and
  01A86E are near-duplicate implementations — which one actually runs
  in the port's race-end flow (capsule logs have sighted 01A718), and
  whether the other is a variant for a different mode.
