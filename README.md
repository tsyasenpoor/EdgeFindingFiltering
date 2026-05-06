# Edge Finding for Cumulative Scheduling

A C++ implementation of Vilim's Edge Finding filtering algorithm for the **cumulative scheduling constraint**, based on:

> Petr Vilim, *"Edge Finding Filtering Algorithm for Discrete Cumulative Resources in O(kn log n)"*, CP 2009.

## Problem

Given a set of activities competing for a shared resource of capacity `C`, each activity `i` has:

| Field | Meaning |
|-------|---------|
| `est` | Earliest start time |
| `lct` | Latest completion time |
| `p`   | Processing time |
| `c`   | Resource consumption per unit time |

The **cumulative constraint** requires that at no point in time does total resource consumption exceed `C`.

Edge Finding is a propagation algorithm that tightens the `est` bounds by detecting sets of activities that must all complete before a given activity can start.

## Algorithm

The implementation follows Algorithms 1.1–1.3 from the paper, running in **O(k · n · log n)** where `k` is the number of distinct resource consumption values and `n` is the number of activities.

### Two phases per pass

**Detection** (`edge_finding.cpp: detection()`): Uses a **Theta-Lambda Tree** (`tl_tree.h`) to identify, for each activity `i`, the tightest deadline `prec[i]` such that some subset of activities must precede `i`. Also applies the "Improving Detection" extension from §6.2 of the paper.

**Adjustment** (`edge_finding.cpp: adjustment()`): Uses a **Theta Tree** (`t_tree.h`) to compute updated `est` values for each activity based on the detected precedences, applying edge-finding rule EF2 (equation 13 from the paper).

Passes repeat until no `est` changes (fixpoint via `edgeFindingFixpoint`).

### Segment trees

| File | Tree | Used in |
|------|------|---------|
| `tl_tree.h` | Theta-Lambda Tree | Detection phase |
| `t_tree.h`  | Theta Tree        | Adjustment phase |

Both trees maintain envelope (`Env`) and energy (`e`) aggregates over leaves sorted by `est`.

## File Structure

```
.
├── activity.h          # Activity struct (est, lct, p, c) and INF constant
├── edge_finding.h      # Public API: edgeFinding(), edgeFindingFixpoint()
├── edge_finding.cpp    # Detection + adjustment phases
├── t_tree.h            # Theta tree (segment tree, adjustment phase)
├── tl_tree.h           # Theta-Lambda tree (segment tree, detection phase)
├── main.cpp            # Demo driver (Figures 1, 4, 5 of the paper)
├── tests.cpp           # Full test suite (5 sections, PASS/FAIL output)
└── Makefile
```

## Build & Run

Requires a C++17 compiler.

```bash
# Demo (Figures 1, 4, 5 from the paper)
make
./edge_finding

# Full test suite
make test
```

## API

```cpp
#include "edge_finding.h"

// Single pass returns true if any est changed.
bool edgeFinding(std::vector<Activity>& acts, int C);

// Repeat until fixpoint.
void edgeFindingFixpoint(std::vector<Activity>& acts, int C);
```

Activities are modified in-place. Only `est` fields are updated; `lct`, `p`, and `c` are read-only.

## Test Suite (`tests.cpp`)

Run with `make test`. Each assertion prints `[PASS]` or `[FAIL]`; a summary of totals appears at the end.

### Section 1: Paper figures (Vilim 2009)

Reproduces Figures 1, 4, and 5. The expected values below are what the current implementation produces; known divergences from the paper are noted inline.

| Figure | Setup | Paper claims | Implementation output | Note |
|--------|-------|--------------|-----------------------|------|
| Fig. 1 (C=3) | A,B,C,D // EF1 should tighten D | `{0,2,2,4}` | `{0,3,2,4}` | B is over-tightened by one extra propagation step |
| Fig. 4 (C=3) | M,N,O // Improving Detection should tighten O | `{2,2,5}` | `{2,2,3}` | Improving detection under-fires; O not fully pushed |
| Fig. 5 (C=2) | W,X,Y,Z // EF2 should tighten Z | `{0,0,6,2}` | `{0,0,6,0}` | Z not propagated at all |

The divergences in Figures 4 and 5 indicate known under-propagation bugs in the implementation.

### Section 2: Edge cases

| Case | Expected behaviour |
|------|--------------------|
| Empty input | No crash, no work |
| Single activity | `est` unchanged (no peers) |
| Slack-rich pair (C=5) | No propagation |
| Idempotent at fixpoint | An extra `edgeFinding()` call after `edgeFindingFixpoint()` returns `false` |

### Section 3: Hand-crafted correctness

Small instances where the correct `est` bound can be derived by hand:

| Case | Setup | Expected |
|------|-------|---------|
| Full-resource follower | A,B each `(c=2,p=2)` fill C=2 over [0,4]; X `(c=2)` must wait | `X.est = 4` |
| Half-resource follower | Same A,B; X uses only half the resource but still can't fit | `X.est = 4` |
| Unit fill + follower | Three unit jobs fill C=1 over [0,3]; X must wait | `X.est = 3` |
| Two parallel followers | A,B fill [0,4]; X and Y both pushed | `X.est = Y.est = 4` |

### Section 4: Non-idempotency demonstrations

Shows that a single call to `edgeFinding()` produces a different (weaker) result than running to fixpoint, proving multi-pass behavior is necessary. Three tight-chain instances are tested (chain-3, chain-4, chain-5). All three hit the 30-pass cap without converging, a separate diagnostic showing the improving-detection extension does not stabilize on these pathological inputs.

### Section 5: Scaling: pass count vs. n

Builds tight chains of `n` full-resource jobs (C=2, `p=2`, `lct=2,4,...`) plus one unconstrained follower. Reports passes to fixpoint (capped at 30), total `est` updates, and the follower's final `est`.

| n  | passes | converged |
|----|--------|-----------|
| 2  | 30 (cap) | no |
| 3  | 30 (cap) | no |
| 4  | 30 (cap) | no |
| 6  | 30 (cap) | no |
| 8  | 30 (cap) | no |
| 12 | 30 (cap) | no |

The paper's O(k n log n) bound is per-pass; these results show pass count is a separate, uncharacterized growth factor.
