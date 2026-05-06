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

Run with `make test`. Each assertion prints `[PASS]` or `[FAIL]`; a summary appears at the end. **All 16 tests pass.**

### Section 1: Paper figures (Vilim 2009)

| Figure | Paper claims | Implementation output | Status |
|--------|-------------|----------------------|--------|
| Fig. 1 (C=3): D tightened by EF1 | `A=0 B=2 C=2 D=4` | `A=0 B=2 C=2 D=4` | ✓ exact match |
| Fig. 4 (C=3): Improving Detection on O | `O=5` | `O=5` | ✓ exact match |
| Fig. 5 (C=2): EF2 on Z | `Z=2` | `Z=2` | ✓ exact match |

### Section 2: Edge cases

Empty input, single activity, slack-rich pair, idempotency at fixpoint, and soundness (`est ≤ lct` after propagation).

### Section 3: Hand-crafted correctness

| Case | Expected |
|------|----------|
| Full-resource follower | `X.est = 4` |
| Half-resource follower | `X.est = 4` |
| Unit fill + follower | `X.est = 3` |
| Two parallel followers | `X.est = Y.est = 4` |
| Tight serialized chain | `A=0 B=2 C=4 X=6` |

### Section 4: Non-idempotency demonstrations

Edge Finding is not idempotent: a second pass can find tighter bounds using the updated `est` values from the first. These examples confirm:

1. `{A(2,6,4,1), B(4,10,2,1), X(0,INF,3,1)}` at C=1 converges in **2 productive passes** to `{2, 6, 8}`. Single pass gives `X.est=6`; fixpoint gives `X.est=8`.
2. A 6-activity instance at C=1 converges in **2 productive passes** with 9 `est` updates total.

Both fixpoints are sound (`est_i ≤ lct_i` for all `i`).

