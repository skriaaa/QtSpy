# Alloc Profiler Debug Design

Date: 2026-07-10

## Context

QtSpy has an existing `AllocProfiler` implementation for Windows that uses Detours to hook CRT allocation/free paths, captures stacks, aggregates live and total allocation statistics, resolves symbols lazily, and displays the data in `MemoryMonitorDlg`.

The next goal is to make the profiler useful for Qt C++ Windows desktop applications where QtSpy is loaded at runtime and the user's project cannot be changed. The profiler only needs to support Debug builds.

## Goals

- Monitor heap allocations caused by user code without requiring headers, macros, allocator replacement, linker changes, or source edits in the target project.
- Support two views while the final product direction is still being evaluated:
  - Direct allocation view: allocations that appear to come directly from user code.
  - Business attribution view: allocations triggered by user call chains, even when the actual allocation happens inside Qt/STL/framework code.
- Prioritize leak investigation: live bytes, live count, mark/diff, and call stack drill-down are more important than raw total allocation throughput.
- Still expose allocation hot spots by total bytes and total count.
- Use Debug information as the main path: PDB symbols, file/line data, Debug CRT free paths, and Debug CRT heap layout assumptions are acceptable dependencies.

## Non-Goals

- Release-build accuracy is not a target.
- The profiler will not require user project code changes.
- The profiler will not claim perfect semantic detection of every C++ `operator new` and `operator delete`. It will infer user-facing allocation sites from CRT hooks, stack frames, modules, and source paths.
- System-wide allocation tracking is not a target; only the current process loaded with QtSpy is in scope.

## Recommended Approach

Use the existing CRT hook design as the foundation and specialize it for Debug builds.

The profiler should continue to hook the actual CRT module loaded in the target process. In Debug builds, it should prefer `_malloc_dbg` and `_free_dbg` when available, with `malloc` and `free` kept as compatibility paths. Hooking `operator new/delete` directly is not the primary approach because MSVC and CRT forwarding paths can vary by module and toolchain, and Qt/STL internals use the same operators. CRT-level hooks provide broader pairing and reuse the current implementation.

## Collection Architecture

The hook hot path should stay small:

- Allocate/free through the true CRT function.
- Capture pointer, size, event sequence, and raw stack frames.
- Apply a cheap module-level guard where possible.
- Store raw stack information and address bookkeeping.
- Avoid symbol resolution, source-path checks, and UI formatting.

Live allocation state is maintained as:

- `addr -> size + stackId + seq + flags`
- `stackId -> raw frames + captured metadata`
- `stackId -> live/total aggregation`

Free events should continue to use an asynchronous queue. The sequence number model must stay: allocation and free events both receive monotonic sequence numbers so stale free events do not incorrectly cancel newer allocations after address reuse.

Debug CRT support should include:

- Prefer `_malloc_dbg/_free_dbg` when exported by the loaded CRT.
- Keep `malloc/free` hooks as fallback or supplemental paths.
- Preserve `HeapWalk`/reap validation to clean up entries that were missed because of Debug CRT delete/free path differences.
- Account for Debug CRT block headers when validating heap liveness.

## User Code Classification

User-code detection has two stages.

First, classify by module:

- Main executable is a user module by default.
- Non-skipped DLLs are candidate user modules.
- Skip Qt, CRT, MSVC runtime/STL, Windows API, DbgHelp, Detours, and QtSpy modules.

Second, refine by source path after symbols are ready:

- Paths under Qt source/build trees are framework frames.
- Paths under Visual Studio, MSVC, STL, and Windows Kits are system/toolchain frames.
- Paths under QtSpy itself are profiler frames.
- Candidate user-module frames with framework/system source paths are downgraded to non-user frames.
- Frames without file/line data can still be used by module inference, but the UI should mark them as module-inferred.

This combined rule is the default source of truth for both views.

## Attribution Modes

### Direct Allocation View

The direct view answers: "Where does user code appear to directly allocate?"

The attribution scan starts near the CRT allocation frame and walks outward. The first user frame can be used if it is close enough to the allocation path and the intervening frames are CRT/new/malloc plumbing. If the scan crosses Qt/STL/framework frames before reaching user code, the allocation should be treated as framework-internal work triggered by user code, not as a confident direct allocation.

Rows in this view should include a confidence indicator:

- High: user frame found near CRT/new plumbing with source line.
- Medium: user frame inferred by module without source line.
- Low: user frame exists only after framework frames; normally filtered out or displayed as low confidence.

