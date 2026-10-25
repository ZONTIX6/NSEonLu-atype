# Structure Index Implementation Plan

> **For agentic workers:** Execute the following tasks in this session. The user has approved the Markdown index and requested comprehensive coverage and member lookup.

**Goal:** Provide a Chinese searchable Markdown data dictionary covering repository C structures and unions, with member lookup and one-command regeneration.

**Architecture:** A standard-library Python scanner preserves source locations and extracts records without evaluating conditional compilation. A Markdown renderer resolves type relationships within source scope and keeps duplicate definitions distinct. PowerShell and CMD expose regeneration on Windows.

**Tech Stack:** Python 3.10+, unittest, Windows PowerShell 5.1+, Markdown.

## Global Constraints

- Preserve unrelated changes in `application/chassis/chassis.c` and `modules/algorithm/chassis_spin_config.h`.
- Scan all repository C/H sources; exclude generated output and caches. Include vendor sources, private/local definitions, portable copies and test stubs with explicit groups.
- Preserve raw declarations, source comments and line numbers; do not invent member semantics or imply all conditional branches are active.
- No compiler, network dependency, firmware modification or additional package installation.
- Run appropriate verification and create a local Git commit containing this task's files only.

## Task 1: Scanner and data model

**Files:** Create `scripts/generate-structure-index.py`, `tests/structure_index/test_structure_index.py`.

- [x] Write behavior tests with hand-checked C fixtures for regular, tagged, anonymous, nested and local records; packed syntax; arrays, bitfields, callbacks and comma declarations; duplicate names and conditional branches; comment/string masking and encoding.
- [x] Run `python -m unittest discover -s tests/structure_index -v` and observe the missing implementation fail.
- [x] Implement `parse_source(source, path)` and repository discovery with standard-library tokenization and balanced delimiters. Reject unsupported declarations explicitly.
- [x] Verify fixtures and audit all repository definitions before rendering.

## Task 2: Searchable Markdown and update commands

**Files:** Extend generator/tests; create `docs/structure-index.md`, `scripts/update-structure-index.ps1`, `scripts/update-structure-index.cmd`; update `README.md`.

- [x] Test Chinese module search, member ownership, duplicate identity, nested links, repository-wide discovery, deterministic generation and stale `--check` behavior with temporary directories.
- [x] Implement `generate(root)` returning Markdown. Include per-source coverage, type/member indexes, full member declarations, comments, conditions and nested relationships.
- [x] Implement `--root` for isolated test fixtures and `--check` for read-only freshness checking; write only after complete successful generation.
- [x] Add the PowerShell launcher with explicit interpreter override, local discovery and nonzero failure propagation; add double-click CMD launcher with `STRUCTURE_INDEX_NO_PAUSE=1` for automation.
- [x] Add README links and commands, then generate the complete dictionary.

## Task 3: Verify and review

- [x] Run unit and CLI tests, update from outside the repository directory, and verify `--check` passes with no output changes on a second generation.
- [x] Check every generated source link against a file and one-based source line; verify known project, vendor, private and anonymous definitions and representative member reverse lookups.
- [x] Request an independent code review using `requesting-code-review`; address substantive findings and repeat affected checks.
- [x] Review `git diff --check` and staged file list, then create a local commit and report the commit identifier.

## Verification results

- 26 standard-library unittest tests pass, covering CLI freshness checks, Windows checkout line endings, write failure preservation, legacy encodings, macro fields and aliases in addition to C declarations.
- The repository inventory contains 631 C/H files, 867 structs, 76 unions and 7056 member records, including all 127 USB telemetry X-macro fields and FreeRTOS optional integrity members.
- Source locations and generated internal anchors were audited; no unsupported record headers were found in the current repository. Empty placeholder files remain listed without fabricated line links.
- Both Windows launchers run from an unrelated working directory; automatic and explicit Python discovery work, and `-Check` propagates a stale-index failure.
- Independent review findings were addressed with regressions: indented/inline declaration macros, X-macro field coverage, searchable typedef aliases, and atomic output publication.
