# Road Rash 3 Vita — Native-Death Bisect Ledger

Started 2026-10-18 (Sun night). Purpose: recap-first pickup sheet for the
substitution-bisect campaign against the native death at demo tick ~5353.
One row per build; fill in the result column from Lola's screen report +
breadcrumb, never from logs alone for anything visible/playable.

## Ground truth going in (all verified 2026-10-18)

- **Interpreter fault CLOSED.** Build AJ (rr3vita-aj.vpk) — the first build
  where the game fiber does zero stdio/file IO (the engine's two fiber
  `fprintf`s deleted; that fix is now baked into the engine tree
  permanently) — ran the interpreter past tick 10,020 with phase-1 counts
  bit-identical to the PC interpreter (insn=8,124,470, yld=138,274 at tick
  600). Lola closed it manually; no crash. The interpreter's instant death
  was the fiber's second file write, whatever its form (proved by AI:
  a bare 7-byte sceIoWrite as the second write still killed it).
- **Native fault SEPARATE and untouched by the fiber fix.** Build AK
  (rr3vita-ak.vpk, plain native, fiber fix in tree) died at 5353, exactly
  like the classic native builds. Its last checkpoint, tick 5340, was
  canonical to the digit: frame hash 9F2A8CA6, insn=15,602,171,
  yld=208,505 — a bit-perfect run, then a cliff within the next ~60
  ticks. Not drift; one specific thing breaking in one specific moment.
- **Oracle method = substitution, not frame-diffing.** AK vs AJ frame
  hashes stay locked only through ~tick 1920; after that the two attract
  sequences drift in phase (same hashes, different arrival times) because
  interp and native pace the demo differently. So the interpreter convicts
  by replacing recompiled functions, not by hash comparison in the race.

## Method

- 637 guest functions total. Each bisect build runs one half through the
  interpreter capsule (checkpoint/rollback guard: a bad substitution falls
  back to native instead of corrupting state) while the other half stays
  native.
- Dies at ~5353 → culprit is in the half that stayed native.
  Death moves or vanishes → culprit is in the substituted half.
- ~10 builds from haystack to one named function; then read that
  function's generated code for the bad access.

## Ledger

| Build | Substituted via interp capsule | Result at ~5353 | Narrows to |
|-------|-------------------------------|------------------|------------|
| AL    | Upper half: 319 of 637 functions, address 0x1C686 and up | **SAILED** — Lola reported "6500 and counting" (2026-10-18 ~19:41 PDT); breadcrumb-38 runs to tick 7320, closed manually still alive, full native pace ~50 ms roll. Phase-1 maskstat: tried=1, hit=1 (upper-half functions barely entered during boot) | **Upper 319** — faulty function is in the substituted half |
| AM    | Top 160 of the upper 319, address 0x2378C and up | **SAILED** — Lola reported "sailed right passed" (2026-10-18 ~20:07 PDT); breadcrumb-39 runs to tick 6420, closed manually still alive. Phase 1 digit-identical to AL; its single boot substitution's exit PC 0x3A4C0 sits inside the top-160 range. Race leans on the top 160 ~once/frame: by frame 4800, tried=5,285, hit=5,282 (rest fell back to native by capsule design) | **Top 160** (0x2378C and up) |
| AN    | Top 80 only, address 0x27D3E and up (rr3vita-an.vpk, 2,924,361 bytes, MD5 20766b255a4f00a1aceca4610b76e73; delivered 2026-10-18 20:19 PDT) | _pending — Lola's demo-only test result not yet reported as of 20:21 PDT_ | Dies at ~5353 → culprit in the 80 between 0x2378C and 0x27D3E; sails → culprit in the top 80 |
|       |                               |                  |            |

## State as of 2026-10-19 12:00 (supersedes the AN-era pickup below)

- **Killer B NAMED: `func_024A4C`** (the ~5,579 death). Chain AQ–AV with
  the top 80 substituted throughout, so A was suppressed and the deaths
  isolate B cleanly. Unaffected by the correction below.
- **Killer A: in dispatch-table idx 557–596** (guest 0x27D3E–0x3A4CE).
  Established by replaying every verdict mechanically (mask set sails ⇒
  A inside it; dies at 5353 ⇒ A outside it): AL → upper 319, AM → top
  160, AN → top 80, AX → idx 557–596.
- **CORRECTION (owned 2026-10-19):** the AX inference was recorded
  FLIPPED in-session ("A in the upper 40"). AX substituted the *lower*
  40 of the top 80 and sailed, which puts A in the lower 40. Builds
  AY–BC then bisected the wrong half (idx 597–636) and died on schedule;
  BC's elimination "naming" of `func_1FA064` was void, and **BD
  (substituting {024A4C, 1FA064}) dying at 5353 acquitted 1FA064** and
  exposed the flip. func_1FA064 *is* the game's ROM-checksum self-test
  as a code fact, but it is not connected to the 5353 death.
- **Killer C:** a long AX run died at tick ~18,797 with A and B both
  substituted — a third, late-attract death, unlocalized. Needs a
  20k+ tick soak once A and B are both substituted in one build.
- **Standing rule added at Lola's suggestion (2026-10-19):** before
  every bisect build, replay all prior verdicts against the candidate
  set and confirm the new mask is a subset of it. BE (substituting
  024A4C + idx 557–576) passes this audit; its result halves the 40.

## Pickup checklist (recap-first, per Lola's working preferences)

1. Read AN's row above first. AL and AM both sailed; the suspect set is
   the top 160 functions (0x2378C and up). AN (top 80, 0x27D3E and up)
   was delivered 2026-10-18 20:19 PDT — its result is the next datum.
2. Record: substituted range, Lola's reported death tick (or "sailed
   past"), and the surviving half.
3. Next build substitutes half of AN's surviving suspect half only —
   never re-test a cleared range (lower 477 functions are cleared).
4. Breadcrumb notes/scheduler markers stay OFF the game fiber. Scheduler
   file IO is fine; fiber file IO is the poison. No exceptions for
   diagnostics — that is how builds AE–AI re-created the interp death.

## Update 2026-10-19 23:17 — BE verdict, BF built

- **BE** (mask 024A4C + idx 557–576, MASK2 [0x27D3E, 0x28E4C)):
  **SAILED** (Lola, 2026-10-19 23:17) → **Killer A ∈ idx 557–576**.
- **BF** (built 2026-10-19 late): mask 024A4C + idx 557–566
  (MASK2 [0x27D3E, 0x28B76)). Pre-build audit replayed clean; dispatch
  anchors re-verified (024A4C=506, 027D3E=557, 028B76=567, 028E4C=577).
  BF sails ⇒ A ∈ 557–566; BF dies ~5353 ⇒ A ∈ 567–576
  {028B76, 028C04, 028C2C, 028C54, 028CDC, 028D5A, 028D9C, 028DC6,
  028DD4, 028E18}.
- BE long run (2026-10-19 23:24): still running at tick 15,000+ per
  Lola. A-verdict already banked at 5353; watch whether it passes AX's
  18,797 death point (killer C evidence either way).
- BF first build (23:19–23:28) completed per its log but the VPK and
  objects vanished from build-bf before delivery (same artifact-loss
  gremlin as build AM). Rebuilt with copy+hash chained into the same
  command.
- **BE died at tick 18,797** (Lola, 2026-10-19 23:27) — EXACTLY AX's
  death tick, with a different mask set (AX: 557–596 substituted;
  BE: only 557–576). Conclusion: Killer C is independent of the whole
  idx 557–596 range via dispatch; it is deterministic (same tick,
  two builds). C is elsewhere, direct-called (mask-invisible), or a
  state fault at a fixed demo point. A-hunt unaffected.
- **BF delivered** 2026-10-19 23:32: rr3vita-bf.vpk, 2,924,565 B,
  MD5 c2f6e745e6bd186254ec5a58bfa4b1. Masks verified in CMakeCache
  (MASK 024A4C–024AF8; MASK2 027D3E–028B76). Verdict pending.
- CORRECTION to the "BF delivered" line above: the VPK vanished from
  disk minutes after delivery (Lola: "File not found", 23:35) — the
  artifact-loss gremlin again; both background-built BF attempts lost
  their outputs, while foreground writes persist. BF is being rebuilt
  in timeout-bounded foreground chunks; delivery is NOT banked until
  Lola has the file.
- **BF ACTUALLY delivered** 2026-10-19 23:40 (supersedes the phantom
  23:32 line and its MD5): built via foreground timeout-chunks
  (32→64 objects, link+pack verified), copied to your_files and
  re-verified on disk in a separate foreground call: 2,924,792 B,
  MD5 d10421e608e758c480e38517e86da0b2. Verdict pending.
- **BF verdict: SAILED** (Lola, 2026-10-19 23:48) → **Killer A ∈
  idx 557–566** {027D3E, 028030, 02823A, 02834A, 0283AA, 02880C,
  02894C, 0289C4, 028A5C, 028AF4}.
- **BG**: mask 024A4C + idx 557–561 (MASK2 [0x27D3E, 0x2880C)).
  Audit clean. BG sails ⇒ A ∈ 557–561; dies ~5353 ⇒ A ∈ 562–566.
- **BG delivered** 2026-10-19 23:58: rr3vita-bg.vpk, 2,924,791 B,
  MD5 f2020e86a5cee61289aaf552eb9dc838 (foreground chunk build,
  artifact verified on disk in a separate foreground call).
  Verdict pending: sails ⇒ A ∈ 557–561; dies ~5353 ⇒ A ∈ 562–566.
- **BF long run died at 18,797** (Lola, 2026-10-19 23:59) — third
  build, third identical tick (AX, BE, BF; mask sets 557–596 /
  557–576 / 557–566). Killer C is deterministic and independent of
  the entire A range via dispatch. Triple-confirmed.
- **BG verdict: SAILED** (Lola, 2026-10-20 00:08) → **Killer A ∈
  idx 557–561** {027D3E, 028030, 02823A, 02834A, 0283AA}. Lola
  called the night there ("no build").
- Next when she resumes — **BH**: mask 024A4C + idx 557–558
  (MASK2 [0x27D3E, 0x2823A)). BH sails ⇒ A ∈ {027D3E, 028030};
  dies ~5353 ⇒ A ∈ {02823A, 02834A, 0283AA}. Build via foreground
  chunks only; verify the VPK on disk in a separate foreground call
  before announcing size/MD5.
- **BH delivered** 2026-10-20 ~13:49: mask 024A4C + idx 557–558
  (MASK2 [0x27D3E, 0x2823A)). Pre-build audit passed: verdict
  replay consistent (AX/BE/BF/BG nested sails -> A in 557-561),
  dispatch anchors re-verified in generated/rr3_dispatch.c
  (027D3E / 028030 / 02823A are consecutive entries), BF<->BG
  cache diff confirmed the four mask vars are the only knobs.
  Built via foreground timeout-chunks (64 objects); CMakeCache
  masks verified on the built tree; artifact copied to
  your_files and re-verified in a separate foreground call:
  rr3vita-bh.vpk, 2,924,795 B, MD5 0bd54c78c8c53d740df65a2111357522.
  Verdict pending: sails => A in {027D3E, 028030}; dies ~5353 =>
  A in {02823A, 02834A, 0283AA}. (A death at 18,797 instead is
  Killer C, not A.)
- **BH verdict: DIED at tick 5353** (Lola, 2026-10-20 14:46) ->
  **Killer A in idx 559-561** {02823A, 02834A, 0283AA}. Masking
  557-558 did not save it, so A is not 027D3E / 028030. Death at
  the classic A tick (5353, as AK/AO/AW), not B's 5579 and not
  C's 18,797 - a clean A verdict.
- Next - **BI**: mask 024A4C + idx 559-560 (MASK2 [0x2823A,
  0x283AA)). BI sails => A in {02823A, 02834A}; dies ~5353 =>
  A = 0283AA (named outright).
- **BI delivered** 2026-10-20 ~14:56: mask 024A4C + idx 559-560
  (MASK2 [0x2823A, 0x283AA)). Audit passed (BH death replayed
  against the set; anchors 02823A/02834A/0283AA re-verified as
  consecutive dispatch entries). Foreground chunk build, 64
  objects; cache masks verified on the built tree; artifact
  copied to your_files and re-verified: rr3vita-bi.vpk,
  2,924,857 B, MD5 5444335ed4ad35f56d06674b9ed62c04. Verdict
  pending: sails => A in {02823A, 02834A}; dies ~5353 =>
  A = 0283AA, named outright.
- **BI verdict: SAILED** (Lola, 2026-10-20 15:09) -> **Killer A
  in idx 559-560** {02823A, 02834A}. Two names left.
- Final naming build - **BJ**: mask 024A4C + idx 559 only
  (MASK2 [0x2823A, 0x2834A)). BJ sails => A = func_02823A;
  dies ~5353 => A = func_02834A. Either outcome names A.
- **BJ delivered** 2026-10-20 ~15:20: mask 024A4C + idx 559 only
  (MASK2 [0x2823A, 0x2834A)). The naming build for Killer A.
  Audit passed; foreground chunk build, 64 objects; cache masks
  verified; artifact copied to your_files and re-verified:
  rr3vita-bj.vpk (size/MD5 as announced in chat). Verdict pending:
  sails => A = func_02823A; dies ~5353 => A = func_02834A.
- **BJ verdict: SAILED** (Lola, 2026-10-20 15:26) -> **KILLER A
  IS func_02823A** (idx 559, guest 0x2823A). Named by clean
  elimination: BH died with it native, BI sailed with it masked
  alongside 02834A, BJ sails with ONLY it masked. The A hunt is
  CLOSED. First look: body in generated/rr3_part27.c line 5047,
  719 lines of generated C, direct-call heavy (bypasses
  dispatch) - including a direct call to func_1FA064, the ROM
  checksum function BC once falsely convicted (innocent; BD
  acquitted it). Note BJ's config masks BOTH named killers
  (024A4C + 02823A), so BJ is functionally the "both killers
  routed to interpreter" build - its sail clears both the 5353
  and 5579 checkpoints. Still at large: Killer C, deterministic
  at tick 18,797. Remaining work: (1) desk diagnosis of WHY
  02823A (and 024A4C) kill natively - split-stack bookkeeping
  (g_split_sp_popped / g_rte_pending) is the prime suspect
  machinery; (2) a >=20,000-tick soak (BJ can serve) to face C;
  (3) endgame: bake the two routings into a ship-shape build.
- **Killer C hunt opened** (Lola's call, 2026-10-20 15:29). C:
  deterministic death at demo tick 18,797 in AX/BE/BF (B masked +
  A-ranges 557-596 / 557-576 / 557-566) - independent of the
  whole A range and B via dispatch. Candidates: a dispatched
  recomp function elsewhere in the table, a direct-called
  (mask-invisible) one, or an engine/state fault at a fixed
  demo point.
- **BK delivered** 2026-10-20: FULL-table mask (MASK
  [0x200, 0x1FA065), no MASK2) - every dispatched function runs
  through the interpreter capsule. The discriminator build:
  BK sails past 18,797 => C is a dispatched recomp function
  (mask-bisect at depth); BK dies at 18,797 => C is NOT recomp
  code (engine/state) and the hunt changes shape. Foreground
  chunk build, 64 objects; cache verified; artifact in
  your_files (size/MD5 as announced in chat). Note: full-interp
  pace may be slow; reaching 18,797 may take a while. Parallel
  evidence: Lola to run BJ's demo long and send breadcrumb.txt
  (ux0:/data/rr3vita/breadcrumb.txt) from the C death.
- **BK verdict: SAILED, 23,000+ ticks** (Lola, 2026-10-20 16:02).
  With the ENTIRE table interpreted there is no death at 18,797,
  so **Killer C IS a recomp function reached via dispatch** -
  the engine/state-only hypothesis is dead. (The BJ breadcrumb
  errand is moot; BK answered the question better.) C candidates
  after replaying AX/BE/BF (all died at 18,797 with B + A-range
  chunks masked; strongest = AX): C is NOT B and NOT in
  idx 557-596 via dispatch. Candidates: idx 0-505 and 597-636.
  C bisect needs long runs (>18,797; 23,000 is the proven mark).
  Tooling: mask system extended to THREE ranges so A and B stay
  surgically masked while range 3 bisects C.
- **BL**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x200, addr(idx 273)) = idx 0-272.
  Sails past ~20k => C in idx 0-272; dies 18,797 => C in
  idx 273-505 or 597-636. (Balanced split of the 546
  candidates: 273 vs 273.)
- **BL delivered** 2026-10-20 ~16:15: THREE ranges - MASK
  [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A, 0x2834A) (A) +
  MASK3 [0x200, 0x199B4) (idx 0-272). Mask infra extended to
  MASK3 this session (CMakeLists + rr3_dispatch_mask.c).
  Anchors re-verified from the table (506=024A4C, 557=027D3E,
  559=02823A, 273=0199B4). Foreground chunk build, 64 objects;
  cache verified (all six values); artifact copied to
  your_files and re-verified: rr3vita-bl.vpk (size/MD5 as
  announced in chat). Verdict pending: sails past ~20k =>
  C in idx 0-272; dies 18,797 => C in idx 273-505 or 597-636.
- **BK addendum** (Lola, 2026-10-20 16:14): she let BK keep
  running past the 23,000 report and **closed it herself at
  37,000+ ticks, still sailing**. So the full-interpreter build
  is clean to at least 37k - no fourth lurking death in the
  interpreted/engine path up to there, and C's native-only
  nature is confirmed over a 2x-longer window.
- **BL verdict: DIED at tick 7279** (Lola, 2026-10-20 16:25) -
  a FOURTH killer (**Killer D**). Not A (5353), not B (5579),
  not C (18,797); BL never reached C's depth, so BL says
  nothing about C. Deduction: every build that ever passed
  7279 (AX, BE, BF - all died at 18,797) had idx 557-566
  masked; BF is the tightest. BL masked B + A(559) + idx
  0-272, leaving 557-566 (except 559) native, and died. BG/BH/
  BI also masked the cluster; BJ's "sails" certified only the
  5353 checkpoint, never 7279. => **Killer D in idx {557,
  558, 560, 561, 562, 563, 564, 565, 566}** (027D3E, 028030,
  02834A, 0283AA, 02880C, 02894C, 0289C4, 028A5C, 028AF4) -
  the cluster sitting on A. Death order: A 5353, B 5579,
  D 7279, C 18,797; each hides behind the earlier ones.
- **BM**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x27D3E,
  0x28B76) (idx 557-566, the A/D cluster) + MASK3 [0x200,
  0x199B4) (idx 0-272) = BL's C question re-asked with D's
  whole neighborhood suppressed. Dies 7279 => D outside the
  cluster (model broken); dies 18,797 => C in idx 273-505
  or 597-636; sails past ~20k => C in idx 0-272.
- **BM delivered** 2026-10-20 ~16:35: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x27D3E, 0x28B76) (idx 557-566, the A/D
  cluster) + MASK3 [0x200, 0x199B4) (idx 0-272). One chunk
  was interrupted by a service restart; progress verified
  on disk (objects persisted) before continuing. Foreground
  chunk build, 64 objects; cache verified (all six values);
  artifact copied to your_files and re-verified: rr3vita-bm.vpk
  (size/MD5 as announced in chat). Verdict pending - three-way:
  dies 7279 => D outside the cluster, model broken; dies
  18,797 => C in idx 273-505 or 597-636; sails past ~20k =>
  C in idx 0-272.
- **BM verdict: DIED at tick 7279** (Lola, 2026-10-20 17:15)
  with the whole idx 557-566 cluster masked -> **the Killer D
  cluster deduction is FALSIFIED** (owned: it was my inference
  from BL, and BM's delivery had this branch pre-written).
  Mechanical replay: BF passed 7279 with a native set that
  CONTAINS BM's entire native set, so no single native
  function can be D. Comparing died-at-7279 builds (BL, BM)
  vs passed-7279 builds (AX, BE, BF), the ONLY mask
  difference is idx 0-272 (the low table) - masked in BL/BM,
  native in AX/BE/BF. BK masks it too but sails (nothing
  native left). NEW MODEL (hypothesis, not fact): D is an
  INTERACTION - a low-table function running through the
  interpreter capsule arms a fault that a native function
  springs at tick 7279. The capsule framing (A7-neutral peek,
  split-stack bookkeeping the interp handles differently)
  is the suspect bridge - same machinery suspected for A/B.
- **BN**: trigger bisect #1. MASK [0x24A4C, 0x24AF8) (B) +
  MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x200, addr(137))
  (idx 0-136). Dies 7279 => trigger in idx 0-136; passes
  7279 => trigger in idx 137-272. Run long as well: dies
  18,797 => C not in idx 0-136; sails past ~20k => C in
  idx 0-136. One build, two reads.
- **BN delivered** 2026-10-20 ~17:25: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x200, 0x10A1E)
  (idx 0-136). Trigger bisect #1 under the interaction model.
  Foreground chunk build, 64 objects; cache verified (all
  six values); artifact copied to your_files and re-verified:
  rr3vita-bn.vpk (size/MD5 as announced in chat). Verdict
  pending, double read: dies 7279 => D-trigger in idx 0-136;
  passes 7279 => trigger in idx 137-272; AND if run long:
  dies 18,797 => C not in idx 0-136; sails past ~20k =>
  C in idx 0-136.
- **BN verdict: passed 7279, DIED at tick 7456** (Lola,
  2026-10-20 17:37) - a NEW tick, 177 later. The D death is
  MASK-DEPENDENT: full low table interpreted (BL/BM, 273
  fns) => 7279; lower half only (BN, idx 0-136) => 7456;
  none (BF) => no death until C. Simple single-trigger
  bisect framing is dead too. Best current model: TWO
  triggers - one in idx 137-272 (death 7279, dominates when
  both halves are interpreted) and one in idx 0-136 (death
  7456, exposed when the first is native). Systematic
  capsule mishandling with multiple low-table victims fits
  all data; BK still acquits the pure-interp path. BN gave
  no C read (died at 7456).
- **BO**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x10A1E, 0x199B4) (idx 137-272, the
  other half alone). Dies 7279 => two-trigger model holds
  (D1 in 137-272); dies 7456 => rethink; passes both and
  dies 18,797 => 7279 needs BOTH halves interpreted;
  sails past ~20k => C in idx 137-272.
- **BO delivered** 2026-10-20 ~17:45: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x10A1E,
  0x199B4) (idx 137-272 alone). Foreground chunk build, 64
  objects; cache verified (all six values); artifact copied
  to your_files and re-verified: rr3vita-bo.vpk (size/MD5 as
  announced in chat). Verdict pending: dies 7279 =>
  two-trigger model holds, D1 in idx 137-272; dies 7456 =>
  rethink; passes both ticks and dies 18,797 => the 7279
  death needs both low halves interpreted; sails past ~20k
  => C in idx 137-272.
- **BO verdict: passed 7279 AND 7456, DIED at tick 7956**
  (Lola, 2026-10-20 18:08) - a fifth outcome, not in BO's
  four-way reading (owned). The full D table by interpreted
  low-table set L: L={} (BF) => no D death; L={137-272}
  (BO) => 7956; L={0-136} (BN) => 7456; L={0-272} (BL/BM)
  => 7279; L={all} (BK) => no death in 37,000. Monotonic:
  more low-table interpretation => earlier death; total
  interpretation => none. MODEL (fits all data): D is
  CUMULATIVE capsule drift - each capsule run of a
  low-table function slightly corrupts host-side state
  (capsule stack/A7 handling is the suspect), and a native
  function eventually trips on the accumulated drift; with
  no native functions left, nothing trips. **D is an
  artifact of the bisect tool, not a game bug.** Testable
  prediction: maskstat hit counts at death should be
  roughly constant across BL/BN/BO (breadcrumb.txt would
  show it). STRATEGY CHANGE: stop fighting D; bisect C
  with UPPER-table masks only (candidates idx 273-505 +
  597-636), never interpreting the low table. If C is in
  the low table after all, deal with the capsule then.
- **BP**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x199B4, addr(410)) (idx 273-409).
  Dies 18,797 => C in idx 410-505 or 597-636; sails past
  ~20k => C in idx 273-409; dies at a D tick (~7-8k) =>
  the drift model is wrong in a new way (low table is
  native in BP).
- **BP delivered** 2026-10-20 ~18:20: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x199B4,
  0x2271A) (idx 273-409). Low table fully native (no D
  drift expected). Foreground chunk build, 64 objects;
  cache verified (all six values); artifact copied to
  your_files and re-verified: rr3vita-bp.vpk (size/MD5 as
  announced in chat). Verdict pending: dies 18,797 => C in
  idx 410-505 or 597-636; sails past ~20k => C in idx
  273-409; dies at a D tick (~7-8k) => drift model wrong
  in a new way.
- **Breadcrumb-44 analysis** (Lola sent ux0 breadcrumb,
  2026-10-20 18:24; file MD5 b6f0bfd4c0526d14fed286e96a34f740,
  67 sessions; S62=BK, S63=BL, S64=BM, S65=BN, S66=BO):
  (1) DETERMINISM: BL and BM maskstat series are BYTE-
  IDENTICAL at every 1200-frame mark (6000: tried 26348,
  hit 6967) and both died at 7279 - the D process is a
  deterministic function of the interpreted set, not a race.
  The 557-566 cluster contributed zero hits through frame
  6000 (its dispatches start later).
  (2) TRAFFIC SHAPE: low-table dispatch traffic is ZERO
  until ~frame 3600 in every session (BO tried=0 through
  3600; BF-era B+cluster tried=0 through 3600, 110 by 4800)
  then ramps hard (tried flood 4800-6000) - the demo changes
  character mid-run (attract -> race); every death lives
  after the ramp begins. idx 0-136 carries steady traffic
  from frame 0 (~1200 hits/1200 frames); idx 137-272 wakes
  only after ~3600.
  (3) DRIFT PREDICTION TEST - FAILED as stated: extrapolated
  capsule hits at death are ~8.5k (BL) vs ~5.0k (BN) vs
  ~4.8k (BO), NOT a constant budget. The monotone/drift
  framing survives (death earlier with more low-table
  interpretation; absent at both extremes), but the poison
  is not one-unit-per-capsule-hit - rates vary by segment
  because the demo's dispatch mix changes. Also noted:
  in the 4800-6000 window most in-range dispatches are
  DECLINED by the capsule (tried >> hit; non-entry
  addresses) and run native - mixed-mode traffic is denser
  than hit counts suggest.
- **BP verdict: DIED at tick 14,481** (Lola, 2026-10-20
  18:40) - another new tick, NO C read. Breadcrumb-45 S67:
  BP's capsule accepted EVERY in-range dispatch (tried ==
  hit at all checkpoints), traffic 0 until ~3600 then
  steady; 6,742 hits by frame 13,200, extrapolated ~8,000
  at death - same dose neighbourhood as BL (~8.5k). The
  drift is CAPSULE-GENERAL, not low-table-specific: any
  high-traffic interpreted range kills a long run before
  18,797. The capsule is only trustworthy for EARLY deaths
  (A/B reads were clean because they detonate at
  5353/5579, before dose accumulates, on near-zero-traffic
  ranges).
- **Capsule code read** (rr3_dispatch_mask.c rr3_mask_run +
  engine-defer m68k_interp_run_framed): the A7 seam is
  sound BY DESIGN - framed run peeks the depth-0 RTS (does
  not pop), mask_run performs the single pop (+=4) on an
  exact peek match, full g_cpu checkpoint + rollback on
  any mismatch. Two live drift suspects stand: (1) CYCLE
  ACCOUNTING - the interp accounts cycles per instruction
  (interp_account_cycles), the native bodies their own way;
  every capsule run swaps accounting methods for that
  slice, so machine timing state drifts with dose; pure
  modes are self-consistent, mixed mode is not. (2)
  FAILED-RUN MEMORY POLLUTION - rollback restores g_cpu
  only, NOT guest RAM writes made by a run that later
  fails its peek; BL/BN's flood windows had ~8,400 failed
  dispatches per 1,200 frames. BP had ZERO failures and
  still died, so (2) cannot be the whole story; (1) can.
  Neither is provable from the desk today; both convict
  the tool for long runs.
- **BQ (the gated bisect)**: new RR3_MASK3_GATE_TICK option
  - the MASK3 range only routes through the capsule once
  genesis_sim_tick_count() >= gate. A/B masks stay ungated
  (near-zero traffic; BF proved they reach C cleanly).
  BQ: MASK B + MASK2 A + MASK3 idx 273-409 gated at tick
  18,000. Dose before the C window drops >95% (only ticks
  18,000-18,797 of exposure). Dies 18,797 => C not in
  273-409; sails => C in 273-409; dies at a drift tick
  shortly after 18,000 => raise the gate and retry.
  Caveat noted: if C's fatal call SETS UP before 18,000
  and detonates at 18,797, a gated build could still die
  with C in range - a lower-gate retry addresses it.
- **BQ delivered** 2026-10-20 ~18:52: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x199B4,
  0x2271A) (idx 273-409) WITH RR3_MASK3_GATE_TICK=18000
  (new option; patch in CMakeLists + rr3_dispatch_mask.c:
  MASK3 clause also requires genesis_sim_tick_count() >=
  18000). Foreground chunk build, 64 objects, [100%] vpk
  target; cache verified (all seven values); artifact
  copied to your_files and re-verified: rr3vita-bq.vpk
  (size/MD5 as announced in chat). Verdict pending: dies
  18,797 => C in idx 410-505 or 597-636; sails past ~20k
  => C in idx 273-409; dies at a drift tick shortly after
  18,000 => gate too early, raise and retry.
- **BQ verdict: SAILED to tick 19,985** (Lola, 2026-10-20
  21:55; she closed it there). Replay: with C outside the
  gated range BQ must die at exactly 18,797 (AX/BE/BF all
  did); it cleared 18,797 by 1,188 ticks with ~1,985 ticks
  of gated exposure and NO drift death => **Killer C is in
  idx 273-409** (137 functions), and the tick gate works:
  dose control restored clean late-death bisect reads.
  Also: no native-set killer detonated in (18,797, 19,985]
  under BQ's native set.
- **BR**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x199B4, addr(342)) (idx 273-341)
  gated at tick 18,000. Sails past 18,797 => C in
  273-341; dies at exactly 18,797 => C in 342-409; dies
  at a new tick shortly after 18,000 => gate noise
  (unlikely at this dose), raise gate and retry.
- **BR delivered** 2026-10-20 ~22:05: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x199B4,
  0x1E0AC) (idx 273-341) gated at tick 18,000. Foreground
  chunk build, 64 objects; cache verified (all seven
  values); artifact copied to your_files and re-verified:
  rr3vita-br.vpk (size/MD5 as announced in chat). Verdict
  pending: sails past 18,797 => C in idx 273-341; dies at
  exactly 18,797 => C in idx 342-409; dies at a new tick
  shortly after 18,000 => gate noise, raise gate, retry.
- **BR verdict: DIED at exactly tick 18,797** (Lola,
  2026-10-20 22:26). Replay: idx 273-341 gated-interpreted
  from 18,000 and the run still detonated on C's own tick
  => **Killer C is in idx 342-409** (68 functions). Gate
  produced no drift noise (death on the exact tick, not
  shortly after 18,000).
- **BS**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x1E0AC, addr(376)) (idx 342-375)
  gated at tick 18,000. Sails past 18,797 => C in 342-375;
  dies at exactly 18,797 => C in 376-409.
- **BS delivered** 2026-10-20 ~22:37: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x1E0AC,
  0x20608) (idx 342-375) gated at tick 18,000. Foreground
  chunk build, 64 objects; cache verified (all seven
  values); artifact copied to your_files and re-verified:
  rr3vita-bs.vpk (size/MD5 as announced in chat). Verdict
  pending: sails past 18,797 => C in idx 342-375; dies at
  exactly 18,797 => C in idx 376-409.
- **CORRECTION (2026-10-20 22:57)**: Lola stated her report
  convention explicitly - a bare tick number means the
  build DIED at that tick; she says in words when she
  closes a run or sails. This corrects the BQ entry above:
  BQ **DIED at tick 19,985** (it was NOT closed by Lola).
  The C verdict survives (BQ passed exactly 18,797, which
  no C-outside build has done), but the death at 19,985
  reveals **Killer E**: a native function outside idx
  273-409 and outside {A, B} (BQ interpreted 273-409 from
  tick 18,000 and still died), previously invisible
  because every earlier build died at C first. Gated dose
  to 19,985 was only ~1.5k hits - far below every observed
  lethal drift dose - so E is a genuine killer, not drift.
  Death order now: A 5353, B 5579, C 18,797, E 19,985
  (D = capsule drift artifact, not a killer).
- **BS verdict: DIED at tick 18,811** (Lola, 2026-10-20
  22:56) - 14 ticks past C. Two live models:
  (1) C in idx 342-375 (suppressed in BS, passing 18,797)
  AND a sixth killer E2 in idx 376-409 detonating at
  18,811 - native in BS, interpreted in BQ (which is why
  BQ never saw it and lived to E at 19,985).
  (2) C is really in idx 376-409, and interpreting
  342-375 from 18,000 perturbed timing enough (the drift
  phenomenon) to delay C's detonation by 14 ticks.
- **BT (the discriminator)**: MASK [0x24A4C, 0x24AF8) (B) +
  MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x20608, 0x2271A)
  (idx 376-409) gated at 18,000. Dies at exactly 18,797
  => model 1 (C in 342-375; E2 confirmed in 376-409,
  hunted after C). Passes 18,797 and 18,811, dies 19,985
  (E) or sails => model 2 (C in 376-409).
- **BT delivered** 2026-10-20 ~23:10: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x20608,
  0x2271A) (idx 376-409) gated at tick 18,000. Foreground
  chunk build, 64 objects (one chunk interrupted by a
  service restart; on-disk progress verified at 29 objects
  before resuming); cache verified (all seven values);
  artifact copied to your_files and re-verified:
  rr3vita-bt.vpk (size/MD5 as announced in chat). Verdict
  pending: dies at exactly 18,797 => model 1 (C in
  342-375, E2 real in 376-409); passes 18,797 + 18,811 and
  dies 19,985 or sails => model 2 (C in 376-409).
- **BT verdict: DIED at tick 20,027** (Lola, 2026-10-20
  23:28) - but it PASSED 18,797, 18,811, and 19,985 first.
  Replay: with idx 376-409 gated-interpreted, C's
  detonation vanished => **Model 2 CONFIRMED: Killer C is
  in idx 376-409** (34 functions). BS's 18,811 death was
  C's own detonation delayed +14 ticks by interpreting
  342-375 - **E2 does not exist** (model-1 construct,
  retired). BT also passed E's 19,985 and died at 20,027:
  **E's tick is config-sensitive too** (+42 under BT's
  config) - the delay phenomenon applies to E as well.
  E remains a real native killer outside the dispatched
  idx 273-409 set (or direct-called); its hunt waits until
  C is named. Death order: A 5353, B 5579, C 18,797,
  E ~19,985 (+/- tens of ticks by config).
- **BU**: MASK [0x24A4C, 0x24AF8) (B) + MASK2 [0x2823A,
  0x2834A) (A) + MASK3 [0x20608, addr(393)) (idx 376-392)
  gated at 18,000. Sails past ~18,900 => C in 376-392;
  dies in the ~18,797-18,8xx window => C in 393-409 (a
  small +wobble vs 18,797 is expected noise per the BS/BT
  delay pattern; the read window is what matters).
- **BU delivered** 2026-10-20 ~23:40: MASK [0x24A4C, 0x24AF8)
  (B) + MASK2 [0x2823A, 0x2834A) (A) + MASK3 [0x20608,
  0x21748) (idx 376-392) gated at tick 18,000. Foreground
  chunk build, 64 objects; cache verified (all seven
  values); artifact copied to your_files and re-verified:
  rr3vita-bu.vpk (size/MD5 as announced in chat). Verdict
  pending: sails past ~18,900 => C in idx 376-392; dies in
  the ~18,797-18,8xx window => C in idx 393-409.
- **BU verdict: DIED at tick 20,027** (Lola, 2026-10-21
  00:01, with "no build" - session paused at her word).
  BU sailed past 18,797 with idx 376-392 gated-interpreted
  => **Killer C is in idx 376-392** (17 functions). BU and
  BT both died at exactly 20,027: that is E's detonation
  under configs where part of 376-409 is interpreted (BQ,
  full 273-409 interpreted, died to E at 19,985). E is
  native outside the dispatched idx 273-409 set in every
  config seen. NEXT (when Lola resumes): halve 376-392 -
  gate-mask idx 376-384 vs leaving 385-392 native.
- **BV - DELIVERED 2026-10-21, verdict pending**: B + A +
  idx 376-384 ([0x20608, 0x21152)) gated at 18,000;
  2,924,693 B, MD5 ef9b3649d2da1e96ed94c4c8d4f79cc1.
  Readings: sails past ~18,900 => C in idx 376-384 (then
  expect E at ~19,985-20,027); dies ~18,797-18,8xx =>
  C in idx 385-392. NOTE: the dispatch table's entries
  begin at line 681 of rr3_dispatch_mask.c (the gate patch
  shifted them one line below the prose offset in older
  notes); anchors re-verified against the file before
  this build (idx 375=0x20592, 385=0x21152, 392=0x2172C,
  393=0x21748).
- **BV verdict: DIED at tick 20,027** (Lola, 2026-10-21
  11:53, bare tick = death per her convention). BV sailed
  past 18,797 with idx 376-384 gated-interpreted =>
  **Killer C is in idx 376-384** (9 functions). The 20,027
  death is Killer E - the third build in a row (BT, BU,
  BV) to die at exactly 20,027 in configs interpreting
  part of idx 376-409. NEXT: halve 376-384 - gate-mask
  idx 376-380 vs leaving 381-384 native.
- **BW - DELIVERED 2026-10-21, verdict pending**: B + A +
  idx 376-380 ([0x20608, 0x20F04)) gated at 18,000;
  2,924,692 B, MD5 60923ae6f26f64f0e3bec09b6c268ae8.
  Readings: sails past ~18,900 => C in idx 376-380;
  dies ~18,797-18,8xx => C in idx 381-384. (Session mode
  per Lola 2026-10-21 11:54: keep building until she
  sleeps.)
- **BW verdict: DIED at tick 20,027** (Lola, 2026-10-21
  12:27, bare tick = death). BW sailed past 18,797 with
  idx 376-380 gated-interpreted => **Killer C is in idx
  376-380** (5 functions). E has now taken BT, BU, BV
  and BW at exactly 20,027. NEXT: halve 376-380 -
  gate-mask idx 376-378 vs leaving 379-380 native.
- **BX - DELIVERED 2026-10-21, verdict pending**: B + A +
  idx 376-378 ([0x20608, 0x20974)) gated at 18,000;
  2,924,646 B, MD5 57c095f2a0b8ef62843f5eb26a5a039f.
  Readings: sails past ~18,900 => C in idx 376-378;
  dies ~18,797-18,8xx => C in idx 379-380.
- **BX verdict: DIED at tick 20,027** (Lola, 2026-10-21
  13:14, bare tick = death). BX sailed past 18,797 with
  idx 376-378 gated-interpreted => **Killer C is in idx
  376-378** (func_020608, func_02075C, func_0207EE).
  E's streak at exactly 20,027 is now five builds.
  NEXT: gate-mask idx 376-377 vs leaving idx 378 native -
  a death at ~18,797 names C as func_0207EE outright.
- **BY - DELIVERED 2026-10-21, verdict pending**: B + A +
  idx 376-377 ([0x20608, 0x207EE)) gated at 18,000;
  2,924,641 B, MD5 1938d7c2ea9b1a99bde65a37fc9831bc.
  Readings: sails past ~18,900 => C in idx 376-377;
  dies ~18,797-18,8xx => **C = func_0207EE (idx 378),
  named outright**.
- **BY verdict: DIED at tick 18,797 exactly** (Lola,
  2026-10-21 13:50, bare tick = death). BY interpreted
  idx 376-377 (gated) and left idx 378 native; BX had
  interpreted all of 376-378 and sailed. Mechanical
  replay: C in 376-378 (BX), C not in {376, 377} (BY
  died at C's exact tick with both substituted) =>
  **KILLER C = func_0207EE (guest 0x207EE, idx 378).
  NAMED 2026-10-21.** All three early killers are now
  convicted: A = func_02823A (5353), B = func_024A4C
  (5579), C = func_0207EE (18797). Remaining: Killer E
  (~19,985-20,027, native outside dispatched idx
  273-409 or direct-called). Tooling note: A+B+C now
  occupy all three mask ranges, so the E hunt needs a
  fourth gated mask range (MASK4) added to the tool.
- **BZ - DELIVERED 2026-10-21, verdict pending**: FIRST
  build with all three convicted killers masked: B
  [0x24A4C, 0x24AF8) + A [0x2823A, 0x2834A) + C
  [0x207EE, 0x20974) gated at 18,000. Also the MASK4
  shakedown: the tool now has a fourth gated range
  (RR3_MASK4_*; patched into rr3_dispatch_mask.c +
  CMakeLists.txt 2026-10-21), configured empty here.
  2,924,651 B, MD5 76423f64034b1b2a4f3cfd45d6ecf725.
  Readings: dies ~19,985-20,027 => E baseline confirmed,
  hunt opens; sails past ~20,100 => E was entangled with
  the convicted set, rethink first.
- **BZ verdict: DIED at tick 20,027** (Lola, 2026-10-21
  14:22, bare tick = death). The A+B+C mask stack sailed
  through 5,353 / 5,579 / 18,797 and died on E's exact
  tick - baseline confirmed, MASK4 tooling sound, E hunt
  OPEN. E's candidate space: dispatched functions
  outside idx 273-409 (acquitted by BQ), or direct-called.
  NEXT (CA): MASK4 = idx 0-409 ([0x200, idx-410 start))
  gated at 19,200 - the acquitted middle rides along, so
  sail => E in idx 0-272; death ~20,027 => E in idx
  410-636 or direct-called.
- **CA - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 0-409 ([0x200,
  0x2271A)) gated at 19,200; 2,924,892 B, MD5
  2108ef1b25219b33f5363f9909797b5. Readings: sails past
  ~20,100 => E in idx 0-272; dies ~19,985-20,027 => E
  in idx 410-636 or direct-called.
- **CA verdict: DIED at tick 20,041** (Lola, 2026-10-21
  14:54, bare tick = death) - 14 ticks past E's canonical
  20,027. Same signature as BS (+14): heavy interpretation
  of the neighborhood delays a native killer's detonation.
  CA interpreted idx 0-409 from 19,200 and E still fired
  => **E is native in idx 410-636, or direct-called; the
  low table (idx 0-272) is acquitted.** NEXT (CB): MASK4 =
  idx 410-523 gated at 19,200 - sail => E in 410-523;
  death ~19,985-20,041 => E in 524-636 or direct-called.
- **CB - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 410-523 ([0x2271A,
  0x25554)) gated at 19,200; 2,924,904 B, MD5
  c76b91a307be7fff61b43c2c555d443f. Readings: sails past
  ~20,100 => E in idx 410-523; dies ~19,985-20,041 => E
  in idx 524-636 or direct-called.
- **CB verdict: DIED at tick 20,027 exactly** (Lola,
  2026-10-21 15:27, bare tick = death). CB interpreted
  idx 410-523 from 19,200 and E fired on his canonical
  tick => **E is in idx 524-636, or direct-called.**
  NEXT (CC): MASK4 = idx 524-580 gated at 19,200 - sail
  => E in 524-580; death => E in 581-636 or direct-called
  (and a full-span acquittal build would settle the
  direct-call question outright).
- **CC - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 524-580 ([0x25554,
  0x28F30)) gated at 19,200; 2,924,898 B, MD5
  d5eb95024c02ef0ce73bb497bdbda90c. Readings: sails past
  ~20,100 => E in idx 524-580; dies ~19,985-20,041 => E
  in idx 581-636 or direct-called.
- **CC verdict: DIED at tick 20,027 exactly** (Lola,
  2026-10-21 15:56, bare tick = death). Idx 524-580
  acquitted. Surviving dispatched candidates: idx
  581-636 (56) - everything else is acquitted (0-272 by
  CA, 273-409 by BQ, 410-523 by CB, 524-580 by CC).
  NEXT (CD) is the ACQUITTAL BUILD, not a half-split:
  MASK4 = the whole span idx 581-636 ([0x28F30,
  0x200000)) gated at 19,200. Sail => E is dispatched in
  581-636, bisect inside. Death => E is direct-called,
  PROVEN - change weapons.
- **CD - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 581-636, the whole
  surviving span ([0x28F30, 0x200000)) gated at 19,200;
  2,924,748 B, MD5 f9d357f765311b8e13a8936e3c276bc1.
  Readings: sails past ~20,100 => E is dispatched in idx
  581-636, bisect inside it; dies ~19,985-20,041 => E is
  direct-called, PROVEN - every dispatched function in
  the game is then acquitted.
- **CD verdict: SAILED - still running at 50,000+ when
  Lola closed it** (her words, 2026-10-21 16:57). The
  longest run of the hunt (BK's full-interp benchmark was
  37,000+). With idx 581-636 interpreted from 19,200,
  E's death never came => **Killer E is a DISPATCHED
  function in idx 581-636. The direct-call hypothesis is
  DEAD.** Every other dispatched function in the game is
  acquitted for E. NEXT (CE): halve the span - MASK4 =
  idx 581-608 gated at 19,200; sail => E in 581-608,
  death ~20,027 => E in 609-636.
- **CE - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 581-608 ([0x28F30,
  0x3A9DE)) gated at 19,200; 2,924,745 B, MD5
  21b831c84e5ab7560a87b1ae53c703a3. Readings: sails past
  ~20,100 => E in idx 581-608; dies ~19,985-20,041 => E
  in idx 609-636.
- **CD run update** (Lola, 2026-10-21 17:09): she let CD
  keep running after the 50,000+ report and closed the
  game herself at ~63,000, still sailing. Certified sail
  for the A+B+C+idx-581-636-interpreted config is now
  ~63,000 ticks - the hunt's longest run by ~26,000 over
  BK's 37,000+ benchmark.
- **CE verdict: SAILED** (Lola, 2026-10-21 17:36, in
  words = sail). CE interpreted idx 581-608 from 19,200
  and passed E's window => **Killer E is in idx 581-608**
  (28 functions). NEXT (CF): halve - MASK4 = idx 581-594
  gated at 19,200; sail => E in 581-594, death ~20,027
  => E in 595-608.
- **CF - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 581-594 ([0x28F30,
  0x3601E)) gated at 19,200; 2,924,531 B, MD5
  0403f30a49572f7dd4432a7a6229d06e. Readings: sails past ~20,100 => E in
  idx 581-594; dies ~19,985-20,041 => E in idx 595-608.
  NEW STANDING READOUT (Lola's request, from CF onward):
  top-right of the screen shows "BUILD <label>" with
  "#<count>" under it - the build's letter and its
  running count in the lettered series (CF = #84,
  grounded in 84 build dirs a-cf on disk). Baked in per
  build via -DRR3_BUILD_LABEL / -DRR3_BUILD_NUMBER;
  code in main.c + main_mainthread.c draw_stats,
  strings-verified in CF's ELF ("BUILD %s", "#%d").
- **CF verdict: DIED at tick 20,027 exactly** (Lola,
  2026-10-21 18:10, bare tick = death). Idx 581-594
  acquitted => **Killer E is in idx 595-608** (14
  functions). NEXT (CG): halve - MASK4 = idx 595-601
  gated at 19,200, stamp CG / #85; sail => E in 595-601,
  death ~20,027 => E in 602-608.
- **CG - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 595-601 ([0x3601E,
  0x3A4EA)) gated at 19,200; stamp CG / #85 (verified in
  the ELF); 2,924,582 B, MD5
  af08d0a0916b061b9bd2bd02df8aa016. Readings: sails past
  ~20,100 => E in idx 595-601; dies ~19,985-20,041 => E
  in idx 602-608.
- **CG verdict: DIED at tick 20,042** (Lola, 2026-10-21
  18:41, bare tick = death) - inside E's established
  wobble band (19,985 / 20,027 / 20,041 / 20,042 across
  configs; the interpreted neighborhood shifts his tick,
  it is the same killer). Idx 595-601 acquitted =>
  **Killer E is in idx 602-608** (7 functions). NEXT
  (CH): MASK4 = idx 602-605 gated at 19,200, stamp
  CH / #86; sail => E in 602-605, death in band => E in
  606-608.
- **CH - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 602-605 ([0x3A4EA,
  0x3A50A)) gated at 19,200; stamp CH / #86 (verified in
  the ELF); 2,924,703 B, MD5
  9611fb8ec93e2bd7bcd5b8994cec97b9. Readings: sails past
  ~20,100 => E in idx 602-605; dies in the wobble band
  => E in idx 606-608.
- **CH verdict: DIED at tick 20,027 exactly** (Lola,
  2026-10-21 19:16, bare tick = death). Idx 602-605
  acquitted => **Killer E is in idx 606-608 - THREE
  functions left.** NEXT (CI): the BY play - MASK4 =
  idx 606-607 gated at 19,200, idx 608 left native and
  exposed, stamp CI / #87. Death in the band => **E =
  idx 608, named outright**; sail => E in {606, 607} and
  one build finishes it.
- **CI - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 606-607 ([0x3A50A,
  0x3A66A)) gated at 19,200; idx 608 (func_03A66A) left
  native and exposed; stamp CI / #87 (verified in the
  ELF); 2,924,590 B, MD5 9204dcd833de4be1ac256d135143c5d0.
  Readings: dies in the wobble band => **E = func_03A66A
  (idx 608), NAMED**; sails past ~20,100 => E in
  {func_03A50A, func_03A53C} and one build finishes it.
- **CI verdict: SAILED** (Lola, 2026-10-21 19:48, in
  words = sail). CI interpreted idx 606-607 from 19,200
  with idx 608 native, and passed E's window =>
  **Killer E is in {idx 606 func_03A50A, idx 607
  func_03A53C}; idx 608 func_03A66A is ACQUITTED.**
  NEXT (CJ) is the guaranteed finish: MASK4 = idx 606
  alone ([0x3A50A, 0x3A53C)) gated at 19,200, stamp
  CJ / #88. Dies in the band => **E = func_03A53C**;
  sails => **E = func_03A50A**. Both outcomes name E.
- **CI extended run: DIED at tick 27,452** (Lola,
  2026-10-21 19:57 - after the sail report, the same run
  kept going and died). **KILLER F DISCOVERED.**
  Localization by set difference: CD (idx 581-636 ALL
  interpreted from 19,200) sailed to ~63,000; CI (only
  idx 606-607 interpreted) died at 27,452. Everything
  else native was identical, so **F is a dispatched
  function in idx 581-636, NOT 606/607** (53 candidates:
  581-605 + 608-636). F was invisible until now because
  every build since BZ died at E (<=20,042) first - the
  death-order pattern again. Lola's guess "linked to
  one of those two" is close in spirit (same span) but
  mechanically F is neither of the two - they ran
  interpreted in CI and the run still died. F's hunt
  starts once CJ names E: base = A+B+C+E masked, bisect
  inside 581-636 minus E.
- **CJ - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + MASK4 = idx 606 alone ([0x3A50A,
  0x3A53C)) gated at 19,200; stamp CJ / #88 (verified in
  the ELF); 2,924,565 B, MD5
  9ac7b916eedb2fae47cc4f874c1b2636. Readings: dies in
  E's band => **E = func_03A53C**; sails past ~20,100 =>
  **E = func_03A50A** - either way E is NAMED. Secondary
  watchpoint: a sailing CJ run that continues should
  meet Killer F at ~27,452.
- **CJ: SAILED past E, then DIED at tick 27,452** (Lola,
  2026-10-21 20:31 - "Sailed and crashed at 27452").
  **KILLER E = func_03A50A (idx 606) - CONVICTED.** CJ
  interpreted 606 alone and the E death vanished;
  func_03A53C (idx 607) ran native and is ACQUITTED.
  All four named killers now convicted: A func_02823A,
  B func_024A4C, C func_0207EE, E func_03A50A. E joins
  the permanent mask set in every build from CK on.
  **KILLER F CONFIRMED**: CJ died at exactly 27,452 -
  F's tick, second config in a row (CI, CJ). Determin-
  istic. Candidate count CORRECTED on recount: idx
  581-636 = 56 functions; minus 606/607 = **54 candi-
  dates** (581-605 = 25, 608-636 = 29); the 53 in the
  previous entry was an arithmetic slip.
- **CK - building**: A + B + C (gated 18,000) + E
  PERMANENT via MASK4 ([0x3A50A, 0x3A53C), gate 0 =
  always on; new MASK5 added to the system) + MASK5 =
  F-lower group idx 581-608 ([0x28F30, 0x3A9DE)) gated
  at 26,600 (includes the acquitted 607 - harmless:
  sail => F in {581-605, 608}; die at 27,452 => F in
  609-636, 28 functions). Stamp CK / #89.
- **CK - DELIVERED 2026-10-21, verdict pending**: A + B +
  C (gated 18,000) + E permanent (MASK4 gate 0) + MASK5
  F-lower idx 581-608 ([0x28F30, 0x3A9DE)) gated 26,600;
  stamp CK / #89 (verified in the ELF); 2,925,088 B,
  MD5 da74286e3dd03193ad07071b1f31207e. Readings: dies
  at exactly 27,452 => F in idx 609-636 (28 functions);
  sails past ~27,600 => F in {581-605, 608} (26).
- **CK: DIED at tick 7,456** (Lola, 2026-10-21 21:03).
  Analysis: 7,456 is BN's drift-death tick exactly, and
  no build in the BZ-CJ line (E native or gated 19,200)
  ever died there - they all reached >=18,797. CK's only
  functional delta from CJ was E UNGATED (MASK4 gate 0,
  "permanent"), which puts func_03A50A's early-demo
  calls through the capsule from tick 0 and raises the
  early capsule dose - a drift-class death, caused by
  the gate-0 design choice, owned here. (MASK5's clause
  is gate-inert before 26,600 and is the fallback
  suspect only if CL repeats this.) STRUCTURAL FIX,
  standing from CL on: convicted killers are gated just
  before their windows (the proven pattern), never
  ungated.
- **CL - building**: EXACTLY CJ's proven config (A + B +
  C gated 18,000 + E gated 19,200) + MASK5 F-lower idx
  581-608 ([0x28F30, 0x3A9DE)) gated 26,600. Stamp CL /
  #90. Readings as CK's: die at 27,452 => F in 609-636;
  sail past ~27,600 => F in {581-605, 608}. A repeat
  7,456 death would indict the MASK5 patch itself.
- **CL - DELIVERED 2026-10-21, verdict pending**: CJ's
  config + MASK5 (F-lower gated 26,600); stamp CL / #90
  (verified in the ELF); 2,925,088 B class, MD5 in the
  delivery message. Readings: die at 27,452 => F in
  609-636; sail past ~27,600 => F in {581-605, 608};
  die at 7,456 again => the MASK5 patch itself is
  indicted and CM becomes the CJ-verbatim control.
- **CL: SAILED** (Lola, 2026-10-21 21:42). Two verdicts
  in one: (1) **F in idx 581-605 or 608** (26 candidates;
  MASK5's 581-608 range minus acquitted 606/607);
  (2) CL did NOT die at 7,456 - the E-gate explanation
  for CK's death is CONFIRMED and the MASK5 patch is
  acquitted. Drift rule stands: killers gated, never
  ungated.
- **CM - building**: CL verbatim except MASK5 narrowed
  to idx 581-593 ([0x28F30, 0x36014), anchors verified:
  594 = 0x36014, 605 = 0x3A506, 608 = 0x3A66A) gated
  26,600. Even 13/13 split: dies at 27,452 => F in
  {594-605, 608}; sails => F in 581-593. Stamp CM / #91.
- **CM - DELIVERED 2026-10-21, verdict pending**: CL
  config with MASK5 narrowed to idx 581-593 ([0x28F30,
  0x36014)) gated 26,600; stamp CM / #91 (verified in
  the ELF); 2,925,xxx B, MD5 in the delivery message.
  Readings: dies at 27,452 => F in {594-605, 608} (13);
  sails => F in 581-593 (13).
- **CM: DIED at tick 27,452** (Lola, 2026-10-21 22:21 -
  bare tick). F not in 581-593 => **F in {594-605, 608}**
  (13 candidates).
- **CN - building**: CM verbatim except MASK5 = idx
  594-599 ([0x36014, 0x3A4E2), anchors verified: 594 =
  0x36014, 600 = 0x3A4E2) gated 26,600. Dies at
  27,452 => F in {600-605, 608} (7); sails => F in
  594-599 (6). Stamp CN / #92.
- **CN - DELIVERED 2026-10-21, verdict pending**: CM
  config with MASK5 = idx 594-599 ([0x36014, 0x3A4E2))
  gated 26,600; stamp CN / #92 (verified in the ELF);
  MD5 in the delivery message. Readings: dies at
  27,452 => F in {600-605, 608} (7); sails => F in
  594-599 (6).
- **CN: SAILED** (Lola, 2026-10-21 22:58). F in idx
  594-599 (6 candidates: 0x36014, 0x3601E, 0x3A4A8,
  0x3A4CE, and two more up to 0x3A4E2).
- **CO - building**: CN verbatim except MASK5 = idx
  594-596 ([0x36014, 0x3A4CE), anchors verified: 594 =
  0x36014, 596 = 0x3A4A8, 597 = 0x3A4CE) gated 26,600.
  Sails => F in {594, 595, 596}; dies at 27,452 =>
  F in {597, 598, 599}. Stamp CO / #93.
- **CO - DELIVERED 2026-10-21, verdict pending**: CN
  config with MASK5 = idx 594-596 ([0x36014, 0x3A4CE))
  gated 26,600; stamp CO / #93 (verified in the ELF);
  MD5 in the delivery message. Readings: sails => F in
  {594, 595, 596}; dies at 27,452 => F in {597, 598,
  599}.
- **CO: DIED at tick 27,452** (Lola, 2026-10-21 23:37,
  reported "27452 no build" - the BUILD stamp was not
  visible to her). F not in 594-596 => **F in {597,
  598, 599}** (3 candidates). Stamp fix: the stamp
  lived inside draw_stats behind the s_stats_on gate;
  from CP it is drawn unconditionally in
  display_present (both main files) - presenter-only,
  no sim impact.
- **CP - building**: CO verbatim except MASK5 = idx
  597-598 ([0x3A4CE, 0x3A4E2), anchors verified: 597 =
  0x3A4CE, 600 = 0x3A4E2) gated 26,600; plus the stamp
  fix above. Dies at 27,452 => F = 599 (named); sails
  => F in {597, 598}. Stamp CP / #94.
- **BUILDS PAUSED at Lola's word (2026-10-21 23:43).**
  CP stands configured and half-built (23/64 objects)
  in build-cp/ - resume = finish CP's build when she
  says go. State at pause: F in {597, 598, 599}; CP
  (MASK5 = 597-598) readings stand as recorded. The
  "keep building until I sleep" mode is superseded by
  this pause; do not auto-resume.
- **BUILDS RESUMED at Lola's word (2026-10-22 11:08).**
  Pre-build double-check CAUGHT A CONFIG ERROR: the
  as-configured cache held MASK5_HI = 0x3A4E2, but the
  mask predicate is dispatch-start-address in [LO, HI)
  (rr3_dispatch_mask.c:1420-1422), and the verified
  anchors are 597 = 0x3A4CE, 598 = 0x3A4D2, 599 =
  0x3A4DE, 600 = 0x3A4E2 - so [0x3A4CE, 0x3A4E2) masks
  {597, 598, 599}, the whole surviving set: CP would
  have carried no bisect information and a death would
  have contradicted CO. CORRECTED pre-build to
  MASK5_HI = 0x3A4DE => MASK5 = {597, 598} exactly, as
  designed; all other cache values re-verified verbatim
  (B/A/C/E masks + gates 18000/19200/26600, CP/#94).
  Readings unchanged: dies 27,452 => F = 599 named;
  sails => F in {597, 598}.
- **CP - DELIVERED 2026-10-22**: full rebuild after the
  MASK5_HI correction (mask defines are target-wide);
  foreground chunks on a memory-tight VM. Verified on
  disk: rr3vita-cp.vpk, 2,925,081 B, MD5
  222faaf56cce2160901e91e31f79a6d2 (your_files copy
  MD5-identical); ELF carries the stamp strings
  ("BUILD %s", "CP", "#%d") and the stamp is drawn
  unconditionally from display_present - CP is the
  stamp fix's first carrier. Verdict pending (Lola):
  dies at 27,452 => F = idx 599 (func_03A4DE) NAMED,
  the last killer; sails => F in {597, 598}, one final
  narrowing build.
- **CP: VERDICT PENDING - "sails" CALLED TOO EARLY**
  (Lola, 2026-10-22 11:59, correcting her 11:58 call:
  the run is STILL GOING, not there yet). The
  F-in-{597, 598} conclusion recorded from the early
  call is WITHDRAWN until she gives the final word on
  the finished run. **Final word given 2026-10-22
  12:01: "it did sail", run at tick 28,000+ - CP:
  SAILED, certified past the 27,452 death point.
  F in {597, 598} STANDS.** Final narrowing (paper only,
  unbuilt)
  build CQ: MASK5 = idx 597 alone ([0x3A4CE, 0x3A4D2),
  anchors 597 = 0x3A4CE, 598 = 0x3A4D2) gated 26,600,
  all else CP verbatim. Dies at 27,452 => F = 598
  (func_03A4D2) NAMED; sails => F = 597 (func_03A4CE)
  NAMED. Stamp CQ / #95.
- **CQ - DELIVERED 2026-10-22**: configured fresh;
  cache diff vs CP = exactly three lines (label CQ,
  #95, MASK5_HI 0x3A4DE -> 0x3A4D2) => MASK5 =
  {idx 597} alone, everything else CP verbatim. One
  transient mid-build Error 2 (resource flake; retry
  compiled clean through [100%]). Verified on disk:
  rr3vita-cq.vpk, 2,925,104 B, MD5
  103a39dcf6c0042248b2f1b72b70578b (your_files copy
  MD5-identical); ELF strings carry "CQ" + "BUILD %s".
  Verdict pending (Lola): dies at 27,452 => F = 598
  (func_03A4D2) NAMED; sails => F = 597 (func_03A4CE)
  NAMED. Either way the killer list completes.
- **CQ VERDICT: DIED at 27,452** (Lola, 2026-10-22
  12:46 - bare tick per her convention). With idx 597
  interpreted from tick 26,600 the death stands =>
  **KILLER F = idx 598 = func_03A4D2. CONVICTED.**
  THE KILLER LIST IS COMPLETE: A func_02823A (idx
  559), B func_024A4C (idx 506), C func_0207EE (idx
  378), E func_03A50A (idx 606), F func_03A4D2 (idx
  598). Endgame build CR: MASK5 = {598} exactly
  ([0x3A4D2, 0x3A4DE) - 598's start to 599's start),
  gate 26,600, all else CQ verbatim; long demo soak
  past 63,000 ticks (CD's benchmark) is its test.
  Lola's own soak plan (2026-10-22 12:47): first
  checkpoint 50,000; if it gets there she pushes for
  a 100,000 soak. CR configured 2026-10-22: cache
  diff vs CQ = label CR/#96 + MASK5_LO 0x3A4CE ->
  0x3A4D2, MASK5_HI 0x3A4D2 -> 0x3A4DE (MASK5 =
  {idx 598} exactly); all else identical.
- **CR - DELIVERED 2026-10-22**: verified on disk:
  rr3vita-cr.vpk, 2,925,133 B, MD5
  4f70409323f3a9a1ddf0dcb254488401 (your_files copy
  MD5-identical); ELF carries the "CR" stamp strings.
  All five killers masked in one build for the first
  time (B/A ungated; C gate 18,000; E gate 19,200; F
  gate 26,600). Verdict pending (Lola): her soak plan
  - 50,000 checkpoint first, then a 100,000 push. A
  death at any point names a sixth killer (or drift);
  a completed 100,000 soak = native path stands.

## CR soak — 100,000 CLEARED (2026-10-22)

Lola's progress reports on the single CR run: 19,000 (past the A/B/C
death zones) → 32,000 (past all five death points) → 50,000 (her first
checkpoint, cleared) → 60,000+ → 70,000+ (past CD's ~63,000 — the
longest certified run of the hunt at that moment) → **"100,000+" at
14:34, stated in words = the run passed her final soak target**.

Verdicts this one run certifies:
- The five convicted killers are the COMPLETE demo death list
  through 100,000 ticks: A func_02823A, B func_024A4C, C func_0207EE,
  E func_03A50A, F func_03A4D2.
- CR masks only the five single functions — unlike CD, which masked
  the whole 581–636 span — so the rest of the span stands acquitted
  by this run as well.
- The capsule-drift specter (D) did not materialize at 100k under
  the gated-mask configuration.

The native-path stability phase's soak gate is PASSED. Next phase is
desk work (read the convicted functions' generated code for the
common defect), then perf, audio output, and the core 0/1/3 split.

## CR soak final tick + first gameplay crash (2026-10-22, breadcrumb-46)

- The soak session's breadcrumb runs to **play tick 123,360** (past
  the 100,000+ Lola reported live), maskstat totals at frames=122,400:
  **tried=116,967 hit=116,967 — zero failed capsule runs in the
  entire soak.** End mode (Lola closed it vs died ~123,4xx) NOT yet
  confirmed — asked; the breadcrumb alone cannot distinguish.
- Lola then played interactively. The play session: clean boot,
  phase 1/2 nominal, phase 3 reached tick 2,340 (last written line),
  died before the 2,400 sample. Capsules 94 tried / 94 hit. insn
  rate and roll (~50) normal to the end — a sudden death, not a
  stall.
- The session's sampled frame PC sat at 0x020558 for its last ~540
  ticks. 0x020558 is mid-function inside **func_0203E8 (idx 374)**
  (start 0x203E8, next 0x20592; sample offset 368) — NOT a function
  start, and NOT gameplay-exclusive: the soak's attract loop also
  sat in this same state for stretches (ticks 122,280–123,120)
  without harm. So this is a game-state neighborhood, not a named
  killer. The fatal dispatch could be anything executed in the
  60-tick window after the last sample, plausibly input-dependent
  (branches the attract loop never takes).
- Standing read: the demo verdict (five killers, complete through
  123k ticks) is untouched. The gameplay death is a new class —
  input-path — and the demo-only bisect cannot reproduce it.
  Next evidence needed: Lola's account of what she did/pressed and
  what the screen showed, then targeted instrumentation in that
  state rather than another demo bisect.

## Soak end mode + gameplay crash context (Lola, 2026-10-22 15:19)

- **Soak end mode CONFIRMED by Lola: the Vita went to sleep.** The
  run did not die and she did not close it in-app. Final soak record:
  **CR passed 123,360+ ticks with zero capsule failures (116,967/116,967)
  and ended only by console sleep.** The demo stability verdict is
  complete and clean.
- **Gameplay crash context (her words):** racing the first track
  "like normal"; she was **attacking while being attacked** and
  **trying to use the boost** to make the bike go faster. Her own
  hedge, preserved verbatim: "Might have crashed cuz I was trying
  to boost, MIGHT."
- Read: combat + boost are player-input paths the attract loop does
  not exercise the same way; correlation with the boost is her
  suspicion, not yet evidence. Decisive cheap test = reproduction
  protocol (race without boost vs race with boost). Desk work on
  the race-state neighborhood (idx 374 region + input paths) runs
  in parallel.

## CS (#97) — input-logging build DELIVERED (2026-10-22)

- Config = CR's five gated masks, cache-verified line by line
  pre-build (B 0x24A4C/0x24AF8; A 0x2823A/0x2834A; C 0x207EE/0x20974
  g18000; E 0x3A50A/0x3A53C g19200; F 0x3A4D2/0x3A4DE g26600).
  The ONLY change vs CR: in the phase-3 loop (src/main.c), every
  change of the Genesis pad stream is appended to the breadcrumb
  as `input tick=%u pad=%04X prev=%04X`, written from the sim
  thread in the same pattern as breadcrumb() — never the game
  fiber (AJ rule respected). Reproduction target: Lola's track-1
  gameplay crash at ~2,340 (combat both ways + boost attempt).
- rr3vita-cs.vpk: **2,924,900 B, MD5 dba8773805aec69c8e79bc6fb7901b28**
  (build-dir and your_files copies identical). ELF verified:
  "BUILD %s", "CS", "#%d", and the input format string all present.

## CS TEST RESULT — died racing, tick ~3199–3240 (breadcrumb-47, 2026-10-22)

- Session identified cleanly (the file also carries the CR soak and
  CR's gameplay session ahead of it). CS's session: phases nominal,
  first input at tick 1958, **last play sample tick 3180, last input
  tick 3199, death in the (3199, 3240) window** — about 900 ticks
  deeper into the race than CR's gameplay death (~2340–2400).
- Sampled PC parked at **0x020558 (func_0203E8)** for the last
  ~540 ticks again; fbh still changing at the final samples —
  rendering alive to the end; sudden death, insn rate normal.
- Capsules: the frames=1200 maskstat (tick 2100) reads **tried=0
  hit=0** — NO killer dispatch in the first 2,100 ticks of this
  race (CR's race had 94 by then; different race evolution). The
  2100→death window has no checkpoint (next at 3300), so capsule
  involvement at the death is UNRESOLVED — the gap CT closes.
- Input decode (GPAD: B=0x10 accel, C=0x20, A=0x40, START=0x80,
  L=0x04 R=0x08): B held nearly the whole race; steering L/R on top.
  **A was never pressed all session; START once, 3 ticks, at 2014.**
  From ~tick 3090 to death the stream is dense with **B+C and
  B+C+steering combos** (0030/0038/0034) — C is the attack button,
  and this is the combat/boost neighborhood of her CR account. The
  final inputs: tick 3196 B+C, **tick 3199 B+C+LEFT**, then silence.
  Correlation only — but the C-heavy final second is now on record
  in two crashes' neighborhoods (CR by her report, CS by the log).

## CT (#98) — capsule-event logging + purpose stamp (building 2026-10-22)

- Config = CS's five gated masks EXACTLY (build-ct cache diffed
  against build-cs: identical except label CT / number 98).
- Changes vs CS (instrumentation only):
  1. `g_mask_last_addr` in rr3_dispatch_mask.c, stored at the mask
     dispatch site next to g_mask_tried++ (a plain store — the
     dispatch path still does NO stdio, AJ rule intact).
  2. Phase-3 loop polls g_mask_tried after each step (sim thread);
     on change it appends `capsule tick=%u addr=%06X tried=%u
     hit=%u` to the breadcrumb. Every killer dispatch in the race
     is now tick-stamped with its guest address.
  3. Play-sample cadence 60 → 20 ticks (narrower death window).
  4. CS's input logging carried over unchanged.
- **Purpose stamp (Lola's request, 2026-10-22, standing from CT
  onward):** every test build says in simple words, bottom-left on
  screen, what it is doing/looking for — draw_purpose_stamp() in
  main.c (RR3_PURPOSE_TEXT edited per build), drawn alongside the
  BUILD stamp. CT's text: "CT: LOGGING CAPSULES TO CATCH THE RACE
  CRASH".

## CT (#98) — CAPSULE-EVENT LOGGING + PURPOSE STAMP — DELIVERED 2026-10-22 16:46
- Config: IDENTICAL to CS (diffed cache vs build-cs; only LABEL/NUMBER differ). B [0x24A4C,0x24AF8) ungated; A [0x2823A,0x2834A) ungated; C [0x207EE,0x20974) gate 18000; E [0x3A50A,0x3A53C) gate 19200; F [0x3A4D2,0x3A4DE) gate 26600.
- Changes vs CS: (1) g_mask_last_addr recorded at dispatch; phase-3 loop logs `capsule tick=%u addr=%06X tried=%u hit=%u` on every g_mask_tried change (sim thread); (2) play samples every 20 frames (was 60); (3) FIRST PURPOSE STAMP (Lola's request 2026-10-22 16:30): bottom-left `CT: LOGGING CAPSULES TO CATCH THE RACE CRASH` via draw_purpose_stamp()/RR3_PURPOSE_TEXT; (4) CS input logging carried over.
- Artifact verified on disk (foreground): rr3vita-ct.vpk 2,925,650 B, MD5 14633524c618f9e493d35f2337009e14 (build dir == your_files copy). ELF strings verified: "BUILD %s", "CT", "#%d", purpose text, capsule format string.
- Mission (same as CS): race track 1, fight/get fought/boost; on death send breadcrumb + dump. The capsule lines will name any killer dispatch in the final stretch — or prove none fired.

## CT TEST RESULT — 2026-10-23 (breadcrumb-48, MD5 b650b03dfe890f2d81022026b8111700)
- Session: phases 1/2 clean; phase 3 play ticks 920 -> death in (1960, 1980). Third gameplay death (CR (2340,2400); CS (3199,3240); CT (1960,1980)) — no fixed tick; event-driven.
- Capsule log (CT's new instrument) WORKED: 22 lines, ALL addr 02823A = Killer A (func_02823A), ticks 1830-1959 at a ~6-tick cadence, tried=hit=44/44, every capsule checkpoint-clean. ZERO killer dispatches before tick 1830.
- Inputs: 102 changes; A button (0x40) WAS pressed this run (4 inputs with the A bit, ticks 1686-1699, combos A+B and A+B+C) — unlike CS (no A all session). Dense B+C combat around the capsule window; last input tick 1947 (B+C+Left).
- Death is <=21 ticks after the last logged A capsule (1959) and <=33 after the last input. The capsule line is written AFTER the step, so a dispatch that kills the process mid-step leaves no line: the next A dispatch was due ~1965. Two live hypotheses: (H1) a capsule run of A died mid-flight under race state (A's 44 prior runs all verified); (H2) a native crash in the func_0203E8 neighborhood (sampled PC parked at 020558 again — 7 of last samples) between A dispatches.
- Next: CU = CT + mask idx 374 func_0203E8 [0x203E8,0x20592) — demo-proven innocent, so its capsules should verify; if the death vanishes, H2 is convicted; if it still dies on the A metronome, H1. Desk-read of func_02823A's generated code in parallel.

## CU (#99) — CT + MASK idx 374 (func_0203E8) — DELIVERED 2026-10-23 12:11
- Question: split CT's two hypotheses — H1 (a capsule run of Killer A died mid-flight under race state) vs H2 (native crash in the func_0203E8 neighborhood).
- Plumbing (new, minimal): RR3_MASK6_LO/HI/GATE_TICK support added in the MASK pattern's own shape — CMakeLists.txt definition blocks + a MASK6 clause in recomp_dispatch_once (rr3_dispatch_mask.c). Anchor verified from the file: dispatch table line 1060 = 0x0203E8, line 1061 = 0x020592, so [0x203E8,0x20592) is exactly idx 374.
- Config: CT's five masks byte-identical (CMakeCache diff vs build-ct shows ONLY LABEL CT->CU, NUMBER 98->99, +MASK6_LO 0x203E8 +MASK6_HI 0x20592, ungated — 374 is demo-proven innocent). Purpose stamp: "CU: TESTING IF 0203E8 IS THE RACE CRASH". Input + capsule logging carried from CT unchanged; 374 dispatches will log addr=0203E8.
- Artifact verified on disk (foreground): rr3vita-cu.vpk 2,925,603 B, MD5 f9d13ffeb364603f178dc13788754423 (build dir == your_files copy). ELF strings verified on the raw ELF (the .self is compressed): BUILD stamp, CU purpose text, capsule format string all present.
- Read: death gone/past ~2000 with clean 0203E8 capsules => H2 convicted. Still dies on A's metronome => H1; desk-read func_02823A next.

## CU TEST RESULT — 2026-10-23 (breadcrumb-49, MD5 21d57d0eafacc108df6a93b16238f4dd)
- Session: phase 3 play ticks 920 -> last play 3060, last capsule 3075, death in (3075, ~3095). Fourth gameplay death.
- DECISIVE NEGATIVE: capsule lines by addr = {02823A: 196} — ZERO dispatches to 0203E8 all race, so CU's new mask never engaged. The sampled PC parking at 020558 is NOT a dispatch phenomenon: execution spends its sampled life inside 0203E8's body without entering via recomp_dispatch_once (fall-through / direct intra-part branches).
- Killer A: first capsule tick 2004, metronome ~5-6 ticks to 3075, tried=363 hit=363 — all clean, death at a random point ~1,071 ticks into the A regime (CT died ~150 in). No capsule-count correlation. 18 log lines show tried=hit+1 transiently = the presenter thread polling mid-dispatch (a sampling race in the logger, NOT capsule failures — hit always catches up; cf. the demo soak's 116,967/116,967).
- Verdict: H2-as-dispatch DEAD. H1 (capsule crash) WEAKENED (363 clean A runs). New H3: native execution INSIDE func_0203E8's body, entered via non-dispatch paths — a killer the dispatch-mask bisect structurally cannot see. Next: desk pass — read 0203E8's generated body for the convicted-killer defect shape + grep generated code for branch/call targets inside [0x203E8,0x20592).

## Build CV (#100, 2026-10-23) — call-ring build, DELIVERED

Desk discovery first (same day): generated code has TWO call paths — recomp_dispatch_once (masked/logged) and recomp_call_func (direct C call: no dispatch, no mask, no log). 488 distinct functions are direct-called. func_0203E8 is called ONLY via recomp_call_func (from part11 guest $0142C2), which is why CU's mask never engaged and CT/CU never logged it. func_0203E8's body (part22) contains the dynamic per-entity handler call at guest $020556: A6 = [A4], recomp_call_addr(A6) — 020558 is that call's return point, the entity workhorse hub the exception sampler kept parking on.

CV config = CT's five masks exactly (MASK6 config dropped; plumbing remains in the tree). ONE source change-set: a 512-entry call ring in rr3_dispatch_mask.c written by plain stores from BOTH call paths (dispatch pushes the resolved addr; recomp_call_func reverse-maps its pointer through the dispatch table — fn-sorted index built once, binary search per call). main.c drains it between steps: "call tick=%u addr=%06X" on the first sighting of each address per session (256 KB seen-bitmap over the guest space), plus "tail tick=%u" + last 8 ring addresses every 600 frames. Purpose stamp: "CV: LOGGING EVERY CALL TO FIND THE RACE KILLER". Kamp cache diff vs CT: LABEL + NUMBER only.
**rr3vita-cv.vpk: 2,926,421 B, MD5 6178e728adf1adc4240a76752ddc7130** (build dir == your_files copy; purpose + both format strings verified in the raw ELF).
Test: same track-1 race. Expected yield: the breadcrumb names every function the race touched, in first-touch order, from BOTH call paths; if it dies, the tail snapshots + last first-sightings bracket the killer's neighborhood; if a brand-new name appears shortly before the death window, that name is the prime suspect.

## CV result (breadcrumb-50) + Build CW (#101, 2026-10-23) — first suspect named

CV session (breadcrumb-50, MD5 e67031c6a2a7481c7835de7e1d4a70e3; file now holds 102 sessions): 236 first-sighting "call" lines, 2 tail snapshots, ZERO capsule lines (Killer A never dispatched — this race died before the combat metronome started). Last play tick=1560; last line "call tick=1561 addr=02014A" — death in (1561, 1580), the earliest gameplay death yet. func_02014A (idx 392, extent [0x02014A, 0x02019C), 82 guest bytes) was called for the FIRST TIME EVER in the step ending at tick 1561 — ~10 ticks after Lola's one A-press (pad 0038 = B+C+A at tick ~1550). Because the call line was drained, that step COMPLETED: 02014A ran and returned at least once; the fatal step is one of the next ~18 ticks. func_02014A is called ONLY via recomp_call_func (4 sites in part21, e.g. guest $01FDD4); its body (part22.c:577-780) is LEAF arithmetic — entity-proximity math: D6 = [A1+50] - [A2+50], abs, -0x20, byte ops on [A3+108]; no calls, no writes outside registers. Suspect model: either its Nth call under changed entity state, or the state chain it feeds.
CW = CV + MASK6 [0x02014A, 0x02019C) — with a MECHANISM FIX: the dispatch mask cannot see direct calls (the CU lesson), so recomp_call_func now honors the MASK6 range itself, routing masked direct calls through rr3_mask_run with the same tail-frame bookkeeping. Cache diff vs CV: LABEL, NUMBER, MASK6 pair only. Purpose: "CW: MASKING 02014A TO TEST THE SUSPECT".
**rr3vita-cw.vpk: 2,926,327 B, MD5 f943135fb17e49c01e4f83592ee4f1a2** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same track-1 race. Verdicts: capsule lines for 02014A with hit<tried = convicted; race sailing well past ~1580 repeatedly = its native path was the cliff; death with all 02014A capsules clean = the killer is downstream of the state it sets up, and the ring's first-sightings bracket it further.

## CW result (breadcrumb-51) + Build CX (#102, 2026-10-23) — the hit that never lands

Lola's own observation, same report: "Its crashing RIGHT before I can hit someone." The log agrees precisely. CW session (breadcrumb-51, MD5 123e5f1b0c649e5664a8d5acdb13bf23): 39 capsule lines, all addr=02823A (A's metronome, all clean, final 64/64); inputs at the end: B held with LEFT taps, then B+C at 1900, B+C+LEFT at 1904 — she drew alongside and hit the attack button. First-sighting cascade in the closing window: tick 1906 {01D386, 01D406, 02834A} → 1911 {02014A} → 1913 {01D21C, 01D796}; last play 1900; death in (1913, ~1920) — the tightest window yet.
02014A verdict: it was called at 1911 and (per CW's code path) routed through the capsule, and the race died anyway ~2 ticks later with 01D21C→01D796 entering at 1913. Caveat recorded: CW's call_func branch never incremented tried/hit, so 02014A's capsule runs are counter-invisible (the +1 steps at 1912/1913 are poll-timing transients of A's pairs) — its native body may or may not have run on a mismatch fallback; either way its first call completed and the death is downstream. 02014A is the range-finder, not the killer.
Desk identification: func_01D21C ([0x01D21C,0x01D228), 12 guest bytes) is a trampoline whose only act is JSR func_01D796. func_01D796 ([0x01D796,0x01D8B2), 284 guest bytes, 591 generated lines, part19) opens by pushing A3/A2 to a frame and writing entity fields — the attack/hit-resolution routine is the prime suspect: it starts firing as she closes in, and the process dies inside its first few invocations, before the hit lands.
CX = CW config (incl. MASK6 02014A) + MASK7 [0x01D796, 0x01D8B2) in BOTH the dispatch clause and the call_func branch. Two mechanism fixes ride along: the call_func mask branch now counts tried/hit and sets g_mask_last_addr exactly like the dispatch path (capsule lines will name 01D796 truthfully); MASK7 plumbing added (CMake + dispatch clause, same shape as MASK6). Cache diff vs CW: LABEL, NUMBER, MASK7 pair only. Purpose: "CX: MASKING 01D796 THE HIT SUSPECT".
**rr3vita-cx.vpk: 2,926,460 B, MD5 84011bb0b40f612344a6fa06c55ff907** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same track-1 race, and deliberately pick a fight — close on someone and swing. Verdicts: capsule lines naming 01D796 with hit<tried = convicted on the spot; hits landing and the race running on = its native path was the cliff; death with 01D796 capsules all clean = the killer is a callee of 01D796 or the state it writes — the ring keeps bracketing.

## CX result (breadcrumb-52) + Build CY (#103, 2026-10-23) — two more exonerations, bracket tightens

CX session (breadcrumb-52, MD5 5c3153a05243fc74721d1aeb9d2edb80): the counting fix works — capsule lines now name direct-path runs. func_02014A capsuled 5x (ticks 1551-1867), ALL clean. func_01D796 capsuled from tick 1882, ~14 runs across 5 poll lines (53/53, 58/58, 61/61, 64/64, 67/67), ALL clean — and the race SAILED THROUGH CW's exact death window (play sample at 1900 even caught pc=01D226, the 01D796 return point, mid-resolution). Death nevertheless in (1932, ~1940): last capsule 1932 (A, 76/76), last play 1920. Verdict by the demo-bisect logic: a masked function whose capsules all pass while the crash persists is NOT the crash site — 01D796 is EXONERATED as the site, same as 02014A and A before it. The killer is a later link in the hit-resolution chain.
New first-sightings in CX's final approach: 023FCC @1887 and 0249AA @1895 — the only new names between the 01D796 cluster and the death. Desk: func_023FCC (idx 515, [0x023FCC,0x0241B8), 492 guest bytes / 894 lines, part25, direct-called) opens by writing entity state fields (A1+113=0x02, A1+114=0xFF, A1+92, A1+104) and calls func_024844 — a state-transition writer. func_0249AA (idx 526, [0x0249AA,0x024A4C), 71 lines) is a small wrapper calling func_027B84; its extent ends exactly where Killer B begins.
CY = CX + MASK8 [0x023FCC, 0x0241B8) in both paths (MASK8 plumbing mirrors MASK7). One variable: 023FCC first (size + state writes + direct-call); 0249AA is next if CY exonerates it too. Cache diff vs CX: LABEL, NUMBER, MASK8 pair only. Purpose: "CY: MASKING 023FCC THE STATE WRITER".
**rr3vita-cy.vpk: 2,925,829 B, MD5 bb2bcdac6aa8b2ffa6c15adf7d1265f6** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same — pick a fight, swing. If 023FCC capsules run clean and it still dies, CZ masks 0249AA; if it sails, we split to convict.

## CY result (breadcrumb-53) + Build CZ (#104, 2026-10-23) — the chain is a family

CY session (breadcrumb-53, MD5 3e4705ad68d5e098d5ebc35915c4000e): func_023FCC was called ONCE (tick 1430, in a seven-function first-sighting burst as a whole subsystem woke) and capsuled CLEAN (1431, tried=5 hit=5). func_01D796 / 01D21C / 0249AA were NEVER CALLED this race — different fight geometry. The race ran through every previous death window to a last play at 1960 and a last A capsule at 1964 (46/46); death in (1964, ~1980). Last new name before the silence: func_028AF4 @1925 (idx 587, [0x028AF4,0x028B76), 336 lines, part27 body, direct-called from part30). func_0241B8 also debuted @1702.
Pattern across CV/CW/CX/CY: every race dies a few dozen ticks after SOME member of the extended combat family (0x01Dxxx-0x028xxx) is first touched under fight state; each capsuled suspect runs clean and the cliff moves one link later. The family shares the clusters with convicted killers B (024A4C) and A (02823A). Desk common-defect read of the family's generated code remains the strategic alternative to serial masking.
CZ = CY + MASK9 [0x028AF4, 0x028B76) + MASK10 [0x0249AA, 0x024A4C) — TWO masks in one build, a deliberate trade (recorded): her race-and-upload cycles are the scarce resource; if CZ sails we split to convict individually, if it dies two more names clear at once. MASK9/MASK10 plumbing mirrors MASK7/8 in CMake, dispatch clause, and the call_func range checks. Cache diff vs CY: LABEL, NUMBER, MASK9 pair, MASK10 pair only. Purpose: "CZ: MASKING 028AF4 AND 0249AA".
**rr3vita-cz.vpk: 2,926,357 B, MD5 16e7ec65406d7d745d8c7a537d805e32** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same — get into fights. Watch capsule lines for 028AF4/0249AA verdicts.

## CZ result (breadcrumb-54) + Build DA (#105, 2026-10-23) — blanket the named set

CZ session (breadcrumb-54, MD5 80fa8aea90b3cf9d337cb19e358360f8): func_028AF4 called @1830, capsuled (1831 poll caught it mid-flight, tried=21 hit=20 — the deficit did NOT persist in later lines, 47/47 onward, so the run completed clean). func_0249AA never called — remains untested individually. 02014A x5 and 023FCC x1 also capsuled clean. Race died in (1960, ~1980) — the same window as CY — a couple dozen ticks after Lola's B+C+A swing at 1942. New late names: 01B414 @1864, 02266E @1869, 03AF74 @1875.
Score after five gameplay builds: every individually capsuled suspect (02014A, 01D796, 023FCC, 028AF4) runs checkpoint-clean and the race dies anyway, always a few dozen ticks after some combat-family function is first touched under fight state. The marginal value of one-at-a-time masking has collapsed.
DA changes strategy: a table-driven mask (s_table_mask in rr3_dispatch_mask.c, gated by RR3_TABLE_MASK) covering 29 extents — every function first sighted in a combat window across CV/CW/CX/CY/CZ that is not already convicted or exonerated (including 0249AA at last). All ten prior masks retained. Cache diff vs CZ: LABEL, NUMBER, TABLE_MASK only. Purpose: "DA: MASKING THE WHOLE SUSPECT LIST". Read: if DA survives fights that killed CV-CZ, the killer is in the table and we split it down; if DA dies the same way, the killer enters outside call entry (fallthrough or glue) and the desk common-defect hunt becomes the main road with certainty.
**rr3vita-da.vpk: 2,926,770 B, MD5 6af98bc218ab504d3dcbcaffc0365abd** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same track-1 fights; multiple fights if it lets you — the first clean extended fight session is the signal.

## DA result (breadcrumb-55) + Build DB (#106, 2026-10-23) — the table works; one new name

DA session (breadcrumb-55, MD5 2844a952aa58b768b962ccefe3a97b2a): the race ran to play tick 2820+ and died in (2829, ~2850) — roughly 900 ticks past every previous gameplay death — after SUSTAINED landed combat (Lola: "I was able to land a few punches before it crashed this time"). 913 capsule runs, ALL checkpoint-clean (final tried=913 hit=913). Table traffic: func_017C14 capsuled 160x (this fight's workhorse — a second combat cluster around 0x017Cxx/0x020Dxx, distinct from the 0x01D7xx cluster), func_020D2C 62x, A 85 lines; 01D796 never fired — different fight geometry again. The exoneration-by-volume pile grows; the table as a whole clearly carries the fight much further.
Terminal signal unchanged in form: the LAST first-sighting before the silence was func_020974 @2823 (idx 379, [0x020974,0x020D2C), 952 guest bytes, direct-called from rr3_part22.c:3870/4194) — the function immediately AFTER convicted killer C (0207EE ends exactly at 0x020974), and immediately before heavily-clean 020D2C. Death followed within ~6-26 ticks. The suspects keep being the convicted killers' literal neighbors (0249AA ends at B; 02834A follows A; 020974 follows C).
DB = DA (all masks + table) + MASK11 [0x020974, 0x020D2C) in both paths. Cache diff vs DA: LABEL, NUMBER, MASK11 pair only. Purpose: "DB: DA PLUS MASKING 020974 AFTER C".
**rr3vita-db.vpk: 2,926,654 B, MD5 42d9b0fcbf9906169e66f8e48bcf168c** (build dir == your_files copy; purpose + call-ring strings verified in the raw ELF).
Test: same fights. A session that sails past ~2850 into long combat is the signal; if it dies, the approach names in this breadcrumb are the next table extension.

DB TEST RESULT — tenth gameplay death (breadcrumb-56, MD5 8c1045f3bfa806872762c6a7b55ab509; dump psp2core-1792816955): died in (4363, ~4380) — the deepest gameplay run yet, ~1,530 ticks past DA's death and past every prior gameplay window. Lola: "holy moly I got MUCH further on this build! I was getting lost in this game until it crashed." Session totals: 2,491 capsule runs, ALL checkpoint-clean (final tried=2491 hit=2491); 281 first-sighting call lines; 173 play samples; final play tick 4360 (pc=01B0B8), last capsule 020D2C @4363 clean.
MASK11 verdict: func_020974 was first sighted @4356 and capsuled CLEAN @4357 (tried=2484 hit=2484) — yet the session died ~6-17 ticks later anyway. By the standing rule (masked + clean + death persists = not the site) 020974 is EXONERATED as the crash site, but the echo is now twice-repeated (DA: sighted @2823 unmasked-in-table-era, death ~6 ticks later; DB: sighted @4356, capsuled clean, death ~7-17 ticks later) — 020974 is the HERALD: the killer is what it calls or hands off to under fight state. Its callees are the next desk target.
New territory past DA's death (first-sightings): 023828 @2906; 0258DC + 025918 @3431; 025892 @3593; 02378C @3648; then a final-approach cluster @4193-4199: 02138A, 0207EE (Killer C itself — called NATIVELY @4194 because its CR gate is 18,000, unreachable in gameplay sessions; session survived it by ~170 ticks), 021EBE, 020608, 02207A, 01EB36; then 020974 @4356. Session capsule histogram leaders: 017C14 x542, A x401, 020D2C x105 — the second cluster remains the fight's engine room.
DC direction: desk-read func_020974's generated body for its callees; extend the table with the DB session's new combat-window names (023828, 0258DC, 025918, 025892, 02378C, 02138A, 021EBE, 020608, 02207A, 01EB36) — several were already DA-era candidates. Open question for DC: whether to bring Killer C's gate down into gameplay range (it currently never engages below tick 18,000).

DC (#107) BUILT, VERIFIED, DELIVERED — desk-driven build. Desk findings first: func_020974's generated body is a 10-instruction leaf (reads entity byte [A1+119], indexes table 0x35158, sets flags, returns) with NO calls; its ENTIRE [0x020974,0x020D2C) extent after the return is just the run-up to func_020D2C. Both of its call sites (rr3_part22.c:3870, 4194) are inside func_0207EE — KILLER C itself. C's call tree explains DB's whole 4193-4199 first-sighting cluster (020608 after site 1, 021486 after site 2, 024C48 between them): in DB, C was entered natively @4194 (its CR gate of 18,000 never engages at gameplay ticks) and its subtree ran bare. DC = DB config with TWO changes, both announced: (1) RR3_MASK3_GATE_TICK 18000 -> 900, so C is capsuled from the first gameplay tick (C has dispatch entry only, no direct callers — the gate fully controls it; phases 1-2 never reach C, so demo behavior is untouched); (2) the DA table extended 29 -> 40 extents with DB's new combat names + C's callees: 01EB36, 020608, 02138A, 021486, 021EBE, 02207A, 02378C, 023828, 024C48, 0258DC, 025918 — every extent computed from the dispatch table, all verified real entries, sorted, zero overlaps with the table or MASK1-11. Cache diff vs DB: LABEL, NUMBER, MASK3_GATE_TICK only. Purpose: "DC: UNGATING C, TABLE PLUS 11 NAMES".
**rr3vita-dc.vpk: 2,926,280 B, MD5 18f4ce2c518349aebf5487712cf43a94** (build dir == your_files copy; purpose string verified in the raw ELF).
Test: same fights — and this time, if she knocks a rider off, C's whole subtree runs capsuled from tick 900; capsules for 0207EE in the log are the tell. A long sail past 4363 is the signal.

DC TEST RESULT — eleventh gameplay death, AFTER A COMPLETED RACE (breadcrumb-57, MD5 13aceec7d6cbf42ddfb822f0dc52c9af; dump psp2core-1792824252): Lola: "holy wowza!! I managed to finish a race this time!!" — the gameplay hunt's first completed race. Session ran to play tick 11820 (2.7x DB's reach) with 18,200+ capsule runs ALL clean (final tried=18200 hit=18200; maskstat phase-3 frames=10800 tried=18104 hit=18104). The DC tell: Killer C (0207EE) capsuled 1,116 times clean, first sighted @2285 — the gate change worked exactly as designed. Table newcomers pulled weight: 021486 x1471 clean, 02207A x328 clean; 017C14 still the engine room (x1922). Inputs end @11622 with pad=0000 (hands off — consistent with a finished race), after which capsule traffic is post-race metronomes, no fight inputs. Death in (11831, ~11840): the last lines are two brand-new first-sightings @11831 (01A718, 01ABB2) — the crash has moved PAST the finish line into post-race flow. New territory for DD: 023828 @6429 (already tabled), 028E18 + 028D9C @8239, and a late cluster @11496-11831: 01E0AC, 025370, 01C438, 0181D0, 01A718, 01ABB2 (desk-read those two first — likely results/menu/next-race setup).

DD (#108) BUILT, VERIFIED, DELIVERED — post-race build. Desk findings first: func_01A718 (idx 371, [0x01A718,0x01A86E)) is a post-race setup/walk routine — A1=0x00FF09B2 struct, A2=0x00FF741E; copies 0x00FF0994 (or [A1+98]) → 0x00FF099A and [A1+4]/0x00FF024C → 0x00FF7A6C; walks a list counted by [0x00FF043A]; 5 call sites; last function in rr3_part16.c. func_01ABB2 (idx 375, [0x01ABB2,0x01AC16)) is a ZERO-call leaf — reads the shared 0x00FF099A, computes ((X<<8)/D0.w)*[0x00FF024C]>>8, stores into [A2+indexed slot]: a results-table store shape, and its DIVU is guarded (dv==0 branch, no fault path in generated code). The pair shares 0x00FF099A: 01A718 writes it, 01ABB2 reads it — both first sighted @11831 in DC's death window, so both are capsuled as the crash's neighborhood, not a proven site. DD = DC config with ONE change, announced: the table extended 40 -> 48 extents with DC's late names — 0181D0, 01A718, 01ABB2, 01C438, 01E0AC, 025370, 028D9C, 028E18 — every extent computed from the dispatch table, verified sorted and non-overlapping against the table and MASK1-11. Double-check replayed from build-dc's own cache/flags (all eleven masks + gates + TABLE_MASK match the ledger). Cache diff vs DC: LABEL, NUMBER only. Purpose: "DD: POST-RACE NAMES, TABLE PLUS 8".
**rr3vita-dd.vpk: 2,926,790 B, MD5 210b442aa45863fcd8a35296aa5aa4e0** (build dir == your_files copy; verified by foreground ls+md5 twice).
Test: finish a race again and sail past the post-race window (11831, ~11840). If it dies, breadcrumb + dump as usual.

DD TEST RESULT — twelfth gameplay death, AFTER A SECOND COMPLETED RACE (breadcrumb-58, MD5 1eda9f9f8f4d78eed5dada72741eb735; dump psp2core-1792963631): Lola finished a race again — inputs end @11567 pad=0000 (hands off, same finish shape as DC), last play sample 11760, death in (11773, ~11780). The headline: the LAST capsule line is capsule tick=11773 addr=01A718 tried=18012 hit=18011 — the FIRST failed capsule run in the entire hunt (every prior build ended tried==hit; DC had 18,200+ all clean). 01A718 was first sighted @11772 and never completed a clean capsule; the file ends there, so the deficit is permanent, not a transient poll. 01ABB2 NEVER fired — the death came before its turn, so DC's @11831 pair splits: 01A718 is the crash's neighborhood, 01ABB2 still untested. Other new table names fired clean and repeatedly (0181D0, 01C438, 025370, 01E0AC, 028E18); 028D9C never fired (untested, same shelf as 0249AA in CZ). Session capsule histogram: 021486 x1904, 017C14 x1725, Killer C 0207EE x776 clean (the 900 gate still working), 02823A x113. Dump forensics not used (void on this firmware). Reading: 01A718 is now the prime suspect — either its capsule diverged (interpreter/native mismatch, native fallback crashed) or the interpreter died inside the capsule run. DE direction: desk-read 01A718's 5 call sites and its callees before deciding the next mask move.

DE (#109) BUILT, VERIFIED, DELIVERED — one-extent build. Desk findings first: func_01A718's 5 call sites are ALL direct recomp_call_func — 3x func_01ABB2 (returns to 01A798, 01A7EA, and one more) and 2x func_027B84 (returns to 01A7CE and one more). So 01A718's whole subtree is {01A718 body + loop, 01ABB2 leaf, 027B84 leaf}. func_027B84 (idx 644, [0x027B84,0x027BD8)) is a ZERO-call leaf (saves regs, reads 0x00FF7AE4/0x00FF7AE6 words) and a SHARED utility with 10+ call sites across parts 04/05/08/09/16 — which is why DD first sighted it early (@1193, mid-race) rather than post-race. Crucially it was UNMASKED: the only 0x027B84 hit in rr3_dispatch_mask.c was its dispatch-table entry. That reframes DD's death: in the fallback (native 01A718 after the capsule mismatch), its 01ABB2 calls were already capsuled (tabled) while its 027B84 calls ran BARE — so if the crasher lives in a callee, 027B84 is the one place it could hide. DE = DD config with ONE change, announced: the table extended 48 -> 49 extents with 027B84 only, verified sorted and non-overlapping. Double-check replayed from build-dc's cache (all eleven masks + gates + TABLE_MASK). Cache diff vs DD: LABEL, NUMBER only. Purpose: "DE: MASKING 027B84, 01A718'S BARE CALLEE".
**rr3vita-de.vpk: 2,926,775 B, MD5 c82228b14b307e90efaa1fc4dd2bebfe** (build dir == your_files copy; verified by foreground ls+md5 twice).
Test: finish a race and sail past the post-race window (11773, ~11780). Watch 027B84 capsules (it now capsules everywhere, including its race-time callers). If it dies at 01A718 again with another failed capsule, the crasher is 01A718's own native body — both its callees will have been capsuled in the fallback by then.

DE TEST RESULT — thirteenth gameplay death, MID-RACE (breadcrumb-59, MD5 2f5f3cbdd1ed620fb9a20e40420d793d; dump psp2core-1792968229; Lola: "died mid race this time"): last session in the file (boot-start line 41388); DE identity PROVEN by 31 capsule lines for 027B84 — DD had none. Last play sample tick 9300, tail tick 9301; last input tick 9288 pad=0010 (B, mid-combat) — hands still on the buttons, no finish, no hands-off tail. Capsules: final tried=11293 hit=11293 — ALL CLEAN; deficit scan over the whole session shows max deficit 1 across 159 lines, every one resolving on the next capsule line (transient polls per the standing rule) — NO failed capsule, unlike DD. 01A718: ZERO lines; 01ABB2: ZERO lines — the run never reached post-race flow, so DE's question (was 027B84 the hiding place?) is UNANSWERED. 027B84 itself: first call @3056, 31 capsule runs, all clean. Histogram: 021486 x1339, 017C14 x1113, Killer C 0207EE x305 clean (900 gate working), 02823A x70, 02207A x55, 01EB36 x44; 0258DC (DC-added table name) first sighted @7155, capsuled clean @7304/@7367. Last new first-sighting: 0255B0 @7921 (dispatch table line 1304, idx 618, extent [0x0255B0,0x0255E8)) — and NO new first-sighting in the 1,380 ticks between 7921 and the death, which breaks the herald pattern (DC/DD heralds appeared within ~10 ticks of death). Dump forensics not used (void on this firmware). Reading, branch (c): (a) gameplay variance — DC/DD's fights never hit this wall, crasher in already-known code or an uninstrumented path (fallthrough blind spot); (b) 0255B0 with a long fuse — weak herald given the gap; (c) the 027B84 mask itself changed behavior — unproven, but DE is the first build since DB to die mid-race and the only delta from DD is the 027B84 mask, while DC/DD both sailed past 9300. DF = DE config + table 49 -> 50 with 0255B0 only; if DF also dies mid-race, the next test is the 027B84 mask itself (a DD-config re-run to see if post-race is reachable again).

DF (#110) BUILT, VERIFIED, DELIVERED — one-extent build. DF = DE config + table 49 -> 50 with 0255B0 only ([0x0255B0,0x0255E8), idx 618, dispatch table line 1305; sorted insertion between the 025438 and 0258DC extents, mechanically verified sorted and non-overlapping across all 50 extents). Cache diff vs DE: LABEL, NUMBER only. Purpose: "DF: MASKING 0255B0, LAST NEW NAME".
**rr3vita-df.vpk: 2,926,770 B, MD5 edb30b0cf53772fab1736e1e45d988b6** (build dir == your_files copy; verified by foreground ls+md5 twice; purpose string verified in the raw ELF; BUILD stamp format + LABEL/NUMBER defines give BUILD DF #110 on screen).
Test: same track-1 fights. The signal is sailing past ~9301 (DE's death) — and ideally finishing a race again, which would re-open the post-race question DE never got to answer. If it dies mid-race again, breadcrumb + dump as usual, and the next test is the 027B84 mask itself.

DF TEST RESULT — fourteenth gameplay death, MID-RACE (breadcrumb-60, MD5 1435a0b813597292757bdb2adf922f89; dump psp2core-1792971919; Lola: "mid race once more"): last session in the file (boot-start line 45666); last play sample tick 5160, last capsule 5164 (017C14 clean, tried=8306 hit=8306), last input 5144 pad=0010 (B, mid-combat) — death in (~5165, ~5180), hands on buttons. Capsules: final tried=8306 hit=8306 — ALL CLEAN; deficit scan max 1 across 121 lines, every one resolving on the next capsule line (transients per the standing rule). 0255B0: ZERO lines — never fired in this run (DE first sighted it @7921; DF died @5165 before that), so DF's mask was never exercised and this session's log is indistinguishable from a DE session; DF identity rests on provenance (newest session, DF installed). 01A718: zero; 01ABB2: zero — post-race never reached, the second run in a row (DE, DF) that never got there. 027B84: 33 capsule runs, all clean. Histogram: 017C14 x670, 021486 x575, Killer C 0207EE x415 clean (900 gate working), 02207A x86, 02823A x64. Late new names, unlike DE: 02019C @4639, 021DAE @5052, 021A74 @5058, 01C1E8 @5077 — the last ~88 ticks before death, a weaker herald gap than DC/DD (~10) but the first late names since DD. Dump forensics not used (void on this firmware). Reading: the mask-correlation is now 2-and-2 — DC/DD (no 027B84 mask) both finished races; DE/DF (027B84 mask) both died mid-race. Gameplay variance can still explain it (different fights every run), but the correlation is now the strongest signal in the data, stronger than the weak 01C1E8 herald. DG direction (the pre-announced test): DD-config re-run — table 50 -> 48, removing BOTH the 027B84 and 0255B0 extents (0255B0 never fired in DF, so removing it loses nothing observable), all masks/gates exactly as DD, label DG #111. If DG finishes a race again, the 027B84 mask is implicated and comes out permanently (and the 01A718 question re-opens from a different angle); if DG also dies mid-race, H3 is dead and the hunt follows the new names (01C1E8, 021A74, 021DAE) in DH.

DG (#111, built + delivered 2026-10-25): DD-config re-run — table 50 -> 48 (removed {0x027B84,0x027BD8} and {0x0255B0,0x0255E8}, both confirmed absent from the mask table; the two file-wide matches for those addresses are the dispatch-table entries, which stay); cache diff vs DD: LABEL DD->DG, NUMBER 108->111 ONLY (all masks/gates identical, MASK3 gate 900, TABLE_MASK=1); purpose "DG: RETEST WITHOUT 027B84 MASK" verified in the raw ELF. rr3vita-dg.vpk 2,926,878 B, MD5 5368fb3b644e4ca77a380f48783f291a — verified twice on disk by me (build dir == delivered copy). Note: the table still contains {0x0249AA,0x024A4C}, an exact duplicate of MASK10's range — pre-existing in DD's own table, harmless (both mechanisms capsule the same function), left alone to keep the DD re-run exact; cleaning it out is a separate one-line change, not part of this test. AWAITING DG TEST (breadcrumb-61 expected): same track-1 fights; finishing a race implicates the 027B84 mask (it comes out permanently); a mid-race death exonerates it and the hunt follows 01C1E8/021A74/021DAE in DH.

DG TEST RESULT — fifteenth gameplay death, AFTER A THIRD COMPLETED RACE (breadcrumb-61, MD5 47ab2d86e93939cee073a4e58ce90a7e; dump psp2core-1792974405): DG identity by provenance + config signature (027B84 capsule lines: ZERO, unmasked as built — DE/DF had 31/33; first 027B84 call @1493 via the ring). Inputs end @11812 pad=0000 (hands off — third straight finish for an unmasked config: DC, DD, DG). Post-race metronome traffic (017C14 x1861, 021486 x2257, 0181D0) ran clean for 219 ticks after hands-off, then 01A718 was called @12031 and its first capsule run FAILED @12032: tried=16816 hit=16815 — the session's ONLY unresolved deficit (5,056 capsule lines; max deficit 1 elsewhere, every one transient). 01ABB2 never fired (third run in a row). The post-race gap to 01A718 is consistent across all three finishes (DC 209 ticks, DD 205, DG 219) — 01A718 is a fixed station in the post-race sequence, and it has now killed all three builds that reached it: native in DC (death in its window), capsuled in DD and DG (permanent mismatch on its first run, native fallback dies). READING: pre-registered branches (a) AND (c) both fire. (a) The 027B84-mask correlation is now 3-and-2 (unmasked DC/DD/DG all finished; masked DE/DF both died mid-race) — the mask STAYS OUT, implicated but not proven (gameplay variance still a live alternative). (c) 01A718 is twice-convicted at its first capsule run; the crasher lives in or under it, and the next step is the DESK READ of 01A718 before any new build: why its capsule diverges when every convicted demo killer capsuled clean, what its native body does after the fallback, and the roles of its callees (027B84 leaf — bare in both DD and DG fallbacks, but called bare all race long without incident; 01ABB2 — never yet reached). No DH until the desk read reports.

DH (#112) BUILT, VERIFIED, DELIVERED 2026-10-30 — forensics build, DG config unchanged (cache diff vs DG: LABEL, NUMBER only; purpose "DH: WHY 01A718 CAPSULE FAILS" + both new format strings verified in the raw ELF). Desk read of func_01A718 (generated/rr3_part16.c:7634, ~822 lines) first: a three-loop post-race results-table builder — loop 1 walks the pointer table at 0x00FF0440 (count [0x00FF043A]-2), loop 2 walks BACKWARD into the table at 0x00FF03B4, loop 3 backward into 0x00FF03F6; calls: 01ABB2 x3 ($01A794/$01A7E6/$01A864), 027B84 x2 ($01A7C8/$01A846); one DIVU at $01A80C with full overflow semantics; plain return at $01A86C. Capsule mechanics (src/rr3_dispatch_mask.c rr3_mask_run + engine-defer m68k_interp.h): success requires interp status OK AND exit_pc == the return address peeked at A[7]; failure restores g_cpu only (guest-RAM writes persist into the native fallback). Failure statuses: 1=UNIMPL (with g_m68ki_bad_pc/bad_op naming the instruction), 2=GUARD (50M-insn runaway), 3=BADADDR, 0=OK-but-exit-mismatch. Suspects ranked: (i) the DIVU — NO capsule in the whole hunt has ever executed one (01ABB2, the other DIVU owner, has never been capsuled; it never fired in DD/DG), so DIVU is untested interpreter territory; (ii) a loop spinning on post-race counts (GUARD); (iii) exit mismatch. 027B84's body is interpreter-safe (capsuled clean 31-33x in DE/DF). DH instrumentation (dispatch mask + main.c drain, plain stores only, no fiber IO): on any capsule failure, record addr/status/badpc/badop/exit/want/A7 -> breadcrumb line "maskfail tick=.. addr=.. st=.. badpc=.. badop=.. exit=.. want=.. sp=.."; around every native fallback after a failed capsule, record entered/returned -> "fallback tick=.. addr=.. state=1|2" (state stays 1 = died inside the fallback; a state=2 line = the fallback RETURNED and the crash is after 01A718). rr3vita-dh.vpk 2,926,865 B, MD5 c81df41f1f26e3b060865fe54b25a179. AWAITING DH TEST (breadcrumb-62 expected): same track-1 run, finish a race; the maskfail line names the capsule's blocker and the fallback line splits native-inside vs after-return.

DH TEST RESULT — sixteenth gameplay death, MID-RACE (breadcrumb-62, MD5 bcdf7bbac22f5040d78fbbddebab5630; dump psp2core-1793408993; Lola: "worst race yet in a while, I hit 2 cars. but crashed midrace"): last play tick 9740, last input @9744 pad=0010 (hands on, mid-combat) — death in (~9744, ~9760). THE FORENSICS NEVER FIRED: 3,119 capsule lines, ALL clean (final tried=hit=10783); zero unresolved deficits; ZERO maskfail lines; ZERO fallback lines; 01A718 never reached. No new first-sighting after tick 6275 (02378C) — a 3,400-tick herald silence. The final play PC 0141E4 sits inside func_014108, but that address was sampled 34 times through the race (a hot loop, first called @2657) — not a suspect. READING: this death belongs to the capsule-independent mid-race class (CS/CT/CV/CZ/DE/DF family): no mask, capsule or fallback involvement provable; DG-config finished 3 straight races while DE/DF/DH died mid-race at scattered ticks on heavier-combat runs — state/variance-driven, location still unknown (fallthrough blind spot and uninstrumented native paths remain the named unknowns). DH's 01A718 question stays OPEN, not answered either way.

DI (#113) BUILT, VERIFIED, DELIVERED 2026-10-30 — visibility build for the mid-race class. DI = DH exactly (DG mask config + DH maskfail/fallback forensics retained; cache diff vs DG: LABEL, NUMBER only) with ONE change in main.c's call-ring drain: the tail snapshot now runs every 30 frames (was 600) and prints the last 32 ring entries (was 8) plus the current guest PC — so whichever death shows up, the breadcrumb holds a last-calls window at most 30 ticks old; the post-race 01A718 forensics stay armed if the run finishes. Purpose "DI: LAST-CALLS TAIL EVERY 30 FRAMES" + new tail format + maskfail format all verified in the raw ELF. rr3vita-di.vpk 2,926,979 B, MD5 d1d4217f4ffb90fc33d64245946f5b9e. AWAITING DI TEST (breadcrumb-63 expected).

DI TEST RESULT — seventeenth gameplay death, MID-RACE (breadcrumb-63, MD5 a46b89f4f010cf8456e49e4055dc956e; dump psp2core-1793412528): last play tick 5740, last input @5726 pad=0010, death in (~5742, ~5760). All 1,886 capsules clean (one transient deficit @5735 resolved next line); zero maskfail, zero fallback; 01A718 never reached. THE TAILS WORKED — and the DF herald trio is BACK: last first-sightings 021DAE @5636, 021A74 @5642, 01C1E8 @5667, death ~75-105 ticks later. CROSS-SESSION TABLE for the trio (sessions DC..DI): DC finished — trio NEVER sighted; DD finished — NEVER; DG finished — only 021A74 @1680 (early, routine); DE died @9300 — only 021A74 @3243; DF died @5160 — 02019C @4639 + trio @5052/5058/5077; DH died @9740 — 021DAE+021A74 @5003/5008 (mid-race, run continued); DI died @5740 — trio @5636/5642/5667. SHARPEST SIGNAL: func_01C1E8 has been first-sighted in exactly TWO sessions ever (DF, DI) and BOTH died within ~110 ticks of the sighting; it has never appeared in a run that survived past it. Desk: 01C1E8 [0x01C1E8,0x01C3E8) 512B, 586 lines, called ONLY by func_01AF6A (one site), calls only func_01C3E8. 021A74 [0x021A74,0x021DAE) 826B leaf, 021DAE [0x021DAE,0x021E20) 114B — both called by func_02176E. Caveat standing: first-sighting may mark a rare game situation (herald) rather than the site itself — masking the trio tests both readings at once.

DJ (#114) BUILT, VERIFIED, DELIVERED 2026-10-30 — trio-mask build. DJ = DI exactly (DG masks/gates, DH maskfail/fallback forensics, DI 30-frame last-calls tails) with ONE change: the table extended 48 -> 51 extents with {0x01C1E8,0x01C3E8}, {0x021A74,0x021DAE}, {0x021DAE,0x021E20} — mechanically verified sorted/non-overlapping; 027B84 and 0255B0 remain OUT; cache diff vs DG: LABEL, NUMBER only. Purpose "DJ: MASKING DF TRIO 01C1E8 021A74 021DAE" + forensic formats verified in the raw ELF. rr3vita-dj.vpk 2,927,145 B, MD5 43b5ff259a1014ea176ebe24482280a8. AWAITING DJ TEST (breadcrumb-64 expected): if DJ sails past the ~5700 window (and ideally finishes), the trio is implicated as sites/subtree; if it dies in the window with the trio capsuled clean, they are heralds and the hunt moves to their parents (02176E, 01AF6A).

DJ TEST RESULT — FOURTH COMPLETED RACE, then the 01A718 death again (breadcrumb-64, MD5 64d5a768cd54f72bfa52ad86aa3b1a57; dump psp2core-1793415774; Lola: "finished a race, but crashed when it stopped fading"): last input @13070 pad=0000, last play 13300, call tick=13315 addr=01A718, capsule tick=13316 tried=17340 hit=17339 — THE FAILED 01A718 CAPSULE, fourth time (DC native, DD, DG, DJ capsuled). TRIO VERDICT: NONE — 01C1E8/021A74/021DAE were masked but NEVER CALLED in DJ's run (zero capsule lines); the finish neither confirms nor clears them (the triggering situation never arose). The masks stay in, harmless. FORENSICS GAP EXPLAINED: DJ printed ZERO maskfail/fallback lines despite the failed capsule — the failure record is written during the step and the crash follows inside the SAME step, before the front-end's between-steps poll can ever print it. The DH record mechanism cannot capture this death by construction.

DK (#115) BUILT, VERIFIED, DELIVERED 2026-10-30 — one-shot dump build. DK = DJ exactly (51-extent table incl. trio; cache diff vs DG: LABEL, NUMBER only) + the failure record now escapes on its own: the front-end pre-opens ux0:/data/rr3vita/maskfail.txt at boot (fd in g_mask_forensic_fd), and rr3_mask_note_fail, on the FIRST capsule failure of the session, formats the full record (tick/addr/status/badpc/badop/exit/want/sp) and issues exactly ONE sceIoWrite (kernel call, not stdio; snprintf only). In the whole hunt the only real capsule failure is 01A718's, so the write fires exactly at the answer. Purpose "DK: ONE-SHOT MASKFAIL FILE DUMP"; MASKFAIL-ONESHOT format + maskfail.txt path verified in the raw ELF. rr3vita-dk.vpk 2,927,233 B, MD5 e7946219fb7528ab334183011913f269. AWAITING DK TEST (breadcrumb-65 + maskfail.txt expected): the maskfail.txt line reads the 01A718 verdict — st=1 + badpc at/near the DIVU ($01A80C or inside 01ABB2) convicts the interpreter's DIVU; st=2 = a loop spins (GUARD); st=0 + exit!=want = exit mismatch; st=3 = bad address. If maskfail.txt comes back EMPTY, the one-shot write itself is the suspect (fiber IO intolerance) and the fallback plan is a delayed-fallback build.

DK TEST RESULT — FIFTH COMPLETED RACE, same 01A718 death (breadcrumb-65, MD5 ec19be9b1d154512cd928edd6bc25172; dump psp2core-1793421439): last input @15446 pad=0000, last play 15640, call tick=15658 addr=01A718, capsule tick=15659 tried=18040 hit=18039 — the failed capsule, fifth time. Trio never fired again (zero lines, DJ+DK both). Everything now hinges on ux0:/data/rr3vita/maskfail.txt (DK's one-shot dump) — ASKED Lola to upload it 2026-10-30; DL's design waits on its single line (st=1+badpc≈DIVU site / st=2 loop / st=0 exit mismatch / st=3 bad address; empty file = the write path itself is the suspect).

DK MASKFAIL.TXT RESULT (2026-10-30, Lola): THE FILE IS BLANK (0 bytes — created by DK's boot open, nothing in it; the chat client wouldn't even attach it). Source audit: rr3_mask_run calls rr3_mask_note_fail on BOTH failure exits, and the DK dump code + maskfail.txt path are verifiably in the shipped ELF — so the record existed in memory, but neither escape channel delivered it: the between-steps poll prints only if the process survives the step, and the one-shot sceIoWrite's bytes never became durable before the crash. Conclusion: stop trying to extract the record from inside the fatal step.

DL (#116) BUILT, VERIFIED, DELIVERED 2026-10-30 — survivability probe. DL = DK exactly (51 extents, one-shot dump kept, cache diff vs DG: LABEL, NUMBER only) + ONE semantic change: when func_01A718's capsule fails with a valid return address, rr3_mask_run restores the checkpoint, then emulates a BARE RETURN (A7 += 4, PC = expected return, return 1) instead of running the native fallback — new globals g_mask_skip_count/g_mask_skip_addr + a "maskskip" poll line in main.c. The results table stays unbuilt (downstream may misbehave); only 01A718 gets the skip. Purpose "DL: SKIP 01A718 ON CAPSULE FAILURE"; purpose + maskskip + MASKFAIL-ONESHOT formats verified in the raw ELF. rr3vita-dl.vpk 2,927,499 B, MD5 8daf89a49a15ca9748ef6397c05b76be. AWAITING DL TEST (breadcrumb-66 expected): if the poll prints maskfail (st/badpc name the capsule-failure cause at last) AND maskskip, and the process sails past the fade, 01A718's native subtree is convicted and the hunt bisects INSIDE it (01ABB2 / 027B84 next). If DL still dies at the same tick, the poison precedes execution — the failed capsule's persisted guest-RAM writes become the prime suspect.

DL TEST RESULT — MID-RACE DEATH, AND THE TRIO VERDICT AT LAST (breadcrumb-66, MD5 bdc28105c72ecd3cb30ee02668963741; dump psp2core-1793424515): died mid-race in (~3906, ~3920), hands on (last input @3906 pad=0038); 01A718 never reached (skip untested, stays armed). The trio FIRED this run — 021DAE first @3787 (capsule), 021A74 @3791, 01C1E8 @3819 (capsule; last capsule of the session @3904, tried=hit=4708) — ALL 1,182 capsules CLEAN, zero deficits, no maskfail, and the process died ~90-110 ticks after 01C1E8's first sighting, the same window as DF (~88) and DI (~100). VERDICT: THE TRIO ARE HERALDS, NOT THE SITE — capsuled clean, death in the window anyway. The killer correlates with the game situation that summons the trio, not with their execution. Desk: parents are routine early traffic (02176E sighted @1339, called from the hot loop func_014108; 01AF6A @1154, called from func_014434 at 3 sites) — the trio children are the parents' RARE branches.

DM (#117) BUILT, VERIFIED, DELIVERED 2026-10-30 — parents build. DM = DL exactly (51 extents incl. trio, 01A718 skip armed, one-shot dump; cache diff vs DG: LABEL, NUMBER only) + table 51 -> 53 with the trio's parents {0x01AF6A,0x01B414} and {0x02176E,0x021936} (mechanically verified sorted/non-overlapping). Purpose "DM: MASKING TRIO PARENTS 02176E 01AF6A" + maskskip format verified in the raw ELF. rr3vita-dm.vpk 2,927,600 B, MD5 8963d85c93b0cd49fadaa72b786d9841. AWAITING DM TEST (breadcrumb-67 expected): if DM sails past the trio window (~3800-3950 in DL's pacing) the killer is inside a parent's subtree; if it dies in the window with parents capsuled clean, the hunt widens to the situation's other participants (the parents' callers 014108/014434 and their other children).

DM TEST RESULT — SIXTH COMPLETED RACE; 01A718 AGAIN; TWO FORENSIC THEORIES KILLED AT THE DESK (breadcrumb-67, MD5 5b7b7ad60ce41c8676883743bced5305; dump psp2core-1793427407): last input @12119 pad=0000, last play 12340, call @12344, capsule @12345 tried=17860 hit=17859 — 01A718's capsule failed a SIXTH time and the process died at the fade. Parents capsuled HEAVILY and clean all run (02176E 386 lines, 01AF6A 2,023 lines; trio never fired separately — inside the parents' capsules or uncalled); the DL-class mid-race window was passed. NO maskfail line, NO maskskip line — THE SKIP DID NOT SAVE IT (either the failure is the st=99 no-return branch the skip doesn't cover, or the caller's continuation dies on the unbuilt table within the fatal step). Dump forensics ATTEMPTED PROPERLY on this dump (gunzip -> ELF CORE, 51 phdrs): the dump does NOT contain the main module's image or its BSS page — symbol-located reads of g_mask_fail_* are NOT IN DUMP and even the static "MASKFAIL-ONESHOT" format string is absent. Verdict: this firmware's cores exclude the module; the fail record is NOT recoverable from dumps. Split-globals theory CHECKED AND DEAD: nm on DM's ELF shows exactly ONE copy each of g_mask_fail_count / g_mask_forensic_fd / g_mask_skip_count and a single dispatch-mask object — the instrumentation wiring is sound; the record's non-delivery is a fatal-step timing fact, not a wiring bug. Six deaths, both engines, same function: suspicion shifts to 01A718's split-stack/RTE return contract vs the capsule's plain-RTS exit model, and to the caller.

DN (#118) BUILT, VERIFIED, DELIVERED 2026-10-30 — mask the (only) caller. Desk: func_01A718 has exactly ONE call site, in func_185E3E (generated/rr3_part30.c:2012). func_185E3E [0x185E3E,0x186192) 852 B, routine traffic (first called ~tick 1025, 20 mentions in breadcrumb-67) that reaches 01A718 only on its post-race branch. DN = DM exactly (53 extents incl. parents + trio, skip armed, one-shot dump; cache diff vs DG: LABEL, NUMBER only) + table 53 -> 54 with {0x185E3E,0x186192} (verified sorted). Inside 185E3E's capsule, 01A718 is emulated as an internal call — a genuinely different execution configuration from both native and its own capsule. Purpose "DN: MASKING 01A718 CALLER 185E3E" verified in the raw ELF. rr3vita-dn.vpk 2,927,726 B, MD5 1c2e2f9c5725aa66d75f06f3d74d18d3. AWAITING DN TEST (breadcrumb-68 expected): a finish that sails past the fade convicts the 01A718 boundary itself (its own capsule/native contract) and points at running the whole post-race sequence interpreted; an 185E3E capsule failure is fresh data of a new kind; an early/menu death would indict 185E3E's capsule in routine contexts (unlikely — the parents' capsules at similar weight ran clean).

DN TEST RESULT — DIED AT RACE START, AND THE ONE-SHOT FINALLY TALKED (breadcrumb-68, MD5 586a57c7bb835f7f5c08b9b9638c02f9; maskfail.txt MD5 5644e8c7e02925fb275997ecec49680b, 105 B; dump psp2core-1793471045): session only 78 lines; died in (~1081, ~1100) as a race started (Lola's report). ⭐ maskfail.txt HAS CONTENT: "MASKFAIL-ONESHOT tick=1085 addr=185E3E st=99 badpc=000000 badop=0000 exit=000000 want=000000 sp=00FFFFF6" — 185E3E's first masked dispatch hit mask_run's PRE-CHECK: the long at [A7] was 0 (want=0) -> status 99, interpreter never ran. Desk confirmation: mask_run's st=99 is only (peek==0 || peek odd); recomp_push_return DOES write returns to guest [A7] for true JSR calls, so 185E3E at race start is entered by a no-return frame path — the capsule structurally cannot take it; masking it was a category error. The native fallback should then have been a no-op equivalent... but the process died in the SAME step, right after the step's ONE fiber file-IO: the sceIoWrite inside note_fail. By the AJ rule (the game fiber tolerates at most one file/stdio call; the dispatch path must do ZERO), the one-shot write is FIBER POISON and is the best explanation for DN's death (185E3E native is proven harmless across DM's full run). It also re-frames DK/DM: in those builds the write attempt sat inside 01A718's failure step too (file came back blank — the fiber died around the write, before durability). The clean DD/DG/DJ story stands underneath: interp fails on 01A718 -> native fallback -> native death (DC-proven).

DO (#119) BUILT, VERIFIED, DELIVERED 2026-10-31 — de-poison + on-screen record. DO = DM exactly (53 extents: trio + parents in, 185E3E extent REMOVED, skip armed; cache diff vs DG: LABEL, NUMBER only) with TWO source changes: (1) the one-shot sceIoWrite block is REMOVED from rr3_mask_note_fail (pure stores only — the one-shot experiment is closed with exactly one record harvested); (2) draw_purpose_stamp is DYNAMIC: once g_mask_fail_count > 0 the bottom-left stamp switches to "FAIL %06X ST%u PC%06X OP%04X X%06X W%06X" (addr/status/badpc/badop/exit/want) drawn every presented frame by the main thread — the record reaches Lola's EYES even if the process dies before any poll. Purpose "DO: FAIL STAMP ON SCREEN NO FIBER IO" + FAIL format verified in the raw ELF. rr3vita-do.vpk 2,927,376 B, MD5 71344ef0ed95ea6727857b90a38fba1e. AWAITING DO TEST (breadcrumb-69 expected): watch the STAMP at the fade — if a FAIL line appears with ST=1 and badpc ~$01A80C the interpreter's DIVU is convicted at last; ST=2 = the loops spin; ST=0 with exit!=want = exit-contract mismatch; if DO sails (skip engaged cleanly with the poison gone) the post-race wall is behind us.

DO TEST RESULT (breadcrumb-69, MD5 71a0cb1384e3b15c7a3d71a1e613ebbb; dump psp2core-1793474281): died at the fade AGAIN — call tick=14927 addr=01A718, capsule tick=14928 tried=19303 hit=19302 (FAILED), then NOTHING: no maskfail poll, no maskskip poll, no further play lines. The record stayed trapped a sixth time: death follows inside the same step, before the front-end poll AND (apparently) before any post-failure frame presented the FAIL stamp. Two live explanations: (i) st=99 — the DL skip never engaged (its code path only covers interp-run failures; the pre-check returns 0 before the skip block) → native fallback → DC-style native death in-step; (ii) skip engaged but 185E3E's continuation died in-step on the unbuilt results. DO's own lesson: every out-of-band channel (poll, one-shot file, on-screen stamp) still depends on the process surviving the fatal step. The only channel left is to NOT DIE.

DP (#120) BUILT, VERIFIED, DELIVERED 2026-10-31 — FREEZE, DON'T DIE. DP = DO exactly (53 extents, cache diff vs DO = LABEL+NUMBER only) + a freeze path: on ANY 01A718 capsule failure (interp-failure arm: DL skip bookkeeping + g_mask_freeze=1; st=99 arm: g_mask_freeze=1 + A7+=4 + PC=0x1860C8, the sole call site's pushed return), every later recomp_dispatch_once/recomp_call_func returns immediately (the step unwinds as no-ops), and the phase-3 loop stops calling genesis_sim_step while continuing to render — the dynamic FAIL stamp stays on screen indefinitely and a one-time "maskfreeze" line (full record) lands in the breadcrumb from the front-end thread. Verified in the raw ELF: purpose "DP: FREEZE ON 01A718 FAIL SHOW RECORD", maskfreeze + FAIL formats, g_mask_freeze symbol, table 53 extents / no 185E3E. rr3vita-dp.vpk 2,927,584 B, MD5 ccfa0050e4c007316e85cb1450c29ed3. AWAITING DP TEST (breadcrumb-70 expected): expected behavior at the fade is a FREEZE, not a crash — screen holds with "FAIL 01A718 ST.. PC...... OP.... X...... W......" bottom-left; Lola reads/screenshots it at leisure; the breadcrumb gains the maskfreeze line. Reading: ST=99 W=000000 → entry-path mystery (peek empty despite push_return at the known call site); ST=1 PC≈01A80C → interp DIVU convicted; ST=2 → loop spin; ST=0 X≠W → exit-contract mismatch. If DP instead sails past the fade, the failure was data-dependent and this run's capsule succeeded. (Build note: DP's make initially looked like "Error 2" — it was the 115 s chunk timer killing make on the big generated parts under parallel load, not a code error; a single full make exited 0.)

DP TEST RESULT (breadcrumb-70, MD5 3825f043462971aeac1d733a1d70234a; dump psp2core-1793476925-0x00001e2783): died MID-RACE at tick ~3711 (Lola: "mid race") — the OTHER class. The freeze never armed (it only arms on a 01A718 failure; the race never finished). Session evidence: 4,149 capsules, ZERO failures (g_mask_fail_count never moved — no maskfail/maskskip/maskfreeze/fallback lines at all), no trio sightings, no dispatch misses, last tail tick=3691 pc=01467E all routine, last calls 021486/02823A/01AF6A/02266E/02176E/03AF74. ⭐ LOG-READING CORRECTION: the dozens of "tried=hit+1" capsule polls throughout DP's racing are a POLL PIPELINING ARTIFACT, not failures — during racing the poll persistently lags hit by exactly 1 (menus show equality; DO showed the same during racing); with fail_count at zero they cannot be failures. Do not read tried≠hit as a failure ever again; the fail POLL is the only failure evidence. ⭐ DUMP WORK (new channel, partial): the psp2core is gzip; decompressed it parses as ELF core with named NOTE segments. THREAD_REG_INFO holds ONE SceArmCpuRegisters per thread (+8..+79; CPSR at +72): ALL THREE threads (main, rr3_sim, dialog worker) show KERNEL PCs (0xe000xxxx) — no user fault PC is stored there. MODULE_INFO gives rr3vita segments: text [0x8102d000, +0x32b640), data [0x81400000, +0xcbc13c); ELF link base is 0x81000000, so runtime slide = +0x2d000. The module DATA segment (BSS: g_cpu, call ring, fail globals) is NOT in the core (LOADs cover text pieces, stacks, the 16 MB heap at 0x8a101000, kernel pages) — the DH-era exclusion verdict stands for BSS. PROCESS_INFO word2 == the hex in the dump filename (DN 0x0A26E1, DO 0x4D258B, DP 0x1E2783 — all < 0x800000; DP/DN land in guest-ROM range, DO in the Genesis unmapped gap): semantics UNPROVEN, but the shape fits "a guest-space address was involved in the fault" (raw guest address dereferenced/jumped to as a host pointer is the standing candidate bug shape). TTY_INFO empty; EVENT_LOG no use. rr3_sim's user registers at dump: r8=0x601, r9=0x0003A50E (inside killer E's extent +4), r12=0xB5D — suggestive, NOT a verdict.

DQ (#121) — two changes on DP: (1) freeze-on-ANY capsule failure (DL/DP skip+freeze generalized from 01A718-only to every masked function, both failure arms; st=99 non-01A718 keeps its one native fallback but freezes if it returns); (2) tail snapshot EVERY FRAME (was every 30) — DP died ≤20 ticks after the last tail; DQ shrinks the blind window to <1 tick so the guest PC + last 32 calls at any mid-race death land in the breadcrumb. Purpose "DQ: FREEZE ANY FAIL + TAIL EVERY FRAME". If DQ dies mid-race with tails current to the death tick, the last tail's pc/ring IS the crime scene; compare its pc against the new dump's filename hex to test the guest-address theory.

DQ (#121) BUILT, VERIFIED, DELIVERED 2026-10-31: DP + freeze-on-ANY capsule failure (both arms generalized; verified purpose/maskfreeze strings + g_mask_freeze in the raw ELF, cache diff vs DP = LABEL+NUMBER only, table 53 extents / no 185E3E) + tail snapshot every frame. rr3vita-dq.vpk 2,928,266 B, MD5 1b69e40ed46363d78c830cf469606d20. AWAITING DQ TEST (breadcrumb-71 expected).

DQ TEST RESULT (breadcrumb-71, MD5 9eb97fd48c582a7753fe5fa95dadf506; dump psp2core-1793482164-0x0000143ced) — ⭐ THE WALL HAS A ROOM. DQ ran a full race and died at the fade; the per-frame tails caught the death in the act: call tick=12666 addr=01A718 → capsule tried at 12667 → FINAL tail tick=12667 **pc=027B84**. The guest was inside func_027B84 (84 B: [0x027B84,0x027BD8)) — executing under the INTERP as part of 01A718's capsule run — when the process died; the capsule never returned (no fail, no skip, no freeze record; the death is INSIDE rr3_mask_run's interp call). pc=027B84 appears exactly ONCE in the whole session (027B84's only live entry here is via 01A718; its many static call sites are cold in this flow). Desk read of func_027B84 (generated/rr3_part27.c:2148): a trivial helper — saves A0/D2/D1, reads two RAM counters (0xFF7AE4/0xFF7AE6), shifts them, table-reads at A0=0xFF7AA4 indexed by them (index masked to 0x1F only AFTER the reads), adds into D0, writes back, increments + masks the counters, restores, returns via the g_rte_pending split-stack convention. Nothing in it justifies a host death on its face — the poison is in HOW the two engines execute this frame (native DC died at the same wall), not in the guest semantics. Dump cross-check: PROCESS_INFO hex = 0x143CED (still guest-range-shaped, semantics still unproven); rr3_sim regs again show the wait-point context (r9 = 0x3A4F6 this time vs DP's 0x3A50E — the E/F-parent neighborhood both times); the ROM image is NOT in the core (file-backed, excluded like module text), so the guest-RAM heap hunt is closed. ⭐ REFRAME: six builds of "capsule fails → fallback dies" is DEAD — the capsule run itself is the fatal act for 01A718's frame. The freeze/skip family cannot fix a death inside the instrumented run.

DR (#122) — AMPUTATE THE WALL: rr3_mask_run returns a bare return for 01A718 BEFORE any execution (peeked return if plausible, else the sole call site's 0x1860C8); neither capsule nor native body ever runs; the rest of DQ (freeze-any, tail-every-frame, 53 extents) is unchanged. If DR sails past the fade, the wall is bypassed and 01A718/027B84 becomes an offline study problem (bench-repro the interp on this frame) instead of a blocker; if the game trips on the unbuilt results table, the trip site names the table's consumer. DR (#122) as built: rr3vita-dr.vpk 2,928,040 B, MD5 70962a9b50cba995991451ca082124cd; cache diff vs DQ = LABEL+NUMBER only; binary verified in the raw ELF (purpose + g_mask_freeze + g_mask_skip_count symbols). TEST PENDING (breadcrumb-72).

REPO CATCH-UP PUSHED 2026-10-26: GlitchedDot/rr3-vita main, commit 7e270ba8b5b98bc13bae835c98b9172c84f7b703 ("Gameplay crash hunt catch-up: table-driven mask + call ring, MASK6-MASK11, DG config (builds DA-DG)"). Pushed paths: CMakeLists.txt, src/rr3_dispatch_mask.c, src/main.c, src/main_mainthread.c, README.md, docs/rr3-crash-hunt-2026-10-19.md — the DA–DG-era changes since the Oct 21 push (666466c). Pre-push check: all six remote files at 666466c matched the Oct 21 staging byte-for-byte (no divergence). Post-push check: remote head is the new commit and remote src/rr3_dispatch_mask.c matches the live file (MD5 7ececa981f9a52ab41fd8fb54b64b0fc, table mask present). ROM, crash dumps, VPKs, build dirs, and generated/ excluded, as always.

## DR TEST RESULT — 2026-11-08 (breadcrumb-72, MD5 b9bdea54a9ec67030ae9385cc8f44d5a; dump psp2core-1794175685-0x00004624eb)
- Session (last segment of the cumulative breadcrumb; 18,127 lines): full race, death at the post-race zoom-out/fade finish (Lola's report, in words). Last tail tick=12094 pc=1860C8; NO tail/play/capsule line after it — the process died inside the sim step that follows the 12094 tail.
- ⭐ THE AMPUTATION ENGAGED, EXACTLY ONCE: `capsule tick=12094 addr=01A718 tried=18539 hit=18539`, `maskskip tick=12094 addr=01A718 n=1`, first-sighting calls at tick=12093: 01A718 → 014E5C → 014BDA. The step containing the skip COMPLETED (capsule/maskskip/call drains + tail all printed after it, from the front-end, post-step). PC at that tail = 1860C8 = the exact pushed return the DR skip set — the bare return landed correctly.
- ZERO capsule failures in the entire session: no maskfail, no maskfreeze; final maskstat tried=hit (17,965 at frames=10800; 18,539 at the final capsule). tried==hit at the end, not the racing poll-lag artifact.
- ⭐⭐ VERDICT: **func_01A718 IS CLEARED AS THE FADE-DEATH SITE.** Its execution is not necessary for the death: never executed (no capsule, no native body, no 027B84 inside it), the game still died at the same visual moment, within one step after returning to its caller. The six-build "01A718 capsule" correlation was a HERALD: 01A718 is the last *masked dispatch* before the fatal sequence, so every dispatch-watching instrument saw it last (same failure shape as the DF herald trio). DQ's "death inside the capsule at pc=027B84" stands as a true observation of THAT build — in DQ the capsule was where the step happened to be executing when the shared poison fired — but DR proves the poison is not in 01A718/027B84 execution. DC (native death) + DQ (interp death) + DR (no execution, death) = the fatal act is elsewhere in the post-race sequence executed from guest PC 1860C8 onward, in the step after the 01A718 call.
- Anatomy of the surviving candidate region (desk, generated/rr3_part30.c, caller = func_185E3E): after the JSR at $1860C2 → 01A718 returns at $1860C8, execution continues NATIVELY through 185E3E's body: direct calls (recomp_call_func — they BYPASS the dispatch mask/capsule entirely) to 014E5C (→014BDA), 029190, 029678, 017996, 013088, 002260, 029678, 002220, 00AC48, 002260, 002220, plus inline wait loops ($18616E/$186184, FF0226 yields) and a tail call to 185CE4 at $18618C. NONE of those direct-call targets are in the 53-extent table mask. The dispatch-mask bisect structurally cannot see this region — a new instrument is needed, starting with a survivable gate.
- Dump: filename hex 0x4624EB recorded only; PROCESS_INFO semantics remain UNPROVEN and dump forensics stays a dead end — no theory built on it.

## DS (#123) — FREEZE AFTER THE 01A718 SKIP (built on DR's verdict)
- Question: does the death live in the step starting at pc=1860C8 (or later)? Gate the sim BETWEEN steps, at the exact moment DR survived to.
- Config: DR identical (cache diff vs build-dr = LABEL DS + NUMBER 123 only; all 11 masks + table mask byte-identical). Source changes in src/main.c ONLY (rr3_dispatch_mask.c untouched, DR amputation kept): (1) purpose text "DS: FREEZE AFTER 01A718 SKIP"; (2) the maskskip poll now, when skip addr == 01A718 and tick >= 10000 (fade window gate; earlier spurious calls can't spoil the test) and freeze not yet set, sets g_mask_freeze and writes `dsfreeze tick=%u pc=%06X addr=%06X` — the main loop already refuses to step while frozen and keeps rendering; (3) the on-screen stamp switches to "DS FROZEN - HALTED AFTER 01A718 SKIP" when frozen with no capsule failure, and the DP maskfreeze breadcrumb print is suppressed for no-failure freezes (no zero-record noise).
- Read: DS freezes and STAYS ALIVE at the fade (frozen frame, stamp visible, dsfreeze line in breadcrumb) => death is in the 1860C8 step or later; DT then frame-advances from the gate. DS still crashes => the step-boundary reading is wrong and the poison acts outside sim stepping (async thread) — reopen that layer. Validity proof required: dsfreeze line present + Lola seeing the frozen frame herself (never from code alone).
- DS as built: **rr3vita-ds.vpk, 2,927,690 B, MD5 7d139b1a74e2d31010f120429bbd5bbf** (build-ds == your_files copy; zip integrity OK, 4 entries). Verified in the raw ELF (velf): purpose "DS: FREEZE AFTER 01A718 SKIP", stamp "DS FROZEN - HALTED AFTER 01A718 SKIP", format strings `dsfreeze tick=%u pc=%06X addr=%06X` + `maskskip…`, BUILD stamp + DS label + #%d, symbols g_mask_freeze / g_mask_skip_count / g_mask_fail_count. Cache diff vs build-dr = LABEL+NUMBER only. DR-era sources snapshotted into build-dr/ (*.dr-source) before the main.c edits. DELIVERED 2026-11-08. TEST PENDING (breadcrumb-73 expected): success shape = freeze at the fade, app alive, stamp reads DS FROZEN, dsfreeze line present.

## DS TEST RESULT — 2026-11-08 (breadcrumb-73, MD5 830e0bd8112fb8b6685dd226ff39aabc; dump psp2core-1794180060-0x000372232f; Lola's report in words: "crashed mid race")
- Session (last segment, 12,295 lines): raced from tick 920, died MID-RACE — last play tick=8540, last tail tick=8544 pc=01467E (the racing dispatcher park), then nothing. The fade never began (post-race sequence starts ~tick 11700 in DR); func_01A718 was never called, no maskskip, so the DS freeze gate never had a chance to engage. **The fade question DS was built to answer remains OPEN — this test is inconclusive for it, in both directions.**
- This is crash class 2 (capsule-independent mid-race death), same class as DP (tick ~3711) and the CT/CU-era deaths. Capsules: 3,507 runs, max tried−hit delta = 1 (the known poll pipelining artifact, hit always catches up), ZERO maskfail/maskfreeze. DS's own changes cannot be implicated: every DS code path engages only on an 01A718 skip, which never fired; during racing DS ≡ DR.
- One new data point, correlation only, NOT a conviction: func_021BAE was first sighted at tick=8455 and joined the capsule metronome (clean runs at 8456, 8462, 8468, …), ~89 ticks before the death — a new regime starting shortly before a class-2 death, reminiscent of the 02823A metronome preceding CT's death. Its capsule runs are clean, so if it is involved, it is via native/direct paths, not its capsule. Recorded as a watch item for future class-2 desk work; no build is being aimed at it yet.
- Dump filename hex 0x372232F recorded only; dump forensics remains a dead end, no theory built on it.

## DT (#124) — DS RETEST, UNCHANGED EXPERIMENT + CENTERED BETA ICON
- Rationale: the dsfreeze gate is built, verified in the ELF, and still untested — the fade is reached by most runs that survive class 2 (DQ, DR both got there). DT re-runs the identical experiment; no new variable enters the crash hunt.
- Config: DS identical (cache diff vs build-ds = LABEL DT + NUMBER 124 only). Source changes in src/main.c ONLY, all cosmetic naming: purpose "DT: FREEZE AFTER 01A718 SKIP", frozen stamp "DT FROZEN - HALTED AFTER 01A718 SKIP", breadcrumb freeze line renamed dsfreeze -> dtfreeze. rr3_dispatch_mask.c untouched (01A718 amputation kept).
- Icon: DT is the first build carrying the centered-BETA icon Lola approved 2026-11-08 (tree sce_sys/icon0.png, palette PNG); DS and earlier shipped the corner version.
- Read: same as DS — freeze + "DT FROZEN" stamp + dtfreeze line at the fade convicts the 1860C8 step-or-later; a crash at the fade reopens the async layer; another mid-race death is class 2 again and the gate stays queued.
- DT as built: **rr3vita-dt.vpk, 2,927,199 B, MD5 03d3a1c4665aaf968a928c897fad415f** (build-dt == your_files copy; zip OK, 4 entries). Verified in the raw ELF: purpose "DT: FREEZE AFTER 01A718 SKIP", stamp "DT FROZEN - HALTED AFTER 01A718 SKIP", format `dtfreeze tick=%u pc=%06X addr=%06X`, BUILD stamp + DT label. Icon inside the package is byte-identical to the centered-BETA tree icon (MD5 a1e00d2b5d28e0fce61af26779860f0a) — first build to ship it. DS-era source snapshotted to build-ds/main.c.ds-source. DELIVERED 2026-11-08. TEST PENDING (breadcrumb-74 expected).

## DT TEST RESULT — 2026-11-08 (breadcrumb-74, MD5 227b15a35141afdcf524cd780e6ff36e; dump psp2core-1794181722-0x00017623e3)
- Session (last segment, 4,566 lines): died MID-RACE AGAIN — last play tick=3700, last tail tick=3702 pc=01467E, then nothing. Near-repeat of DP's 3711 death. Capsule totals tried=hit=4137 at the last poll, ZERO failures; 01A718 never called; the freeze gate never engaged. **Fade question still open; second consecutive class-2 interception of the gate.**
- Late first-sightings func_02266E (tick 3691, 11 ticks before death) and func_03AF74 (tick 3696, 6 ticks before) also appeared in DP's last calls. Desk verdict (same day): BOTH are already inside the 53-extent table mask (02266E = extent [0x2266E,0x226C0); 03AF74 = [0x03AF74,0x03AFBE); table verified sorted), so both already execute via capsule on dispatch AND direct-call paths — and the session's tried=hit balance says those runs SUCCEEDED. They are HERALDS (rare game events preceding the fatal step), exonerated as execution sites. NOTE on method: breadcrumb "capsule" lines are periodic polls (last_addr sample), not per-run logs — tried/hit totals are the ground truth; per-run absence from the poll lines proves nothing.
- Class-2 standing picture (unchanged since DA/CV): the killer is outside call-entry instrumentation — native execution inside a hot function body entered via non-dispatch paths (fall-through/direct branches); the dynamic-call hub at $020556 inside func_0203E8 remains the named neighborhood (DT's tick-3698 tail parks at pc=020558, that call's return point). No cheap instrument exists for it; the entry-surgery design (capsule check at 0203E8's fall-through entry) is the known hard option, held in reserve.

## DU (#125) — FREEZE-GATE RETEST, THIRD ATTEMPT
- DT experiment unchanged (config cache diff vs build-dt = LABEL DU + NUMBER 125 only; main.c changes are naming only: purpose "DU: FREEZE AFTER 01A718 SKIP", stamp "DU FROZEN - HALTED AFTER 01A718 SKIP", freeze line dufreeze). Centered-BETA icon carries over.
- Escalation rule set in advance: if class 2 kills DU too (three straight gate attempts intercepted), the next build pivots to a dedicated class-2 instrument (0203E8-entry design review first) instead of a fourth retest.
- DU as built: **rr3vita-du.vpk, 2,927,242 B, MD5 d3cd90904b9029d87adb09c4d7dbb207** (build-du == your_files copy; zip OK, 4 entries; icon inside byte-identical to the centered-BETA tree icon). Verified in the raw ELF: purpose "DU: FREEZE AFTER 01A718 SKIP", stamp "DU FROZEN - HALTED AFTER 01A718 SKIP", format `dufreeze tick=%u pc=%06X addr=%06X`, DU label. (Build note: the first make was interrupted at 37% by a runtime restart; resumed incrementally to 100% — objects and the final VPK verified on disk as usual.) DELIVERED 2026-11-08. TEST PENDING (breadcrumb-75 expected).

## DU TEST RESULT — 2026-11-08 (breadcrumb-75, MD5 1262d771da80e1ef2d55d56eed0aafae) ⭐⭐ THE GATE WORKED
- Session (last segment, 19,470 lines): full race, finished, post-race sequence reached. At tick=11623 func_01A718 was called (first sighting); the skip step completed: `capsule tick=11624 addr=01A718 tried=17458 hit=17458`, `maskskip tick=11624 addr=01A718 n=1`, then **`dufreeze tick=11624 pc=1860C8 addr=01A718`** — the freeze engaged exactly as designed, at exactly DR's death boundary.
- After the freeze: **1,957 tail lines, ALL at tick=11624 pc=1860C8** — the front-end kept rendering and polling while the sim stayed halted; the tick never advanced again; Lola closed the app herself (her report in words: "it worked and halted, nice!"). ZERO capsule failures in the session, no maskfail, no dump produced by this session's end.
- ⭐⭐ VERDICT: **the fade death is convicted to the sim step that starts at guest PC 1860C8** — the first step after the 01A718 skip step. Halted before that step, the process lives indefinitely; in DR, with nothing halted, that step killed it. The fatal act is inside that step's execution of func_185E3E's continuation from $1860C8: the nine direct calls (014E5C→014BDA, 029190, 029678, 017996, 013088, 002260, 002220, 00AC48), 185E3E's inline code and wait loops, and/or its tail call to 185CE4. First fully successful gate of the hunt — the crash is now a place we can stand and step in from, instead of a wall.

## DV (#126) — CAPSULE THE NINE: ARM THE LATE MASK AT THE SKIP, THEN LET THE STEP RUN
- Design: DU's freeze-on-skip is REMOVED. Instead, rr3_mask_run's 01A718 arm sets a plain-store flag g_late_armed; from that moment, rr3_table_masked ALSO matches a new late set — the nine post-1860C8 targets with dispatch-table extents: 014E5C [014E5C,014F32), 029190 [029190,029678), 029678 [029678,0297F8), 017996 [017996,017A6A), 013088 [013088,0131D6), 002260 [002260,0022A0), 002220 [002220,002260), 00AC48 [00AC48,00B186), 185CE4 [185CE4,185D58). rr3_table_masked is consulted on BOTH the dispatch path and the recomp_call_func direct path, so the direct calls are covered — the first instrument that can see this region. Racing is untouched (mask arms only after the skip, which happens once, in the fade sequence). Late capsule runs are counted (g_late_count/g_late_last_addr, plain stores in rr3_mask_run) and polled by the front-end as `latecaps tick addr n`; the arm itself is polled as `dvarm tick pc`.
- Failure semantics (existing DQ machinery): any late capsule failure bare-returns + freezes with the FAIL record on screen and a maskfreeze breadcrumb line — survivable and exact. If a late capsule SUCCEEDS where the native body would have died, the frame completes and the game proceeds past the fade — then the survivor set is bisected next. If DV still crashes at the fade with all nine clean, the nine are exonerated as a set and the poison is in 185E3E's inline code / wait-loop yields / deeper callees.
- Config: cache diff vs build-du = LABEL DV + NUMBER 126 only. Icon: centered BETA (unchanged).
- DV as built: **rr3vita-dv.vpk, 2,928,061 B, MD5 9d0718ca304f05d6e9340f0e35ea8c60** (build-dv == your_files copy; zip OK, 4 entries; icon byte-identical to centered-BETA tree icon). Verified in the raw ELF: purpose "DV: CAPSULE THE NINE AFTER 1860C8", stamp "DV FROZEN - HALTED", formats `dvarm tick=%u pc=%06X` and `latecaps tick=%u addr=%06X n=%u`, DV label, symbols g_late_armed / g_late_count / g_late_last_addr. Cache diff vs build-du = LABEL+NUMBER only. DU-era sources snapshotted into build-du/ (*.du-source). DELIVERED 2026-11-08. TEST PENDING (breadcrumb-76 expected).

## DV TEST RESULT — 2026-11-08 (breadcrumb-76, MD5 a4f6b8fdbd5ca1d168e32342e987b70d; Lola's photo: results screen, "Lucky Luc" taunt, 1st place $1000, TICK 20415) ⭐⭐⭐ SAILED PAST THE FADE
- Session (last segment, 30,558 lines): full race finished; maskskip tick=17808, dvarm tick=17808 — the late set armed on schedule; **the game proceeded through the fade, the results flow, and kept running to tick 22744** (Lola's photo at 20415 is the screen verdict; no dump accompanied this log; session ends in 185E3E's post-race wait loop, app closed by Lola). ZERO maskfail/maskfreeze; final maskstat tried=hit=86,220. First build since DQ (Oct 31) to survive the post-race fade.
- ⭐⭐⭐ VERDICT: **the nine are convicted as a set** — with them capsuled, the convicted step is harmless. The native body of at least one of the nine is the fade poison.
- Attribution from latecaps (462 polls, g_late_count final 66,427 — the set stays armed through post-race flow; most runs are the 0295C2/029678 results-rendering metronome): in the CRITICAL window — the post-arm remainder of the skip step, whose poll prints at tick=17808 — exactly **2 late runs** occurred, the second being **func_029190** (n=2, addr=029190); by elimination from 185E3E's call order (014E5C first, then 029190) the pair is **014E5C + 029190**. The next step (DR's fatal step) contained ZERO late runs, and 029678 did not run until tick 17864. 017996, 013088, 185CE4 never appear as late runs in the window; 002220/00AC48/002260 appear once each near tick ~18344–18396 (post-race housekeeping).
- REFINED MODEL: the poison is INJECTED during the skip step itself, by the native execution of **func_014E5C and/or func_029190** after returning to 1860C8 (with 014BDA, called first inside 014E5C, as a nested sub-suspect — in DV it was interpreted inside 014E5C's capsule). The corrupted state detonates in the following step, which is why DR died "in the next step" and DU, halted between them, lived. The other seven are bystanders for this frame (capsuled or not, they were not in the window) — they stay capsuled in DW to preserve DV's success context.

## DW (#127) — SINGLE-VARIABLE BISECT: 014E5C RUNS NATIVE, THE REST STAY CAPSULED
- Change vs DV (rr3_dispatch_mask.c ONLY): the late set loses the 014E5C extent [014E5C,014F32). Everything else identical: arm at the 01A718 skip, remaining eight capsuled, latecaps/dvarm lines (labels renamed DW), cache diff vs build-dv = LABEL DW + NUMBER 127 only.
- Read: DW dies at the fade (same wall: after the skip + a step) => **func_014E5C convicted** (desk-read its generated body + 014BDA next). DW sails => 014E5C cleared and func_029190 is convicted by elimination (the only other run in the window); DX then confirms 029190 the same way (native-alone test) before any fix is designed.
- DW DELIVERED 2026-11-08: rr3vita-dw.vpk, 2,928,218 B, MD5 73388559a2308e96eb065493dca7cfd9 (copies identical, zip checked, icon byte-identical). Verified in the raw ELF: purpose "DW: 014E5C NATIVE - IS IT THE ONE?", dwarm + latecaps formats, DW FROZEN stamp, g_late_* symbols. Test pending Lola's run (breadcrumb-77 expected).

## DW TEST RESULT — 2026-11-08 (breadcrumb-77, MD5 023337e1ade80f2f66ee5fc577b5b475; dump psp2core-1794209990-0x0000ca2875; Lola's report in words: "crash mid race") — INCONCLUSIVE (class 2)
- Died mid-race at tick 4638 (last tail tick=4638 pc=01467E; last capsule tick=4636 addr=01AF6A, tried=hit=5,068; maskstat tried=hit balanced; hands on, inputs at 4624/4635). ZERO maskfail/maskskip/dwarm/latecaps — the race never finished, the late set never armed, the fade was never reached. 014E5C is neither convicted nor cleared; DW's changes cannot be implicated (every DW-specific path wakes only after the arm).
- Class-2 death roster: ~3075 (CU), 3691–3711 (DP), 3702 (DT), 4638 (DW), 8544 (DS), 9740 (DH). DW's late first-sightings: 02138A + 0207EE @4455, 02207A @4458 (~180 ticks before death, nothing after) — routine progression sightings, herald pattern, no conviction. No 021BAE this run.
- DX (#128) = DW CONFIG UNCHANGED (label-only retest, the DS→DT→DU precedent): the fade bisect is one finished race away, and class-2 deaths are stochastic — DV and DU both finished. Cache diff vs build-dw must be LABEL DX + NUMBER 128 only.
- DX DELIVERED 2026-11-08: rr3vita-dx.vpk, 2,927,947 B, MD5 8f2a068e2d292daf48e5bf5919accf97 (copies identical, zip checked, icon byte-identical). Verified in the raw ELF: purpose "DX: 014E5C NATIVE - RETEST", dxarm + latecaps formats, DX FROZEN stamp, g_late_* symbols. Test pending Lola's run (breadcrumb-78 expected).

## DX TEST RESULT — 2026-11-09 (breadcrumb-78, MD5 5a39b3de10e69fdc59ab456dd18f9865; dump psp2core-1794211551-0x0000ac3edd; Lola's report in words: "mid race again") — INCONCLUSIVE (class 2)
- Died mid-race at tick 7500 (last tail/play tick=7500 pc=01467E; last capsule tick=7497 addr=01AF6A, tried=hit=10,475; hands on, inputs @7490/7495). ZERO maskskip/dxarm/latecaps — arm never fired, fade never reached. 014E5C still neither convicted nor cleared; DX's config cannot be implicated (as DW's couldn't).
- Class-2 death roster now: ~3075 (CU), 3691–3711 (DP), 3702 (DT), 4638 (DW), 7500 (DX), 8544 (DS), 9740 (DH).
- ⭐ CORRELATION NOTE (watch item, NOT a conviction — the trio verdict stands): func_021BAE was first sighted at tick 7410, **90 ticks before the death** — matching DS (sighted 8455, death 8544, ~89 ticks). Desk check: 021BAE is NOT a dispatch-table function; it is a mid-function entry inside func_021A74's extent [021A74,021DAE), capsuled via that extent (all runs clean). 021A74 is a member of the DF herald trio, whose activity preceded the DF/DI/DL deaths by the same ~90–110 ticks. Five data points now share the ~90-tick lag. The standing read stays HERALD / phase marker (the trio were capsuled and clean in every one of those deaths), but the lag is consistent enough to keep logging sighting-vs-death gaps for 021A74-region activity in future class-2 deaths.
- DY (#129) = DX CONFIG UNCHANGED (label-only retest #2 of the 014E5C bisect). If DY also dies mid-race, raise the class-2 question with Lola explicitly (three straight eaten runs for this config) rather than rolling a fourth silent retest.
- DY DELIVERED 2026-11-09: rr3vita-dy.vpk, 2,928,009 B, MD5 ea228b45b4e3f8991ebf359ec8e37315 (copies identical, zip checked, icon byte-identical). Verified in the raw ELF: purpose "DY: 014E5C NATIVE - RETEST 2", dyarm + latecaps formats, DY FROZEN stamp, g_late_* symbols. Test pending Lola's run (breadcrumb-79 expected).

## DY TEST RESULT — 2026-11-09 (breadcrumb-79 = NAS/FileZilla pull, MD5 6266319e43ffc25b7e9eedbf38931584; Lola's report in words: "It crashed mid race") — INCONCLUSIVE (class 2), NO DUMP WRITTEN
- Session 133 (4,644 lines, fetched from the Vita via the new NAS/FileZilla pipeline): raced cleanly to tick 3759 (last tail tick=3759 pc=03AEB2; last capsule tick=3756, tried=hit=4,295; hands on, last input @3743). ZERO maskskip/dyarm/latecaps — arm never fired, fade never reached. 014E5C STILL neither convicted nor cleared.
- Final-step detail: at tick 3758 a first-sighting call to 03A50E (inside MASK4's extent [03A50A,03A53C)), then one tail at pc=03AEB2 and the log ends. No 021BAE sighting in this session's final window (death came at 3759, earlier than the DS/DX marker pattern).
- ⭐ NO COREDUMP: the newest psp2core in ux0:data/ remains DX's (1794211551). First class-2 death with no dump produced. Fact recorded only — dump forensics remain a dead end; do not build theory on it.
- Class-2 death roster now: ~3075 (CU), 3691–3711 (DP), 3702 (DT), 3759 (DY), 4638 (DW), 7500 (DX), 8544 (DS), 9740 (DH).
- ⭐ ESCALATION FIRED (per the standing note): DW, DX, DY = three straight class-2 deaths eating the 014E5C bisect. Do NOT roll a fourth silent retest. The class-2 question goes to Lola explicitly: pivot the hunt to class 2 (first instrument: a DV-style capsule net armed by the race itself — candidates: the 0203E8 fall-through-entry check from the reserve design, or a broadened racing mask with a freeze 90 ticks after the 021BAE marker) vs keep rolling fade retests (DU and DV both finished; the fade answer is one clean race away).
- CORRECTION (2026-11-09, minutes later): DY DID produce a coredump — psp2core-1794270614-0x0001433973-eboot.bin.psp2dmp (300,816 B). It was NOT in ux0:data/ when the FileZilla listing was read (19 files, newest = DX's); Lola located it and copied it into /Volumes/NAS/RR3/Logs herself at 16:50 PST, then told me to check the folder. The "NO DUMP WRITTEN" line above is RETRACTED — amend it to: dump exists, recovered by Lola. (Where it was on the Vita before she copied it is not established; do not theorize.) Dump uploaded to the VM alongside the breadcrumb.
- ADDENDUM (2026-11-09 16:51, Lola): "All the crash dumps are in the data folder in the vita." — the DY dump WAS in ux0:data/ like every other; my FileZilla listing simply failed to show it (a stale/cached listing is the likely mechanics; not proven, and it doesn't matter). Correct record: dump was in ux0:data/ all along; the miss was in my read, not the Vita. Lesson logged: treat a FileZilla directory listing as a cache-prone view — refresh/re-list before concluding a file is absent.

## DECISION — 2026-11-09: Lola chose KEEP ROLLING THE FADE BISECT
- Presented with the escalation (pivot to class 2 vs keep rolling), Lola picked "Keep rolling the fade bisect". DZ (#130) = DY config unchanged (014E5C native, other eight capsuled), retest #3. The escalation is discharged — retests continue at her direction; class 2 remains a known open class, not the current front.
- DZ DELIVERED 2026-11-09: rr3vita-dz.vpk, 2,928,016 B, MD5 ccc840caabcb1a6994d12b0c61177bd8 (copies identical, zip checked, icon byte-identical). Verified in the raw ELF: purpose "DZ: 014E5C NATIVE - RETEST 3", dzarm + latecaps formats, DZ FROZEN stamp, g_late_* symbols. Cache diff vs build-dy = LABEL DZ + NUMBER 130 only. Test pending Lola's run (breadcrumb-80 expected; the NAS/FileZilla pull route now works — check /Volumes/NAS/RR3/Logs first, refresh FileZilla listings before concluding any file is absent).

## DZ TEST RESULT — 2026-11-09 (breadcrumb-80, MD5 f6a56f04d84f801f9385f04b678a9798; Lola's photo: results screen, Natasha taunt, tick 15284) — ⭐⭐⭐ SAILED PAST THE FADE
- Session 134 (24,374 lines): maskskip tick=13661 addr=01A718 n=1; dzarm tick=13661 pc=1860C8. ZERO FAIL/maskfreeze; final maskstat tried=hit=87,076; session ran past the photo to tick 16766 (post-race flow idling, tails parked at 186164).
- Late runs in DZ: exactly ONE each of 029190 (tick=13662 — inside the skip step itself, one tick after the arm), 002220, 002260, 00AC48; then the 0295C2 (163 polls) / 029678 (155 polls) post-race metronome. 014E5C ran NATIVE throughout (not in the late set) — including in the skip step.
- ⭐⭐⭐ VERDICT — THE FADE BISECT HAS ITS ANSWER: DR (014E5C native + 029190 native) DIED at the fade; DV (both capsuled) SAILED; DZ (014E5C native + 029190 capsuled) SAILED. **014E5C IS CLEARED. func_029190 STANDS CONVICTED**: its NATIVE execution in the skip step injects the poison that detonates a step later. (014BDA, the sub-suspect inside 014E5C, is cleared with its parent.)
- Pre-registered confirmation = EA (#131): 029190 native ALONE, 014E5C back in the capsule set. Expected: the fade death RETURNS. If EA sails, the elimination is wrong — rethink from scratch.
- NEW RESULTS-SCREEN BUGS (Lola-reported from the DZ photo, 2026-11-09 — screen verdicts, log can't adjudicate): (1) the standings show PLAYER A 1st with $1000 although Lola says she did NOT finish first; (2) a ".0" column renders next to each name (the times column showing only ".0"). Both belong to the post-race flow — add to the post-hunt desk-review list (standings/placement computation + time formatting), alongside the convicted-function reviews. Note both DV and DZ ran the whole post-race flow with the late set armed.
- EA DELIVERED 2026-11-09: rr3vita-ea.vpk, 2,928,203 B, MD5 2eb933d5807d424c9be4f7262cc3af14 (copies identical, zip checked, icon byte-identical, EA stamps + g_late_* verified in the raw ELF). Config vs DZ: late set has 014E5C back IN and the 029190 extent REMOVED — 029190 native alone; everything else identical (cache diff vs build-dz = LABEL EA + NUMBER 131 only). PRE-REGISTERED EXPECTATION: the fade death RETURNS (029190 convicted outright). If EA sails, the elimination is wrong — stop and rethink.

## EA TEST RESULT — 2026-11-09 (breadcrumb-81, MD5 f38824a88afe2c71195da73190f2b80e; dump psp2core-1794277462-0x00017936a3) — ⭐⭐⭐⭐ THE FADE DEATH RETURNED: 029190 CONVICTED OUTRIGHT
- Session 135 (18,775 lines): maskskip tick=12603 addr=01A718 n=1; eaarm tick=12603 pc=014C18. Final & LAST line of the file: "tail tick=12603 pc=014C18" with 01A718 + 014E5C at the head of the ring — the process died INSIDE the skip step (earlier than DR, which survived into the following step).
- ZERO FAIL/maskfreeze anywhere in session 135; final maskstat frames=10800 tried=hit=16,732 — every capsule clean, including 014E5C (one latecaps run, tick=12603).
- ⭐ 029190 IS THE ONLY MEMBER OF THE NINE THAT RAN NATIVE in EA — no latecaps entry exists for it, and the death sits in the very step where it executed natively. Confirmation COMPLETE, exactly as pre-registered: **func_029190, native execution in the 01A718 skip step, is the fade killer.** The fade bisect is CLOSED: DR died (both native) / DV sailed (both capsuled) / DZ sailed (014E5C native) / EA died (029190 native alone, dying even earlier — inside the step).
- Next phase (post-hunt desk work): read func_029190's generated code for the actual defect; a permanent capsule of 029190 is a candidate shipping config meanwhile. Class 2 remains open (roster unchanged). Results-screen bugs (standings placement, ".0" times) still on the desk-review list.
- SPEED (Lola's request, 2026-11-09): EA's racing roll measured 112.99 ms (play tick=12600) — she asked for roll under ~90 (sees 130–140 in races, falling asleep testing). EB (#132) = EA config + RR3_THROTTLE=1 (glue_throttle.c: engine glue + Build Y's IRQ-poll dose — only every 16th GLUE_YIELD_IRQPOLL actually switches fibers; insn/yld ≈ 51 in EA suggests poll-yield churn is the main sim cost). EB doubles as the first 029190-capsuled-forever candidate config — NO: EB keeps EA's exact mask config (029190 native) — speed variable isolated; if EB sails the fade with 029190 native, the throttle changed crash behavior and that must be understood before trusting either result.

EB (#132) DELIVERED 2026-11-09: rr3vita-eb.vpk, 2,927,789 B, MD5 2e837b3336acf468efb2f7aeaef11d03. SPEED EXPERIMENT: EA's exact mask config (029190 NATIVE — expected to die at the fade per the conviction) + IRQ-poll yield dose 1-in-16, targeting racing ROLL <90 (EA measured ~113; Lola sees 130-140). BUILD NOTE: first EB attempt (build-eb, -DRR3_THROTTLE=1 → src/glue_throttle.c) was SCRAPPED at verification — glue_throttle.c is build-Y-era and its FORCE_INTERP drive path still fprintf's on the game fiber (fiber poison), unlike the engine glue EA actually shipped. Shipped EB (built in build-ec/) uses src/glue_eb.c = engine glue + dose only (diff vs engine glue = the 12-line dose block; selected via -DRR3_EB_GLUE=1); arm line renamed eaarm→ebarm. Verified: cache diff vs EA = LABEL+NUMBER+EB_GLUE only; late symbols identical to EA (g_late_armed/count/last_addr, s_late_mask); icon byte-identical; zip clean. Pre-registered read: if EB sails the fade with 029190 native, the dose changed crash timing — understand before trusting either result. Class 2 still open.

## EB TEST RESULT — 2026-11-09 (breadcrumb-82, MD5 b1fb769f3cc8a6725a5592f35428ba99; dump psp2core-1794288337-0x0002f6232d, 226,224 B, MD5 f81be79422bc84082d66c81031005f06) — FADE DEATH AGAIN, EXACTLY AS PRE-REGISTERED; SPEED IMPROVED BUT NOT ENOUGH
- Session 136 (32,960 lines): maskskip tick=26209 addr=01A718 n=1; ebarm tick=26209 pc=014C0A; the log ENDS on that step's tail ("tail tick=26209 pc=014C0A", ring head 01A718 + 014E5C). Died INSIDE the skip step, same signature as EA. ZERO FAIL/maskfreeze; final maskstat frames=25200 tried=hit=17,998, all capsules clean; exactly ONE latecaps entry in the session (014E5C, tick=26209) — 029190 again the only one of the nine running native.
- ⭐ VERDICT: the 1-in-16 dose did NOT change crash behavior — EB died at the fade with 029190 native, precisely the pre-registered expectation. The 029190 conviction stands untouched by the timing change. NOT a class-2 death (the race itself completed: 26,209 ticks, far past every class-2 death on the roster).
- SPEED NUMBERS: 1,265 play samples — roll avg 95.5 ms, min 59.8, max 176.5; 43% of samples under Lola's 90 target, 35% still >=100. Better than EA's ~113 spot measure, NOT good enough — she asked for under 90 and a third of racing still runs 100+. Dose 1-in-16 is a step, not the fix; a stronger dose (and/or real perf work) is still owed.
- Watch item: one tail at tick=26072 shows pc=021A74 (herald region) 137 ticks before the death — but this death is fully explained by the fade conviction; no class-2 inference drawn.
- NEXT (Lola, 2026-11-09: "get back to fixing all these problems to make the game play"): EC (#133) = the fix build — func_029190 PERMANENTLY capsuled (extent {029190,029678} added to s_table_mask, so it runs through the capsule on BOTH the dispatch and recomp_call_func paths, from boot, not just post-arm) + EB's dose 1-in-16 kept (proven crash-neutral). Pre-registered: if the 029190 capsule holds everywhere it is called, EC should SAIL the fade and complete the race end-to-end; a capsule failure anywhere would surface as a freeze with the record on screen (DQ machinery), which is diagnosable, not a silent death. Results-screen bugs (standings/".0") and class 2 remain open after EC.

## CORRECTION — 2026-11-09 (Lola): TICKS ARE NOT A RULER
- Lola: "Ignore the tick, I pause here and there and ticks can't be counted on." She pauses during test runs, so absolute tick counts and tick gaps in the logs are NOT comparable across runs and must not carry evidentiary weight.
- Applied retroactively: (1) the EB verdict above does NOT rest on "26,209 ticks, far past the class-2 roster" — it rests on the marker structure (maskskip + ebarm + log ending inside the skip step), which is unaffected; the tick-roster sentence is withdrawn as evidence. (2) The EB 021A74 tail sighting "137 ticks before the death" is withdrawn as a data point — sightings get logged, gaps do not. (3) The standing 021BAE ~90-tick sighting-to-death lag correlation (DS/DX/DY notes) is WEAKENED — the five-point lag pattern may partly be pause noise; the watch item stays as sightings-only, herald verdict unchanged. (4) EC's read criteria use no tick comparisons: sail = her report/photo of the results screen and continued flow; death/freeze = the marker structure in the log.

## EC (#133) TEST RESULT — 2026-11-09 ~22:15 (breadcrumb session 137)
- Lola's report: "Huh, crashed mid race". Breadcrumb 44,302,331 B, MD5 b418bfa7b2e4a5f80572908a427d69a4, session 137 (6,236 lines). Log ends mid-race at tick 4753, tail parked in the racing loop (pc=01467E). Dump fetch: FileZilla remote-path step hit an approval timeout on the Pro — dump not yet pulled (dumps have never decided a verdict in this hunt; fetch on Lola's go-ahead).
- **VERDICT: INCONCLUSIVE for EC's fade question — a CLASS-2 death.** No maskskip, no ecarm, no latecaps: the fade was never reached. Capsules all clean to the end (last maskstat frames=3600 tried=hit=5266; final polls tried=hit=5686; zero FAIL/maskfreeze).
- **NOT an EC regression:** func_029190 was never called at all in the run (zero addr=029190 capsule sightings) — its new permanent capsule wasn't even exercised before the death. The death matches the standing class-2 pattern exactly (capsule-independent, mid-race, outside call-entry instrumentation). EC joins the class-2 victim roster.
- No 021A74/021BAE sightings this session (sightings-only logging per the ticks correction).
- Speed (short run, 192 play samples): roll avg 105.2 ms, min 66.9, max 141.4, 58% of samples >=100 — worse than EB's full-race 95.5 average, though the sample is one partial race. Perf work remains owed regardless.
- Next: ED (#134) = EC-identical retest (label/number/stamps only), per the build-train rule — EC still needs one run that reaches the fade to validate the permanent capsule end-to-end.
- Addendum (2026-11-09 ~22:47): EC coredump fetched on Lola's go-ahead after the earlier approval timeout — psp2core-1794291333-0x0002f127ed-eboot.bin.psp2dmp, 275,312 B, MD5 f63eb17e239c8989799a658fe52b0aef; on the NAS (RR3/Logs) and uploaded to the VM. Filed for completeness; dump forensics remain a dead end and it changes no verdict.

## ED (#134) DELIVERED — 2026-11-09 ~22:52
- EC-identical retest: cache diff vs build-ed = LABEL ED + NUMBER 134 only; glue_eb.c dose 1-in-16; 029190 extent stays in s_table_mask. Purpose "ED: 029190 CAPSULED + DOSE 1/16", frozen "ED FROZEN - HALTED", arm marker edarm.
- Verified on disk this time (a first completion report came from a VM instance that was rolled back to a mid-build snapshot — no VPK existed; the runtime-recovered make rebuilt it, and only the on-disk result was trusted): rr3vita-ed.vpk 2,927,824 B, MD5 d27fe85e8f310b4a36a5a048d02b0a16 (build copy == delivered copy); zip clean; icon byte-identical; velf stamps ED-only (no EC/EB forms); exactly one 029190->029678 extent pair in the VELF; g_late_*/g_mask_* symbols match EC.
- Delivered all three legs: chat attachment + NAS /Volumes/NAS/RR3/VPK/ + Vita ux0:/VPK via FileZilla on the Pro ("transferred 2,927,824 bytes in 2 seconds"). TEST PENDING. Read: same as EC's pre-registration — ED should sail the fade if the permanent capsule holds; a class-2 mid-race death would again be inconclusive, not a refutation.
- FileZilla note: the LOCAL list in the VPK folder is sorted by FILENAME (not date) — Home+Down selects move-xbox-clips.sh; rr3vita-ed.vpk sits at the END of the name sort (End selects it). The remote list is date-sorted. Verify the status bar before every transfer.

## ED (#134) TEST RESULT — 2026-11-09 ~22:52+ (breadcrumb session 138)
- Lola's report: "Crashed mid race". Breadcrumb 45,717,145 B, MD5 b64b9240b781df7d9d93bce985cbe10c, session 138 (7,879 lines). Log ends mid-race: final tail tick 6026, tail parked in the racing loop (pc=01467E). Coredump NOT fetched — a FileZilla key step hit an approval timeout on the Pro after the breadcrumb landed; the dump remains on the Vita (dumps have never decided a verdict in this hunt; fetch on Lola's go-ahead, as with EC).
- **VERDICT: INCONCLUSIVE for ED's fade question — a CLASS-2 death, again.** No edarm, no maskskip, no latecaps: the fade was never reached. Capsules clean to the end (last maskstat frames=4800 tried=hit=6677; final capsule polls tried=hit through 7306/7305 — the single off-by-one is the standing poll-pipelining artifact, not a failure; zero FAIL/maskfreeze).
- **NOT an ED regression, same as EC:** func_029190 was never called at all in the run (zero addr=029190 capsule sightings) — the permanent capsule remains unvalidated end-to-end because no fix-config run has survived to the fade yet. EC + ED = two straight class-2 deaths on the fix configuration.
- No 021A74/021BAE sightings this session (sightings-only logging per the ticks correction).
- Speed (256 play samples): roll avg 102.5 ms, min 59.0, max 142.1, 51% of samples >=100, only 29% under the 90 target. Consistent with EC's 105.2; the 1-in-16 dose is not delivering the under-90 target — perf work remains owed regardless of the crash front.
- Decision point (raised with Lola with this verdict): the fade question has now been eaten twice running on the fix config. Options: (a) pivot the hunt to class 2 — build EE with the reserve instrument (a capsule check at func_0203E8's fall-through entry, the one entry path call-entry instrumentation can't see); (b) keep rolling EC-config retests until one run reaches the fade; (c) her call entirely. No EE build started until she chooses — this is the same shape as the DW/DX/DY escalation she decided on 2026-11-09, and the choice is hers again.

## EE (#135) DELIVERED — 2026-11-09 ~23:2x — THE CLASS-2 PIVOT BUILD
- Decision: after EC+ED both died class-2 on the fix config, the pivot question was put to Lola with the ED verdict; she chose (widget tap + message, 23:03) "Pivot to class 2 — build EE with the 0203E8 fall-through instrument". This supersedes her 2026-11-09 earlier choice to keep rolling fade retests — the fade question is now settled (029190 convicted, fix built), and class 2 is the front.
- Design (desk review before building, per the double-check rule): func_0203E8's dispatch extent [0x0203E8,0x020592) added to s_table_mask (sorted, after 01ED80). It is the race's per-entity workhorse, called DIRECTLY from guest $0142C2 via recomp_call_func with a proper pushed return (0x0142C8) — and recomp_call_func consults rr3_table_masked, so the extent routes every such call through the capsule. Inside its body sits the dynamic handler JSR at $020556 (target from the entity struct, return $020558) — DT's death tail parked at that return point. 029190 stays permanently capsuled; late set and dose 1-in-16 unchanged; cache diff vs build-ee = LABEL EE + NUMBER 135 only.
- Pre-registered reads: (a) a capsule FAIL naming 0203E8 — freeze with the record on screen — convicts the class-2 death to inside 0203E8's framed run; (b) a clean class-2 mid-race death with 0203E8 capsule runs tried=hit clears 0203E8 and moves the hunt elsewhere; (c) sailing a full race implicates 0203E8's native execution AND validates the 029190 fade fix in the same run. Accepted cost: the hot per-frame loop runs interpreted, so roll may worsen beyond ED's ~102 ms — EE is an instrument, not a perf build.
- Verified on disk: rr3vita-ee.vpk 2,927,893 B, MD5 223116cb00f37abbfb81b864facac6b7 (build copy == delivered copy); zip clean (param.sfo, eboot.bin, rr3.bin, sce_sys/icon0.png); icon byte-identical to the tree icon; VELF carries exactly one 0203E8->020592 extent pair and exactly one 029190->029678 pair; stamps EE-only (purpose "EE: 0203E8 + 029190 CAPSULED + DOSE 1/16", frozen "EE FROZEN - HALTED", arm marker eearm; no ED forms). Sources snapshotted as build-ef/*.ef-source.
- Delivered all three legs: chat attachment + NAS /Volumes/NAS/RR3/VPK/ (2,927,893 bytes written) + Vita ux0:/VPK via FileZilla on the Pro ("transferred 2,927,893 bytes in 2 seconds"; the remote list now shows the full build train dm..ee). TEST PENDING.

## EE (#135) TESTED — 2026-11-09 ~23:42 — INCONCLUSIVE (class-2 death); func_0203E8 CLEARED
- Lola's report: "Mid race crash". Breadcrumb session 139 (46,845,165 B, MD5 e69a3caca5cab5ca8ce2f8aa3d418493): log ends mid-race, final tail tick 4974 pc=01467E; NO eearm/maskskip/latecaps (fade never reached), zero FAIL/maskfreeze, 029190 never called (zero sightings).
- THE INSTRUMENT PROVABLY ENGAGED — the check that matters: 571 of 1,625 capsule polls in the session are addr=0203E8 (the most-polled addr after 01AF6A's 636), and the final polls run clean THROUGH the death window: tick 4969 addr=0203E8 tried=2679 hit=2678, tick 4971 tried=2680 hit=2680 (the one-off delta is the standing poll-pipelining artifact — a poll caught a capsule mid-flight; the next poll reconciles). 0203E8's body therefore ran interpreted, hundreds of times, right up to the death, with every framed run committing cleanly.
- VERDICT per pre-registered read (b): func_0203E8 is CLEARED — the class-2 death is NOT in 0203E8's own body. EE joins the class-2 roster (death ~tick 4974). The permanent 029190 fade capsule remains unvalidated end-to-end (still never exercised — 029190 uncalled).
- REFINEMENT OF THE CLASS-2 MODEL: the capsule interprets 0203E8's guest code, but the dynamic handler JSR at $020556 resolves its target through the call machinery — the handler BODIES still execute natively. The named suspect space narrows to those handler targets (loaded per-entity from the struct at A4), i.e. native code entered from inside an otherwise-clean interpreted frame. Next instrument (EF): census the distinct targets the $020556 call dispatches to during racing, then capsule the census leaders.
- Speed: 203 play samples, roll avg 103.1 ms (min 69.2, max 146.1; 49% >=100, 25% <90) — statistically ED's twin (102.5). The feared interpreter tax on the hot loop did NOT materialise (capsule polls are sparse samples; the framed runs interleave with native speed). Perf still owed vs the <90 target, unchanged.
- EE coredump not fetched (dump forensics are a dead end; no verdict depends on it).

## EF DESIGN NOTES — 2026-11-09 late (desk work after EE's verdict)
- The $020556 call, precisely: inside 0203E8's entity loop, A4 = read32(0x34D92 + (int16)D2) loads a RECORD pointer (the 0x34D92 table in ROM: 15 pointers to 16-byte records at 0x34DCE+16k); after field work, A6 = read32(A4) and recomp_call_addr(A6) with return 0x020558. In NATIVE runs the handlers pass through dispatch; under EE's capsule the interpreter resolves the JSR itself, so the handler bodies ran native and UNOBSERVED by any call log — the tail ring is the only call-stream record, and EE/ED tail censuses are dominated by an 0x03Axxx-0x03Bxxx cluster (03B54A/03B55E/03AB40/03A4A8...) whose relation to the $020556 targets is UNPROVEN.
- Static read of the record table's first fields gave values that are NOT plausible handler addresses (0A7F00, 096000, 084000, 074000, 064000, then 207800 x10, interleaved with packed parameter fields) — the naive "record[0] = handler read from ROM" model does not survive contact with the bytes; A4's record base is likely runtime-relocated or the field layout differs. DO NOT build EF on the static table.
- EF as designed: a runtime census of recomp_call_addr targets whose pushed return address is 0x020558 (a precise $020556 filter, not a frequency guess), recorded as a distinct-target set with counts in plain memory (fiber-safe, fb_note pattern), flushed by the existing periodic breadcrumb poll. Open mechanical question to resolve first: whether the interpreter's indirect-JSR path also funnels through recomp_call_addr — if yes, the census sees capsuled-0203E8 runs too; if no, EF runs 0203E8 native for the census run (fade still protected by the permanent 029190 capsule; a class-2 death mid-census still yields the census up to the death).
- Standing warning (herald-trio lesson): the tail-census leaders are HERALDS until the $020556 filter ties a target to the call site. No capsuling by frequency.

## EF DESIGN RESOLVED — 2026-11-09 late (interpreter read; CORRECTS the EE ledger entry above)
- Read of engine-defer/runner/m68k_interp.c: the step function handles MN_JSR purely internally (tgt = addr_ea; push32(fall); *next_pc = tgt) and the framed runner tracks net call depth, interpreting through nested JSR/BSR with NO call-out to native code anywhere (the only external call is glue_check_vblank cycle accounting). CONCLUSION: a capsule run interprets the entry function's ENTIRE dynamic subtree.
- CORRECTION to the EE entry's model refinement: under EE, the $020556 handler bodies did NOT run native — they were interpreted inside 0203E8's framed runs and committed cleanly like everything else. EE therefore clears 0203E8's whole dynamic subtree (handlers included) of native-vs-guest divergence, not just 0203E8's own body. The "suspect space narrows to the handler targets" sentence in the EE entry is WRONG and is superseded by this note. (The Space update dispatched for EE carries that wrong sentence — correct it on the next Space pass.)
- What class 2 can still be, given every capsule verdict so far: (a) a native divergence in a hot function NEVER yet capsuled — the direct-call world (488 functions) minus the handful tested; the tail-census 03xxxx cluster (03B54A/03B55E/03AB40/03A4A8/03B068/03AFEC...) is the hottest untested native set during racing; (b) poison semantics — a guest-faithful write that detonates later (the capsule reproduces it faithfully, so no capsule will ever catch it); (c) a non-CPU-side effect (fiber/timing/audio) — no evidence pointing here yet.
- EF REDESIGN (replaces the census design): the $020556 census is CANCELLED — pointless now, since the handlers are already cleared as part of the subtree. EF instead capsules the top tail-census cluster as a SET via s_table_mask (the way DV's nine-call set was convicted as a set): candidate extents from the dispatch table for the recurring 03xxxx entries (03A4A8, 03A4D2 already MASK5-capsuled at gate 26600 — note MASK5's gate means it only engages late; check each candidate's existing mask coverage first), plus 03AB40, 03AFBE, 03AFEC, 03B068, 03B4CA, 03B54A, 03B55E, 03A66A. Pre-registered reads: a FAIL naming a cluster member convicts it; a sail implicates the set's native execution; another clean class-2 death with the cluster tried=hit clears the set and pushes the hunt to (b)/(c) thinking. Perf risk is real (these are the hottest calls in the game) — accepted for one instrument run, same bargain as EE.
- Before building EF: desk-verify each candidate's dispatch extent + current mask status, and replay that 03A4D2/03A50A-era masks (MASK4/MASK5) already cover two cluster members behind tick gates — EF's set should use PERMANENT table extents so the coverage is from boot, not gated.

## EF (#136) DELIVERED — 2026-11-10 ~00:0x — THE HOT-CLUSTER SET
- Design (resolved by the interpreter read + dispatch desk-check, see EF DESIGN RESOLVED above): EE config carried forward (029190 + 0203E8 permanent capsules, dose 1-in-16) PLUS the ten most frequent tail-census call targets of EE's session capsuled as a set from boot via s_table_mask: 03A4A8 [03A4A8,03A4CE), 03A4D2 [03A4D2,03A4DE), 03A66A [03A66A,03A9DE), 03AB40 [03AB40,03ABC2), 03AFBE [03AFBE,03AFEC), 03AFEC [03AFEC,03B068), 03B068 [03B068,03B1C8), 03B4CA [03B4CA,03B54A), 03B54A [03B54A,03B55E), 03B55E [03B55E,03B570) (minimal extent: two-instruction leaf, next dispatch entry 1.35 MB away across a data gap). All ten desk-verified as exact dispatch entries, none previously table-covered; table stays sorted/non-overlapping (65 extents).
- Pre-registered reads: (a) a capsule FAIL naming a set member convicts it; (b) sailing a full race implicates the set's native execution (then bisect within the set) AND validates the 029190 fade fix; (c) another clean class-2 death with the set tried=hit clears the whole set and pushes the hunt toward poison-semantics / non-divergence models. Perf risk accepted (hottest calls in the game now interpreted).
- Cache diff vs build-ef = LABEL EF + NUMBER 136 only; glue_eb.c confirmed in build.make. Verified on disk: rr3vita-ef.vpk 2,928,170 B, MD5 583c873e5a2c6c8e4bc87ff39e9b82d5 (both copies identical); zip clean; icon byte-identical; VELF extent pairs: exactly one each for 0203E8, 029190 and all ten EF set pairs; EF stamps only (purpose "EF: HOT CLUSTER + 0203E8 + 029190 CAPSULED", "EF FROZEN - HALTED", efarm; no EE/ED forms); g_late_armed/g_mask_tried/g_mask_hit present. Sources snapshotted as build-eg/*.eg-source.
- Delivery: chat attachment + NAS + Vita legs as standard. TEST PENDING.
- EF delivery confirmed complete 2026-11-10 ~00:10: Vita leg via FileZilla on the Pro — local selection screenshot-verified (rr3vita-ef.vpk highlighted, status bar "Selected 1 file. Total size: 2,928,170 bytes"), upload "File transfer successful, transferred 2,928,170 bytes in 2 seconds"; remote /ux0:/VPK listing refreshed after. All three legs done; TEST PENDING stands.

## EF (#136) TEST — 2026-11-10 (breadcrumb session 140, MD5 6fcf87e43f0a7ca6e9af9b0f9fe451c2, 51,521,487 B)
- Grabbed Mini-side (first Mini-pipeline grab: FileZilla on the 2018 mini -> NAS RR3/Logs -> VM; earlier "no route to host" was the macOS Local Network permission, allowed by Lola).
- **FADE FIX VALIDATED END-TO-END.** maskskip + efarm tick 15048 (pc=014C0A); the convicted step ran with 029190 capsuled (call record tick 15048; 014E5C latecaps) and the game SURVIVED it. First fix-config build to finish a race AND cross the fade (EC/ED/EE all died mid-race first; 029190 never called there).
- Capsules: 539,411+ tried=hit, ZERO FAIL, zero freeze — the whole hot-ten set included; no class-2 death this run (does not close class 2).
- NEW leftover: POST-FADE STALL. Picture kept changing ~550 ticks past the arm (fbh changes through tick 15640, last AD174035), then the framebuffer froze while the sim kept stepping (insn still climbing) in a 0021B6 <-> 03A4A8 tail cycle at pc=186164 to the end of the log (~tick 15919). No crash; latecaps 002260 reached n=65,647 post-arm. Roll in the late stretch 95-150 ms.
- Lola's verdict on the run: "Still running — I closed it myself." Run ended by her hand. Whether the frozen tail was a static results screen or a stuck wait is NOT established — EG separates it.

## EG (#137) DESIGN — 2026-11-10 (the A/B)
- EF minus the hot-ten table extents (03A4A8..03B55E block, incl. the unlisted 03AF74 span) = EC's config exactly: 029190 + 0203E8 permanent capsules, dose 1-in-16, standing gated masks unchanged. Stamps: purpose "EG: 029190 + 0203E8 CAPSULED - NO HOT SET", frozen "EG FROZEN - HALTED", arm marker egarm. Built in build-eh; cache diff vs build-eg = LABEL EG + NUMBER 137 only.
- Pre-registered reads: (a) EG crosses the fade and reaches the results screen -> a hot-set member causes EF's stall (03A4A8 prime suspect: it sits inside the stall cycle) — bisect the set for the stall; (b) EG stalls the same way -> stall is hot-set-independent (dose glue / from-boot capsule / the wait itself); (c) class-2 death -> class 2 still live without the set.

- CORRECTION to the EF entry above (Lola's screen report, 2026-11-10 17:07): after the fade, EF played a cutscene and then reached the screen showing 1st/2nd/3rd place. The "post-fade stall" was the standings screen AT REST — there is no stall. EF therefore reads as pre-registered read (b): A FULL SAIL — the hot set's native execution is implicated in class 2 (as a set; no member convicted), and the 029190 fade fix is validated. EG's purpose re-frames accordingly: it is the class-2 A/B (EC config; EC/ED/EE = three straight class-2 deaths without the set). If EG dies class-2 too, the hot set gets bisected for class 2; if EG sails, the set theory weakens.

## EG (#137) BUILT + DELIVERED — 2026-11-10
- Config: EF minus the hot-ten table extents = EC config (029190 + 0203E8 permanent capsules, dose 1-in-16, standing gated masks). Built in build-eh; cache diff vs build-eg = LABEL EG + NUMBER 137 only; glue_eb.c in build.make. Sources snapshotted as build-eh/*.eh-source.
- Verified on disk: rr3vita-eg.vpk 2,927,806 B, MD5 2940f60b119c4ab577e1aa81d752b0d2 (both copies identical); zip clean; icon byte-identical (a1e00d2b5d28e0fce61af26779860f0a); VELF pairs: 029190->029678 exactly 1, 0203E8->020592 exactly 1, all eleven EF pairs 0; stamps EG-only (purpose "EG: 029190 + 0203E8 CAPSULED - NO HOT SET", "EG FROZEN - HALTED", egarm); g_late_armed/g_mask_tried/g_mask_hit present.
- Delivery (all Mini-side): NAS via files.write (2,927,806 B to /Volumes/NAS/RR3/VPK/); Vita leg via FileZilla on the Mini — local selection screenshot-verified (rr3vita-eg.vpk, status bar "Selected 1 file. Total size: 2927806 bytes"), "File transfer successful, transferred 2927806 bytes in 2 seconds"; chat attachment sent. TEST PENDING (Lola).

## EG (#137) TESTED — 2026-11-10: FULL SAIL + NEW CLASS-3 HANG
- Lola's report (18:11): sailed past everything; pressed a button on the standings screen to continue to the next track -> stalled/locked up; she closed it.
- Session 141 (breadcrumb 58,389,132 B, NAS copy; analysed via eg-tail.txt 7.5 MB tail slice; session = 6,867,645 B / 33,836 lines): maskskip+egarm tick 15713 pc=014C16; call tick=15713 addr=029190 capsuled and survived; final maskstat tried=75881 hit=75880 (pipelining artifact), ZERO FAIL. Standings/menu idle ran normally (ticks advancing, pc=18617A, cycle 0021B6 00C340 + hot set; cf. EF rest pc=186164).
- THE HANG: tick 23244 FROZEN across 3,356 tail polls; play lines identical (insn=239891192, yld=3628519, fbh=791346FC) while roll kept updating (34.5->53.5) — host loop alive, guest zero-progress: a native-side hang inside a step, not a crash (no FAIL, no dump). Approach: at tick 23212-23213 the cycle swapped 00C340->0022E2; final ring: 0021B6 0022E2 [hot set] 017A6A [hot set] 185CE4 — the button-press transition path.
- NOTE: 185CE4, 002220, 002260, 00AC48, 013088, 014E5C, 017996, 029678 were ALREADY interpreted post-arm in EG (fade late set). The hang lives in the still-native remainder of the cycle: 0021B6, 0022E2, 017A6A, a hot-ten member, or deeper.
- VERDICTS: (1) Class-2 A/B read (b) FIRES — EG sailed on the EC/ED/EE config family, so the hot-set theory WEAKENS; class 2 stays open/intermittent, the set is no longer prime suspect. (2) NEW CLASS 3: post-standings next-track transition hang (guest-frozen / host-alive).
- EH (#138): EG + late-mask completion — {0021B6,002220} {0022E2,002398} {017A6A,017B04} + the hot ten as LATE capsules, so the entire post-race frontend cycle runs interpreted while racing stays native. Reads: (a) EH reaches track 2 -> native divergence in the late set as a set; bisect. (b) EH hangs identically -> hang is outside the cycle set (host-side wait or deeper direct-call path); desk-read the transition. (c) A FAIL names the culprit.

## EH (#138) BUILT + DELIVERED — 2026-11-10
- Config: EG + late-mask completion (0021B6, 0022E2, 017A6A + hot ten as LATE capsules; 21 late extents total incl. the fade set). Built in build-ei; cache diff vs build-eh = LABEL EH + NUMBER 138 only; glue_eb.c in build.make; sources snapshotted build-ei/*.ei-source.
- Verified on disk: rr3vita-eh.vpk 2,928,013 B, MD5 57e684eb98daa6df916b0e30f6bace40 (both copies identical); zip clean; icon byte-identical; VELF pairs: 029190->029678 1, 0203E8->020592 1, all fourteen new/old late pairs exactly 1; EH stamps only (purpose "EH: EG + POST-RACE FRONTEND CAPSULED (LATE)", "EH FROZEN - HALTED", eharm); g_late_armed/g_mask_tried/g_mask_hit present.
- Delivery: NAS via files.write (2,928,013 B); Vita via FileZilla on the Mini — selection screenshot-verified, "File transfer successful, transferred 2928013 bytes in 1 second"; chat attachment sent. TEST PENDING (Lola).
- Breadcrumb note: the Vita's breadcrumb.txt was deleted after EG's grab (Lola's suggestion) — EH's test starts a fresh file; the cumulative archive (sessions 1-141) lives at NAS RR3/Logs/breadcrumb.txt.

### EH (#138) — TESTED 2026-11-10 ~18:57: INSTRUMENT ARTIFACT — 0021B6 IS ST=99 UNCAPSULABLE; class 3 NOT reached
- Lola's report: "Says fail at the bottom" — the on-screen frozen stamp. Fresh breadcrumb (3,583,545 B, single session; grabbed via the Mini to NAS RR3/breadcrumb.txt, archive untouched): eharm + maskskip tick 12310, call tick=12310 addr=029190 capsuled and survived, latecaps 014E5C x1 then 0021B6 n=2, and the session's only failure: maskfail tick=12311 addr=0021B6 st=99 badpc=000000 badop=0000 exit=000000 want=000000 sp=00FFFFF6.
- st=99 decoded from rr3_mask_run (src/rr3_dispatch_mask.c): the pre-flight guard — expected_ret = peek32([A7]) was 0 (or odd), so the capsule NEVER RAN; freeze machinery halted the sim one tick after the arm, in the post-fade frontend, BEFORE any standings-to-track-2 press. maskfail.txt on the Vita stayed 0 bytes; the record lives only in the breadcrumb.
- Why: func_0021B6 (generated/rr3_part01.c:5663) is a register-saving THUNK — MOVEM.L -(SP) x15, A0 = read32(0x00FF0226), dynamic JSR through A0 if nonzero. It has NO static call sites in the recompiled parts (dispatch-reached only) and is entered by JUMP-chaining in the frontend cycle: no return address sits at [A7] on entry (A7=0xFFFFF6, [A7]=0). The capsule model's core assumption (entry [A7] = return) is false for jump-entered functions — 0021B6 is structurally uncapsulable by this machinery. NOT a divergence, NOT the class-3 culprit, NOT a 0021B6 bug.
- Verdict: EH's pre-registered reads never fired — the class-3 question is untouched. 0022E2 / 017A6A / hot-ten late capsules never got a post-arm call before the freeze.
- EI (#139): EH minus { 0x0021B6, 0x002220 } from s_late_mask. Reads: (a) transition sails -> hang in {0022E2, 017A6A, hot ten} late set; (b) st=99 FAIL on 0022E2 or 017A6A -> that one is also jump-entered, strip and repeat; (c) identical press-hang with the rest capsuled -> hang is in native 0021B6 or deeper (needs entry/exit logging, not capsules).

### EI (#139) — DELIVERED 2026-11-10 (test pending)
- Config: EH minus { 0x0021B6, 0x002220 } in s_late_mask (the st=99-uncapsulable thunk removed; 0022E2, 017A6A + hot ten stay late). Built in build-ej; cache diff vs build-ei = LABEL EI + NUMBER 139 only; glue_eb.c in build.make; sources snapshotted build-ej/*.ej-source.
- Verified on disk: rr3vita-ei.vpk 2,928,200 B, MD5 870dd5a5354fc4808513c1326275f1fc (both copies identical); zip clean; icon byte-identical; VELF pairs: 0021B6->002220 0 (removed), 0022E2->002398 1, 017A6A->017B04 1, all hot-ten pairs 1, 185CE4->185D58 1, 029190->029678 1, 0203E8->020592 1; EI stamps only (purpose "EI: EH MINUS 0021B6 LATE (ST99 UNCAPSULABLE)", "EI FROZEN - HALTED", eiarm); g_late_armed/g_mask_tried/g_mask_hit present.
- Delivery: NAS via files.write (2,928,200 B); Vita via FileZilla on the Mini — selection screenshot-verified ("Selected 1 file. Total size: 2928200 bytes"), "File transfer successful, transferred 2928200 bytes in 2 seconds"; chat attachment sent. TEST PENDING (Lola) — reads as pre-registered under EH's verdict.

### EI (#139) — TESTED 2026-11-10 ~19:50: INSTRUMENT ARTIFACT AGAIN — 185CE4 IS TAIL-ENTERED, st=99; idle cycle mapped
- Fresh breadcrumb (7,094,092 B on the Vita = EH session + EI session; EI session extracted, 3,510,547 B; copies at NAS RR3/breadcrumb.txt, Logs/breadcrumb-ei.txt, Logs/breadcrumb-eh.txt): eiarm + maskskip tick 12067, call tick=12067 addr=029190 capsuled and survived, maskstat tried=77517 hit=77516, and the session's only failure: maskfail tick=12826 addr=185CE4 st=99 want=000000 sp=00FFFFF6.
- The standings idle RAN post-arm (759 ticks): latecaps polls dominated by 03A4A8 (582 polls), 029678 x172, 002220 x2. Freeze play line: tick=12826 pc=18617A a7=FFFFF6 — the frontend step's own stack sits at 0xFFFFF6 with [A7]=0 in this era.
- The frozen tail ring is the map: the idle cycle at the 18617A step runs 03A4A8 -> 0021B6 -> 0022E2 repeating, and the ring's LAST entry is 185CE4 — the step tail-called it (recomp_tail_call sites: rr3_part29.c:7963, rr3_part30.c:476) and the capsule attempt died on the guard. Functions entered DIRECTLY from this step (tail/jump, no return pushed) cannot be capsuled; deeper JSR-called members (03A4A8) capsule fine. 0022E2 was called all through the idle with NO st=99 — its entries carry valid returns.
- Verdict: second uncapsulable cycle member found (185CE4, joining 0021B6). Class 3 still untested — the press was never reached. Generalised rule: capsule the cycle's JSR-called members only; step-entered members need logging, not capsules.
- EJ (#140): EI minus { 0x185CE4, 0x185D58 } from s_late_mask (fade unaffected: 185CE4 was never late-called in the fade era; the convicted step runs it as the step itself, natively). Reads: (a) press sails -> hang in {0022E2, 017A6A, hot ten}; (b) st=99 on 0022E2/017A6A -> strip, capsule road ends, logging takes over; (c) press hangs -> hang in native {0021B6, 185CE4} or deeper; EJ's frozen ring names the loop members.
- PRIORITY SET BY LOLA (2026-11-10 19:54): once class 3 is dealt with and track 2 loads, the NEXT fix — pronto, ahead of perf/audio/desk-review — is the results-screen standings bugs (PLAYER A 1st/$1000 wrong; ".0" times column).

### EJ (#140) — DELIVERED 2026-11-10 (test pending)
- Config: EI minus { 0x185CE4, 0x185D58 } in s_late_mask (both step-entered members now native: 0021B6 + 185CE4; 0022E2, 017A6A + hot ten stay late). Built in build-ek; cache diff vs build-ej = LABEL EJ + NUMBER 140 only; glue_eb.c in build.make; sources snapshotted build-ek/*.ek-source.
- Verified on disk: rr3vita-ej.vpk 2,928,235 B, MD5 f9f24099661008639189768979d8ea67 (both copies identical); zip clean; icon byte-identical; VELF pairs: 0021B6->002220 0, 185CE4->185D58 0, 0022E2->002398 1, 017A6A->017B04 1, all hot-ten pairs 1, 029190->029678 1, 0203E8->020592 1; EJ stamps only (purpose "EJ: EI MINUS 185CE4 LATE (TAIL-ENTERED ST99)", "EJ FROZEN - HALTED", ejarm); g_late_armed/g_mask_tried/g_mask_hit present.
- Delivery: NAS via files.write (2,928,235 B); Vita via FileZilla on the Mini — selection screenshot-verified ("Selected 1 file. Total size: 2928235 bytes"), "File transfer successful, transferred 2928235 bytes in 2 seconds"; chat attachment sent. TEST PENDING (Lola) — reads as pre-registered under EI's verdict. After class 3: Lola's standings-bugs fix goes first (her 2026-11-10 priority).

### EJ (#140) — TESTED 2026-11-10 ~20:21: INCONCLUSIVE — class-2 mid-race death; class-3 instrument never reached the fade
- Lola's report: "crash mid race". Breadcrumb (7,933,449 B = EH+EI+EJ sessions; MD5 ea0859bc5b1aef4b13b8029a23d10aef; copies at NAS RR3/breadcrumb.txt and Logs/breadcrumb-ej.txt): EJ session 839,357 B — NO ejarm, NO maskskip, NO maskfail, 029190 never called, capsules clean (final maskstat tried=hit=1396 at frames=2400). Final tail tick=3928, pc=01467E — the exact class-2 signature of EC/ED/EE. Roll ~118-137 ms.
- Verdict: EJ joins the class-2 roster. NOT an EJ regression — its only delta vs EI is a late-mask extent, and late masks engage only after the fade arm, which never fired; the racing config is unchanged in effect. The class-3 press test remains owed.
- EK (#141): EJ-identical retest (the EC->ED precedent). If EK also dies mid-race, the fix-config tally becomes 5 class-2 deaths vs 2 sails (EF, EG) — at that point re-opening the class-2 front becomes a question for Lola.

### EK (#141) — DELIVERED 2026-11-10 (test pending)
- Config: EJ-identical retest (only stamps differ: purpose "EK: EJ RETEST - CLASS 3 PRESS TEST", frozen "EK FROZEN - HALTED", arm marker ekarm). Built in build-el; cache diff vs build-ek = LABEL EK + NUMBER 141 only; glue_eb.c in build.make; sources snapshotted build-el/*.el-source.
- Verified on disk: rr3vita-ek.vpk 2,927,881 B, MD5 c7d4783fa2c8efda17426c783f795451 (both copies identical); zip clean; icon byte-identical; VELF pairs identical to EJ (0021B6->002220 0, 185CE4->185D58 0, all other late/table pairs exactly 1); EK stamps only; mask symbols present.
- Delivery: NAS via files.write (2,927,881 B); Vita via FileZilla on the Mini — selection screenshot-verified ("Selected 1 file. Total size: 2927881 bytes"), "File transfer successful, transferred 2927881 bytes in 2 seconds"; chat attachment sent. TEST PENDING (Lola) — same reads as EJ: press sail convicts {0022E2, 017A6A, hot ten}; st=99 on 0022E2/017A6A = strip-and-logging; press hang = native {0021B6, 185CE4} or deeper (ring names the loop); another class-2 death = tally 5 deaths vs 2 sails, re-open question goes to Lola.

### EK (#141) — TESTED 2026-11-10 ~21:31: ⭐⭐⭐⭐ CLASS 3 SAIL — TRACK 2 REACHED (Lola: "This is a juicy one~~")
- Breadcrumb (14,954,802 B = EH+EI+EJ+EK sessions, MD5 07a3f63597da2d1783dcb9687a8d7ff1; copies at NAS RR3/breadcrumb.txt, Logs/breadcrumb-ek.txt): EK session 7,021,353 B. TWO maskskips in one session — tick=15180 (n=1, race 1 finish; call tick=15180 addr=029190 capsuled and survived) and tick=23108 (n=2, race 2 finish). Capsules tried=hit through the end (166,687 at frames=22800), ZERO maskfail, roll ~118-126 late.
- The transition fired BETWEEN the standings idle and skip 2: at tick 23105-23107 the step sat at pc=186096 cycling 0022E2/03A4A8/0024FA/0021B6/0022A0 with 0022E2 capsuled throughout; skip 2 armed at 23108 (latecaps 014E5C); the game then ran a SECOND full race (racing stretch pc samples 21320-22520 pre-skip-2 era, and post-skip-2 frontend flow through 017A36/00FC64) and ended at rest in a frontend idle at pc=185CFC cycling 00EB84/03A4A8/0021B6 (last play tick=23980, insn=265.8M, still advancing when Lola closed it / the log ends live).
- VERDICT: with {0022E2, 017A6A, hot ten} capsuled post-arm and {0021B6, 185CE4} native, the standings->track-2 transition COMPLETES — the class-3 hang does NOT reproduce under EK. Against EG (all cycle native -> hang at the same press), the hang therefore implicates the capsuled set {0022E2, 017A6A, hot ten}: one of them diverges natively on the transition path (capsuling it fixes the path). No member is individually convicted — bisect within the set remains available if the hang ever needs a single name, but functionally CLASS 3 IS DEALT WITH: the game now progresses race -> standings -> track 2 -> race 2.
- Class-2 tally stands at 4 deaths (EC/ED/EE/EJ) vs 3 sails (EF/EG/EK) on the fix-config family; class 2 remains open/intermittent, deprioritised.
- NEXT per Lola's 2026-11-10 priority: the standings display bugs (PLAYER A 1st/$1000 wrong; ".0" times column) — desk-review of the standings/placement computation + time formatting. EL (#142) will be the first standings-fix build; its exact change follows the desk read.
- CORRECTION (Lola, 2026-11-10 21:37): EK did NOT finish race 2 — she EXPLODED (bike destroyed, eliminated) in race 2; maskskip n=2 is the elimination ending routing to results, not a finish. The EK entry above overcalled "SECOND race finished." What stands: race 1 finished, the class-3 transition completed, track 2 loaded and raced, and BOTH endings (finish and elimination) routed through standings to the frontend idle cleanly. Races-finished total rises by one (to 11), not two.
- PERF NOTE (Lola, 2026-11-10 21:38): EK's race 2 was much laggier — she saw roll hit 240. Log corroborates: play-line roll avg race 1 = 102.9 ms vs race 2 = 156.9 ms (sampled maxima 198.8 / 218.5; her 240 is the live peak). MECHANISM: the late arm never disarms — after the fade, {0022E2, 017A6A, hot ten} run INTERPRETED for the rest of the session, so race 2 pays the capsule tax on its hottest functions (03A4A8 alone: 147,258 latecaps) while race 1 ran them native; even the standings idle avg'd ~151. Fix shape (queued with the perf work, after the standings fix per Lola's priority): make the late set a transition-window tool — release the hot ten once racing resumes (keep the cheap frontend members), or bisect the set for the one transition member and capsule only it.

### EL (#142) — DELIVERED 2026-11-10: the standings A/B (first post-class-3 build)
- Config: EK + three table-mask extents — {014BDA,014E5C}, {014E5C,014F32}, {0155BE,0158D4} (the results-screen trio: sort / orchestrator / line builder, desk-read scout 2026-11-10). Cache diff vs EK = LABEL EL + NUMBER 142 only. VPK rr3vita-el.vpk: 2,928,272 B, MD5 4e5359d4fa0347c4286c24ba0448cb9f; both copies identical, zip clean, icon byte-identical. VELF pairs: 014BDA=1, 014E5C=2 (table + pre-existing late extent), 0155BE=1, 029190=1, 0203E8=1, step-entered pairs 0; EL stamps only; glue_eb.c in build.make; symbols present.
- Pre-registered reads: standings CORRECT on screen (right order, real tenths) -> a native divergence inside the trio's subtree; bisect within it. Standings STILL wrong -> the records are wrong upstream (race-side writer) and the hunt moves there. A capsule FAIL naming a trio member -> its framed run diverges at a named point. Class-2 death or class-3 relapse would be read as usual (the late set is unchanged from EK).
- TEST PENDING (Lola's screen is the verdict — the log cannot adjudicate display correctness).

### EL (#142) — TESTED 2026-11-10 ~22:11: INCONCLUSIVE — class-2 mid-race death; standings A/B never reached
- Breadcrumb (16,748,826 B = EH+EI+EJ+EK+EL sessions, MD5 9db3511423ba7aba67bff8f14eb794d2; copies at NAS RR3/breadcrumb.txt, Logs/breadcrumb-el.txt): EL session 1,794,024 B. NO elarm, NO maskskip, NO maskfail, 029190 never called; capsules clean (final maskstat frames=6000 tried=hit=2663); final tail tick=7539, pc=01467E — the exact class-2 signature. NOT an EL regression: EL's only delta is the standings trio, which runs only at race end and never ran.
- TALLY: fix-config family now 5 class-2 deaths (EC, ED, EE, EJ, EL) vs 3 sails (EF, EG, EK). The standings question remains untested for the third build running (EJ class 3 instrument era, EK sailed but was pre-standings-fix, EL died). Per the EK-era pre-registration, re-opening the class-2 front is LOLA'S CALL — put to her 2026-11-10 with options: (1) retest EM = EL-identical and keep the standings front; (2) pivot to class 2 properly (new instrument design first); (3) standings via the RAM-dump route (one-shot dump of 0xFF74A0/74C0/74E0 at 0155BE entry on a surviving run, no standings-code change needed to read the tables).

### FRONT SWITCH (Lola, 2026-11-10 22:19) + EM (#143) design
- Lola's call: "Let's fix these crashes first, make it stable so I can reliably start Track 2 THEN focus on fixing that bug on the results screen." CLASS 2 IS THE FRONT; standings parked until stability (reliable Track 2 starts) is achieved.
- Desk analysis for the instrument: the tail line is written after EVERY completed step (main.c, Build DQ) — so a death's final tail = state after the last completed step, and the fatal step is the NEXT one. All five modern deaths (EC/ED/EE/EJ/EL) park at guest PC 01467E, inside func_014434 (the per-frame racing function, part11:2108, dispatch [014434,0146BE)); the final RING entries share NO common last call. Death is data-dependent, inside the step that starts at 01467E.
- func_014434's direct-call children (recomp_call_func sites in its body): 014916, 014A38, 01AF6A (x3), 01B7BC, 01B8B0, 01B9B4 (x3), 01BADE (x3), 01BC78 (x3), 01C7AC (x3), 0256B2 (x2). Membership check against s_table_mask (57 extents): ONLY 01AF6A was already capsuled — cleared as sole cause (it ran interpreted in every death build). The other nine were native in every death.
- EM = EL + the nine capsuled as a set via s_table_mask (dispatch-span extents, sorted, table now 66 extents): {014916,014A38} {014A38,014BDA} {01B7BC,01B8B0} {01B8B0,01B9B4} {01B9B4,01BADE} {01BADE,01BC78} {01BC78,01BDB8} {01C7AC,01CB66} {0256B2,0256FC}. Standings trio stays (race-end only). Reads: FAIL naming a member convicts it; survival past the death band (~3900-7600 ticks) implicates the set + makes EM a stability-fix candidate; clean death with set tried=hit clears all nine → fault is in 014434's own body or the non-guest host side (coredump decode — EC's dump is a gzipped ELF CORE with THREAD_REG_INFO — becomes the next avenue).
- Also noted: 014434's first act writes 0 to 0xFF4FCE — the very word its $01467E instruction copies to 0xFF060A every frame.
- EM BUILT + VERIFIED 2026-11-10 ~22:33 (build-en/, MAKE_EXIT=0, 473 s): 2,928,176 B, MD5 7a8a42534c10ec931bfa0d8dc98a55ff (both copies identical), zip clean, icon MD5 a1e00d2b..., glue_eb.c in build.make, VELF pairs: all nine new extents exactly 1, trio intact (014E5C = 2, table+late as before), 029190/0203E8 = 1, step-entered late pairs = 0; EM stamps only; symbols present. Cache diff vs build-em = LABEL EM + NUMBER 143 only. NOTE (process): the first placement of the nine extents broke s_table_mask sort order in three places (014916/014A38 belong BEFORE the trio; 01B4EC belongs before the 01B7BC chain; 0256B2 belongs AFTER 025370) — caught by the pre-build line-based audit (a regex scan had false-flagged pairs inside comment blocks), fixed and re-verified: 66 extents, sorted, non-overlapping. Delivery: NAS + Vita (Mini pipeline) + chat.

### EM (#143) TEST — log analysis 2026-11-10 ~23:12 (ending adjudication pending Lola's report)
- Breadcrumb grabbed via the Mini pipeline: 21,011,928 B, six sessions (EH..EM), MD5 24ad2596d81da8b50d69cb0754a59582; EM session = 4,263,102 B. Copies: NAS RR3/breadcrumb.txt + Logs/breadcrumb-em.txt; VM uploads/39D12DE7....
- EM SURVIVED RACE 1 ENTIRELY: maskskip + emarm tick=13993 pc=014C18; call tick=13993 addr=029190 capsuled and survived. The historical class-2 death band (~3900-7600) was crossed without incident — per the pre-registered read, the 014434-children set's native execution is IMPLICATED (one run; variance caveat stands).
- Post-arm flow completed: frontend (00FC40/00F7C2), standings/cutscene idle at pc=18611E with framebuffer hash animating (ticks 14060-14240+), then frontend 03B228, then RACE 2 RACING: play lines at 15640/15660/15680 with pcs 0142C8/014218/0141E4, fbh changing every poll, insn advancing to 177.5M. Class-3 transition fine (late set unchanged from EK). The standings trio ran post-arm (latecaps 014E5C x1) — Lola's screen is the A/B verdict; asked.
- Capsules: tried=hit all session (final 92,323), ZERO maskfail. Children engagement (poll census): 01BADE 2046, 01C7AC 586, 01B9B4 25, 01B7BC 9, 0256B2 6, 01BC78 4 (polls are samples; tried totals are ground truth).
- Roll: pre-arm avg 108.8 ms; post-arm avg 113.0 ms; race-2 samples 115-207 ms (children capsules + still-armed late set both tax race 2; latecaps 03A4A8 n=68,137 by the end).
- THE LOG ENDS mid-race-2: final tail tick=15682 pc=0146AE — NOT the 01467E park shared by all five class-2 deaths (EC/ED/EE/EJ/EL). No FAIL, no freeze stamp evidence in-log. Crash-vs-closed cannot be adjudicated from the log; Lola asked directly. If she closed it: EM is a stability candidate and the class-2 front narrows to bisecting the children set + the late-set race-2 release. If race 2 crashed: class 2 persists in race 2 under the children set, at a different step boundary (0146AE, inside func_014434 near its end).

### EM VERDICT (adjudicated 2026-11-10 23:17) + grab-wording convention
- Lola's convention, stated 23:17: "If I say breadcrumbs only, I closed it or it hung/froze. If I say it crashed, it crashed and you'd get both breadcrumbs and the dump file in data folder." EM was reported as "Breadcrumbs!~" only -> NOT a crash. The log shows race 2 actively racing at its end (fbh changing, insn advancing, no frozen tick, no FAIL) -> not a hang either. VERDICT: EM SAILED; Lola closed it herself mid-race-2. The 014434-children set stands implicated as the class-2 fix candidate (1 sail, 0 deaths on the set; variance caveat noted — confirmation by repetition is EN's job).
- Standings A/B: trio ran post-arm but Lola has not reported the screen; verdict still open, carried into EN (trio stays capsuled).
- EN (#144) design: EM config + the EK perf fix — the late set's HOT TEN are released back to native once racing resumes post-arm (the arm never disarming taxed EK's race 2 to roll ~157 and EM's to 115-207). Release trigger (main.c): after the late arm, count steps whose guest PC sits in [0x014000,0x015000); at 300, set g_late_released and log "enrel". The cheap frontend members {0022E2, 017A6A} and the fade-step set stay capsuled post-arm, so the class-3 transition coverage is unchanged. Race 2 in EN = race 1's conditions (children capsuled, hot ten native) — reads: another full sail strengthens the children verdict and gives a race-2 speed read; a race-2 death with enrel logged implicates a released hot-ten member; a race-1 death breaks EM's implication (variance).
- EN BUILT + VERIFIED 2026-11-10 ~23:27 (build-eo/, MAKE_EXIT=0, 477 s): MD5 73c0b13489decffb2636d41d3163d8d4 (both copies identical), zip clean, icon MD5 a1e00d2b..., glue_eb.c in build.make, VELF pairs: nine children exactly 1, trio intact (014E5C = 2), 029190/0203E8 = 1, step-entered late pairs = 0, hot-ten late pairs present (release is the runtime g_late_released flag, symbol verified in the VELF); EN stamps only (enarm + enrel strings present, emarm absent). Cache diff vs build-en = LABEL EN + NUMBER 144 only. Delivery: NAS + Vita (Mini pipeline) + chat.
- EN TEST VERDICT 2026-11-11 ~12:24 (Lola: "I was able to start race 3 just fine. Breadcrumbs"): ⭐⭐⭐⭐⭐ FULL SAIL — TWO RACES FINISHED, RACE 3 STARTED. Breadcrumb 31,713,644 B, SEVEN sessions (EH..EN), MD5 5c6c20bc1093495badedd9ff9fccc897; EN session 10,701,716 B. maskskip n=1 tick=23941 (race 1) + enarm pc=014BDA; **enrel FIRED tick=25269 pc=0146AE** — the hot-ten release works as designed; maskskip n=2 tick=35970 (race 2 finished); race 3 racing at log end (last play tick=37660 pc=0146AE, insn=425.4M advancing; final tail tick=37672 pc=0146AE, racing ring). Capsules tried=hit=200,192, ZERO maskfail. Children polls: 01BADE 2256, 01C7AC 642, 01B9B4 35, 0256B2 22, 01B7BC 17, 01BC78 5 (014916/014A38/01B8B0 zero polls — sampling). Roll: race 1 avg 97.1 (max 152); arm→enrel stretch 152.3; race 2 post-enrel avg 137.5 (max spike 403); race 3 avg 134.6. The release helped (EK race 2 was 156.9, EM samples to 207) but race 2/3 still pay the always-armed late set's non-hot members (latecaps polls dominated by 017996 ×2532 and 013088 ×2053 post-arm) plus the children capsules every race carries. Breadcrumbs-only per her convention = she closed it mid-race-3. **Children set now 2 sails / 0 deaths (EM+EN), three race finishes + a race-3 start across the two builds; Lola's stability criterion (reliably start Track 2) is exceeded — she started Track 3. Class 2 stands answered by the children set as a fix, pending her call on whether stability is declared and the front switches to the standings bug (her stated order).** Standings A/B screen verdict STILL unreported.
- FRONT CALL 2026-11-11 ~12:48 (Lola): stability testing ongoing indefinitely with every build; ACTIVE FRONT = PERFORMANCE. "Roll" is renamed "ms" (frame time) in reporting. Target ~60 ms or lower IF the important logging stays; results screen AFTER a much more playable state.
- EO (#145) DESIGN (desk-read of src/main.c phase-3 loop + glue_eb.c, 2026-11-11): per racing frame the loop did 2–3 SD-card fopen/fprintf/fclose cycles (tail every frame — kept; capsule poll on EVERY step where tried changed = every racing frame; latecaps poll likewise post-arm). Present path is a cheap 2x blit (<1 ms) — not the hog. Capsule checkpoint is a small struct copy — not the hog. The sim step itself is the bulk (native baseline was already ~95–105 ms pre-children). EO changes exactly two overhead knobs on the proven EN config: (1) capsule + latecaps polls SAMPLED 1-in-10 (totals stay exact via maskstat every 1200 frames; maskfail/maskskip/arm/enrel/freeze/play/tail UNSAMPLED, immediate as before); (2) IRQ-poll yield dose 1/16 → 1/32 in glue_eb.c (EB proved 1/16 crash-neutral). Children set, trio, late set, enrel trigger: untouched. Pre-registered reads: ms race-1 avg vs EN's 97.1 — a solid drop says overhead was real; no change says sim-bound (next perf step is engine-side desk work: audio-synth skip / glue fast paths, the parked perf-pass-proper); ANY new death/freeze pattern → dose is suspect #1, EP reverts to 1/16; another sail adds to the ongoing stability tally. Diagnostic ask: Lola reads the SIM number off the on-screen overlay during race 1 (splits sim cost from loop overhead, which the breadcrumb cannot).
- EO BUILT + VERIFIED 2026-11-11 ~12:58 (build-ep/, MAKE_EXIT=0, 460 s): 2,928,063 B, MD5 82b747dd0dcb9fa7990643f36bab8221 (both copies identical), zip clean, icon MD5 a1e00d2b..., glue_eb.c in build.make with the 1-in-32 dose (& 31u) in the shipped snapshot, VELF pairs: children exactly 1, trio intact (014E5C = 2), 029190/0203E8 = 1, step-entered late pairs = 0; EO stamps only (eoarm + enrel strings present, enarm absent); g_late_released symbol present. Cache diff vs build-eo = LABEL EO + NUMBER 145 only. Delivery: NAS + Vita (Mini pipeline) + chat.
- EO DELIVERY COMPLETED 2026-11-11 ~15:42: Vita leg ran after Lola returned (first attempt 13:00 had timed out on an approval card, nothing ran). FileZilla on the Mini: rr3vita-eo.vpk selected + size-verified (2,928,063 B), "File transfer successful, transferred 2928063 bytes in 2 seconds" to ux0:/VPK. All three legs now done; EO test pending.

## EO (#145) TESTED 2026-11-11 ~15:54 — INCONCLUSIVE: mid-race-1 death on the new config; perf gain small
- Grab: breadcrumb 33,618,927 B, NINE boot-starts (MD5 ca404af93a8537fbc027213a6779bd4e; NAS Logs/breadcrumb-eo.txt; VM ~/uploads/771F584F-8DC1-4760-AE1A-7889328CCFA3/). Session 8 = a brief frontend-only boot (22,570 B, ticks to 986, no markers, no crash — a quick launch/close). Session 9 = EO's real run (1,882,713 B). Fresh dump grabbed per her "crashed" wording: psp2core-1794441285-0x000c9628a9-eboot.bin.psp2dmp, 299,568 B, MD5 9380d56917e59394370963d7494b494e (NAS Logs + VM uploads/3F7F4A34-...).
- Death: mid RACE 1 (no eoarm/maskskip — race never finished). Final tail tick=8166 pc=0146AE — inside func_014434 near its end, NOT the 01467E park of every class-2 death. Capsules tried=hit clean to the end (14,002; the tick-8147 tried=hit+1 is the known pipelining artifact), ZERO maskfail. Children engaged (sampled polls: 01BADE 125, 01C7AC 26, 01AF6A 21). 021BAE sighted in polls (watch item; sighting only). Poll sampling confirmed working: 384 capsule polls for a full race-1 run.
- PERF READ: ms avg 91.5 over 363 play samples (min 68.4, max 119.9; halves 90.9/92.0 flat; 31% >=100) vs EN race-1 97.1. The two EO knobs bought ~5.6 ms — real but small; the frame cost is mostly SIM-bound, as the desk pass suspected. ~60 ms will need engine-side work, not IO diet.
- VERDICT: first death on the children config — children set now 2 sails / 1 death (tally, not a conviction either way). EO's pre-registered rule fires conservatively: the death followed the dose change 1/16->1/32 (the only guest-timing knob EO touched), so EP reverts the dose to 1/16 and KEEPS the poll diet (host-side fopen sampling; guest-neutral). EP reads: sail -> dose implicated, 1/16 stays, perf moves engine-side; clean 01467E death -> class-2 variance, children theory weakens (2S/2D) and the front question returns to Lola.
- EP (#146) BUILT + DELIVERED 2026-11-11: EO config with the yield dose reverted to 1-in-16 (glue & 15u verified in the shipped snapshot), poll diet kept. 2,928,058 B, MD5 299d05b4b5b1d8581f150a60e4d86a64; children extents exactly 1 each, trio intact (014E5C=2), stamps "EP: EO POLL DIET KEPT - DOSE BACK TO 1/16" / eparm / enrel, no EO stamps. NAS + chat legs ~16:20; Vita leg delayed ~15 min by a computer-control outcome_unknown lock (a click interrupted by a service restart; read-only snap showed FileZilla cleanly disconnected, nothing partial; chat messages did not clear it — Lola's message sent FROM THE MINI cleared it). Uploaded 16:36: "File transfer successful, transferred 2928058 bytes in 1 second" to ux0:/VPK. EP reads: sail -> dose implicated, 1/16 stays and perf goes engine-side; clean 01467E death -> class-2 variance, children 2S/2D, front question returns to Lola.
- EP (#146) TESTED 2026-11-11 (crashed mid race 1): grabbed breadcrumb 34,534,043 B / 10 sessions (MD5 93cafb31e096a2dad6c3dbaa0f8be418; NAS RR3/Logs/breadcrumb-ep.txt) + FRESH dump psp2core-1794444222-0x0003752e29-eboot.bin.psp2dmp (306,720 B, MD5 38848081f3dd8dfcf760899791074d64). EP session (915,116 B): NO eparm/maskskip/enrel; final tail tick=4428 pc=0146AE — the SAME atypical park as EO, not the classic 01467E; capsules clean tried=hit (3,893), ZERO maskfail; children engaged; ms avg 90.6 (dose revert cost nothing vs EO 91.5). VERDICT: (1) THE DOSE IS CLEARED — EP died at 1-in-16 exactly as EO did at 1-in-32. (2) The only delta EO+EP share that EM+EN lacked is the CAPSULE/LATECAPS POLL DIET (1-in-10 fopen sampling): diet builds 0 sails / 2 deaths; full-poll builds (EM, EN) 2 sails / 0 deaths. Prime suspect: the diet — full-rate polls throttle the main loop between steps; sampling frees it ~10x and may unmask a latent host-timing race. (3) Children tally strictly 2S/2D but confounded by the diet; children theory neither confirmed nor cleared. EQ proposed: EN-identical config (diet reverted, dose 1/16) — a sail convicts the diet; a clean 01467E death convicts variance and properly weakens the children theory.
- EQ (#147) BUILT + DELIVERED 2026-11-11: the EN-config retest (Lola approved via options tap). main.c = EN's own snapshot file with EQ stamps only — verified by diff: EN->EP delta was exactly stamps + the two diet blocks, so the diet is absent by construction; cache diff vs build-eq = LABEL+NUMBER only; glue & 15u. 2,927,873 B, MD5 15f4df4caf76e6ab9a94b39a36ca7bae; children exactly 1 each, trio intact (014E5C=2), stamps "EQ: EN CONFIG RETEST - POLL DIET REVERTED" / eqarm / enrel, no stale markers. All three legs done (Vita: "File transfer successful, transferred 2927873 bytes in 1 second"). EQ reads: sail -> diet CONVICTED (full polls stay; perf moves engine-side); clean 01467E death -> genuine class-2 variance, children theory weakens properly (2S/2D unconfounded); another 0146AE death -> the park is not diet-specific either and the front reopens.
- EQ (#147) TESTED 2026-11-11 ("pick up that breadcrumb" — breadcrumbs-only: closed or hung, no crash): grabbed 38,395,826 B / 11 sessions (MD5 14ab3a7ffe380e57f8c81feb717efccb; NAS RR3/Logs/breadcrumb-eq.txt). EQ session (3,861,783 B): RACE 1 FINISHED — maskskip tick=13374 + eqarm pc=014C08; capsules clean (maskstat 25,832 tried=hit; final poll tried=hit+1 = known pipelining artifact), ZERO maskfail. Session ends ~940 ticks post-arm in the frontend idle (tail pc=03AAF6 cycling 03A4A8/024BC8/024BF6/024BDE; insn still creeping at last play lines) — enrel never fired, race 2 never started. Ms avg 110.9 overall (race-1 half ~106) vs EN 97.1 / diet builds ~91: full polls cost ~10-15 ms. VERDICT: THE POLL DIET IS CONVICTED on the designed A/B — diet builds died in race 1 twice (EO, EP, both 0146AE); the diet-free EN config finished race 1 (EQ), matching EM/EN. Children-config tally without the diet: 3 sails / 0 deaths (EM, EN, EQ); with the diet: 0/2. Full polls stay permanently; perf work moves engine-side. OPEN: whether Lola closed EQ at the standings/frontend or it hung there — her report decides (log pattern matches a screen at rest, EF-style).
- CORRECTION (same evening, Lola's report + re-read): EQ's race 2 DID start. Post-arm timeline: standings idle 18611E (13440-13720) -> her button press (input tick=13721 pad=0010) -> transition flow -> RACING at pc=0146AE/0142C8 from tick ~14040 to ~14280 (13 racing play samples), then back to a frontend idle (03A4A8/024Bxx cycle) where the log ends at 14312. enrel never fired because only ~260 racing ticks elapsed post-arm (< the 300 counter). My first read ("race 2 never started") was wrong — I over-weighted the final frontend tail and the missing enrel and glossed the racing play lines. EQ is a FULLER sail than reported: race 1 finished, transition completed, race 2 started and raced. Diet conviction strengthened. Also noted: final frontend polls show a persistent tried=hit+1 offset across ticks 14309-14312 (zero maskfail, game functional) — unexplained, logged as a curiosity, not a failure claim.

## ER (#148) — profile build (perf front instrument), 2026-11-11
- DESK PASS FIRST: per-access glue overhead sized and dismissed — a racing frame is only ~11k guest instructions, so accessor/watchdog/ring overhead is sub-ms; no accessor lever. gbus decode for ROM/RAM is already direct. spin_check/HYBRID cycles are scheduler load-bearing, untouched.
- ER = EQ config EXACTLY (children set, trio, late set, enrel, dose 1/16, full polls) + phase timers on genesis_sim_step's own hooks in main.c only: pre_raster (after glue_run_game_frame) and post_drain (after machine_run_frame + drain), armed by the phase-3 loop around its step call; display_present timed separately. Buckets in microseconds, printed as "prof game=.. mach=.. rest=.. pres=.. n=1200" with the maskstat line every 1200 frames, then reset. No guest-visible change; hooks were unused (NULL) before.
- PRE-REGISTERED READ: biggest bucket names ES's lever (game -> native/interp/capsule cost; mach -> machine/VDP render path; rest/pres material -> host-side work). ER also adds a stability-tally run on the EQ config.
- BUILD: configured in build-es/ from build-er's cache (LABEL=ER, NUMBER=148; cache diff LABEL+NUMBER only); snapshots build-es/*.es-source. 2,928,160 B, MD5 d60e13155b236a8b60b44c26c27c0a3b, verified on disk (ER stamps only, erarm, prof line format, prof symbols + g_late_released present, nine children pairs exactly 1, trio intact (014E5C pair=2), 029190/0203E8=1, step-entered pairs=0, icon byte-identical, zip clean). Delivered all three legs 2026-11-11 (Vita: "File transfer successful, transferred 2928160 bytes in 1 second").
- NOTE: the backgrounded make finished at 19:15 but its completion never surfaced (nohup inside a drained exec); Lola's 19:43 "How's it coming?" prompted the disk check that found it done. Lesson restated: check disk state, don't trust silent background processes.

## ER test — results screen photo (Lola, 2026-11-12 ~12:10)
- ER RAN on hardware: bottom stamp "ER: PROFILE BUILD - PHASE TIMERS. NO PERF CHANGE", BUILD ER #148, TICK 14298 at the Belladonna results/taunt screen -> a race FINISHED on the EQ config (stability tally +1 finish; run outcome otherwise pending her word / breadcrumb grab).
- STANDINGS A/B EVIDENCE (first screen photo since DZ): with the trio {014BDA, 014E5C, 0155BE} capsuled, the top-3 still shows the ".0" times column on every line (1. PLAYER A .0 $1000 / 2. ODIE .0 $750 / 3. LUCKY LUC .0 $500). Whether PLAYER A 1st is wrong THIS run depends on her actual finishing position — asked, pending.
- CORRECTION to the desk pass: observed prizes are $1000/$750/$500 — NOT the 1000/600/400 ladder inferred from the ROM word table at 0x15EDC. That table does not govern these payouts (or not directly); the ladder claim is withdrawn.
- SIM/ROLL SPLIT (the long-asked overlay reading, at the results screen): SIM 36.70 ms vs ROLL 125.54 ms. At this screen most of the frame is OUTSIDE genesis_sim_step — loop-side work (polls/tail SD writes, present, pacing) dominates here, contrary to the racing-phase picture (frame ~sim-bound). ER's prof lines + a breadcrumb grab will quantify per-phase; racing-phase SIM still unmeasured.
- STANDINGS A/B VERDICT (Lola, 2026-11-12): she finished **6TH** in the ER race; the screen showed PLAYER A 1st/$1000. With the trio capsuled the standings are STILL WRONG -> the fault is NOT in the trio's native-vs-interpreted execution of the sort/render; the order data was already wrong when the trio read it (race-side writer), OR the bug lives in a non-trio screen function (e.g. 00AC48), OR the trio capsules do not actually engage for these calls (to check in the ER breadcrumb polls). Standings hunt moves to the DATA: what the race writes into 0xFF7414/74A0/74C0/74E0, and who writes the player's position. (When the standings front opens per her order — playable state first — the RAM-dump instrument at 0155BE entry is the objective first step; the mini-lane swap trace continues in its own chat.)

## ER (#148) TEST — session 12 verdict (grabbed 2026-11-12)
- Grab: Vita breadcrumb 42,558,376 B, 12 sessions (EH..ER), MD5 744ae6e2d26570e39279439245be5334; NAS Logs/breadcrumb-er.txt. ER session 4,162,550 B.
- FLOW: maskskip n=1 tick=13470 + erarm pc=014C18 (race 1 finished); enrel FIRED tick=14965 (race 2 started, hot ten released); session ends tick 15226 with the guest RACING in race 2 (she closed it, "Breadcrumbs"). Capsules tried=hit=94,217, ZERO maskfail. **ER = FULL-FLOW SAIL. Diet-free children config: 4 sails / 0 deaths (EM, EN, EQ, ER).**
- TRIO ENGAGEMENT: latecaps census has 014E5C x1 post-arm — the results chain ran capsuled; with her 6th-place finish vs the PLAYER A 1st screen, the standings A/B verdict (data-side fault) stands.
- PROF VERDICT (the build's payload): game=0.00 / mach=41.3-42.2 / rest=0.18-0.20 / pres=6.10-6.18 across all 11 samples. (1) game~0 is ARCHITECTURE, not a bug: glue_run_game_frame is only the kick — native game execution happens INSIDE machine_run_frame via the interleave scheduler, so "mach" ~41.7 ms is the entire hybrid step (machine + interleaved native game + capsule interpretation). The game/machine seam does not exist as sequential phases. (2) pres=6.1 ms — display_present is NOT sub-ms; the EO desk pass was wrong about that too. (3) HEADLINE: step total ~42 ms vs racing frame avg roll=109.8 (max 208.4) -> **~62 ms per frame is LOOP-SIDE work outside step+present — the largest single slice of the frame** (per-frame tail fopen/fprintf/fclose, capsule/latecaps poll fopen cycles, input, vblank wait — not yet split). The EO-era assumption "frame cost is mostly the sim step" is CONVICTED FALSE by direct measurement.
- CAUTION (pre-registered for ET): the poll-diet conviction's working theory says loop IO time may be load-bearing (main-loop throttle between steps). Cutting loop IO is therefore NOT automatically safe — ES first splits the loop's cost; any IO cut gets its own A/B against the stability tally.
- ES (next): loop-split profile — same EQ config + ER prof kept, plus per-frame timing of the DV poll block, the tail write, and the frame remainder (other), printed as a prof2 line with maskstat. Biggest loop bucket names ET's lever; pres (6.1 ms, NEON blit) is a known safe independent slice.

### ES (#149) — BUILT, VERIFIED, DELIVERED 2026-11-12; TEST PENDING (loop-split profile)
EQ/ER config exactly + ER's step profile kept + main.c-only loop buckets: POLLS (the counter-poll region after the step + its v1-flag clear), RING (call-ring drain + every-frame tail snapshot write), FRAME (per-iteration accumulated roll delta); printed with maskstat as `prof2 step=%.2f polls=%.2f ring=%.2f pres=%.2f other=%.2f frame=%.2f n=1200`, other = frame − step − polls − ring − pres. Stamps: purpose "ES: LOOP PROFILE - POLLS/RING TIMERS, NO PERF CHANGE", frozen "ES FROZEN - HALTED", arm marker esarm. NO PERF CHANGE by design (added work is three rdtsc-class timer pairs per frame, sub-µs); full-rate polls kept (diet convicted — never retest); dose 1-in-16. Built in build-et/ (config cache diff vs ER: LABEL+NUMBER only); source audit clean (0 comment-mentions of polls in main.c — the bucket name never appears in prose, so anchor discipline held); the chrono stub check caught that <chrono> was already in via display.h (s_prof_t0 type consistent).
- **OVERLAY RENAME folded in at Lola's request (2026-11-12 12:23)**: the on-screen stats line now reads "SIM %.2f %.2f ms" — the ROLL label is gone, frame time is the number followed by "ms". ES's main.c object was already compiled when she asked, so it landed via a 5-second incremental rebuild before delivery; the delivered binary is the rebuilt one (noted here so the MD5 below is unambiguous). Standing for all future builds.
- 2,928,493 B, MD5 36ba1fd8d80c68d77114540a0c5cc4e9 (identical both copies), zip clean. Verified on the raw VELF: purpose/frozen/esarm strings, prof + prof2 format strings, overlay string "SIM %.2f %.2f ms" present with zero "ROLL %.2f" remnants, g_late_released + prof/prof2 symbols, nine children pairs exactly 1, trio intact (014E5C pair 2), 029190/0203E8 = 1, step-entered pairs 0, glue_eb.c in build.make, icon0.png byte-identical to tree (MD5 a1e00d2b5d28e0fce61af26779860f0a).
- Delivered all three legs 2026-11-12: chat attachment, NAS RR3/VPK, Vita ux0:/VPK via FileZilla ("File transfer successful, transferred 2928493 bytes in 2 seconds").
- PRE-REGISTERED READ: the biggest prof2 bucket names ET's lever (polls vs ring/tail vs other). pres (~6.1 ms, NEON blit) stays a known safe independent slice. Any loop-IO cut gets its own stability A/B — the diet theory says that time may be load-bearing.

### ES (#149) — TESTED 2026-11-12 (TWO sessions; breadcrumb 49,394,853 B / 14 sessions, MD5 cb7020608e1b6194afa3b535c3f03200; dump psp2core-1794522656-0x0000f034fd-eboot.bin.psp2dmp, 305,200 B, MD5 0da478b489b7b94c24499124b2c22902)
- **Session 13 — DIED mid-race-1 at the 0146AE park, SLEEP/WAKE-TAINTED.** Lola's account: started playing, the Vita slept through her appointment (~2 h), she unlocked, played a little more, and it crashed shortly after. Log: final tail tick=5120 pc=0146AE, capsules tried=hit (3,574 at last maskstat), ZERO maskfail, no maskskip (race 1 never finished). NO timing fingerprint of the sleep exists in the log (max smoothed roll 141.7 — Vita system time evidently freezes in standby, so the suspend is invisible to the breadcrumb); the log can neither prove nor exonerate the sleep, but everything in it is consistent with her account.
- **Session 14 — ⭐ FULL SAIL, uninterrupted.** maskskip + esarm tick=15603 (race 1 finished), enrel tick=16724 (race 2 started, hot ten released), session ends in a post-race frontend with tried=hit=97,100, ZERO maskfail; she closed it. Her report matches: "played perfectly fine, was able to get to the next race just fine."
- **TALLY UPDATE — the 0146AE pattern sharpens:** uninterrupted runs of the diet-free children config: EM, EN, EQ, ER, ES-14 = **5 sails / 0 deaths**. Disturbed runs: EO + EP (poll diet) and ES-13 (OS suspend/resume) = **3 deaths, ALL at pc=0146AE, all capsules clean**. Working model: a latent host-timing sensitivity at that park, kept dormant by normal full-rate loop pacing; both a 10× poll cut and a suspend/resume can wake it. ES's config is NOT convicted — the same build sailed clean when run uninterrupted. (Practical corollary for testing: a Vita sleep mid-run invalidates the run as stability evidence either way.)
- **PROF2 VERDICT (the build's purpose — the loop split, racing windows):** step ~41.5, **ring ~32.2 (rock-steady in every sample of both sessions)**, pres ~6.4, other ~11.5, polls typically 5–18 racing BUT unstable — climbing across each session (S13: 4.9→7.2→15.9) and spiking to 40–62 in S14's late/frontend windows (frames 140–152). **RING — the call-ring drain + every-frame tail snapshot, pure logging IO — is the biggest loop bucket**, ahead of polls' racing norm. Polls' growth with session length/file size is a secondary warning (49 MB breadcrumb; append IO may degrade). ET's lever is RING, subject to the diet-theory caution: if loop TIME is the stabilizer, cheapening the tail is itself a disturbance — ET as designed (content-identical tail via persistent buffered FILE*, no per-frame fopen/fclose) is the A/B that tests it.

### ET (#150) — BUILT, VERIFIED, DELIVERED 2026-11-12; TEST PENDING (cheaper-tail A/B; Lola picked this fork via options tap 2026-11-12 15:28)
ES config exactly + ONE change, in main.c's ring/tail block only: the block's breadcrumb handle is now PERSISTENT (`static FILE *tail_fp`, opened once, reused) with `fflush` at block end instead of fopen/fclose per frame. Same lines, same order, same per-frame flush durability (the final tail before a crash still reaches the card before the next step); the polls region (own fopen/fclose cycles, incl. the capsule poll's fclose), the play-line breadcrumb(), and the maskstat write are all UNTOUCHED. Prof + prof2 kept, so the ring saving is directly measurable against ES's 32.2 ms. Stamps: purpose "ET: CHEAPER TAIL A/B - PERSISTENT BC HANDLE, SAME LOGGING", frozen "ET FROZEN - HALTED", arm etarm. Diff vs ES snapshot is exactly stamps+etarm+the tail_fp mechanism (audited). Built in build-eu/ (cache diff LABEL+NUMBER only).
- 2,928,542 B, MD5 e90fa38d7a91be19d09ce9e7e4095c2d (identical both copies), zip clean. Verified on the raw VELF: stamps/etarm/prof2/ms-overlay strings, children pairs exactly 1, trio intact (014E5C pair 2), 029190/0203E8 = 1, step-entered pairs 0, glue_eb.c in build.make, icon byte-identical (a1e00d2b5d28e0fce61af26779860f0a).
- Delivered all three legs 2026-11-12: chat attachment, NAS RR3/VPK, Vita ux0:/VPK via FileZilla ("transferred 2928542 bytes in 1 second").
- PRE-REGISTERED READ: if ring collapses (32.2 → low single digits) AND ET sails uninterrupted runs, the cheaper tail is a keeper and the frame drops toward ~78 ms; if ET dies at 0146AE, loop TIME is convicted as load-bearing and ES's open/close pattern is restored permanently. A mid-run Vita sleep invalidates the run as evidence (ES-13 lesson).

### ET (#150) — TESTED 2026-11-12 (session 15; breadcrumb 58,835,398 B / 15 sessions, MD5 a4aadcb181c1e9ab0c457925e3ca7d4b; NAS Logs/breadcrumb-et.txt; Vita file now over the 50 MB upload cap — grabbed to NAS first, zipped ON THE MINI per the 2026-11-11 rule: breadcrumb-et.txt.zip 3,878,006 B; no dump — Lola's word was "Breadcrumbs", she closed it herself)
- **⭐ FULL SAIL, uninterrupted — THE CHEAPER-TAIL A/B PASSED BOTH LEGS.** Flow: maskskip n=1 tick=11938 + etarm (race 1 finished); enrel FIRED tick=13752 pc=0146AE (race 2 started, hot ten released); maskskip n=2 tick=30309 (race 2 finished — **Lola WON race 2, her report; she credited the speedup**); session ends tick=31183 with the guest RACING in race 3 (pc=0146AE), she closed it. Capsules tried=hit=167,906+, ZERO maskfail.
- **RING COLLAPSED: prof2 ring avg 5.88 (max 7.64, n=25) vs ES's ~32.2 — the per-frame fopen/fclose was the cost; −26 ms/frame BANKED.** Frame: prof2 windows 67.8–82.1; play-line roll avg 75.7 (max 94.6, 1516 samples) vs ER all-samples 112.9 / ES-14 racing 112.4. **The persistent tail handle is the new standing pattern. Loop TIME in the tail is NOT load-bearing** — the diet conviction is thereby narrowed to the POLLS region specifically (write frequency/pacing there), not loop IO in general.
- **TALLY: diet-free children config, uninterrupted runs: 6 sails / 0 deaths (EM, EN, EQ, ER, ES-14, ET).**
- Her verdict (2026-11-12 19:38–20:02): "this speed is much more tolerable", "I was actually having fun finally. Managed to get first in race 2 cuz of this speed up." — the playability verdict matches the numbers.
- NEXT LEVER (from ET's own prof2): polls still climbs across the session (2.74 in the first window → 10–21 in later windows) — the capsule poll does its own fopen/fclose on EVERY capsule-counter change (nearly every racing frame) against a now-58 MB breadcrumb. EU applies the same proven pattern there (persistent poll handle + fflush per line), one variable, own A/B. step ~41.5 and pres ~6.4 and other ~10 are the remaining slices after that.

### EU (#151) — BUILT, VERIFIED, DELIVERED 2026-11-12; TEST PENDING (cheaper capsule-poll A/B)
ET config exactly + ONE change, in main.c's polls region only: the capsule poll (Build CT block — fires on every g_mask_tried change, i.e. nearly every racing frame) now writes through a PERSISTENT handle (`static FILE *poll_fp`, opened once) with `fflush` after every line, instead of fopen/fclose per change. Same line, same order, same per-write durability. The rare one-shot writers in the region (maskfail, fallback, latecaps, arm, enrel, freeze) keep their exact fopen/fclose paths — maskfail especially is the crash witness and is untouched. Tail (ET's tail_fp), play-line breadcrumb(), maskstat all untouched. Stamps: purpose "EU: CHEAPER CAPSULE POLL A/B - PERSISTENT POLL HANDLE, SAME LOGGING", frozen "EU FROZEN - HALTED", arm euarm. Source diff vs ET snapshot = stamps + euarm + poll_fp declaration + the capsule-poll block (audited line by line); dispatch mask and glue byte-identical to ET. Built in build-ev/ (cache diff LABEL+NUMBER only).
- 2,928,768 B, MD5 22b196e6cd59906123bbc2925697725e (identical both copies), zip clean. Verified on the raw VELF: EU stamps/euarm (etarm count 0), prof2 + capsule format strings, "SIM %.2f %.2f ms" overlay, late/mask symbols present, children pairs exactly 1 x9, trio 1/2/1, 029190/0203E8 = 1, step-entered pairs 0, glue_eb.c in build.make, icon byte-identical (a1e00d2b5d28e0fce61af26779860f0a).
- Delivered all three legs 2026-11-12: chat attachment, NAS RR3/VPK, Vita ux0:/VPK via FileZilla ("transferred 2928768 bytes in 1 second").
- PRE-REGISTERED READ: if polls flattens (ET: 2.7 early climbing to 10-21 late -> steady low single digits) AND EU sails uninterrupted runs, the persistent poll handle is a keeper and the frame should settle toward the low 60s; if EU dies at 0146AE, the polls region's open/close pacing itself is convicted load-bearing (distinct from the tail, which ET cleared) and ET's pattern is restored for the poll in EV. A mid-run Vita sleep invalidates the run as evidence.

### STAGED FOR EV — purpose-line marquee (Lola's request, 2026-11-12 20:41, from her EU photo)
EU's 67-char purpose text overflowed the 52-char bottom strip and ghosted (visible in her photo: the wrapped tail overdrawing the strip). Her instruction: "The description on the bottom is finally getting too long. Please make it scroll. Right to left please. Fix this in future builds." Implemented NOW in src/main.c's draw_purpose_stamp so EV inherits it: if the purpose line exceeds the strip (maxc = (960-24)/18 = 52 chars) and it is not an alert, draw a 52-char window over (line + 4-space gap) repeated, offset = (process time / 200 ms) mod (len+4) — constant right-to-left pace at any frame rate; box width caps at screen width, so no spill/ghosting. FAIL and FROZEN lines set an `alert` flag and never scroll; short lines render exactly as before. Window math host-tested (offsets 0/20/wrap verified). EU itself is UNCHANGED (already delivered; its snapshots in build-ev/ predate this edit) — EV's source diff vs EU must list the marquee as a known second delta alongside whatever EV's experiment is.

### EU (#151) TEST — 2026-11-12 ~21:00 (breadcrumb 64,084,226 B, 16 sessions EH..EU, MD5 fdba059e89c751c45247d76f4f7a2609; NAS RR3/Logs/breadcrumb-eu.txt)
Lola's word was "Breadcrumbs" — she closed it herself; NO dump grabbed. Session 16 (5,248,828 B): maskskip n=1 tick=18368 + euarm (race 1 finished); enrel FIRED tick=19383 pc=0146AE (race 2 started); session ends tick=19577 with the guest RACING in race 2 (final tail pc=01C98C, children in the ring). Capsules tried=hit at every maskstat (28,285 at frames=18000); the final capsule poll caught the known tried=hit+1 pipelining artifact at the moment she closed it (29,969/29,968) — not a failure. ZERO maskfail.
- PROF2: **polls FLAT ~2-3 ms across all 15 windows (racing 2.07-2.99)** — ET's late-session climb to 10-21 ms is GONE. One 8.19 window includes the race-transition one-shot writers. Frame: prof2 windows 59.0-68.4; play-line roll avg 66.1 (935 samples, max 88.0) vs ET 75.7. step ~41-42, ring ~6, pres ~6.5, other ~10.4 — all as ET.
- **VERDICT (pre-registered): THE CHEAPER-POLL A/B PASSED BOTH LEGS** — polls flattened AND an uninterrupted sail. The persistent poll handle is a KEEPER; the fopen/fclose-per-write pattern is now cleared on both hot writers (tail ET, capsule poll EU), and the poll-diet conviction narrows further: neither hot writer's open/close pacing was load-bearing. **Tally: 7 sails / 0 deaths uninterrupted (EM, EN, EQ, ER, ES-14, ET, EU).**
- DESK READ for the next slice (main.c phase-3 loop): "other" (~10.4) = read_pad + input-change log (rare) + step bookkeeping + the every-20-frames play-line write + the every-1200 stats writes + **sceDisplayWaitVblankStart(), which sits AFTER the frame timestamp and so lands in the next frame's "other"**. Work sums to ~57.3 ms while the period sits ~67 — right at 4 vblank periods (66.7 ms): the wait's quantisation padding is the prime suspect for most of "other", and the frame structure is a CLIFF (work < 50.1 ms -> period 50.1; < 33.4 -> 33.4), not a slope.

### EV (#152) — DESIGN (loop profile 2: split "other"; + Lola's marquee)
EU config exactly + profile-only additions in main.c: three accumulators printed with prof2 every 1200 frames as `prof3 wait=%.2f play=%.2f top=%.2f n=1200` — wait = around sceDisplayWaitVblankStart(); play = the every-20 play-line write + the every-1200 stats writes; top = loop entry (read_pad + input log) to step start. Residual = other - wait - play - top should land ~0-1 if the loop map is complete. PLUS the purpose-line marquee staged earlier (Lola's request; present-side, cost lands in pres). Stamps: purpose "EV: LOOP PROFILE 2 - WAIT/PLAY/TOP TIMERS + SCROLLING TEXT", frozen "EV FROZEN - HALTED", arm evarm.
- PRE-REGISTERED READ: if wait ~= 8-11 of other's ~10.4, vblank quantisation is convicted as the bulk of "other" and the perf strategy becomes "drive total work under the 50.1 ms cliff" (step-side work + pres), with any change to the wait itself treated as a host-pacing A/B needing its own stability read (the poll-diet lesson). If wait ~= 0-2, play/top/residual names the real tenant. EV changes no pacing, no writers, no guest code — a death on EV would be a marquee/profile artefact hunt, not a config signal.

### EV (#152) — BUILT, VERIFIED, DELIVERED 2026-11-12; TEST PENDING (loop profile 2 + marquee)
EU config exactly + profile-only prof3 (wait/play/top accumulators, printed with prof2 every 1200 frames) + Lola's purpose-line marquee (staged earlier; present-side, cost lands in pres). No writer, pacing or guest changes — the wait is TIMED, not touched. Source diff vs EU snapshot = stamps (EV purpose/frozen/evarm) + marquee hunks + prof3 hunks (53 changed lines, audited); dispatch mask and glue byte-identical to EU. Built in build-ew/ (cache diff LABEL+NUMBER only).
- 2,929,175 B, MD5 8797b04459a90b78636bcf1d32962bc6 (identical both copies), zip clean. Verified on the raw VELF: EV stamps/evarm (euarm count 0), prof3 + prof2 + capsule format strings, "SIM %.2f %.2f ms" overlay, late/mask symbols present, children pairs exactly 1 x9, trio 1/2/1, 029190/0203E8 = 1, step-entered pairs 0, glue_eb.c in build.make, icon byte-identical (a1e00d2b5d28e0fce61af26779860f0a). The clean compile also validates the marquee code.
- Delivered all three legs 2026-11-12: chat attachment, NAS RR3/VPK, Vita ux0:/VPK via FileZilla ("transferred 2929175 bytes in 1 second").
- Read: prof3 wait/play/top vs prof2's other (~10.4). wait ~= 8-11 convicts vblank quantisation; residual (other - wait - play - top) should land ~0-1 if the loop map is complete. EV's purpose text is 59 chars (>52), so Lola will see the marquee scroll on this build.

### EV (#152) TEST — 2026-11-12 ~21:44 (breadcrumb 69,267,773 B, 17 sessions EH..EV, MD5 a4d6b93165360c93c9e0cbde00a02839; NAS RR3/Logs/breadcrumb-ev.txt)
Lola's word was "Breadcrumbs" — she closed it herself; NO dump grabbed. Session 17 (5,183,547 B): maskskip n=1 tick=17940 + evarm (race 1 finished); enrel FIRED tick=19026 pc=0146AE (race 2 started); session ends tick=19401 in a post-race frontend flow; capsules tried=hit=25,650, ZERO maskfail. **EV SAILED — tally 8 sails / 0 deaths uninterrupted.** Frame avg 66.2 (927 samples) — EU's pace held; the marquee cost nothing measurable (pres 6.46 vs EU ~6.5).
- **PROF3 VERDICT: wait = 8.67-11.20 across all 15 windows, avg ~9.5 — the vblank wait IS "other" (~10.47).** play ~0.52, top ~0.35, residual ~0.3: THE LOOP MAP IS COMPLETE, every ms of the frame is now accounted for (step 41.00 + polls 2.51 + ring 5.77 + pres 6.46 + play 0.52 + top 0.35 + wait 9.5 + residual ~0.3 = frame 66.21). Work ~56.9 pads to 66.7 = exactly 4 vblank periods — **the quantisation model is CONFIRMED by direct timing**, not inference. The frame is a CLIFF at vblank multiples: work < 50.1 -> period 50.1; < 33.4 -> 33.4.

### EW (#153) — DESIGN (chase the 50.1 ms cliff: one-write tail + wide blit)
EU/EV config + prof/prof2/prof3 kept + TWO work cuts, both pure output-side mechanics (no pacing change, no writer removed, no guest change; the wait is untouched and will simply absorb less):
1. RING (~5.8): the every-frame tail line is 34 separate fprintf calls (header + 32 entries + newline). EW builds the identical line in a stack buffer and issues ONE fwrite (+ the same end-of-block fflush). Byte-identical output. Expected ring -> ~1-2.
2. PRES (~6.5): display_present's 2x upscale does 4 scalar 32-bit stores per source pixel (320x224). EW packs each pixel into one 64-bit store per output row (2 stores/pixel; row pointers verified 8-byte aligned: (48+y*2)*960*4 and 160*4 are multiples of 8). Identical pixels out. Expected pres -> ~3-4.
- Expected work: ~56.9 - ~4 (ring) - ~3 (pres) ~= 49.9 — RIGHT at the 50.1 cliff edge. PRE-REGISTERED READ: frame ~50 (3 vblanks) + uninterrupted sail = both cuts keepers, next cliff 33.4 needs step-side work (different league). Frame stays ~66 with ring/pres down and wait up ~15 = cuts banked as headroom, EX tips it over (polls formatting next). A 0146AE death would convict ring/pres host work as load-bearing — restore per-region and rethink.

### EW (#153) — BUILT + DELIVERED 2026-11-12
EV config + prof/prof2/prof3 kept; changes main.c-only (audit: 41 lines, three hunks): (1) the every-frame tail line built in a 384 B stack buffer and written with ONE fwrite (was 34 fprintfs), byte-identical, same end-of-block fflush; (2) display_present's 2x blit packs each source pixel into ONE 64-bit store per output row (2 stores/px, was 4; row pointers verified 8-byte aligned); (3) stamps EW / ewarm. Built in build-ex/: 2,928,980 B, MD5 2815e824f2de23fe6932c15e78680dd3; verified on the VELF (stamps, ewarm=1/evarm=0, tail format, prof/prof2/prof3 format strings present with literal n=1200, children 1x9, trio 1/2/1, 029190/0203E8=1, step-entered 0, glue_eb.c in build.make, icon byte-identical); delivered chat + NAS RR3/VPK + Vita ux0:/VPK (FileZilla "transferred 2928980 bytes in 2 seconds"). TEST PENDING. Read per the pre-registration in the EW design entry.

### EW (#153) TEST — 2026-11-12 ~22:09 (breadcrumb 70,089,334 B, 18 sessions EH..EW, MD5 6a27bfd270e771c7b560eb9903bab7d7; NAS RR3/Logs/breadcrumb-ew.txt; dump psp2core-1794550141-0x00040137f5-eboot.bin.psp2dmp, 301,328 B, MD5 ce155715959e431660ea8cbf08d9daec)
Lola's report: "crashed" — breadcrumb + fresh dump grabbed. Session 18 (821,561 B): **DIED mid-race-1**, final tail tick=3820 pc=**0146AE**, no maskskip/ewarm, capsules tried=hit=3,740, ZERO maskfail. The exact diet-death signature on a diet-free build.
- THE CUTS: pres 6.46 -> **3.34** (the wide blit worked, -3.1 ms). Ring UNCHANGED (6.16 vs EV 5.77 — noise): the one-write tail is timing-neutral, as expected at the syscall level (34 fprintfs into a fully-buffered FILE + one fflush == one fwrite + fflush: same bytes, same single write). Polls 2.34 flat. Frame avg **61.2** (was 66.2); wait 7.2-7.4 (did NOT grow to hold 66.7 — the clean cliff model is an approximation; observed period = work 53.7 + wait 7.3 = 61.0).
- **READ (per the pre-registration): the only timing delta vs EV is pres -3.1 / frame -5. The tail change is a proven no-op (same bytes, same flush, same ring time). So the trigger is the blit/present change or the period drop it caused.** Death roster at 0146AE now: EO + EP (poll diet), ES-13 (suspend/resume), EW (wide blit). Sails EM/EN/EQ/ER/ES-14/ET/EU/EV all ran pres ~6.5 scalar blit, frame >= 66.1. ET/EU remain the counterexamples to any simple "loop got faster" theory (they cut file open/close time and sailed) — the tripwire's exact physics is still unknown, but empirically the safe zone is the EV loop shape at ~66 ms. **Tally: diet-free children config 8 sails / 1 death (EW).**

### EX (#154) — DESIGN (per-region restore A/B: EW minus the wide blit)
EW config EXACTLY except the blit reverts to the scalar 4-store loop (the pre-EW form). The one-write tail STAYS (proven timing-neutral, byte-identical). Stamps EX / exarm; prof/prof2/prof3 kept. PRE-REGISTERED READ: EX sails -> the wide blit (or the -5 ms period drop it caused) is convicted; pres stays scalar and the perf front treats the present path as defended ground. EX dies at 0146AE -> the tail no-op stands accused alongside pure variance, and the front restores EV exactly and freezes the loop.

### EX (#154) — BUILT + DELIVERED 2026-11-12
EW config with ONLY the wide blit reverted to the scalar 4-store loop (audit: 20 lines, stamps + blit hunk; the one-write tail kept — proven timing-neutral; dispatch mask + glue byte-identical to EW). Built in build-ey/: 2,929,327 B, MD5 577f544c0f722c7e665791cbed043fa0; verified on the VELF (EX stamps/exarm=1/ewarm=0, prof/prof2/prof3 strings, tail format, children 1x9, trio 1/2/1, 029190/0203E8=1, step-entered 0, glue_eb.c in build.make, icon byte-identical); delivered chat + NAS RR3/VPK + Vita ux0:/VPK (FileZilla "transferred 2929327 bytes in 1 second"). TEST PENDING. Read per the pre-registration in the EX design entry.

### EX TEST — 2026-11-12 (session 19; Lola's word: "Breadcrumbs" — she closed it)
Breadcrumb 75,061,267 B (EH..EX), MD5 7093fd396742d90fc27442bc542c3771, NAS Logs/breadcrumb-ex.txt; session 19 = 4,971,933 B. ⭐ FULL SAIL, uninterrupted: maskskip n=1 tick=17778 + exarm (race 1 finished); enrel FIRED tick=18508 (race 2 started, hot ten released); session ends tick=18686 with the guest RACING in race 2; capsules tried=hit (24,622 at the last maskstat), ZERO maskfail. prof2: pres back to ~6.3 (scalar blit restored), step ~41-42, polls flat, ring ~6, other ~10.5, wait ~9.5; racing frame ~66-67, play-line avg 66.0 — indistinguishable from EV/EU. ⭐ VERDICT per the pre-registration: EW'S WIDE BLIT — OR THE FRAME-PERIOD DROP IT CAUSED — IS CONVICTED. The one-write tail is exonerated twice (timing-neutral in EW, present in sailing EX). TALLY: 9 sails / 1 death uninterrupted for the diet-free config; the EV loop shape (scalar blit, pres ~6.5, frame ~66 ms) is the defended configuration. PERF STATE: every loop-side bucket is at its floor with the important logging kept (step ~41.7, polls ~2.6, ring ~6, pres ~6.3, wait ~9.5); the only remaining lever is step-side work, and any period-moving change is now proven tripwire-adjacent — next move is Lola's call.

### EY (#155) — BUILT + DELIVERED 2026-11-12 (in-step profile; Lola chose "attack the sim step")
EX config + prof4 ONLY: three clock reads per scanline inside machine_run_frame split the step's ~42 ms into chunk (begin_scanline + glue_run_game_chunk), mid (latched V-int + step_z80 + IRQ delivery + deferred-VDP flush) and rend (gvdp_render_scanline + ARGB + sink), printed with the prof lines as "prof4 chunk/mid/rend/rest" every 1200 frames (rest = mach - chunk - mid - rend). FILE-TARGET CATCH: the first attempt instrumented src/genesis_machine_noz80.c and failed at LINK — the shipped config compiles the ENGINE's original genesis_machine.c (${ENGINE}/runner/video/), NOT the src/ variants (noz80/stages are old bisect options, like sim_step_noaudio.c vs the engine sim_step.c). The engine original differs from the noz80 copy by exactly the 5-line NOZ80 step_z80 short-circuit; the scanline loop is identical. Fix per the glue_eb.c house pattern: src/genesis_machine_prof4.c = engine file + timer hunks only (diff-audited, 23 lines), selected by a new -DRR3_PROF4=1 (CMakeLists branch added ahead of RR3_NOZ80); src/genesis_machine_noz80.c restored byte-identical. Cache diff vs EX = LABEL+NUMBER+PROF4. Built in build-ez/: 2,929,773 B, MD5 159d89ad2a0a8ff5136f705823952979; verified on the VELF (EY stamps/eyarm=1/exarm=0, prof4 format string, prof4 symbols linked, children 1x9, trio 1/2/1, 029190/0203E8=1, step-entered 0, glue_eb.c + genesis_machine_prof4.c in build.make, icon byte-identical); delivered chat + NAS RR3/VPK + Vita ux0:/VPK (FileZilla "transferred 2929773 bytes in 1 second"). TEST PENDING. Read: profile-only — expect frame ~66 and a sail; the prof4 split names EZ's target (chunk -> drill into the glue chunk, native vs capsule time; rend -> VDP render work; mid -> Z80/IRQ path). A 0146AE death on a pure-instrument build would indict the per-line clock reads themselves.

### EY TEST — session 20, grabbed 2026-11-12 (breadcrumb 77,434,574 B, MD5 d28d11fadfca8a7ae1f47d22f92787ce; NAS Logs/breadcrumb-ey.txt; dump psp2core-1794554633-0x0004e02199-eboot.bin.psp2dmp, 301,536 B, MD5 b928c3d2f511122f6f923129c800d198)
Lola: "Dang, crashed" -> breadcrumb + fresh dump grabbed. Session 20 (2,373,307 B): DIED mid-race-1, final tail tick=9404 pc=0146AE, NO maskskip/eyarm (race 1 never finished), capsules tried=hit=14,014, ZERO maskfail. prof2: step ~42.4, polls ~2.3, ring ~6.0, pres ~6.4, frame ~67.5 — the loop shape and frame period are the DEFENDED ones (frame marginally HIGHER than EX's 66.0). The ONLY delta vs EX is prof4's ~786 clock-read syscalls per frame INSIDE the step. VERDICT (pre-registered read): the in-step timer reads themselves are CONVICTED as the perturbation — the 0146AE park is sensitive to host cadence inside the step, not just to loop shape or frame period. Death roster at 0146AE: EO+EP (poll diet), ES-13 (suspend/resume), EW (wide blit/period drop), EY (in-step syscalls). TALLY: 9 sails / 2 deaths. Per-line syscall instrumentation is a dead end; frame-granularity timers (ER-style) remain the safe instrument.
PROF4 ITSELF DELIVERED (7 clean windows before the death, rock steady): chunk=~4.5 ms (68K game chunk), mid=~2.6 ms (Z80 + IRQ + VDP flush), rend=~30.7 ms (VDP scanline render + ARGB conversion + sink), rest=~4.3 ms (largely the timer reads' own cost). THE RENDER IS THE STEP — ~73% of the ~42 ms; the 68K execution is only ~4.5 ms. Any rend cut is a step-timing change and therefore a formal stability A/B under the EW/EY lesson; the defended EX config (~66 ms) remains the fallback island.

### PARK DIG (2026-11-12, Lola's call: "dig into why the park is timing-sensitive") — desk investigation
Read: func_014434's tail (generated/rr3_part11.c), sim_step.c, glue_eb.c (yield/vblank/interrupt/watchdog paths), and EY's psp2core fully parsed (ELF notes, THREAD_REG_INFO, MODULE_INFO, readelf-symbolicated VELF).
FINDINGS: (1) The "park" is func_014434's per-frame physics bookkeeping: it samples the substep counter at 0xFF4FCE (incremented inside the physics functions) around back-to-back calls to the capsuled children and stores deltas at 0xFF060A/0610/0616. Steps suspend there because that is where racing cycles are spent; it is the capsule-densest stretch of racing code. (2) The step is a SEALED deterministic unit: fixed cycles per machine frame, NO wall-clock reads inside (the "2-wall-frame" safety net and both watchdogs count guest cycles/instructions, not time). Host timing cannot steer the guest through any path in the sim core. (3) EY's dump: 3 threads (main, rr3_sim, SceCommonDialogWorker); the sim thread's saved PC sat inside SceLibKernel with no fault PC in our module — the shape of a libc/kernel-detected abort (heap-corruption class), not a wild jump in emulator code. Our binary loads at its VELF addresses (no rebase); psp2core omits clean file-backed text pages, so stack scans cannot recover the fault chain (the sim stack region in the dump is bulk heap data).
WORKING THEORY (supersedes "timing sensitivity" as the frame): deaths are per-BUILD, not per-run — EO/EP/EW/EY died in race 1 every run; EQ..EX sailed every run. A latent memory bug whose detonation depends on the binary's data layout fits: each rebuild shuffles BSS addresses, so "bad layout" builds die deterministically and "good layout" builds sail. Runner-up seam if layout fails: the atomic V-int delivery (full g_cpu swap + handler run between chunks, possibly against a capsule-suspended fiber) — still cycle-deterministic, so it cannot explain per-build reliability on its own. EZ is the discriminator.

### EZ (#156) — BUILT + DELIVERED (2026-11-12)
EX config EXACTLY + one inert 4 KB BSS array (s_layout_probe_ez, __attribute__((used)), never read or written) in src/main.c — a pure data-layout probe, zero behaviour change; stamps EZ/ezarm. No prof4 (engine genesis_machine.c back in the build), scalar blit, one-write tail, all masks/glue byte-identical to EX (audit clean; cache diff = LABEL+NUMBER only). 2,929,107 B, MD5 b64521c8543666056fbfed638e4983c3. PROBE VERIFIED on the VELF: g_ram, g_cpu and g_mask_tried all shifted by exactly +0x1000 vs EX; symbols before the probe unmoved. Delivered chat + NAS + Vita (FileZilla "transferred 2929107 bytes in 1 second").
PRE-REGISTERED READ: EZ dies at 0146AE -> layout CONVICTED, the hunt becomes finding the overflow (renderer/sink and other heavy guest-driven writers first). EZ sails -> layout weakens (one probe is one layout sample); fall back to the capsule/interrupt seam with a layout series (multiple probe sizes) as the follow-up discriminator.

### EZ TEST (2026-11-13, reported "Breadcrumbs" — she closed it herself)
Grab: Vita breadcrumb 81,031,587 B (22 boot-starts; session 21 was an 81-byte boot blip, session 22 is the run, 3,596,932 B); NAS Logs/breadcrumb-ez.txt (+ .zip 5,297,518 B), MD5 82beb28e0f1c22475f08d7b4eb889fc9. Vita copy NOT deleted.
SESSION 22: FULL SAIL, uninterrupted. maskskip n=1 tick=11694 + ezarm pc=014C0E (race 1 finished); enrel FIRED tick=13036 pc=0146AE (race 2 started); session ends tick=13258 RACING in race 2 (she closed it). Capsules tried=hit=24,187 at the final poll, ZERO maskfail. Frame: play avg 68.8 (617 samples); prof2 frame avg 68.85, step 41.70, pres 6.50 — the defended EX shape; the probe cost nothing measurable.
VERDICT (pre-registered read, applied as written): EZ SAILED -> the LAYOUT THEORY WEAKENS. A verified +4 KB shift of g_ram/g_cpu/g_mask_tried did not flip a known-good build bad. One probe is one layout sample — layout is not cleared — but the simplest form of the theory (any layout change reshuffles the dice) took the hit: EZ is a pure data change vs EX and sailed like EX. Pattern that survives reassessment: both deaths (EW wide blit, EY prof4) changed CODE executed in the hot loop/step; the one build changing only data addresses sailed. Deaths track hot-code changes, which alter the deterministic step's cycle accounting (chunk-boundary alignment of capsule suspensions vs the atomic V-int delivery) — the capsule<->interrupt seam returns to prime suspect. TALLY: 10 sails / 2 deaths (sails EM, EN, EQ, ER, ES-14, ET, EU, EV, EX, EZ; deaths EW, EY).

### FA (#157) — BUILT + DELIVERED (2026-11-13)
EZ config EXACTLY (inert BSS probe kept, so EZ is the exact base) + ONE change: the VDP renderer flips from the scalar per-pixel path to the engine's own BATCHED path (GENESIS_BATCHED_PLANES=1: span fetch + LUT layer-select, maintained in genesis_vdp.c alongside the scalar path; our CMakeLists had it pinned to 0 since forever — now an RR3_BATCHED option). Stamps FA/faarm. Cache diff vs EZ = LABEL+NUMBER+RR3_BATCHED only. 2,929,886 B, MD5 fb18b7bac7fc36e83b10cd07d46ef7f4. VERIFIED: VELF carries "BATCHED" (SCALAR absent — the mode-name function's scalar string was dead-stripped), FA stamps, faarm x1, all mask extents at expected counts, glue_eb.c x13, icon identical. Delivered chat + NAS + Vita ("transferred 2929886 bytes").
CONTEXT: prof4 put rend (render + ARGB convert + sink) at ~30.7 of the ~42 ms step — the last big lever. The batched path attacks the per-pixel fetch cost inside gvdp_render_scanline. If rend drops ~10+ ms, step falls under ~32 and total work under ~50.1 -> the frame period should snap to the 50.1 ms plateau.
PRE-REGISTERED READ: FA sails + frame at ~50 = the batched renderer is a keeper and becomes the standing config; ~66 = the cut didn't bite (rend is dominated by something else — convert/sink — and we split further). FA dies at 0146AE = the renderer flip joins EW/EY as a convicted hot-code perturbation; revert to the EX/EZ shape and the seam theory hardens. ALSO NEW FAILURE MODE TO WATCH: wrong pixels (batched/scalar divergence) — Lola's eyes are the verdict; any visual corruption = revert regardless of speed.

### FA TEST (2026-11-13, reported "Breadcrumbs" — she closed it herself)
Grab: Vita breadcrumb 85,729,783 B (23 boot-starts; session 23 = FA's run, 4,698,196 B); NAS Logs/breadcrumb-fa.txt (+ .zip 5,611,233 B), MD5 585fd9268d7591236172b2ac1e6a04dc. Vita copy NOT deleted.
SESSION 23: FULL SAIL, uninterrupted. maskskip n=1 tick=15078 + faarm pc=014BEA (race 1 finished); enrel FIRED tick=16704 (race 2 started); session ends tick=17226 RACING in race 2 (she closed it). Capsules tried=hit (27,757 at last maskstat), ZERO maskfail.
PERF: prof2 avgs (13 windows) step 25.69 (EZ: 41.70 — the batched renderer cut ~16 ms from the step), polls 3.45, ring 5.98, pres 6.38, other 10.65, FRAME 52.16; play-line avg 52.3 (816 samples, max 76.0) vs EZ's 68.8. The frame sits at the 50.1 ms plateau edge (work ~41.6 ms = 3 vblank periods, occasional spill to 4).
VERDICT (pre-registered read): FA SAILED + FRAME AT THE ~50 PLATEAU -> THE BATCHED RENDERER IS A KEEPER and becomes the standing configuration. TALLY: 11 sails / 2 deaths.
THEORY REFINEMENT: FA was a large hot-code change inside the step and sailed — "hot-code change" alone does not kill. The renderer consumes no guest cycles (host-only work), so capsule suspension points in guest-cycle space are identical between EZ and FA; the seam alignment never moved. The two deaths (EW: present-path/period change; EY: host syscalls interleaved inside the step) both perturbed host timing structure around the guest, not guest compute. Pixel correctness: no visual complaint from Lola with the report (her eyes remain the verdict — flagged to her explicitly).
NEXT CLIFF: 33.4 ms needs total work < 33.4 (currently ~41.6): ~8 ms across step remainder (rend now ~14-15: convert + sink loops) and loop side (pres 6.4 is the largest single loop bucket; the wide blit's conviction is due a retest on the FA base now that a period drop alone is proven survivable).
FA PICTURE CHECK CLOSED (2026-11-13 13:55, Lola): asked whether the batched renderer's picture looked right, she answered "You'd know if the game wasn't looking right lol" — the picture was correct; the pre-registration's pixel item is affirmatively settled, not merely un-complained-about.

### FB (#158) BUILT + DELIVERED (2026-11-13) — 33.4 ms cliff chase, step 1
Design: FA config EXACTLY (batched renderer, layout probe kept) + TWO cuts, each independently measured by the prof2 split (step vs pres):
(1) PALETTE PRE-SWAP: new patched machine copy src/genesis_machine_preswap.c (house pattern, -DRR3_PRESWAP=1) — colour_cb stores palette colours pre-swapped into display order (A8B8G8R8); the WS-bar sentinel is opaque black (swap-invariant); the host-palette branch (never enabled in this build) swaps after its copy for correctness. main.c sink_row is now a plain memcpy — the per-pixel R/B swap (71,680/frame) is gone.
(2) WIDE BLIT RETEST: display_present back to 64-bit packed stores (2/px). EW died with this blit but EW also dropped the period 66->61; on the FA base the period is 50.1 either side (work ~41.6 -> ~38.5), so the blit stands or falls ALONE this time.
Built in build-fc/ (cache diff vs build-fb = LABEL+NUMBER+RR3_PRESWAP only; sources snapshotted *.fc-source). VERIFIED: 2,929,570 B, MD5 ddac12589440b2ec006b7899014ddcc3 (both copies identical, zip clean). VELF: FB purpose x1, FB FROZEN x1, fbarm x1, faarm x0, mode string "BATCHED" x1 with "SCALAR" x0 (FA's x2 count included FA's own purpose text), preswap machine x13 + glue_eb x13 in build.make, children 1x9, trio 1/2/1, 029190 x1, 0203E8 x1, layout probe symbol present, icon MD5 a1e00d2b... Delivered all three legs (FileZilla "transferred 2929570 bytes in 1 second").
PRE-REGISTERED READ: FB sails + pres ~3.3 + step down a little = both cuts keepers, WIDE BLIT EXONERATED (EW's death then belongs to the period drop/interaction). FB dies at 0146AE = FC splits the cuts (FA + preswap only): FC sails -> blit convicted outright; FC dies -> preswap convicted. Expected work ~36-37 ms: the 50.1 plateau hardens (fewer 66.7 spills) but 33.4 likely needs one more rend-core idea after FB's numbers. FB tests will be Vita breadcrumb sessions 24+.

### FB TEST (2026-11-13, reported "Dang, crashed mid race")
Grab: Vita breadcrumb 86,651,920 B (24 boot-starts; session 24 = FB's run, 922,137 B); NAS Logs/breadcrumb-fb.txt (+ .zip 5,671,912 B), MD5 d2a9cf76f5238150503b8ea4bd3107c2. Dump psp2core-1794614384-0x00022c3605-eboot.bin.psp2dmp, 299,104 B, MD5 bb089e5f6140ca42824da5941bb1ceab (NAS Logs + VM). Vita copies NOT deleted. (Grab had two hiccups: the Mini dropped offline briefly mid-grab and recovered; a Finder approval card timed out once and was re-asked with Lola watching.)
SESSION 24: DIED mid-race-1. Final tail tick=4207 pc=0146AE; NO maskskip/fbarm/enrel; capsules tried=hit (3,099 at last maskstat), ZERO maskfail. The exact tripwire signature.
BOTH CUTS WORKED AS DESIGNED (prof2, 2 windows before death): step 24.62 (FA 25.69 — preswap shaved ~1 ms), PRES 3.33 (FA 6.38 — the wide blit bit, -3.05 ms), ring 6.64, polls 1.90, other 8.52, FRAME 45.01; play avg 45.4 (165 samples, max 57.9). Note the frame sat BELOW the clean 50.1 plateau at work ~36.4 — the vblank quantisation is a fuzzy edge (phase drift), as EW's 61.2 also showed.
VERDICT (pre-registered read): FB DIED AT 0146AE -> THE SPLIT RUNS. FC = FA + preswap only, wide blit reverted. FC sails -> wide blit CONVICTED OUTRIGHT (second death, this time with no period confound to hide behind). FC dies -> preswap convicted. TALLY: 11 sails / 3 deaths.
THEORY NOTE: the death roster (EO/EP poll diet, EW blit+period, EY in-step syscalls, FB preswap+blit) vs sails at frames 52-69 (FA 52.2, EX 66, EZ 68.8) still has no clean period threshold (EY died at frame 67.5) — every death changed host code in or around the step/present path; the capsule/interrupt seam remains the working model.

### FC (#159) — BUILT + DELIVERED 2026-11-13 (the split, leg 1)
FB's pre-registered split: FA config EXACTLY + the palette pre-swap ONLY — src/genesis_machine_preswap.c via -DRR3_PRESWAP=1 and the sink memcpy kept; the wide blit REVERTED to the scalar 4-store loop in display_present. Batched renderer kept (-DRR3_BATCHED=1), layout probe kept. Stamps: purpose "FC: PRESWAP ONLY - BLIT REVERTED, SPLIT A/B" (43 chars — static), frozen "FC FROZEN - HALTED", arm fcarm. Sources snapshotted as build-fd/*.fd-source; configured in build-fd/ (31 -D args; cache diff vs build-fc = LABEL+NUMBER only). Tracked make exit 0 (514 s).
VERIFIED on disk: 2,929,474 B, MD5 2c3106f41d757e05547d0b8279dd3c50 (build-fd copy and your_files/rr3vita-fc.vpk identical; zip clean, 4 entries: param.sfo 912, eboot.bin 1,723,735, rr3.bin 2,097,152, icon 10,347). VELF: FC purpose x1, FC FROZEN x1, fcarm x1, FB stamps x0, "BATCHED" x1 with "SCALAR" x0, genesis_machine_preswap.c x13 AND glue_eb.c x13 in build.make, layout-probe symbol present, children 1x9, trio 1/2/1, 029190 x1, 0203E8 x1, icon MD5 a1e00d2b... OK.
PRE-REGISTERED READ: FC sails -> the WIDE BLIT is CONVICTED OUTRIGHT (deaths in EW and FB, both with it; preswap then stands as a keeper). FC dies at 0146AE -> the PRESWAP is convicted (revert it; the blit is then retestable later on a clean base). Pixel check: the preswap is the only pixel-semantics change still in — any visual wrongness, revert it regardless. EXPECTED NUMBERS if healthy: pres back to ~6.4 (scalar blit), step ~24.6 (preswap's cut kept), frame ~48-50. FC's tests will be Vita breadcrumb sessions 25+.

### FC TEST (2026-11-13, reported "Breadcrumbs" — she closed it herself; no dump)
Grab: Vita breadcrumb 91,659,893 B (25 boot-starts; session 25 = FC's run, 5,007,973 B); NAS Logs/breadcrumb-fc.txt (+ .zip 5,991,022 B), MD5 27c14010e2ce8df5f3929ee8414634d1. Vita copy NOT deleted.
SESSION 25: FULL SAIL, uninterrupted. maskskip n=1 tick=16672 + fcarm pc=014C18 (race 1 finished); enrel FIRED tick=17955 pc=0142C8 (race 2 started); session ends tick=18596 in race-2 flow; capsules tried=hit=28,770 at final poll, ZERO maskfail.
NUMBERS (prof2, 14 windows): step 24.93 (the preswap's cut held — FB read 24.62), pres 6.56 (scalar blit, exactly as predicted), polls 2.77, ring 5.87, other 10.33, FRAME 50.46; play avg 50.7 (884 samples, max 68.9).
VERDICT (pre-registered read, applied as written): FC SAILED -> THE WIDE BLIT IS CONVICTED OUTRIGHT. It died in both builds that carried it (EW #153, FB #158) and no build without it has died on this config line. THE PRESWAP IS EXONERATED AND IS A KEEPER. FC (= FA batched renderer + preswap + layout probe) IS THE NEW STANDING CONFIGURATION: frame ~50.5, step ~24.9. TALLY: 12 sails / 3 deaths.
PATTERN TABLE (all five deaths changed BETWEEN-STEP or PRESENT-path host timing: poll diet EO/EP, wide blit EW/FB, in-step clock reads EY; all step-INTERNAL compute changes sailed: batched renderer FA, preswap FC, BSS layout EZ).
PERF STATE: work ~40.2 ms (step 24.9 + polls 2.8 + ring 5.9 + pres 6.6) on the 50.1 plateau. The 33.4 cliff needs ~7 ms more, and the only bucket big enough is the VDP render core inside the step (rend remainder ~13-14: batched core ~10 + convert gather ~2-3). The wide blit is convicted, so present is floored at ~6.6 on the CPU path. Next perf step = render-core surgery (e.g. direct-ARGB batched rendering to skip the convert gather) — a design fork for Lola, alongside banking FC and opening the results-screen fix (mini-lane synthesis queued).

### FD (#160) — ARGB render fusion (BUILT + DELIVERED 2026-11-13, test pending)
Lola picked "render-core surgery: chase 33.4 ms" after FC's sail. Desk read of the pixel path: four passes over every pixel per frame (span fetch -> composite -> separate index->ARGB convert loop in the machine -> sink copy). FD deletes the convert pass: src/genesis_vdp_argb.c (engine genesis_vdp.c + gvdp_render_scanline_argb, a clone of the batched composite whose final store goes through the palette LUT into a uint32 row) via -DRR3_ARGB=1 in the VDP_SRC chain; src/genesis_machine_argb.c (= preswap copy + argb render-block hunk; NULL-sink replays still use the untouched index render) via the MACHINE chain. Pre-swap kept, batched kept, probe kept, blit scalar. Sources snapshotted build-fe/*.fe-source; cache diff vs build-fd = ARGB+LABEL/NUMBER only.
VERIFIED: 2,930,891 B, MD5 34f639e5a26e757dab41b48ce93dbf55; copies identical; zip clean; VELF: FD purpose x1, FD FROZEN x1, fdarm x1, FC stamps x0, BATCHED x1/SCALAR x0, gvdp_render_scanline_argb symbol present, probe present; children 1x9, trio 1/2/1, 029190 x1; build.make cites genesis_machine_argb.c + genesis_vdp_argb.c + glue_eb.c x19 each; icon MD5 a1e00d2b5d28e0fce61af26779860f0a. Delivered all 3 legs (chat, NAS RR3/VPK, Vita ux0:/VPK — FileZilla receipt 2,930,891 B in 1 s).
PRE-REGISTERED READ: FD sails + step down ~1.5-3 ms (to ~22-23.5) + pres ~6.6 unchanged = fusion is a keeper; the remaining 33.4 gap then lives in the composite loop itself (span-composite rewrite = the bigger second incision). FD dies at 0146AE = the fusion is convicted (revert to FC's config; the split would separate VDP clone vs machine hunk). ANY VISUAL WRONGNESS -> revert the fusion first; it is the only pixel-semantics change. Honest framing given to Lola up front: this cut alone likely does NOT crack 33.4 (work ~40 -> ~37-38); it is the safe first incision and its number sizes the second.

### FD TEST (2026-11-13) — DIED mid-race-1 at 0146AE; FUSION CONVICTED per pre-registration
Lola reported "Crashed". Grab: breadcrumb 92,724,007 B (26 boot-starts), NAS Logs/breadcrumb-fd.txt + zip 6,065,192 B, VM copy MD5 b97a68e7051ccf8e4d83d70c984c31a0; dump psp2core-1794620491-0x0002b722d9-eboot.bin.psp2dmp, 273,504 B, MD5 (see daily log) — newest in ux0:/data, paired by recency; Vita copies not deleted. SESSION 26: died mid-race-1, final tails at pc=0146AE (ticks 4675-4677), zero maskfail, capsules tried=hit=6,367 at the final poll, no fdarm (race 1 never finished). PROF2 (3 windows, short session): step 25.4 avg (24.63/25.67/25.81), pres 6.48, frame 51.1 — THE STEP DID NOT DROP vs FC's 24.93; flat-to-slightly-worse.
VERDICT (pre-registration applied as written): the fusion fails BOTH keeper tests — it died at 0146AE AND it bought no step time. THE ARGB RENDER FUSION IS CONVICTED; revert to FC's configuration (batched renderer + preswap, no argb). TALLY: 12 sails / 4 deaths.
THEORY UPDATE: FD is the FIRST death from a purely step-internal compute change — the clean pattern (deaths = host-timing/present-path changes only; step-internal changes sail) is BROKEN. FA's batched renderer changed the same hot path and sailed; FD's fusion changed it and died with zero gain. What now discriminates deaths from sails is not obvious: candidates remain binary/codegen layout of the hot loop, or an as-yet-unseen defect in the fusion itself (Lola's picture report pending — visual wrongness would point at a real bug; a clean picture points at the tripwire). Either way the fusion is out: no speed, one death.

### FE (#161) — composite span rewrite (BUILDING 2026-11-13)
Lola picked "FC + composite-loop span rewrite" after FD's conviction. FE = FC config (batched renderer, preswap machine, layout probe, scalar blit) + ONE change: src/genesis_vdp_span.c via -DRR3_SPAN=1 (new VDP_SRC branch after ARGB). The batched composite loop is replaced by a RUN walker: maximal runs of pixels sharing the same layer-select key (and, in S/H mode, the same sprite operator bytes) emit as a masked copy from the winning layer; each pixel's key is computed exactly once (the probe that ends a run hands its key forward). bgdiag keeps a per-pixel loop; the scalar (non-batched) path keeps the original loop verbatim under #else.
HOST DIFFERENTIAL PROOF (new method, keep it): a harness fabricated 800 randomized VDP states (400 pure-random VRAM, 400 low-entropy structured; H40/H32, S/H on/off, window both ways, hscroll 0/2/3, 2-cell vscroll, plane sizes 32/64/128, IM2 cases, random sprites/backdrops) and rendered full frames with the original and span TUs — outputs BYTE-IDENTICAL in both batteries (30.8 MB compared per battery). The picture is proven safe on the host before the Vita build exists.
HOST TIMING CAVEAT (told to Lola with delivery): x86 -O2 runs the span version ~3% faster on structured data, ~40% slower on adversarial random data — the host cannot prove an A9 win; the composite may not be the fat left in rend (FA's 16 ms was probably mostly the span FETCH). FE is therefore a probe as much as a cut.
PRE-REGISTERED READ: FE sails + step down >= ~2 ms = the composite had real money in it, keeper, and the fused fetch+composite renderer is the next swing. FE sails + step flat/up = composite is done; revert, aim next at fetch/sprites or bank FC. FE dies at 0146AE = span rewrite convicted. Visual wrongness = revert (unlikely by construction per the differential proof; her eyes still rule).

### FE DELIVERED 2026-11-13
2,930,517 B, MD5 86920e304e393d0e0b9ab93fe48dad51. Built in build-ff/ (sources snapshotted as *.ff-source); cache diff vs FC exactly LABEL/NUMBER/SPAN. Verified on disk: FE purpose/frozen/fearm stamps x1 each, FD stamps x0, BATCHED x1 / SCALAR x0, build.make cites genesis_vdp_span.c + genesis_machine_preswap.c + glue_eb.c (argb x0), children pairs 1x9, trio 1/2/1, {029190,029678} x1, s_layout_probe_ez present, gvdp_render_scanline grown to 3,140 B, icon MD5 a1e00d2b..., zip clean, your_files copy MD5-identical. Delivered all three legs: chat attachment, NAS RR3/VPK/, Vita ux0:/VPK/ via FileZilla on the mini (receipt: transferred 2930517 bytes in 1 second; first click landed a row off on rr3vita-fd — corrected by Up-arrow + status-bar check before Enter). Test pending; session 27+.

### FE TESTED 2026-11-13 — FULL SAIL, step -0.97 ms (under the keeper bar)
"Breadcrumbs" (she closed it). Grab: Vita breadcrumb 97,579,446 B (27 boot-starts); NAS Logs/breadcrumb-fe.txt + zip 6,385,946 B; VM copy MD5 46263b8873b2a7688d4608ec6262181e. No dump fetched (not a crash, per her wording). Vita copy not deleted.
Session 27 (4,855,439 B): maskskip n=1 tick=14526 + fearm pc=014C0A (race 1 finished); enrel tick=16183 (race 2 started; the poll-window PC read 0146AE at that instant — a hot racing address, NOT a death; the run continued 1,600+ ticks past it). Session ends tick=17792 in live latecaps flow, capsules still cycling. ZERO maskfail. Final poll tried=94383 hit=94382 — the 1-gap is a poll landing mid-capsule-run (tried increments at entry, hit at exit; the 17791 poll was tried=hit), not a miss: no maskfail, no freeze, game live at close.
Prof2 (14 windows): step 23.96, polls 3.71, ring 5.95, pres 6.53, other 9.85, frame 50.00; play avg 49.8. Vs FC: step 24.93 -> 23.96 = -0.97 ms; frame 50.46 -> 50.00 (still the 50.1 plateau; work ~40.0).
VERDICT vs pre-registration: the >=2 ms keeper bar is NOT met, and "flat/up" is not met either — the composite had ~1 ms of real money in it (~4% of step), not the ~7 the cliff needs. Grey zone the pre-registration didn't price. Recommendation put to Lola: keep FE (sail, small real gain, pixel-exact by proof, biggest capsule sample yet at 94k) and take the fused fetch+composite swing from here — it replaces this composite anyway and attacks the fetch/buffer side where FA's evidence says the money was. Alternatives: revert to FC and stop render surgery (bank ~50 ms), or the small fetch-hoist swing. TALLY: 13 sails / 4 deaths.

### FF (#162) — fused fetch+composite renderer (BUILDING 2026-11-13)
Lola's call after FE's verdict: keep FE, take the fused swing. FF = FE config (batched renderer, preswap, layout probe, scalar blit, FE's run-walk composite) + ONE change: src/genesis_vdp_fused.c via -DRR3_FUSED=1 (new VDP_SRC branch after ARGB). The two-phase render (fetch whole line into plane_a/plane_b buffers, then composite) becomes one pass: each fetch segment (<=16 px, bounded by 2-cell vscroll columns / window edge, exactly the segments the original fetch loop already used) is composited immediately by composite_segment() — FE's run walker with segment-relative plane indexing — so the packed plane bytes never round-trip through full-line buffers (~143 KB/frame of buffer stores+loads gone). The widescreen diagnostic path (s_ws_bgdiag, never enabled by the port) falls through to the original two-phase flow verbatim; the scalar path is untouched.
HOST DIFFERENTIAL PROOF: three-way — original engine TU vs FE span TU vs fused TU — byte-identical in BOTH batteries (400 randomized states each; random-VRAM and structured-VRAM; 31,104,000 B compared per battery). Scalar (BATCHED=0) build of the fused file also compiles. Host timing again neutral (orig 0.394 vs fused 0.401 on structured) — noted honestly: FE's walker also showed no host win yet paid ~1 ms on the Vita; prof2 arbitrates.
PRE-REGISTERED READ: FF sails + step down >= ~2 ms from FE's 23.96 = fused is a keeper and the standing config; the 33.4 cliff likely still needs one more idea after this (step would be ~22, work ~38). FF sails + step flat (<1 ms either way) = buffer round-trips weren't the cost either; renderer is done, bank FE/FF shape and re-aim (sprites/fetch inner loop) or stop the chase. FF dies at 0146AE = fusion convicted, FE stands.
FF DELIVERED 2026-11-13: 2,931,057 B, MD5 35173e22bda7d00dfc3419d1930b9b1b — verified on disk (FF stamps/fused provenance in build.make, children 1x9, trio 1/2/1, 029190 x1, layout probe in, icon MD5 ok, zip clean). gvdp_render_scanline grew to 3,484 B (fused path + verbatim diag fallback). Delivered all three legs (chat + NAS RR3/VPK + Vita ux0:/VPK, FileZilla receipt "transferred 2931057 bytes"). One approval-card timeout mid-delivery (remote-path Return) was retried on Lola's "Ask" go-ahead. TEST PENDING (sessions 28+).

FF DELIVERED 2026-11-13 (build-fg): 2,931,057 B, MD5 35173e22bda7d00dfc3419d1930b9b1b. Verified: FF purpose/frozen/ffarm x1 each, FE stamps x0, BATCHED x1/SCALAR x0, build.make cites genesis_vdp_fused.c + genesis_machine_preswap.c + glue_eb.c x13 each (span x0, argb x0), children 1x9, trio 1/2/1, 029190 x1, s_layout_probe_ez present, gvdp_render_scanline 3,484 B (fused + diag paths), icon a1e00d2b..., zip clean, your_files copy identical. Delivered all three legs: chat attachment, NAS /Volumes/NAS/RR3/VPK/, Vita ux0:/VPK (FileZilla receipt "transferred 2931057 bytes in 3 seconds"). Lola picked FF herself after FE's test (options tap 19:56): FE kept, fused taken. TEST PENDING — next session will be 28.

### FF TEST (2026-11-13) — DIED mid-race-1 at 0146AE; FUSED RENDERER CONVICTED on both legs
Lola reported "Crashed". Grab: breadcrumb 98,985,122 B (28 boot-starts), NAS Logs/breadcrumb-ff.txt + zip 6,470,740 B, VM copy MD5 5112fd8596a8aeac7e2389ee1a2; dump psp2core-1794632224-0x0003e03d1d-eboot.bin.psp2dmp, 274,672 B, MD5 c5fdd2e5721273a7c9d287856cd703df — newest in ux0:/data, paired by recency; Vita copies not deleted. Grab ran as ONE computer session (cc-87) per the new ask-once rule — no mid-grab prompts.
SESSION 28 (1,405,676 B — short): DIED mid-race-1, final tails ticks 6012-6014 at pc=0146AE; ZERO maskfail; capsules tried=hit=6,396 at the last maskstat (frames=4800); no ffarm/maskskip/enrel (race 1 never finished). PROF2 (4 windows): step 22.13/25.08/25.09/23.34 = avg ~23.91 vs FE's 23.96 — FLAT (Δ ≈ -0.05 ms); frame ~48.6.
VERDICT (pre-registration applied as written): FF fails both keeper legs — it died at 0146AE AND bought no step time. THE FUSED FETCH+COMPOSITE RENDERER IS CONVICTED; FE stands as the standing config (step ~24, frame ~50). TALLY: 13 sails / 5 deaths.
PATTERN UPDATE: both render FUSIONS (FD argb convert-fusion, FF fetch+composite fusion) died at 0146AE with zero speed gain; both NON-fusing renderer changes (FA batched planes, FE span composite — each keeps the engine's two-phase fetch-then-composite structure) sailed. Two data points, same shape: merging phases in the VDP render is convicted as a CLASS, not just two builds. The plane-buffer round-trips (~143 KB/frame) measurably cost nothing on the Vita — the render's ~24 ms step is not memory-traffic-bound in the places cut so far.

### FG (#163) — sprite-loop pass on FE (BUILDING 2026-11-13)
Lola's call after FF's conviction: FG = sprite-loop pass on FE (structure stays, no fusion). FG = FE exactly (span composite, preswap, batched, layout probe, scalar blit) + ONE change: src/genesis_vdp_spr.c via -DRR3_SPR=1 (new VDP_SRC branch after SPAN). Only the batched pixel loop inside sprite_render_line changes; walk order, x==0 masking, pixels_budget, the four per-line memsets, the scalar path, the write pattern (idx/op/hi only on drawn pixels) and the composite are byte-for-byte FE's.
THE PASS: (1) fully transparent cell rows (bits == 0) skipped whole; (2) per-cell clipping (fx_lo/fx_hi) replaces the per-pixel bounds test; (3) nibble extraction specialised by hf inline — non-hf shift 28-4fx, hf shift 4fx (algebraically the original's (fx ^ (hf ? 7 : 0))); (4) the S/H operator test hoisted per sprite into an op_ops loop variant (can only fire when sh_mode && pal == 3); (5) the first-wins test reads s_spr_op instead of drawn[] — file-wide audit: s_spr_op is written ONLY in the branch that set drawn[sx]=1 (both paths), both are memset to 0 at the top of the same call, and nothing else writes s_spr_op, so op == drawn at every test point; drawn[] stays for the scalar path.
HOST DIFFERENTIAL PROOF: harness rebuilt with THREE batteries (400 states each): 1 random VRAM, 2 structured VRAM, 3 crafted SAT (80 chained sprites — clustered Y, collision cluster, x==0 masking sprite, off-edge X, pal-3 operators forced on half, flips, transparent tile band, H40/SHI and H32/no-SHI alternating, IM2 cases) with sprite_overflow/sprite_collision folded into the compared stream per line. Engine orig vs FE span vs FG spr vs spr-scalar (BATCHED=0): BYTE-IDENTICAL in all three batteries (~31.8 MB each). Picture and flags proven safe pre-build.
PRE-REGISTERED READ: FG sails + step down >= ~1.5 ms from FE's 23.96 = sprite loop had real money, keeper. FG sails + step flat (<1 ms) = sprite loop wasn't the money either; bank FE shape, renderer avenues narrow to fetch internals or stop. FG dies at 0146AE = sprite pass convicted, FE stands. Desk estimate: sprite pixel iterations are ~25-50k/frame in a racing scene; the pass might plausibly take 0.5-1.5 ms — this may land under the keeper bar like FE did; prof2 arbitrates.
FG DELIVERED 2026-11-13 (build-fh): 2,930,791 B, MD5 72a12a833968e5810e0e0b696f3e549c. Verified on disk: FG purpose/frozen/fgarm x1 each, FF/FE stamps x0, build.make cites genesis_vdp_spr.c + genesis_machine_preswap.c + glue_eb.c x13 each (span/fused/argb x0), children 1x9, trio 1/2/1, {029190,029678} x1, s_layout_probe_ez present (4096 B), gvdp_render_scanline 3,226 B, BATCHED x1, icon MD5 a1e00d2b..., zip clean, your_files copy MD5-identical. Cache diff vs FE exactly LABEL/NUMBER/-SPAN/+SPR. Delivered all three legs: chat attachment, NAS RR3/VPK/, Vita ux0:/VPK (FileZilla receipt "transferred 2930791 bytes in 2 seconds"). FIRST delivery under the new ask-once discipline: batched combo navigation (no Quickconnect ritual — plain navigation reconnected, per Lola's tip), one continuous pass, no timeout, no mid-delivery check-ins. TEST PENDING — next session will be 29.
FG TEST 2026-11-13 (session 29, grab per her "Breadcrumbs" — closed in live flow, no dump): FULL SAIL. Breadcrumb 103,074,465 B (29 boot-starts), NAS Logs/breadcrumb-fg.txt + zip 6,755,183 B, VM copy MD5 76b2fc1dd82bfa43004303a7b5191060. Session 29: 4,089,333 B; maskskip n=1 + fgarm tick=12730 pc=014C18 (race 1 finished); enrel tick=13913 (race 2 started); zero maskfail; capsules tried=hit=90,397; final tails tick 15053 in live varied flow. Prof2 (11 windows): step 24.09, polls 3.56, ring 6.08, pres 6.46, other 9.88, frame 50.07; play avg 50.0.
VERDICT (pre-registration applied): sails + step FLAT (+0.13 vs FE 23.96, inside window noise) — THE SPRITE LOOP WASN'T THE MONEY EITHER. FG is output-identical to FE and sailed; it stands as current (= FE in effect). Renderer scoreboard: FA batched fetch -16 ms (the only big win); FE composite -0.97; FG sprite ~0; FF fusion 0 + died; FD fusion 0 + died. The step's remaining ~24 ms is not in composite, sprites, or buffer round-trips; the only unprobed rend component is the plane FETCH internals. Work ~40.2 ms total; the 33.4 cliff needs step <~17 ms. TALLY: 14 sails / 5 deaths.
Grab mechanics note: first grab under ask-once discipline — batched navigation (reconnect by navigation only), batched fresh-click+Compress worked first try; whole chain one pass.

## FH (#164) — fetch+sink pass, last renderer piece (chosen by Lola 2026-11-13 22:01)
Design (desk read of spr VDP + preswap machine): the remaining renderer surface is exactly two loops. (1) fetch_plane_span's per-pixel nibble loop (variable shifts, ~143k px/frame across both planes). (2) The machine sink gather rowbuf[x] = s_cram_argb[idxbuf[x]] (71,680 gathers/frame, inside the step's rend bucket). Everything else in the fetch is already hoisted (row_base per call, one name read + 4 pattern bytes per cell, vram_read_word tiny/static).
FH = FG config + both loops, structure preserved, no fusion:
- src/genesis_vdp_fetch.c (copy of spr file): fetch_plane_span gains a full-cell fast path (fx==0 && n==8): the 4 pattern bytes expand pairwise; H-flip reverses byte order and swaps nibbles within each byte. Partial cells keep the original loop verbatim. Sprite pass (FG) and span composite (FE) ride unchanged inside the same file.
- src/genesis_machine_fetch.c (copy of preswap): the sink gather unrolled x4 + scalar tail; element-wise identical.
- Selected by -DRR3_FETCH=1 in both CMake chains (FETCH branch ahead of SPR in VDP chain, ahead of PRESWAP in MACHINE chain; PRESWAP flag stays set — the fetch machine file is the preswap lineage). Stamps: purpose "FH: FETCH+SINK PASS - LAST RENDERER PIECE", frozen "FH FROZEN - HALTED", arm fharm.
Proof plan: VDP harness 4-way (orig/span/spr/fetch) byte-identical in the 3 FG batteries; sink unroll proven by a host unit test (scalar vs unrolled verbatim loop bodies over random rows, lengths 0..517 + full widths).
PRE-REGISTERED READ: FH sails + step down >=1.5 ms from FG's 24.09 = keeper (cliff still needs step <17 — out of reach of this lever; any follow-on is Lola's call). FH sails + flat (<1 ms) = RENDERER CLOSED — composite, sprites, fetch, sink all probed flat after FA; bank the 50 ms frame; perf front moves off the renderer or ends. FH dies at 0146AE = pass convicted, FG stands.
Desk estimate: fetch expansion ~0.5-1 ms, sink unroll ~0.5-1.5 ms; combined plausibly 1-2.5 ms — keeper bar is reachable but not likely; prof2 arbitrates.
FH PROOF (2026-11-13, pre-build): VDP harness 4-way (engine orig / FE span / FG spr / FH fetch) BYTE-IDENTICAL in all 3 batteries (hashes b1 93ff08be..., b2 5a7e401a..., b3 0380dd5c... — same streams as the FG run). Sink unroll unit test: scalar vs unrolled verbatim, identical over 3,000 random rows (n 0..1024). Fetch TU compiles scalar (BATCHED=0) clean. Cache diff vs FG exactly LABEL/NUMBER/-SPR/+FETCH. Build running in build-fi.
FH DELIVERED 2026-11-13 (build-fi): 2,930,799 B, MD5 ffa3293cfe7fa3b40b276ca45248b5d6. Verified on disk: FH purpose/frozen/fharm x1 each, FG stamps x0, build.make cites genesis_vdp_fetch.c + genesis_machine_fetch.c + glue_eb.c x13 each (spr/span/fused/argb/preswap-machine x0), children 1x9, trio 1/2/1, {029190,029678} x1, s_layout_probe_ez present, gvdp_render_scanline 3,226 B, icon a1e00d2b..., zip clean, your_files copy identical. Cache diff vs FG exactly LABEL/NUMBER/-SPR/+FETCH. Delivered all three legs: chat, NAS RR3/VPK/, Vita ux0:/VPK (receipt "transferred 2930799 bytes in 1 second"). TEST PENDING — next session will be 30.
Delivery-discipline note: Lola flagged the asks again during this delivery ("damn all these asks") — the ~5-min grant still lapsed across the chain. Tightened flow for next time: bare-minimum call chain (activate, 2 batched combos, click, Return), verify selection from click receipts/status OCR instead of extra screenshot reads, skip the session-end call (sessions expire on their own).
FH TEST 2026-11-13 (session 30, grab per her "Breadcrumbs" — closed in live flow, no dump): FULL SAIL. Breadcrumb 107,561,652 B (30 boot-starts), NAS Logs/breadcrumb-fh.txt + zip 7,042,052 B, VM copy MD5 eb3a22abe42420dcb2ee982dc3784ba9. Session 30: 4,487,177 B; maskskip n=1 + fharm tick=15755 pc=014C0A (race 1 finished); enrel tick=16659 (race 2 started); zero maskfail; capsules tried=hit at final maskstat; final tails tick 16830 in live race-2 flow. Prof2 (13 windows): step 24.04, polls 2.61, ring 5.89, pres 6.51, other 10.02, frame 49.08; play avg 49.0.
VERDICT (pre-registration applied): sails + step FLAT (-0.05 vs FG 24.09) — THE RENDERER IS CLOSED. Composite, sprites, fetch, and sink all probed after FA's batched-fetch win (-16 ms); none holds the ~7 ms the 33.4 cliff needs (step would have to fall under ~17 ms). FH stands as current (= FG in effect, output-identical). The 50 ms frame is banked. TALLY: 15 sails / 5 deaths.
Grab note: the remote FileZilla list re-sorted by mtime (breadcrumb now 3rd row); the first coordinate click took interior_label_misses.toml and a corrected click took floor_coverage.prev.toml before the offset click + dialog verification landed breadcrumb — two tiny strays (878 B + 2,040 B) now sit in NAS RR3/ root alongside their Logs copies; harmless. Lesson stands: screenshot-verify the row before Return, every time — list order is volatile.

## FI (#165) — standings watch build (front opened by Lola 2026-11-13 22:38; her ordering: standings fix, then any other bugs found, AUDIO LAST)
Question (from the parked mini-lane synthesis): who writes a FINISHER's +98 (race time) during a race? Statically the only write32 to +98 is func_01E0AC's correction onto DNF-gated records; records start each race with +98 = stale RAM (func_015B40 never writes it); builders func_01A718/01A86E copy +98 verbatim; ".0" everywhere <=> every +98/6 is a multiple of 10; roster order survives equal keys, PLAYER A first — matching the symptom (Lola 6th, shown 1st, $1000).
Design (EY-safe: no syscalls/calls in the step — inline compares + ring stores only; all IO in the main-loop poll):
- src/glue_fi.c (copy of glue_eb.c, -DRR3_WATCH=1 ahead of EB_GLUE in the glue chain): m68k_write8/16/32 (the single funnel for native + interpreter guest writes) test each write against 5 ranges and push {cyc, pc, addr, val, sz, kind} into an 8192-entry ring (drop counter on overflow): kind1 record A +98 (0xFF0A14-17), kind2 record B +98 (0xFF0E94-97), kind3 0xFF7532, kind4 0xFF05D6, kind5 builder-select flag 0xFF022C.
- src/main.c (guarded RR3_WATCH_BUILD): per-frame drain of the ring into the breadcrumb as "watch tick=.. pc=.. addr=.. sz=.. val=.. kind=.." lines; while the late set is armed, a throttled "recsnap" line (every 30 frames, max 40): flag022C, A/B +98/+102/+104/+116, results-buffer slots 0-7 at 0xFF741E.
- Config: FH exactly + WATCH. Stamps: purpose "FI: STANDINGS WATCH - +98 WRITER HUNT", frozen "FI FROZEN - HALTED", arm fiarm.
READ OF THE LOG (pre-registered): (a) +98 writes appear with a racing writer PC and real values -> the stamp exists; the bug is downstream (sort/builder) and the writer PC names the code to compare against hardware behaviour. (b) +98 writes appear only from func_01E0AC-class PCs onto DNF records, finishers never written -> the stamp is missing in the port: hunt why (capsuled/masked writer? dropped write path?) with the writer-PC census of ALL record-region writes as the next probe. (c) No +98 writes at all AND recsnap shows stale/garbage +98 at arm -> builders consume leftover state, confirming (b)'s shape. Stability: FI is a diagnostic; a 0146AE death would mark the watch itself as a perturbation (unlikely class — compares only), verdict then rides on whatever log was captured before the death.
Lola's run for FI: one full race, finish it, let the standings screen sit a few seconds, then close. Breadcrumb as usual.
FI BUILD/DELIVERED (2026-11-13): FH config + WATCH exactly (cache diff vs build-fi: LABEL/NUMBER/WATCH only; build.make cites glue_fi.c + genesis_vdp_fetch.c + genesis_machine_fetch.c). First compile failed on a brace slip in the inserted main.c block (outer brace unclosed), fixed, snapshot refreshed, MAKE_EXIT=0. Verified on disk: 2,932,174 B, MD5 641fa0120d3825f84968d640f3044d8b; FI stamps x1, FH x0; watch/recsnap/watchdrop format strings in the VELF; g_watch_ring = 0x28000 B (8192 x 20 B); layout probe present; children 1x9, trio 1/2/1, 029190 x1; icon a1e00d2b; zip clean; copies identical. Delivered all three legs 2026-11-13 ~22:52 (FileZilla receipt "transferred 2932174 bytes in 1 second"). Delivery friction: two malformed batch shapes (top-level action + actions together; window_id in a batch) errored, and a mid-batch Return is refused (Return must be the last step of a batch or its own call) - the chain took more asks than it should; Lola flagged it again. TIGHTENED CHAIN for deliveries: activate -> one batch per path (Return LAST) -> screen.snap -> double-click the row. Lola: "Filezilla is always open when doing this."

## FI (#165) — DELIVERED 2026-11-13
Config: FH exactly + RR3_WATCH (glue chain: glue_fi.c ahead of EB_GLUE; VDP genesis_vdp_fetch.c, machine genesis_machine_fetch.c; cache diff vs FH = LABEL/NUMBER/+WATCH only). Verified on disk: 2,932,174 B, MD5 641fa0120d3825f84968d640f3044d8b; FI stamps x1 each, FH x0; watch/recsnap/watchdrop format strings present in the VELF; build.make cites glue_fi.c (glue_eb.c x0); g_watch_ring 0x28000 B (= 8192 x 20 B ring); layout probe present; extents: children 1x9, trio 1/2/1, 029190 x1; icon a1e00d2b...; zip clean; your_files copy identical. First compile failed on a brace slip in the inserted main.c block (outer block unclosed); fixed with one } and rebuilt clean (MAKE_EXIT=0, ~8 min). Delivery: NAS via files.write (2,932,174 B); Vita via FileZilla one continuous chain (paths set by batched combos, Return sent standalone where the runtime required it as a final step; final click on the rr3vita-fi.vpk row started the upload directly) — receipt "File transfer successful, transferred 2932174 bytes in 1 second". FI is on the Vita at ux0:/VPK/rr3vita-fi.vpk. NOTE from this delivery: mid-batch Return is refused by the control runtime unless it is the last step or its own call — future chains: put Return last or standalone. Lola flagged the ask count again mid-delivery (22:51); the chain still took activate + 2 path calls + snap + click. TEST PENDING: one full race, finish, standings sit a few seconds, close — then read watch/recsnap lines per the pre-registered fork.

## FI (#165) TEST — session 31: FULL SAIL, and the watch ANSWERED the question
Grab: breadcrumb 112,495,322 B (31 boot-starts), NAS Logs/breadcrumb-fi.txt + zip 7,344,968 B, VM copy MD5 f6d4d49c61d0036c0af1f274312ddead. Session 31 = 4,933,670 B. Zero maskfail; maskskip n=1 + fiarm tick=17004 pc=014C16 (race 1 finished); no enrel (Lola ran one race per protocol, closed in the frontend — final tail pc=186164); capsules tried=hit; prof2 step 24.55 (watch cost ~0.5), polls 9.60 (drain IO), frame 56.74. TALLY: 16 sails / 5 deaths.
WATCH FINDINGS (590 watch lines, zero drops):
- THE STAMP EXISTS. Exactly one sz=4 stamp write in the race window: tick=16753 pc=01E0E6 addr=FF0A14 val=00002A6E — func_01E0AC writes record A's +98 = 0x2A6E (10,862 frames ≈ 3:01) just before race end. Pre-registered branch (b)/(c) DEAD for record A: the data is written during the race.
- flag022C = 0000 in all 40 recsnaps -> func_01A718 was the selected builder (matches Task 13).
- THE RESULTS BUFFER NEVER FILLS: recsnap slots 0-7 at 0xFF741E read 0x00000000 in all 40 snaps (arm tick 17004 through +1170 ticks).
- Record B's +98 at the arm = 0xBBBBBBBB (never stamped; B may be a non-finisher/phantom record — its +116 index read 9).
- Post-arm the record region is reused as text scratch (00FC12 writes 0x20202020 spaces at tick 17005; 00F7F2 writes ASCII 'w','t','C','3' at tick 17907 in the frontend) — only recsnap n=1 (at the arm) shows true record state: A t=00002A6E w=0000 f=C0 i=0; B t=BBBBBBBB w=2791 f=02 i=9.
ROOT CAUSE (settled, source-verbatim): src/rr3_dispatch_mask.c, rr3_mask_run, Build DR block — "func_01A718 is NEVER executed — neither capsule nor native. DQ caught the process dying inside its capsule run (guest PC in func_027B84 at the final drain; the capsule never returned), and DC died with it fully native. So: bare return... The results table stays unbuilt." The crash-hunt era deliberately skipped the results builder to get past the post-race deaths, knowing the table stays unbuilt. With the copier skipped, the buffer stays zeroed: sort keys equal -> roster order stands (PLAYER A 1st), times /6 of zero -> ".0" on every line, prizes follow the wrong order. EVERY symptom matches. The trio A/B (ER photo) was bound to clear the trio — the builder is not in the trio. Note the DQ death inside the builder's capsule was at func_027B84 — the long-implicated address lives in the builder's subtree.
## FJ (#166) — un-skip the builder (design + pre-registered read)
One-variable change from FI (watch KEPT for verification): in rr3_mask_run, the 01A718 intercept no longer bare-returns; it sets g_late_armed = 1 exactly as DR/DV did (the post-race protections key off the arm) and falls through to the normal capsule path — 01A718 is JSR-called from 185E3E with a valid return (0x1860C8) and its extent already routes it here. If the capsule fails, the generalized DQ machinery bare-returns + freezes with the record (no death inside the step). Stamps: purpose "FJ: BUILDER UNSKIPPED - 01A718 RUNS", frozen "FJ FROZEN - HALTED", arm fjarm.
READ: (a) fjarm + no freeze + recsnap buf slots nonzero with plausible times + Lola's standings show real times and her real position -> FIX CONFIRMED; a follow-up build then strips the watch for the standing config. (b) maskfreeze with a failure record -> the record names the failing member of the builder's subtree (027B84?) and the next probe targets it. (c) builder runs, buffer still zeros -> the copy target differs from the synthesis's 0xFF741E; recheck part16.
Lola's run for FJ: same as FI — one full race, finish, standings sit a few seconds, note her actual finish position + what the screen shows, close.
FJ BUILD/DELIVERED (2026-11-13): FI config + the un-skip exactly (cache diff vs build-fj: LABEL/NUMBER only; build.make cites glue_fi.c + fetch pair). First make was killed by a runtime restart ~4 min in; resumed from surviving objects, MAKE_EXIT=0. Verified: 2,932,176 B, MD5 1a2d618fd5fb9eea9e7969e209b3df70; FJ stamps x1, FI x0; watch ring + layout probe present; extents children 1x9, trio 1/2/1, 029190 x1, builders {01A718,01A86E} x1; dispatch source == build snapshot; icon a1e00d2b; zip clean; copies identical. Delivered all three legs ~23:27 (FileZilla receipt "transferred 2932176 bytes in 2 seconds"). Delivery friction: the first double-click at y=496 hit the ".." row and navigated the local pane UP a directory instead of uploading; recovered (re-set path, double-click at y=512). LESSON: for the first file row under "..", click y=512 not 496 — coordinate clicks land ~a row high here.

## FJ (#166) TEST — session 32: DIED mid-race-1 at 0146AE; the un-skip never got to run
Grab: breadcrumb 114,471,980 B (32 boot-starts), NAS Logs/breadcrumb-fj.txt + zip 7,486,635 B, VM copy MD5 f1cb09967218d72cf6d4201b964823b2; dump psp2core-1794641553-0x00029738c3-eboot.bin.psp2dmp, 297,248 B (also in NAS Logs). Session 32 = 1,976,658 B. NO fjarm — race 1 never finished. Final tails ticks 7794-7796 at pc=0146AE (fatal step in flight at 03AEA6); zero maskfail; tried=hit at last full maskstat (11,372); prof2 step 25.21, frame 50.51; 365 watch lines, all boot/frontend class — the builder is race-end only, so the un-skip code never executed in this run. VERDICT: the death is the per-build tripwire lottery on a changed hot TU (rr3_dispatch_mask.c is in the step's dispatch path; FI with the skip intact sailed on the identical watch config). The un-skip is neither convicted nor cleared — it was never exercised. TALLY: 16 sails / 6 deaths.
## FK (#167) — FJ + layout re-roll (design + pre-registered read)
FK = FJ exactly + one inert 2048-byte BSS array in main.c (s_layout_probe_fk; the EZ re-roll precedent — a layout shift in a TU other than the changed one). Stamps: purpose "FK: BUILDER UNSKIPPED + LAYOUT RE-ROLL", frozen "FK FROZEN - HALTED", arm fkarm.
READ: sails + fkarm + recsnap buffer nonzero + Lola's screen right -> the un-skip is the fix and FK's layout is the keeper shape. Dies at 0146AE again -> re-roll once more with a different size; two consecutive deaths on un-skip builds would start implicating the dispatch change itself and force a different un-skip shape (e.g. restore the block's size with a same-length capsule call).
FK BUILD/DELIVERED (2026-11-13): FJ + s_layout_probe_fk[2048] in main.c (cache diff vs build-fk: LABEL/NUMBER only). Verified: 2,932,003 B, MD5 09d63489bc14cec792ad47e4fe6d2080; FK stamps x1, FJ x0; watch strings present; probes ez 4096 + fk 2048 both in symtab; ring 0x28000; extents children 1x9, trio 1/2/1, 029190 x1, builders x1; icon a1e00d2b; zip clean; copies identical; sources == build-fl snapshots. Delivered all three legs ~23:52 (FileZilla receipt "transferred 2932003 bytes in 2 seconds"; the y=512 double-click offset worked first try).

## FK (#167) TEST — STANDINGS FIX CONFIRMED (Lola's photo, 2026-11-14)
Belladonna results screen on FK: 1. ODIE 3:03.9 $1000; 2. LUCKY LUC 3:04.7 $750; 3. NATASHA 3:05.8 $500; 4. PLAYER A 3:06.3 $400. Real finishing order, real times with tenths, correct prize ladder, PLAYER A placed 4th (her real finish). The un-skipped builder ran and filled the results buffer; the sort and printer did the rest. Pre-registered read (a) met on the screen side. Known residuals did NOT bite: the tail fix-up top-2 name swap (ODIE/LUCKY LUC order matches their times) and the index-1 phantom row (four real rows only). Overlay at the photo: TICK 27417, SIM 18.01, 56.44 ms. Log-side confirmation (fkarm, recsnap slots, tried/hit) pending the session-33 grab when Lola closes the run. THE RESULTS-SCREEN FRONT IS CLOSED. TALLY: 17 sails / 6 deaths. Next: FL = FK minus the watch (glue back to glue_eb.c, RR3_WATCH off), un-skip kept as the standing config.

## FK LOG CONFIRMATION — session 33 (grabbed 2026-11-14)
Breadcrumb 122,436,727 B (33 boot-starts), NAS Logs/breadcrumb-fk.txt + zip 7,896,789 B, VM copy MD5 fd1cf0b2179fc409735e473794687e3e. Session 33 = 7,964,747 B. fkarm tick=25356 pc=01ABC0; NO maskskip line (the un-skip's signature — the counter no longer increments for 01A718 because the builder ran); zero maskfail; tried=hit=93,494; closed by Lola in the frontend (final tails pc=186164, tick 29156). recsnap n=1 at the arm: flag022C=0000 (01A718 selected); record A t=00002BAB; RESULTS BUFFER FILLED: buf = 00002BAB 00000000 00002B1F 00002B4B 00002B91 00002C57 00002C04 00002CA6. Cross-check vs her photo (slot k = record index k; value/6 = tenths): slot0 (PLAYER A) 0x2BAB=11179 -> 3:06.3; slot2 0x2B1F=11039 -> 3:03.9 (ODIE 1st); slot3 0x2B4B=11083 -> 3:04.7 (LUCKY LUC 2nd); slot4 0x2B91=11153 -> 3:05.8 (NATASHA 3rd) — all four match the screen to the tenth. Slot1 = 0 (index-1 record unstamped; no phantom row reached the screen). Buffer stays frozen in later recsnaps while the record region is reused as text scratch. 591 watch lines, zero watchdrop. THE FIX IS CONFIRMED FROM BOTH SIDES — screen and data. Results-screen front CLOSED for good.
## FL (#168) — standing-config candidate (design + pre-registered read)
FL = FK minus the watch: RR3_WATCH dropped (glue chain falls back to glue_eb.c; main.c's watch block compiles out via RR3_WATCH_BUILD). The un-skip stays (source-level in rr3_dispatch_mask.c); both layout probes stay; fetch pair stays. Cache diff vs build-fl: LABEL/NUMBER + WATCH removal only; build.make cites glue_eb.c. Stamps: purpose "FL: STANDINGS FIX - STANDING CONFIG, WATCH OFF", frozen "FL FROZEN - HALTED", arm flarm.
READ: sails + flarm + Lola's screen shows correct standings again -> the watch-free standing config is SETTLED and the standings fix ships in its final shape. Dies at 0146AE -> layout re-roll (probe size change); the watch strip is exactly the kind of binary change that draws the lottery.

## FL (#168) BUILT + DELIVERED (2026-11-14)
Verified: 2,930,903 B, MD5 f7d6c11498cf5a2ba2a4c9228c4c2656 (both copies identical); FL stamps x1 each; watch strings ZERO (recsnap/watch tick/watchdrop absent; g_watch_ring symbol gone); FK stamps zero; both layout probes present (fk 2048, ez 4096); extents children 1, trio 1/2/1, 029190 x1, builders x1; build.make cites glue_eb.c + genesis_vdp_fetch.c + genesis_machine_fetch.c; icon exact; sources byte-identical to build-fm/*.fm-source. Delivered all three legs 2026-11-14 ~11:59: NAS RR3/VPK copy, chat attachment, Vita ux0:/VPK (FileZilla receipt "transferred 2930903 bytes in 1 second"). TEST PENDING Lola: the usual protocol; read = sails + flarm + her screen shows correct standings -> standing config SETTLED.

## FL (#168) TESTED — FULL SAIL; STANDING CONFIG SETTLED (2026-11-14)
Lola's photo (Ikira standings): 1. ODIE 2:48.4 $1000, 2. PLAYER A 2:50.1 $750, 3. PEARL 2:50.8 $500 — real order, real tenths, correct prizes, watch-free build. Session 34 (grab: breadcrumb 130,353,827 B, Logs/breadcrumb-fl.txt + zip 8,318,520 B, VM MD5 78326e9dd0e4ea1fe89a071d99d7c735; session = 7,917,100 B): flarm tick=25681 pc=027B84; enrel tick=27702 (she raced on into race 2); NO maskskip; zero maskfail; tried=hit=93,734; closed by Lola mid-race-2 (final tails ordinary race ring, pc=0146AE is just the in-race PC). prof2 step 24.27. TALLY: 18 sails / 6 deaths. The standings fix now ships in the watch-free standing config — the results-screen arc (ER photo -> FI watch -> FJ/FK un-skip -> FL) is complete. Per Lola's ordering, what remains: other bugs as found, then AUDIO LAST.
Grab hygiene change (Lola, 2026-11-14): the FileZilla staging copy at RR3/breadcrumb.txt is now DELETED after the era copy in Logs is verified — breadcrumbs live in Logs only.

## Phase 1 (Lola's roadmap, 2026-11-14): fix every little issue -> audio -> visuals/features

### Tripwire root-cause hunt - OPENING STATE (desk, 2026-11-14)
- Death roster at 0146AE (all mid-race-1, capsules clean, tried=hit, zero maskfail): EO+EP (poll diet), ES-13 (suspend/resume, tainted), EW (wide blit), EY (in-step clock syscalls), FB (preswap+blit), FD (ARGB fusion), FF (fetch+composite fusion), FJ (dispatch-only change). Sails with real deltas: ET (tail cheapened), EU (poll handle), FA (batched fetch, step -16ms), FC (preswap), FE/FG/FH (renderer internals), EZ (inert 4KB BSS), EQ (stamps only), FK (FJ + inert 2KB re-roll), FL (watch off).
- EVERY single-variable theory has a counterexample: cadence (FJ died at FL-identical cadence; ET/EU cut loop time and sailed), layout (EZ's verified +0x1000 BSS shift sailed; FJ died), renderer-change (FA/FE/FG/FH sailed), poll-diet (convicted as a MODULATOR by the EO/EP vs EQ A/B, not the root: EW/EY/FB/FD/FF/FJ died diet-free). Data fits: a standing per-run race-1 risk that several unrelated perturbations RAISE. Two live mechanisms: (1) host heap corruption (EY dump = libc/kernel-detected abort, all thread PCs in SceLibKernel, no fault PC in module) - but note the audit: NO malloc/free anywhere in interp/machine/vdp hot paths (grep = 0); guest writes are masked into static BSS; renderer sink writes go to mapped display memory, not heap. (2) capsule<->vblank seam: interp services glue_check_vblank BETWEEN interpreted instructions (interp_account_cycles), firing the VBla handler (native code) mid-capsule; the handler can dispatch a NESTED rr3_mask_run (no re-entrancy guard in mask_run; vblank service has its own guard). Nested capsule checkpoints are stack locals (safe), but the outer framed runner's depth tracking never sees the handler's native JSR/RTS traffic. Glue also has a WALL-CLOCK "2-wall-frame safety net" inside glue_check_vblank - a host-timing input the sealed-step finding did not cover (it lives in glue, reached via accounting). The glue's game-fiber stack guard (32MB stack, abort at 28MB via crash_report + exit(2)) does NOT match the death shape (abort + psp2core, no crash report) - parked.
- The missing datum in EVERY death: which host phase was executing when the abort fired. All logging stops at the last completed frame; the tail method says the death is in the NEXT step OR in present/host work after it. FM is the witness build for exactly this.

### FM (#169) - tripwire witness build (building 2026-11-14)
- FL exactly + main.c only: a write-ahead phase witness. Helper rr3_phase_mark() (persistent append handle, ET/EU pattern, fprintf+fflush per mark) writes "ph S tick=" before genesis_sim_step, "ph P" before the poll/tail block, "ph V" before display_present. Stamps: purpose "FM: TRIPWIRE WITNESS - PHASE MARKERS" / "FM FROZEN - HALTED" / fmarm. No in-step instrumentation (EY), no cadence removal (EO/EP); markers ADD writes in the direction ER sailed with.
- PRE-REGISTERED READS: FM dies -> last marker names the dying phase (S = inside the step: native/interp/vblank/render/DMA; P = poll/tail stdio; V = present/vblank-wait) and the hunt narrows to that bucket. FM sails -> tally +1 and the witness STAYS in subsequent builds until it catches a death (it is cheap and main.c-only).
- FM BUILT + VERIFIED + DELIVERED 2026-11-14: 2,931,048 B, MD5 20c7fb92002939aab8aea4922e3bdc9e (build + your_files copies identical); zip OK; icon MD5 a1e00d2b... exact; FM stamps x1 each, "ph %c tick=%u" x1, FL stamps 0, watch strings 0; build.make = glue_eb.c + genesis_vdp_fetch.c + genesis_machine_fetch.c; extents children x1 (x9), trio 1/2/1, builders x1, 029190 x1. Cache diff vs build-fm = LABEL/NUMBER only. All three legs: NAS RR3/VPK, chat, Vita ux0:/VPK (FileZilla receipt "transferred 2931048 bytes in 1 second"). TEST PENDING - FM tests will be Vita session 35+.
- FM TEST VERDICT 2026-11-14 ("Breadcrumbs" - Lola closed it herself mid-race-2): session 35 (5,830,233 B of the 136,184,070 B file; NAS Logs/breadcrumb-fm.txt + zip 8,780,067 B, VM zip MD5 ee4159f4e9b44b0f149860daebbe16bc). FULL SAIL: fmarm x1 (race 1 finished, builder ran on the witness build), NO maskskip, ZERO maskfail, enrel tick=18141, tried=hit=90,337 at last maskstat (final poll 92,608/92,608). WITNESS PROVEN WORKING: ph S=17,709 / P=17,709 / V=17,708 - one marker trio per frame; the log ends mid-cycle in race 2 exactly where she closed it (ph V -> ph S -> ph P -> capsule). Witness cost measured: prof2 frame 68.21 -> 77.64 vs FL's 57.29 (step bucket 24.27 -> ~29 because the S marker's fflush lands inside the ts0-ts1 window; polls/other absorbed the P and V marks). Under Lola's 90 ms line, but the witness is NOT free (~+15-20 ms) - if a cheaper witness is ever needed, raw write() to an O_APPEND fd replaces stdio+fflush. TALLY: 19 sails / 6 deaths. The witness stays in subsequent builds until it catches a death.

### Phase 1 item 2 - func_029190 desk review (2026-11-14, interim)
- Conviction record (EA-era, airtight A/B): native 029190 in the 01A718 skip step = death (DR/EA/EB); capsuled = sail (DV/DZ onward, hundreds of fade crossings since EC). 014E5C/014BDA cleared by DZ.
- WHAT IT IS (generated/rr3_part28.c, 1,707 lines of generated C): the post-race fade/palette routine. VDP register init via control port (A0=0xC00004, the standard 0x80xx-0x92xx sequence), CRAM address set (0x60000000), only 10 straight-line data-port longword writes - NOT a flood. A 0x2C0-longword guest-RAM staging clear at 0xFFDDAE (fill 0x07FF07FF, ends 0xFFE8AE, in-RAM, harmless). Palette shade loops (counts 0x40/0x80) keyed on a TWO-LEVEL table lookup: D7 = read16(0xFF7532) - the same guard-input unknown from the standings hunt - indexes a ROM table at 0x029DB4 for an offset that steers A0 (= 0xFF7AF2 + stack-supplied offset + 0x800 + table offset); then calls func_006CAC with A1=0xFFDDAE.
- NO host-OOB visible in its own body: every write is an m68k_write to a guest address (masked into g_ram) or a VDP port write. Callees: 00F780 x3, 03A4A8 x2, 011BA8 x2, 029934, 0297F8, 017996, 00FBC6, 006CAC, 006C84, 003CAC. 0297F8 and 029934 have NO other static callers in the generated code - they are exclusive to 029190's subtree. The shared callees (03A4A8, 017996) are cleared by massive native exposure elsewhere (03A4A8 runs native all race long).
- VERDICT on the posed question: the permanent capsule is a FIX, not a lid. (1) The interpreter is the reference semantics for the same code and the fade completes correctly under it; (2) the divergent unit is contained - 029190's exclusive callees have no other native call path, and the capsule covers the whole subtree on both the dispatch and recomp_call_func paths; (3) shared callees are exposure-cleared. RESIDUAL: the exact native defect (which instruction/callee diverges, and how it becomes a host abort) is still unproven. The decisive experiment exists if wanted: a native-vs-interp differential run of this one function on the host cosim harness (rr3-cosim), feeding it the skip-step state. Not yet run - Lola's call.

### Phase 1 item 3 - func_027B84 RESOLVED: CLEARED (2026-11-14)
- The function (generated/rr3_part27.c:2148, 84 guest bytes): pushes A0/D2/D1; reads counters at 0xFF7AE4/0xFF7AE6; shifts each left 1; reads a table at A0=0xFF7AA4 indexed by the shifted counters (brief-index 0x1000/0x2000); adds the two reads into D0; writes back, increments and masks the counters to 0x1F; pops and returns via the split-stack convention.
- The one suspicious shape (index scaled BEFORE the counter mask is applied) feeds only m68k_read16 guest-bus reads - the accessor masks the address into the 24-bit map, so the worst case is a wrong-but-harmless guest word; and since every call masks the counters on exit, the index is bounded in practice (max read 0xFF7AA4+0x3E, in-RAM). No host-OOB path exists in this function.
- EXPOSURE: 12+ static call sites across parts 04,05,08,09,13,16,17,21,22,26,28. It is in NO mask extent, so it runs NATIVELY from all of them in every build - menus, racing, results - sailing and dying alike. A helper this hot cannot be a conditional killer. It has also run INTERPRETED inside the successful FK and FL builder capsules (builder completed both times), and FL's arm poll caught the guest at pc=027B84 in a healthy full-sail run.
- DQ RECLASSIFIED: DQ's death (process died inside 01A718's capsule run, final drain pc=027B84) names where the guest was STANDING when the host died, not the cause - the same host-abort signature family as the 0146AE deaths (zero guest-side failure records). 027B84 is removed from the suspect list. If a builder capsule ever dies again, FM's phase witness is the instrument on watch for it.

### Phase 1 item 4 - results-pipeline audit (2026-11-14)
- INDEX-1 PHANTOM: CLOSED, faithful original behaviour. Task 6 (mini-lane, graded) established both builders CAN write slot 1, but func_014BDA's first step skips slot 1 (0xFF7422) - rider index 1 vanishes from the order table and the never-overwritten 16th slot becomes the phantom 0xFFFF DNF row. 014BDA runs capsuled (trio) = reference semantics, so the port reproduces the original exactly. FK buffer decode + FK/FL photos consistent (no garbage row displayed). Not a port bug.
- TOP-2 SWAP (guest $014E40 tail fix-up): CLOSED, covered. It lives inside func_014BDA's capsuled extent - interpreted, faithful by construction; did not fire in FK/FL (log decodes matched both photos to the tenth).
- ⭐ func_01A86E PATH: A REAL LATENT PORT DEFECT (mode-gated, never exercised). Selection (verified at line level in generated/rr3_part30.c): at $1860AC, 185E3E computes word@0xFF022C - 1; zero (flag == 1) calls func_01A86E at $1860B8 (return $1860BE), else func_01A718 at $1860C2; both merge at $1860C8. 01A86E = {0x01A86E, 0x01AADC}, 622 bytes, 11 buffer write sites (vs 01A718's 5), an initial 0xFFFFFFFF fill of all 16 slots, and extra handling of the second fixed record at 0xFF0E94. Setters of FF022C=1 are mode/state machinery only (func_004EF8's entry test on 0xFF736C / 0xFF736A-2, writing mode word 0xFF7530; func_0108EC's save/convert routine over the 0xFF7460 block, restored by func_010A1E) - never race code; every observed race end (FI/FK/FL/FM) had the flag 0. Which playable mode sets it is not statically determinable.
- THE HOLE: of all 85 extents in src/rr3_dispatch_mask.c, NONE covers or starts at 0x01A86E (the only builder extent is {0x01A718, 0x01A86E}, hi-exclusive). On the 01A86E path: (1) 01A86E and its results-region callees run FULLY NATIVE - never validated in any build; (2) rr3_mask_run's 01A718 special case never fires, so g_late_armed NEVER SETS and the late set never engages - the post-standings transition then runs in the pre-EK configuration, the one that produced EG's class-3 freeze at the standings->next-track transition. (029190 stays capsuled regardless - its table extent is permanent - so the fade death itself is not re-exposed; the exposure is the transition hang + unvalidated native builder subtree.)
- FIX SHAPE (FN, awaiting Lola's call): mirror the 01A718 treatment - add table-mask extent {0x01A86E, 0x01AADC}; extend rr3_mask_run's arm special case to 01A86E; extend the st=99 hardcode (01A86E's sole call-site return is 0x1860BE, vs 0x1860C8 for 01A718); keep FM's phase witness. TEST ROUTES: (a) identify the mode at the desk (trace func_004EF8's callers / the 0xFF7530 mode word) and have Lola play that mode; (b) a one-off test build forcing FF022C=1 at race end to prove capsule+arm on that path, then discard.

### Phase 1 item 5 - transition-hang cluster: CLOSED by configuration + track record (2026-11-14)
- The cluster (EG's post-standings freeze; EK's remedy): {0022E2, 017A6A, hot ten} capsuled post-arm, {0021B6, 185CE4} native (structurally uncapsulable - step-entered tail-calls with no return address at [A7]). No member was ever individually convicted; the set-as-a-whole remedy worked immediately.
- TRACK RECORD SINCE EK (2026-11-10): every build that finished a race transitioned successfully - EK, EL-EM-EN line, EQ, ER, ES, ET, EU, EV, EX, EZ, FA, FC, FE, FG, FH, FI, FK, FL, FM (30+ transitions, zero hangs). The mid-race deaths in that span (EO/EP/EW/EY/FB/FD/FF/FJ) never reached the transition. Deliberate decision: do NOT bisect the cluster member-by-member - a hang gives no crash record to bisect against, the remedy (post-arm capsules = reference semantics) is cheap and permanent, and the hunt value is nil next to item 4's live hole.
- CAVEAT (the real content of this review): the cluster's protection is ARM-DEPENDENT. It engages only after the 01A718 arm - which is exactly what the 01A86E path (item 4) bypasses. Closing this item therefore rides on the FN fix: capsule 01A86E + arm on that path too.

### Phase 1 item 6 - 021BAE watch item: RETIRED with cause (2026-11-14)
- What it was: func_021BAE is a mid-function entry inside func_021A74 [021A74,021DAE), called directly by game code and capsuled via that extent; sightings are capsule/call poll lines (addr=021BAE) appearing at a metronome cadence in some race flows. DS/DX/DY-era notes built a ~90-tick sighting-to-death lag correlation (5 points).
- Why it retires: (1) the lag evidence is VOID under Lola's ticks rule (2026-11-09 - gaps may be pause noise; the correlation was withdrawn as evidence at the time and never re-established); (2) every 021BAE capsule run in every logged session is CLEAN (tried=hit at its own polls, e.g. FJ session 32: 13188/13188, 13216/13216); (3) the herald/phase-marker model explains all sightings - it is a periodic handler in some race flows, and deaths happen mid-race; (4) modern record is flow-dependent, not death-linked: FK/FL/FM full sails have ZERO sightings, FJ's death run had 17 (all clean) - presence tracks race content, not fate; (5) FM's phase witness supersedes sighting-logging as the death instrument. No build will be aimed at 021BAE. Sightings may still be noted in passing, but the watch is closed.

### Item 4 addendum - the 01A86E mode IDENTIFIED (2026-11-14, at Lola's asking)
- ROM mode-select string (rr3.bin): "1 Player / 2 Players Alternating / 2 Players Split Screen / 2 Players Mano a Mano" (plus "no password for Mano a Mano mode", "SET PLAYER MODE", and a "Player A $0 / Player B $0" money table).
- func_004EF8 arm 1 re-read at line level (correcting the loose Task-13 summary): after writing mode word 0xFF7530=2 and clearing the flag pair, it computes word@0xFF736A - 2: ==0 or <0 -> 0xFF7530=1, flags stay 0; >0 (FF736A >= 3) -> FF022A=1 AND FF022C=1. So FF022C=1 iff the player-mode selector FF736A > 2 - with the four-mode menu, that split lands exactly on {2 Players Split Screen, 2 Players Mano a Mano}: the two modes where BOTH humans are on track in the same race. {1 Player, 2 Players Alternating} (one human on track at a time) keep the flag 0 and use func_01A718.
- Corroboration: 01A86E is precisely the builder that special-cases BOTH fixed player records (A at 0xFF09B2 and B at 0xFF0E94 - B's +98, with its +116 index byte at 0xFF0EA6), while 01A718 special-cases only A's. Lola plays 1P campaign only - which is why the flag was 0 at every observed race end (FI/FK/FL/FM) and the path never fired in months of testing.
- TEST ROUTE (natural, no forced build needed): start a 2 Players Split Screen (or Mano a Mano) race, finish it as P1 (P2 idles/DNFs), and watch the results + transition. That exercises 01A86E + the missing arm on the current config (expect the class-3-style transition hang if the hole is live) and, after the FN fix, proves the fix.
- DEFERRAL (Lola, 2026-11-14): the FN fix for the 01A86E path is POSTPONED until after V1 releases - "skip 2 player stuff, do that at a later date." The hole ships in V1 as a KNOWN ISSUE: in 2 Players Split Screen / 2 Players Mano a Mano, the results builder (01A86E) runs unprotected (native, no arm), so the post-standings transition may hang (class-3 shape). 1P and Alternating modes are unaffected. Fix shape stays as designed above (extent {01A86E, 01AADC} + arm + st=99 return 0x1860BE + witness). Natural test when it lands: one split-screen race finished as P1.
- PHASE 1 DESK LIST STATUS (2026-11-14): items 2 (029190 - capsule is the fix), 3 (027B84 - cleared), 4 (pipeline audit - phantom/swap faithful; 01A86E hole found, fix deferred post-V1), 5 (transition cluster - closed by track record, rides on the arm), 6 (021BAE - retired) are ALL RESOLVED. Item 1 (0146AE tripwire root cause) remains open by design: FM's phase witness is armed in the standing build line and the hunt resumes the moment a death is witnessed.
- FN (#170) - 2P MENU GATE, PLAN B (Lola's pick, 2026-11-14): FM config + src/glue_fn.c (copy of glue_eb.c selected by -DRR3_FN_GLUE=1). (1) Boot-time verify-then-overwrite text patch in the g_rom copy block: the three 22-byte SET PLAYER MODE slots (0xA7B8 Alternating, 0xA7CE Split Screen, 0xA7E4 Mano a Mano) are renamed to "Not Yet Available" (space-padded) only if the original text still matches; count in g_fn_text_patches, logged once per boot as fnmenu. (2) Selector clamp in the m68k_write8/16/32 funnels: any write to word@0xFF736A with value > 1 is stored as 0 (the game's own uninitialised default, treated as the 1-player family by func_004EF8 and every other consumer), so choosing a renamed entry starts a normal 1P game and the unprotected 01A86E builder can never be selected in V1; clamps counted (g_fn2p_clamps/g_fn2p_last), logged as fn2p. All three 2P entries gated (Alternating included - one clean V1 story). FM's ph S/P/V witness kept. Verified: 2,931,777 B, MD5 9cc589760bb033d74372296306a4a81d; FN stamps x1, gate strings x1, witness x1, watch 0, extents nominal. Delivered all three legs 2026-11-14 (FileZilla receipt 2,931,777 B in 1 s). TEST PENDING (session 36+): menu shows three "Not Yet Available" entries; a renamed pick logs fn2p and starts 1P; fnmenu textpatches=3; race arms (fnarm) and sails. NOTE: the clamp acts on the selector only - func_0108EC's transient self-restoring FF022C flip in the save/convert path is untouched (same as real hardware in 1P play). The deferred post-V1 01A86E capsule fix is unchanged and will take a later build letter.
- FN (#170) TESTED 2026-11-14 (session 36, Lola's photo + grab): BROKEN AT BOOT - NOT a sail, NOT a tripwire death. The guest hangs inside func_000690's boot delay loop (writes 0x6F8 to word@0xFF0226 and yield-polls it to zero; the clearing service never completes): framebuffer frozen on the solid red backdrop (fbh=A89B91F4 from phase 2 on), a7=FFFFF2 at both phase ends (reset SSP is FFFFF6 - FN sits one return-address deep where FM had already exited), phase-1 insn 7.62M / yld 157K vs FM's 5.84M / 58K (the yield-spin burning from the start), 2,646 phase-3 frames with zero dispatch calls, zero capsules (tried=hit=0), ph S/P/V cycling normally, prof2 step 18.4 / frame 50.9 - the HOST loop stayed perfectly healthy; she closed it herself. GATE VERDICT FROM THE LOG: fnmenu textpatches=3 (the rename DID apply to the g_rom shadow); fn2p lines ZERO (the selector clamp never fired - the game died before any menu, so the gate mechanism itself is UNEXERCISED, neither convicted nor cleared). Session 36 = 254,189 B; NAS Logs/breadcrumb-fn.txt (136,438,259 B) + zip 8,811,781 B, VM MD5 c9560b7fddfcd8ca9f2bad00320cab36. Tally unchanged (19/6 - a boot hang is neither).
- FN ROOT-CAUSE STATE: by construction the only FN delta live at boot is the 66 patched ROM bytes in g_rom (clamp inert unless a >1 selector write occurs - none did; drain block = one fopen at first poll + integer compares; stamps inert). The one alternative construction cannot exclude: glue_fn.c is a NEW glue TU, so binary layout shifted (tripwire-family latent-bug territory - a layout-sensitive failure in the VBlank service path would present exactly as this hang). The game's own ROM checksum (func_1FA064, called from func_000690) was checked at the desk: its 32-bit sum's result is DISCARDED in func_000690 (D0.W zeroed at $0006AA with no compare) and the sum does not match the header longword at 0x18C on the pristine ROM either - checksum is NOT the mechanism, on current evidence. The host cosim refuses patched ROMs outright (CRC32 identity gate in the engine's PC runner main.c - NOT present in the Vita glue), so no host A/B is available. BISECT: FO (#171) = FN minus the text patch (src/glue_fo.c, -DRR3_FO_GLUE=1; clamp + witness + drain kept; fnmenu will read textpatches=0). FO boots -> patch convicted (rework the rename's delivery); FO hangs -> glue layout convicted (re-roll).
- PROCESS NOTE (Lola's correction, 2026-11-14 16:46): a fault report + photo is NOT a grab instruction. The assistant started the FileZilla grab chain on the photo alone; Lola: "That's cuz I didn't tell you to grab anything, yet. Learn to wait please." Standing rule reaffirmed: wait for her grab word ("grab"/"breadcrumbs"/"crashed") before starting a chain, even when the fault is obvious.
- FO (#171) - FN BISECT, NO TEXT PATCH (built + delivered 2026-11-14): FN config with src/glue_fo.c (-DRR3_FO_GLUE=1) = glue_fn.c with ONLY the boot-time text-patch block removed (diff vs glue_fn.c verified: the patch block and nothing else; the selector clamp, gate globals and drain all stay; fnmenu will read textpatches=0). First make failed on a stray closing brace left by the removal edit (assistant's error, bisect file only - FN source untouched); fixed, diff re-verified, rebuild MAKE_EXIT=0. Verified: 2,931,678 B, MD5 4c0540a52cb73cb3a43dd283ee7741e9; FO stamps x1, FN/FM stamps 0, foarm x1, fnmenu/fn2p formats x1, "Not Yet Available" 0, ph witness x1, extents nominal, icon byte-identical. Delivered all three legs (FileZilla receipt 2,931,678 B in 2 s). TEST PENDING (session 37+): boot is the decisive read - title/menus reached => the ROM text patch is convicted; same red-screen hang => the glue TU layout is convicted and the remedy is a re-roll. A full race (foarm, sail) additionally clears the clamp-bearing build line.
- FO (#171) TESTED 2026-11-14 (session 37, Lola: "Breadcrumbs - was able to start a race, and selected 2 player and mode did not change"): BISECT VERDICT DELIVERED + GATE PROVEN. Boot signature IDENTICAL TO FM's session 35 in every field (phase1: a7=FFFFF6 fbh=578D4437 insn=5,843,212 yld=58,382; phase2: a7=FFFFF6 fbh=F958B175 insn=8,487,081 yld=69,985) => FN's boot hang was the ROM TEXT PATCH AT INIT, convicted; the glue-TU layout alternative is dead (FO keeps the new glue family and boots perfectly). fnmenu textpatches=0 (no patch in FO, as designed). **fn2p tick=1217 sel=2 n=1 - THE SELECTOR CLAMP FIRED ON HARDWARE: Lola picked "2 Players Alternating" (selector value 2), the funnel stored 0, the mode never changed, and she raced 1P - the gate mechanism is PROVEN end-to-end (the same comparison catches 3/4 for Split/Mano).** She closed the run herself mid-race-1 (tick ~1646, 746 witness trios, zero maskfail/maskskip; race unfinished, so no tally change - the bisect question was the deliverable). Session 37 = 229,942 B; NAS Logs/breadcrumb-fo.txt (136,668,201 B) + zip 8,828,415 B, VM MD5 d470db5b456b28a3920b35b90b80fd28.
- FP (#172) - THE GATE, FIXED DELIVERY (built + delivered 2026-11-14): FN's design with the rename moved out of glue init: src/glue_fp.c (-DRR3_FP_GLUE=1) keeps the selector clamp + gate globals and adds rr3_fn_apply_text_patch() (the same verify-then-overwrite of the three 22-byte slots); src/main.c's FN drain block now CALLS it at the first poll (boot complete) before printing fnmenu, so fnmenu textpatches=3 is the expected read. FM's ph witness kept; stamps FP / fparm. Verified: 2,931,827 B, MD5 2773ed7891913322f4de00897c721f70; FP stamps x1, FO/FN stamps 0, fparm x1, gate strings x1, "Not Yet Available" x1, ph x1, extents nominal, icon byte-identical. Delivered all three legs (FileZilla receipt 2,931,827 B in 2 s). TEST PENDING (session 38+): SET PLAYER MODE shows three "Not Yet Available" entries; a renamed pick logs fn2p and starts 1P; full race arms (fparm) and sails. If FP sails, the gate ships in V1 as the standing mitigation for the deferred 01A86E hole, and the gate+rename come out post-V1 when the capsule fix lands.
- FP (#172) TESTED 2026-11-14 (session 38, Lola: "Welp, crashed" + "Didn't see the not yet available"): **DIED AT THE 0146AE TRIPWIRE MID-RACE-1 — AND THE WITNESS CAUGHT IT.** Session 38 (1,834,516 B; NAS Logs/breadcrumb-fp.txt 138,502,717 B + zip 8,968,930 B, VM MD5 841e389e0ca2b1db8df5247acf75e7af; dump psp2core-1794708931-0x00038c38d7-eboot.bin.psp2dmp 275,952 B in NAS Logs, recorded not parsed — dump forensics stay a dead end). Boot signature identical to FM (phase1 insn 5,843,212 fbh 578D4437; phase2 fbh F958B175 insn 8,487,081). Final tail tick=6677 pc=0146AE; tried=hit=6037 at last maskstat; ZERO maskfail, zero maskskip; no fparm (race unfinished). **ph counts S=5778 / P=5777 / V=5777; the log's last lines are "ph V tick=6677" then "ph S tick=6677" and nothing after — the S marker for the dying frame was written and P never came: THE DEATH IS INSIDE genesis_sim_step.** First witnessed tripwire death; phase-1 item 1's instrument delivered its answer for this death (host-phase = the step itself, not poll/tail stdio, not present/vblank wait). TALLY: 19 sails / 7 deaths.
- FP GATE READS: fnmenu textpatches=3 (rename applied — and file order proves WHEN: the fnmenu line sits at file line 10, immediately after phase2-done, i.e. the patch was in g_rom from the first poll at phase-3 entry, ~tick 900, BEFORE any menu draw). fn2p fired THREE times (tick=1183 sel=2, tick=1255 sel=2, tick=1265 sel=3 — she tested Alternating twice and Split Screen once; every pick clamped to 0, she raced 1P). **BUT Lola did not see "Not Yet Available" anywhere.** Desk resolution (same evening): the drawer path is verified end-to-end (func_004BB2 reads the pointer via m68k_read32 from table 0xA746 -> slots 0xA7B8/0xA7CE/0xA7E4, unique in the ROM; the bus reads ROM from the same g_rom the patch writes — genesis_bus.c line ~300) — so the long slots are NOT what the mode picker displays. The picker's Mode values come from the SHORT-NAME record table at 0xA75A+: [len-word-prefixed, NUL-terminated] "   solo   " @0xA760, "take turns" @0xA770, "split screen" @0xA782, "mano a mano" @0xA796 (the settings screen at 0x58E2 has the Mode/Level/Music labels these values sit beside). The long slots feed some other display (func_004BB2's list). FIX (FQ): patch BOTH sets — long slots keep "Not Yet Available" (17 ch, padded to 22); short slots get "Not Avail" (9 ch) space-padded to each slot's original length (10/12/11), len words and NULs untouched; also apply the patch at the phase1->phase2 transition in main.c (after func_000690's fragile delay completes — phase1-done's healthy insn count proves it) with the first-poll call kept as backstop, covering the alternative theory that a screen pre-renders during phase 2. fnmenu expectation becomes textpatches=6.
- FQ (#173) - GATE V2 + RE-ROLL (building 2026-11-14): FP config + src/glue_fq.c (-DRR3_FQ_GLUE=1) whose rr3_fn_apply_text_patch() patches all six slots (3 long + 3 short, verify-then-overwrite, counted together); main.c calls it at the phase1-done transition AND at the first poll (idempotent backstop); selector clamp unchanged (proven x4 across FO/FP); FM's ph witness kept (it caught FP's death in-step — it stays); inert 3072-byte BSS probe added in main.c as the tripwire layout re-roll (EZ 4096 + FK 2048 probes stay). Stamps FQ / fqarm; purpose "FQ: GATE V2 - BOTH NAME SETS, EARLY PATCH".
- FQ (#173) DELIVERED 2026-11-14: built in build-fr/ (cache diff vs build-fq = LABEL FQ / NUMBER 173 / FP_GLUE->FQ_GLUE only; build.make cites glue_fq.c; 6 fq-source snapshots). Verified: 2,931,816 B, MD5 aa9685f75f52e7069a3b5da1f2e000ef; ZIP OK; icon byte-identical; FQ stamps x1, FP stamps 0, fqarm x1, fnmenu/fn2p x1, "Not Yet Available" x1, "Not Avail" x3, ph witness x1, extents nominal (fade/builder/trio1/printer x1, trio2 x2); probes ez 4096 / fk 2048 / fq 3072 all present in the VELF. Delivered all three legs (FileZilla receipt 2,931,816 B in 2 s; remote /ux0:/VPK now holds 11 rr3 VPKs). TEST PENDING (session 39): boot clean (phase-1 patch-timing risk is the one new unknown - if it hangs like FN, the wedge is in phase 2 and the patch retreats to first-poll with the short names kept); SET PLAYER MODE / Mode picker shows "Not Avail" for the three 2P values; fnmenu reads textpatches=6; a pick logs fn2p and starts 1P; a full race arms (fqarm) and sails (tally would go 20/7).
- FQ (#173) INTERIM 2026-11-14 (Lola's photo, OPTIONS screen, tick 1647): BOOTS CLEAN and **THE RENAME IS VISIBLE ON HARDWARE** — the mode picker row reads SOLO (highlighted) / NOT AVAIL / NOT AVAIL / NOT AVAIL; Mode line shows "1 Player". The phase1-done patch timing is SAFE (no FN-style hang — the wedge stays a phase-1-only story), and the short-name patch is the display fix FP needed. Overlay: SIM 23.57, 57.01 ms. Race verdict (fqarm / sail) still to come — Lola: "this build works. Now let me race".
- FQ (#173) TESTED 2026-11-14 (session 39) — **FULL SAIL; GATE V2 PROVEN END TO END; TALLY 20 sails / 7 deaths.** Grab: NAS Logs/breadcrumb-fq.txt 147,850,614 B (39 boot-starts) + zip 9,614,548 B, VM zip MD5 777bbbc5c7ff08e3c1b390e500da5138, session39.txt 9,347,887 B / 129,001 lines. Boot signature identical to FM (phase1 insn 5,843,212 fbh 578D4437; phase2 fbh F958B175 insn 8,487,081). **fnmenu textpatches=6 gate=on** at file head — the phase1-done apply patched all six slots (3 long + 3 short). **fn2p x4** (tick=1201 sel=3, 1246 sel=2, 1253 sel=3, 1262 sel=2 — she poked Split Screen and Alternating twice each; every pick clamped to 0; clamp now proven across FO/FP/FQ). **fqarm tick=28766 pc=01ABC0** (race 1 finished, standings builder ran, NO maskskip — the un-skip is standing); **enrel tick=29520**; zero maskfail; tried=hit at every maskstat (last full poll 26,235/26,234 is the benign mid-capsule poll artifact; final capsule line 27,964/27,963 mid-run at close). Witness healthy: ph S=29,303 / P=29,302 / V=29,302. Lola closed the run herself mid-race-2 with the loop cycling normally (final tail tick=30202). prof2 last: step 28.82, frame 74.91 — witness overhead in line with FM, under her 90 ms line. **VERDICT: gate v2 (rename both name sets at phase1-done + selector clamp + witness) is the STANDING V1 CONFIGURATION.** The 2P modes are unreachable in V1, which is the mitigation for the deferred post-V1 01A86E hole. Phase 1 is closed except the passive tripwire item (witness armed; FP's in-step catch stands).
- PHASE 2 STARTED 2026-11-14 (Lola's fork call after FQ's sail): AUDIO SCOPING PASS done at the desk. Headline: **the whole Genesis audio pipeline already exists inside the shipped build** — superzazu Z80 emulated and stepped in lockstep (CMakeLists:303), bus captures every YM2612/PSG write into the cycle-stamped event queue (genesis_bus.c), ymfm YM2612 + SN76489 compiled in (CMakeLists 292-302), and genesis_sim_step drains the mixer every frame into main.c's REAL s_fm/s_psg buffers (main.c:56-60) — FQ's step time already includes full FM+PSG rendering; the samples are rendered and discarded (zero sceAudio code in the port). Phase 2 = a Vita output backend (mix FM/1 + PSG/8, resample to 48 kHz, ring, audio thread on sceAudioOut) + a rate strategy for the ~50 ms frame (recommendation: playback follows game cadence, A/B-able against realtime-with-underruns). Scoping doc: files/rr3-phase2-audio-scoping-2026-11-14.md. First audible build would be GR (#174, from build-fs/) = FQ + output stage only.
- GR (#174) DELIVERED 2026-11-14 — **FIRST AUDIBLE BUILD (Phase 2, Stage A).** FQ config exactly (cache diff = LABEL/NUMBER only; glue_fq.c; FETCH) + main.c-only audio output stage + SceAudio_stub link. Design: rr3_audio_start() at phase-3 entry opens sceAudioOut MAIN port (48 kHz stereo, grain 1024, max volume) and starts the rr3_audio feeder thread; rr3_audio_push() runs post-step in the poll phase (after the witness P marker; skipped when g_mask_freeze halts stepping) — mixes s_fm (÷1) + s_psg (÷8) with the host's nearest-neighbour indexing, resamples into a 32,768-frame lock-free ring; FOLLOW=1 stretches each frame's audio across the EMA-measured wall period (clamp 5-250 ms) so playback is continuous at game cadence; FOLLOW=0 (compile switch) = true 800/frame realtime. Breadcrumb: one-time "audio-on port=%d rate=48000 grain=1024 follow=1"; "audstat frames/fm/psg/out/und/ovr" on the maskstat cadence. Witness + gate v2 + standings un-skip kept; 1024 B probe s_layout_probe_gr added (readelf: gr 1024 @81381eb0, fq 3072, fk 2048, ez 4096). Verified: 2,932,973 B, MD5 512ca0e9b0a8b9dc1690648376cfb9c8; ZIP OK; icon byte-identical; stamps purpose/frozen/grarm x1, fqarm 0; fnmenu/fn2p/ph/audio-on/audstat formats x1; gate strings intact; extents nominal (trio2 x2 expected); sceAudioOut OpenPort/Output/SetVolume linked. Delivered all three legs (FileZilla receipt "transferred 2932973 bytes in 1 second"; remote /ux0:/VPK now 12 rr3 VPKs). TEST (Lola's ears are the verdict): title music + engine audible and CONTINUOUS (not bursts); pitch follows the slow game speed by design; audstat fm~888/psg~3729 per print with out climbing and und/ovr sane; a full race arms (grarm) and sails -> tally 21/7. Session will be 40.
- GR (#174) TESTED 2026-11-14 (session 40): **DIED mid-race-1 at the 0146AE tripwire — the witness's SECOND catch, again INSIDE genesis_sim_step** (ph S=4447/P=4446/V=4446; last line "ph S tick=5346"; final tail tick=5346 pc=0146AE; tried=hit 6027, zero maskfail, no grarm). Boot signature identical to FM. Gate fine (fnmenu textpatches=6; fn2p x0 — no 2P pokes this session). **Tally 20/8.** Grab: NAS Logs/breadcrumb-gr.txt 149,304,455 B (40 boot-starts) + zip 9,739,629 B (VM MD5 de4ac063e9b8ad925e7fe0036f43517a; session40.txt 1,453,831 B, 20,707 lines); dump psp2core-1794726699-0x000576258f-eboot.bin.psp2dmp 281,616 B in Logs (recorded, not parsed). Staging copies deleted.
- GR AUDIO VERDICT (Lola's ears + counters): she heard "mostly a bunch of crackling, no engine sounds". **The counters acquit the transport completely**: audio-on port=7; audstat fm=889/888, psg=3732 every print (production never flatlined); out grew at exactly the follow-cadence rate (~3,284 frames/step); **und=19 total for the whole session (17 of them the startup transient), ovr=0** — the ring was fed continuously, so the crackle is in the sample CONTENT or the resample, not starvation. Prime suspects: nearest-neighbour 4.1x stretch aliasing (both chips, PSG worst) and unverified mix content. GS answers both: linear-interpolated mix + a one-shot capture of the first 96,000 mixed frames to ux0:/data/rr3vita/audcap.raw for off-device waveform analysis.
- GS (#175) DELIVERED 2026-11-14 — GR + three changes, main.c only: (1) linear-interpolated mix replaces nearest-neighbour (16.16 fixed-point cursor over the PSG timeline, FM proportional); (2) audcap diagnostic — the first 96,000 mixed frames are captured and written once to ux0:/data/rr3vita/audcap.raw from the poll phase ("audcap frames=96000" breadcrumb line) for off-device waveform analysis; (3) inert 2560 B probe s_layout_probe_gs (readelf: gs 2560 @81381ec0, gr 1024). Verified: 2,932,940 B, MD5 04c5c81f6f631a6cb8a761e6b7abe3cc; ZIP OK; icon byte-identical; GS stamps x1, grarm 0; audio-on/audstat/audcap formats x1; extents nominal. Delivered all three legs (FileZilla receipt "transferred 2932940 bytes in 1 second"). TEST: ears again (music recognisable? engine present? crackle gone or changed?) + stability (full race -> gsarm -> tally 21/8); on her grab word, also pull audcap.raw and waveform-analyse it here. Session will be 41.
- GS (#175) TESTED 2026-11-14 (session 41): **DIED mid-race-1 at the 0146AE tripwire — the witness's THIRD in-step catch** (ph S=3730/P=3729/V=3729, S last; final tail tick=4629 pc=0146AE; tried=hit, zero maskfail, no gsarm). Boot identical; gate fine (textpatches=6). audstat healthy again (fm 889/888, psg 3732, und=15 all in the first 1200 frames, ovr=0). **Tally 20/9.** Grab: NAS Logs/breadcrumb-gs.txt 150,520,554 B (41 boot-starts) + zip 9,843,657 B (VM MD5 b5fc91982bec194f16e361127e1d12c8); dump psp2core-1794728442-0x00059c36d1-eboot.bin.psp2dmp 302,000 B in Logs; audcap.raw in Logs as audcap-gs.raw + VM copy (MD5 7afc4027ea577252c9242cd42372dd37). Staging copies deleted.
- GS AUDIO VERDICT — THE MIX IS CLEAN; THE CRACKLE IS DOWNSTREAM OF THE RING WRITE. Lola: "very slow music but still crackly" (slow = follow-cadence by design; music now recognisable — interpolation helped audibility). audcap waveform analysis (the exact bytes the mixer handed the ring): peak 5,634 (no clipping), RMS ~2,039 continuous in every 100 ms window (no dropouts), sample deltas mean 24 / max 398 with ZERO spikes, grain boundaries indistinguishable from elsewhere — no crackle, no seams, no gaps in the content. Spectrum: 86.9% of energy below 100 Hz (wandering 28-52 Hz dominant = the stretched bass line), blocks differ per push (live music, not a loop); L==R in this window (mono bass-led intro moment). Conclusion: corruption enters between the ring and the speakers — ring read / audio thread / sceAudioOut config — despite the clean counters. GT discriminates: capture the grains AT THE RING EXIT (what the thread actually sends sceAudioOutOutput) and diff against the entry capture.
- DESK FIND while building GT (from the shipped source, src/main.c rr3_audio_thread): GR/GS hand sceAudioOutOutput a SINGLE static blk reused every grain - no ping-pong. If Output returns once the buffer is queued while the driver still reads it, the next fill tears the playing grain: crackle over recognisable music with all counters clean - the exact GR/GS symptom, and invisible to both audstat and the entry capture (audcap copies intended bytes at the ring write). GT therefore bundles the ping-pong double buffer (the fix candidate) with the ring-exit capture (audcap2 taps each grain just before Output; still discriminates ring corruption, and if crackle persists double-buffered with a clean audcap2, the sceAudioOut configuration itself is convicted next - port type/grain).
- GT (#176) DELIVERED 2026-11-15 — GS + the audio-thread handoff fix + instrumentation: (1) ping-pong double buffer in rr3_audio_thread (GR/GS reused ONE blk for every sceAudioOutOutput - the torn-grain suspect); (2) ring-exit capture audcap2 (each grain copied just before Output, written once to ux0:/data/rr3vita/audcap2.raw, "audcap2 frames=96000"); entry audcap kept as control; (3) inert 1792 B probe s_layout_probe_gt (readelf: gt 1792 @81382ec8, gs 2560). Verified: 2,933,117 B, MD5 8c3f93735f44b5d523e21e64a75bd949; ZIP OK; icon byte-identical; GT stamps x1, gsarm 0; audio-on/audstat/audcap/audcap2 formats x1; extents nominal. Delivered all three legs (FileZilla receipt "transferred 2933117 bytes in 2 seconds"). TEST: ears first - crackle gone? (if yes, the single buffer was it; Stage B tuning follows) - plus a full race for 21/9. On the grab, pull BOTH audcap.raw and audcap2.raw and diff them. Session will be 42. Lola confirmed by ear that GS's audcap.wav (sent to her as an attachment) is clean music she did NOT hear on the Vita - content-vs-handoff separation now ear-verified from both sides.
- GT (#176) TESTED 2026-11-15 (session 42): DIED mid-race-1 at the tripwire, final tail tick=5288 pc=0146AE; witness ph S=4389/P=4388/V=4388 - S LAST, the witness's FOURTH in-step catch (FP, GR, GS, GT). Boot signature identical to FM; tried=hit (6584/6584); zero maskfail; no gtarm. audstat healthy (fm 889/888, psg 3732, und=15, ovr=0). TALLY 20/10. AUDIO - THE CRACKLE IS CONVICTED, END TO END: Lola heard recognisable-but-very-slow music, "slight progress" (tears gone - the double buffer did its job). The audcap2 reconstruction is decisive: the exit capture contains four zero runs (4096 startup + 2048 @6144 + 1024 @72704 + 768 @95232); delete them and the remaining 88,064 frames are SAMPLE-IDENTICAL (100.00%) to the entry capture. The ring, thread and double-buffered handoff deliver PERFECT bytes - the only corruption is INSERTED SILENCE: GR/GS/GT's all-or-nothing grain policy output a full silence grain whenever the audio thread woke to fewer than 1024 frames in the ring; in follow mode producer/consumer rates match only on average, the ring level sawtooths, and the thread keeps catching the trough. Those punched-in holes are the crackle. Grab: NAS Logs/breadcrumb-gt.txt 151,953,254 B + zip 9,966,486 B (VM zip MD5 7715475e...; audcap MD5 1723dd8c..., audcap2 MD5 859c5a71...; ~/workspace/rr3-gt-grab/, session42.txt 1,432,700 B); dump psp2core-1794730090-0x00061b27f3-eboot.bin.psp2dmp 274,928 B in Logs (recorded, not parsed).
- GU (#177) = GT + the wait fix, BUILDING 2026-11-15: the audio thread no longer outputs silence on a short ring - it sleeps 2 ms and re-checks until a full grain exists (s_aud_underruns now counts trough episodes). Ping-pong + both captures kept (GU's audcap2 should equal audcap from frame 0 if the path stays perfect). Inert 3328 B probe s_layout_probe_gu re-rolls the tripwire layout. Stamps GU / guarm. Read: crackle GONE (only the slow follow-cadence music) -> Stage A passes, Stage B tuning next; a full race arms guarm -> 21/10.
- GU (#177) DELIVERED 2026-11-15 - GT + the wait-for-full-grain fix (audio thread sleeps 2 ms and re-checks instead of outputting silence on a short ring; underruns now count trough episodes) + 3328 B probe. Verified: 2,932,949 B, MD5 afffbd6c1574400569a9613f27866270; ZIP OK; icon byte-identical; GU stamps x1, gtarm 0; both captures armed; probes gu 3328 @81382ec8, gt 1792; extents nominal. Delivered all three legs (FileZilla receipt "transferred 2932949 bytes in 1 second"). TEST: ears - clean slow music with NO crackle closes the crackle hunt (Stage A passes, Stage B tuning next); GU's audcap2 should equal its audcap from frame 0. A full race arms guarm -> 21/10. Session will be 43.
- V1 RELEASE GATE set by Lola 2026-11-15 (main chat): no V1 release until the port runs at 60 FPS with minimal or no drops. Standing config is ~50 ms frame / ~29 ms step (~20 FPS); the 16.7 ms budget means the step alone overruns it, so a ~3x engine/sim-side speedup is now a RELEASE BLOCKER (renderer front closed; sim is one sequential stream, no multicore split). Recorded in MEMORY.md and GOAL.md; the Space's What's Next picks it up at the next milestone update.
- GU (#177) TESTED 2026-11-15 (session 43) - FULL SAIL, first sail since FQ and first with audio: guarm tick=17660 pc=027B96 (race 1 finished), enrel tick=19128, zero maskfail, tried=hit at close (91993), boot signature identical to FM; Lola closed it mid-race-2 (final tail tick=19524). TALLY 21/10. AUDIO: TRANSPORT PROVEN PERFECT - audcap and audcap2 are BYTE-IDENTICAL over all 96,000 frames (MD5 3775304cea08553a2b4451ab41c89681 both): the wait-for-full-grain policy delivers the mix to the driver with zero inserted silence from frame 0. The crackle mechanism is DEAD. Capture content: continuous music (RMS steady ~1600-2400, peak 6461, 62 zero samples), broader spectrum than GT's capture (32% <100 Hz) - more of the arrangement audible. Lola's ears: "still sounds like it's trying", engine hints here and there, another instrument voice audible - what remains is the follow-cadence 3-4x interpolation stretch itself at ~1/3 game speed, not a transport defect. STAGE A PASSES (continuous, synced, sample-perfect delivery). Remaining audio quality is the rate strategy, now coupled to the 60 FPS V1 gate: at full speed follow = realtime and the stretch retires itself.
- GV (#178) DELIVERED 2026-11-15 - GU + RR3_AUDIO_FOLLOW 0 (the realtime A/B Lola picked): each step's audio plays at true pitch, 800 frames/step, bursts separated by the wait policy idling between pushes; captures kept; 2816 B probe. Verified: 2,933,400 B, MD5 ccba3f04d56fa24390f93037cd5d7703; ZIP OK; icon byte-identical; GV stamps x1, guarm 0; probes gv 2816 @81382eb0, gu 3328, gt 1792; extents nominal. Delivered all three legs (FileZilla receipt "transferred 2933400 bytes in 7 seconds"). TEST: ears - true-pitch bursts vs GU's continuous stretch; which strategy she prefers is the Stage B rate verdict. audio-on line will read follow=0. A full race arms gvarm -> 22/10. Session will be 44.
- GV (#178) TESTED 2026-11-15 (session 44): DIED mid-race-1 at the 0146AE tripwire - final tail tick=3815 pc=0146AE; witness ph S=3815 LAST (fifth in-step catch: FP, GR, GS, GT, GV). Boot signature identical to FM; audio-on follow=0 (A/B live); fnmenu textpatches=6; tried=hit 3650/3650, zero maskfail, no gvarm/enrel. audstat: fm=889/psg=3732 exact, out exactly 800/step, und=1876 (wait episodes - the burst mechanism itself), ovr=0. audcap == audcap2 BYTE-IDENTICAL again (MD5 568dd8fe4183c29ff7f6774273d65590) - transport perfect in realtime mode too; capture content peak 9433 RMS 2632 (fuller than GU's stretched capture). LOLA'S EARS: "I heard music and such that time, very stuttery but I heard it" - true pitch reads as MUSIC (vs GU's 'still trying'), chopped by the burst gaps as designed. TALLY 21/11. Grab: NAS Logs breadcrumb-gv.txt 158,997,960 B + zip 10,515,768 B (VM MD5 14959c259164df2b339afce4ae83f110), audcap-gv.raw + audcap2-gv.raw, dump psp2core-1794772238-0x0000df3515-eboot.bin.psp2dmp 302,464 B (recorded, not parsed); staging copies deleted. Rate-strategy preference still hers to call - it sets GW's FOLLOW value; the perf hunt (60 FPS gate) is the standing next front either way.
- GW (#179) DELIVERED 2026-11-15 - FIRST PERF-HUNT BUILD (60 FPS gate). Lola's rate verdict recorded: bursts (FOLLOW=0) are the standing mode, not GU's stretch. Diagnostic: GV + prof5 - src/sim_step_prof5.c (RR3_PROF5) is the engine sim_step.c verbatim + ONE rr3_prof5_mark() call between machine_run_frame and audio_mixer_drain (diff-verified: header + extern + call only); main.c folds run/drain buckets in prof_post_drain and prints "prof5 run=/drain=" on the maskstat cadence. Baseline it splits (GV session 44 prof): step 30.23 = game 5.30 + mach 24.83 + rest 0.10. 2304 B probe. Verified: 2,933,545 B, MD5 50abaacb058a82e5439716ef75c3f52a; ZIP OK; icon byte-identical; GW stamps x1, gvarm 0; prof5 x1; probe gw 2304 @81382ec8; rr3_prof5_mark linked. Delivered all three legs (FileZilla receipt "transferred 2933545 bytes in 2 seconds"). TEST: prof5 lines name the first lever - run-fat -> VDP/Z80 frame work; drain-fat -> audio render work (move/trim). A full race arms gwarm -> 22/11. Session will be 45.
- GW (#179) TESTED 2026-11-15 (session 45) - LOLA CLOSED IT HERSELF ("Breadcrumbs"); grab = breadcrumb + both captures, no dump. Boot identical to FM; fnmenu textpatches=6; fn2p x0; tried=hit 11,185; zero maskfail; NO gwarm/enrel - race 1 unfinished when she closed it (22,286 ticks, long session incl. menu/pause time); final ph S-last is the expected shape of a manual close mid-step, NOT a witnessed death. TALLY UNCHANGED 21/11. Captures BYTE-IDENTICAL again (MD5 b37856189e0196e5774541c00fdcc89c) - transport perfect in the prof5 build too. NAS Logs: breadcrumb-gw.txt 170,125,224 B + zip 11,324,371 B (VM zip MD5 31c8c2dcf993dd30e8dc6f25a96639c6), audcap-gw.raw + audcap2-gw.raw. *** THE PROF5 READ: run=18.47 ms, drain=2.47 ms (rock-steady across prints). The step (~26.1-26.4 this session; game 5.1-5.3, mach 20.94, rest 0.09) is 71% machine_run_frame, 20% native game frame, 9% audio drain. THE AUDIO DRAIN IS CHEAP - moving/trimming it buys ~2.5 ms at best. THE PERF HUNT'S TARGET IS machine_run_frame ITSELF. *** GX (#180) splits it per scanline (prof6, EY prof4's armed-clock mechanism on the fetch machine copy): chunk (per-line 68K) / mid (Z80 + IRQ + deferred flush) / rend (gvdp_render_scanline + palette convert + sink).
- GX (#180) DELIVERED 2026-11-15 - prof6 line split of machine_run_frame. GW + src/genesis_machine_prof6.c (RR3_PROF6): the FETCH machine copy + EY prof4's armed-clock mechanism (3 clock reads/scanline, armed only around the phase-3 step): chunk = per-line glue_run_game_chunk, mid = latched V-int + step_z80 + IRQ delivery + deferred flush, rend = gvdp_render_scanline + palette convert + sink. Variant diff vs fetch copy = header/globals + 4 mark sites only. prof5 kept (run/drain still printed). 3584 B probe. Verified: 2,933,276 B, MD5 8c5b04c4d3e723fc5c4747babc1db833; ZIP OK; icon byte-identical; GX stamps x1, gwarm 0; prof5+prof6 x1; prof6 globals linked. Delivered all three legs (FileZilla receipt "transferred 2933276 bytes in 2 seconds"). TEST: prof6 lines name the lever inside run's 18.47 ms - rend-fat -> VDP render work; chunk-fat -> the 68K chunk path; mid-fat -> Z80/IRQ work. A full race arms gxarm -> 22/11. Session will be 46.
- GX (#180) TESTED 2026-11-15 (session 46) - FULL SAIL, tally 22/11. Lola closed it mid-race-2 ("Breadcrumbs"). Boot signature identical to FM; fnmenu textpatches=6 gate=on; gxarm tick=13184 pc=001B4A - RACE 1 FINISHED; enrel tick=14056; zero maskfail; tried/hit clean (final poll gap of 1 = mid-capsule poll landing). Captures BYTE-IDENTICAL again (MD5 20482a54344214292371ca87ada0f590 both) - transport perfect in the prof6 build. audstat healthy (fm=888 psg=3732 ovr=0; und=10313 = FOLLOW=0 burst waits). Grab: NAS Logs breadcrumb-gx.txt 174,620,556 B + zip 11,713,954 B (VM MD5 ec74f6585e539535e2c51ef7a4aa0bd7), audcap-gx.raw + audcap2-gx.raw; staging copies deleted. PROF6 READ (the payload, steady across 11 prints): chunk ~4.8 ms, mid ~2.65 ms, REND ~13.4 ms - the scanline render block is ~72% of machine_run_frame and ~half the whole step. Pre-registered branch taken: rend-fat -> VDP render work is the first lever. Note: prof6's own clock reads taxed the frame (prof2 frame 87.28 vs GW's ~61) - diagnostic overhead, EY precedent. GY (#181) = prof7: split rend into scan (gvdp_render_scanline) / conv (host-palette refresh + s_cram_argb conversion) / sink, same armed-clock mechanism, zero behaviour change.
- GY (#181) DELIVERED 2026-11-15 - prof7 rend split. GX + src/genesis_machine_prof7.c (RR3_PROF7, chain before PROF6): the prof6 machine + two clock reads inside the render block splitting rend's ~13.4 ms into scan (gvdp_render_scanline) / conv (host-palette refresh + s_cram_argb conversion loop) / sink (the sink call). prof5+prof6 lines kept. Variant diff vs prof6 = globals line + 2 render-block edits only. 4096 B probe. Verified: 2,933,664 B, MD5 c4d3f582ecd02c9fce29a2776a8c2d71; ZIP OK; icon byte-identical; GY stamps x1, gxarm 0; prof5/6/7 x1; prof7 globals linked. Delivered all three legs (FileZilla receipt "transferred 2933664 bytes in 1 second"). TEST: prof7 lines name the first lever - scan-fat -> inside gvdp_render_scanline; conv-fat -> palette/convert caching; sink-fat -> the framebuffer copy. A full race arms gyarm -> 23/11. Session will be 47.
- GY (#181) TESTED 2026-11-15 (session 47): DIED mid-race-1 at the tripwire - final tail tick=5454 pc=0146AE, witness ph S=4555/P=4554/V=4554 (S LAST, SIXTH in-step catch: FP GR GS GT GV GY). tried=hit 4805, zero maskfail, no gyarm/enrel. audstat healthy (fm=888 psg=3732 ovr=0 und=2813 burst waits). Captures BYTE-IDENTICAL (MD5 3ef2506160d4a5350d4e9e24d023250a). Dump psp2core-1794786652-0x0002d53867-eboot.bin.psp2dmp (303,184 B) in Logs, recorded not parsed. TALLY 22/12. PROF7 READ (the payload, 3 prints stable): scan=11.53-12.25, conv=1.37, sink=0.43 - SCAN-FAT decisively: gvdp_render_scanline is ~86% of rend, ~65% of run, ~40% of the step. conv and sink acquitted as levers. GZ = prof8 inside the scanline (spr/fetch/comp).
- GZ (#182) DELIVERED 2026-11-15 - prof8 inside the scanline. GY + src/genesis_vdp_prof8.c (RR3_PROF8, VDP chain before ARGB): the FETCH VDP + armed-clock buckets inside gvdp_render_scanline - spr (sprite_render_line + host callback), fetch (batched plane-span fetch loop), comp (FE span composite to the exit return). Machine stays prof7, prof5 kept; GZ prints prof5/6/7/8. First make failed on a missing <stdint.h> in the variant header (fixed, incremental rebuild MAKE_EXIT=0). 1536 B probe. Verified: 2,934,242 B, MD5 5f32bbf492286cc947836e403de53c56; ZIP OK; icon byte-identical; GZ stamps x1, gyarm 0; prof8 globals linked. Delivered all three legs (FileZilla receipt "transferred 2934242 bytes in 1 second"). TEST: prof8 lines name the first real lever inside scan (~12 ms): spr-fat -> sprite layer; fetch-fat -> plane fetch; comp-fat -> composite. A full race arms gzarm -> 23/12. Session will be 48.
- GZ (#182) TESTED 2026-11-15 (session 48) - FULL SAIL; PROF8 READ: FETCH-FAT. Lola closed it herself mid-race-2 (final tail tick=20859; ph S-last at close = the manual-close shape). gzarm tick=19182 pc=027B96 (race 1 finished); enrel tick=20505; tried=hit (27,489 at the final capsule poll), zero maskfail. Boot signature identical to FM; fnmenu x1 (gate on). audstat healthy (fm=888/889, psg=3732, und=15,001 FOLLOW=0 burst episodes, ovr=0); captures BYTE-IDENTICAL again (MD5 0f2af9a2229e2999f68951e104af286c both). TALLY: 23/12. PROF8 READ (16 prints, rock-stable): spr ~1.0-1.3 ms, fetch ~6.8-7.0 ms, comp ~4.0-4.5 ms (sum ~12.4 = prof7 scan). FETCH is 57% of scan, comp 35%, spr 8% - the fetch-fat branch of the pre-registered read is taken: fetch_plane_span's tile expansion is the first real lever. Session colour: prof6 last chunk=3.85 mid=2.62 rend=13.44; prof5 last run=20.11 drain=3.45 (diagnostic overhead from four stacked profiles).
- HA (#183) CONFIGURED 2026-11-15 - the fetch lever. GZ + src/genesis_vdp_tab8.c (RR3_TAB8, VDP chain before PROF8): the prof8 VDP with fetch_plane_span's tile expansion made table-driven - the two output bytes per pattern byte are a pure function of (attr, byte), precomputed once into s_exp_n/s_exp_f (8 attrs x 256, normal + h-flip packings, 8 KB); the partial-tile path expands the whole tile and copies the slice. HOST DIFFERENTIAL: byte-identical to the fetch variant across 36 randomized config/seed batteries (H40/H32, all hscroll modes, 2cell vscroll, window, SHI, interlace-2, plane sizes) via ~/workspace/rr3-vdp-harness. Host wall time flat (host CPU insensitive; the Vita prof8 read is the judge). prof5/6/7/8 all kept so the fetch number compares apples-to-apples with GZ's 6.8-7.0. 2048 B probe. Pre-registered: fetch drops materially -> table fetch is a keeper and comp (~4.3) is next; fetch flat -> the cost lives elsewhere in the span loop (name fetch / span churn) and gets profiled finer.
- HA (#183) DELIVERED 2026-11-15 - the fetch lever, table-driven tile expansion. Verified: 2,934,065 B, MD5 65fac553b994f92f9def5a8e3084ed4e; ZIP OK; icon byte-identical; HA stamps x1, gzarm 0; prof5/6/7/8 x1; probe ha 2048; s_exp_n/s_exp_f linked (4096 B each). Delivered all three legs (FileZilla receipt "transferred 2934065 bytes in 1 second"). TEST: prof8 fetch vs GZ's 6.8-7.0 is the verdict - a material drop makes the table fetch a keeper and comp (~4.3 ms) the next lever; flat acquits the expansion math and points inside the span loop. A full race arms haarm -> 24/12. Session will be 49.
