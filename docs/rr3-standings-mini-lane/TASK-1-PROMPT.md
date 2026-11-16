# TASK 1 — paste this whole block into OpenCode on the mini (Qwen3 8B, Build)

---

Work in this directory only: ~/rr3-standings-mini-lane

SETUP (do these first, in order; each step has its own exit):
1. Confirm the file source/rr3_part11.c exists in this directory.
   If it does not, write the single line "BLOCKED: source/rr3_part11.c not found"
   into a new file named standings-swap-verdict.md and stop. Do not search
   the rest of the disk, do not download anything, do not research fixes.
2. Confirm line 5000 of source/rr3_part11.c begins the function
   func_014BDA (it should read `void func_014BDA(void) {`).
   If it does not, write the single line "BLOCKED: func_014BDA not at line 5000"
   into standings-swap-verdict.md and stop.

TASK (single shot — one function, one file, one output file):
Read func_014BDA in source/rr3_part11.c, from line 5000 to the end of the
function (the closing brace before the next `void func_` definition).
This function bubble-sorts a table of 16 four-byte entries starting at
guest address 0xFF74E0.

Answer exactly one question: when two adjacent entries are swapped,
exactly which bytes/words are exchanged?

Write your answer to a NEW file named standings-swap-verdict.md in
~/rr3-standings-mini-lane (do not edit any other file), containing:
1. Every memory read and write that forms the swap, in execution order,
   each with: the line number in rr3_part11.c, the exact line of code
   quoted, and the guest offset it touches (for example entry+0, entry+2).
2. A one-sentence verdict, one of:
   - "FULL: the swap exchanges all 4 bytes of each entry."
   - "PARTIAL: the swap exchanges only <say exactly which parts>."
   - "NO SWAP FOUND: <say what the code does instead>."
3. One sentence on what happens when the two compared keys are equal:
   are the entries swapped or left in place? Quote the comparison line
   and give its line number.

RULES:
- Use only lines you actually read in this file. Quote them exactly.
- Do not open any other source file. Do not propose a fix or a patch.
- Do not end with a question. When the file is written, stop and give a
  two-line summary: the verdict sentence, and how many read/write lines
  you listed.

---
