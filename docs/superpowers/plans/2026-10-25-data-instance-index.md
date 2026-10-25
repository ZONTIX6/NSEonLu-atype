# Data Instance Index Implementation Plan

**Goal:** Find actual data holders and pointer access paths from Chinese purpose or member name.

**Architecture:** Reuse the existing source/type inventory; discover typed variable declarations and their lexical scopes; resolve direct assignment evidence; render persistent data roots and recursively expanded member paths into a separate Markdown index.

**Constraints:** Python standard library; preserve existing unrelated firmware changes; all source origins remain visible; no invented runtime addresses or accessors; local temporaries and extern declarations are explicitly classified.

**User scope update:** The default entry is now a concise common-data reference (remote, motors, meter, capacitor, referee, IMU and velocity), containing pointer names, a few useful member expressions and source locations. The exhaustive instance renderer remains optional through `--full`, writing a separate file; normal one-click updates keep the primary reference short.

The compact default has 10 category rows, checked against real member paths and source locations; all 48 regression tests pass. Review corrected the RC note to identify the control-task snapshot without claiming that its unused second element holds historical data.

- [x] Write failing fixture tests for global/static/local instances, multi-declarations, arrays, externs, function/parameter exclusion, anonymous objects and direct assignment scope.
- [x] Expose the existing repository type inventory without changing type-index output apart from a link to the new primary index, then implement declaration discovery.
- [x] Test and implement recursive pointer/object access paths, alias pointer depth, cycles and ambiguous type candidates.
- [x] Generate `docs/data-instance-index.md`; integrate both indices with the existing Windows update/check commands and README links.
- [x] Verify real motor, power meter, supercapacitor, RC, chassis velocity and power-controller data roots; audit every generated source/type link.
- [x] Run relevant regression tests, deterministic generation, launchers and independent review; fix substantive findings; commit only task changes.

## Verification

- 46 tests pass: 26 existing type-index regressions and 20 data-instance tests, including conditional call syntax, real lexical scopes, primitive typedef/loop shadows, inline pointer typedefs and array aliases.
- Current repository inventory: 631 C/H files; 449 persistent instance/extern declarations and 1567 local instances. Counts describe declarations, not unique runtime addresses.
- All 3842 generated source locations, 2640 links to type definitions and 17705 internal instance links were checked. Real motor, power meter, capacitor, remote, velocity and power-controller member paths are present.
- Power meter and capacitor roots resolve to their initialization-local allocation pointers; the power-control pointer resolves to static storage. Anonymous object member writes are not counted as variable declarations.
- Windows CMD/PowerShell launchers update/check both documents from an unrelated current directory. Both outputs are deterministic.
- Independent read-only review verified the reported fixes and found no remaining blocking P1/P2 issues.