### Business Attribution View

The business view answers: "Which user call sites are responsible for live memory or allocation volume?"

The scan skips Qt, CRT, STL, Windows API, and other non-user frames and attributes the allocation to the nearest user frame in the call chain. This view should include allocations triggered by calls such as `new QWidget`, `QVector::push_back`, or `QString` operations, even when the final heap allocation occurs inside framework code.

### Attribution Types

Each displayed site should carry an attribution type:

- `DirectUserAlloc`: allocation appears directly caused by user code.
- `UserTriggeredFrameworkAlloc`: allocation happens inside framework/library code but is triggered by a user call chain.
- `UnknownOrSystem`: no reliable user frame was found.

The UI can initially show only the first two types and keep unknown/system data hidden unless a diagnostic mode is added later.

## API Shape

Extend the public API with an attribution mode rather than duplicating collection:

```cpp
namespace AllocProfiler
{
	enum class AttributionMode
	{
		DirectAllocation,
		BusinessAttribution
	};

	enum class AttributionType
	{
		DirectUserAlloc,
		UserTriggeredFrameworkAlloc,
		UnknownOrSystem
	};

	struct SiteView
	{
		quint64 stackHash = 0;
		QString module;
		QString symbol;
		QString fileLine;
		quint64 liveBytes = 0;
		quint64 liveCount = 0;
		quint64 totalBytes = 0;
		quint64 totalCount = 0;
		AttributionType attributionType = AttributionType::UnknownOrSystem;
		int confidence = 0;
		bool sourceLineReady = false;
	};

	QVector<SiteView> topSites(int n, bool byLive, AttributionMode mode);
	QVector<SiteView> diffVsMark(int n, bool byLive, AttributionMode mode);
}
```

Existing overloads can remain temporarily and delegate to `BusinessAttribution` to avoid a large UI change in one step.

## UI Design

In the Memory Allocation tab:

- Add a compact view switch: "Direct allocation" and "Business attribution".
- Default to Business attribution because leak investigation is the main workflow.
- Keep sorting by live bytes by default.
- Add a "Type/Confidence" column.
- Keep the existing min-size filter, symbol merge option, clear button, double-click backtrace dialog, and symbol detail dialog.
- Mark symbol status clearly:
  - Loading: addresses/modules may be shown.
  - Ready: source lines and refined user classification are available.

The backtrace dialog should continue to show the full captured stack after filtering framework/system frames by default. A future diagnostic toggle can show all frames if needed, but it is not required for the first implementation.

## Error Handling and Edge Cases

- If Detours attach fails, `attach()` should report not attached and the UI should not pretend monitoring is active.
- If `_malloc_dbg` is unavailable, fall back to `malloc` and mark the session as less Debug-CRT-specific.
- If symbols are not ready, use module inference and refresh rows once symbols become available.
- If a free event is stale because the address was reused, ignore it.
- If a pointer is reallocated at the same address before a queued free is processed, adjust the previous live accounting when recording the new allocation.
- If HeapWalk cannot enumerate a heap safely, skip that heap and keep the profiler running.
- `thread_local` recursion guards must remain around hook internals and symbol/cache work to avoid profiler self-allocation loops.

## Verification Plan

Use a Debug Qt test application loaded with QtSpy and PDBs available.

Scenarios:

- Direct `new/delete` in a user function appears in Direct allocation view and live count returns to zero after delete.
- Direct leaked `new` appears in mark/diff with the user source line.
- `QVector`, `QString`, or `QWidget` calls from user code appear in Business attribution view and are not misrepresented as high-confidence direct allocations.
- Allocations from Qt internals without a user frame do not appear in normal user views.
- Repeated allocate/free at the same address does not corrupt live accounting.
- Debug CRT delete path is handled through `_free_dbg` or corrected by reap validation.
- Before symbols finish loading, rows show module/address fallback; after symbols are ready, rows refresh with symbol and file/line.

## Implementation Order

1. Add attribution enums and compatibility overloads.
2. Separate raw stack capture from display attribution logic.
3. Implement module and source-path user-frame classification helpers.
4. Implement Direct allocation and Business attribution scans.
5. Route `topSites` and `diffVsMark` through the selected attribution mode.
6. Add the UI view switch and Type/Confidence column.
7. Add focused Debug test scenarios and manual verification notes.

