// Benchmark driver for edge-finding plots.
// Usage:
//   ./bench runtime  k n reps   → print median µs for reps single-pass runs
//   ./bench passes   n          → print passes-to-fixpoint for staggered chain of length n
//   ./bench diverge  n passes   → simulate buggy behaviour (no lct cap), print A.est per pass
//   ./bench converge n passes   → real (fixed) behaviour, print first-activity est per pass

#include "edge_finding.h"
#include "t_tree.h"
#include "tl_tree.h"
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <numeric>
using namespace std;
using us = chrono::microseconds;
using clk = chrono::high_resolution_clock;

// ---------- instance generators ----------

// k distinct capacity values (1..k), n activities, resource cap C = k+1.
// Using C = k+1 is the tightest valid choice: it allows all k capacity values
// while ensuring no single activity monopolises the resource (c_max = k < C).
static vector<Activity> gen_random(int n, int k, int C, mt19937& rng) {
    vector<int> caps;
    for (int i = 0; i < k; i++) caps.push_back(i + 1);          // 1..k
    vector<Activity> acts;
    acts.reserve(n);
    uniform_int_distribution<int> cap_dist(0, k - 1);
    for (int i = 0; i < n; i++) {
        int c   = caps[cap_dist(rng)];
        int p   = (rng() % 4) + 1;
        int est = rng() % (2 * n);
        int lct = est + p + (rng() % (2 * n)) + 1;
        acts.push_back({est, lct, p, c});
    }
    return acts;
}

// Tight serialized chain of length n (full-resource, single capacity).
static vector<Activity> gen_tight_chain(int n, int C) {
    vector<Activity> acts;
    for (int i = 0; i < n; i++)
        acts.push_back({0, 2 * (i + 1), 2, C});   // c = C (full resource)
    acts.push_back({0, INF, 2, C});                // follower
    return acts;
}

// Staggered chain of length n (each activity fills a window of 3, C=1).
static vector<Activity> gen_staggered(int n) {
    vector<Activity> acts;
    for (int i = 0; i < n; i++)
        acts.push_back({0, 3 * (i + 1), 3, 1});
    acts.push_back({0, INF, 1, 1});
    return acts;
}

// ---------- buggy adjustment: no lct cap (reproduces divergence) ----------
static void adjustment_buggy(vector<Activity>& acts, int n, int C,
                              const vector<int>& est_order,
                              const vector<int>& leaf_pos,
                              const vector<int>& prec) {
    // Identical to the real adjustment but WITHOUT the lct-1 cap.
    // We reproduce the relevant logic here inline for clarity.
    // (We just re-expose the piece that was wrong.)
    // For the plot we only need the per-pass est values, so we replicate
    // the full edgeFinding logic but swap cap = min(pi, lct-1) → cap = pi.

    // We build a minimal version: call the real adjustment but then undo the
    // cap. Instead, we expose a standalone buggy function.
    // SIMPLE APPROACH: just call adjustment with a hacked prec that ignores the
    // lct constraint. We achieve this by using a version of the update-table
    // lookup that uses pi directly (no lct-1 cap). Rather than duplicating
    // 80 lines, we patch acts after the real call to re-apply the buggy logic.
    //
    // Easiest: just implement the one-liner difference.
    // We copy-paste the adjustment loop here with cap = pi instead of min(pi, lct-1).
    // (This is intentionally wrong — it's the pre-fix version for the plot.)

    set<int> cap_set;
    for (auto& a : acts) cap_set.insert(a.c);

    vector<int> by_lct_asc(n); iota(by_lct_asc.begin(), by_lct_asc.end(), 0);
    sort(by_lct_asc.begin(), by_lct_asc.end(),
         [&](int a, int b){ return acts[a].lct < acts[b].lct; });

    map<int, vector<pair<int,long long>>> utbl;
    ThetaTree tt(n, C);

    for (int c : cap_set) {
        tt.reset();
        long long upd = -INF;
        auto& uvec = utbl[c];
        for (int j : by_lct_asc) {
            tt.add(leaf_pos[j], acts[j].est, acts[j].e(), c);
            int split = tt.maxestPos((long long)acts[j].lct, c);
            if (split >= 0 && c > 0) {
                long long ea = tt.queryPrefixEnv(split);
                long long eb = tt.querySuffixE(split + 1);
                long long Env_jc = eb + ea;
                long long num = Env_jc - (long long)(C - c) * acts[j].lct;
                if (num > 0) {
                    long long d = (num + c - 1) / c;
                    upd = max(upd, d);
                }
            }
            uvec.push_back({acts[j].lct, upd});
        }
    }

    // EF2 — buggy: cap = pi (no lct-1 limit)
    for (int i = 0; i < n; i++) {
        int pi = prec[i];
        auto it_tbl = utbl.find(acts[i].c);
        if (it_tbl == utbl.end()) continue;
        auto& vec = it_tbl->second;
        // BUG: use pi directly, not min(pi, lct-1)
        auto it = upper_bound(vec.begin(), vec.end(),
                              make_pair(pi, (long long)INF));
        if (it == vec.begin()) continue;
        --it;
        long long nv = it->second;
        if (nv > (long long)acts[i].est) acts[i].est = (int)nv;
    }
}

// Buggy single-pass edgeFinding (for divergence plot).
static bool edgeFinding_buggy(vector<Activity>& acts, int C) {
    int n = (int)acts.size();
    if (n == 0) return false;

    vector<int> est_order(n); iota(est_order.begin(), est_order.end(), 0);
    sort(est_order.begin(), est_order.end(),
         [&](int a, int b){ return acts[a].est < acts[b].est; });
    vector<int> leaf_pos(n);
    for (int pos = 0; pos < n; pos++) leaf_pos[est_order[pos]] = pos;

    vector<int> old_est(n);
    for (int i = 0; i < n; i++) old_est[i] = acts[i].est;

    // detection (identical to real)
    ThetaLambdaTree tl(n, C);
    for (int pos = 0; pos < n; pos++) {
        int i = est_order[pos];
        tl.setTheta(pos, acts[i].est, acts[i].e());
    }
    vector<int> by_lct(n); iota(by_lct.begin(), by_lct.end(), 0);
    sort(by_lct.begin(), by_lct.end(),
         [&](int a, int b){ return acts[a].lct > acts[b].lct; });
    vector<int> prec(n, -INF);
    for (int j : by_lct) {
        while (tl.rootEnvL() > (long long)C * acts[j].lct) {
            int pos = tl.findResponsible();
            int i   = est_order[pos];
            prec[i] = acts[j].lct;
            tl.unset(pos);
        }
        tl.setLambda(leaf_pos[j], acts[j].est, acts[j].e());
    }
    for (int i = 0; i < n; i++)
        prec[i] = max(prec[i], acts[i].est + acts[i].p);

    adjustment_buggy(acts, n, C, est_order, leaf_pos, prec);

    for (int i = 0; i < n; i++)
        if (acts[i].est != old_est[i]) return true;
    return false;
}

// ---------- main ----------
int main(int argc, char** argv) {
    if (argc < 2) { cerr << "usage: bench <mode> ...\n"; return 1; }
    string mode = argv[1];

    if (mode == "runtime") {
        // ./bench runtime k n reps
        int k    = stoi(argv[2]);
        int n    = stoi(argv[3]);
        int reps = stoi(argv[4]);
        int C    = k + 1;

        mt19937 rng(1234);
        vector<long long> times;
        times.reserve(reps);
        for (int r = 0; r < reps; r++) {
            auto acts = gen_random(n, k, C, rng);
            auto t0 = clk::now();
            edgeFinding(acts, C);
            auto t1 = clk::now();
            times.push_back(chrono::duration_cast<us>(t1 - t0).count());
        }
        sort(times.begin(), times.end());
        cout << times[reps / 2] << "\n";   // median µs

    } else if (mode == "passes") {
        // ./bench passes n  → staggered chain
        int n = stoi(argv[2]);
        auto acts = gen_staggered(n);
        int passes = 0;
        while (edgeFinding(acts, 1)) passes++;
        cout << passes << "\n";

    } else if (mode == "rand_passes") {
        // ./bench rand_passes k n trials  → mean,max passes for random instances
        int k      = stoi(argv[2]);
        int n      = stoi(argv[3]);
        int trials = stoi(argv[4]);
        int C      = k + 1;
        mt19937 rng(42);
        long long total = 0;
        int mx = 0;
        for (int t = 0; t < trials; t++) {
            auto acts = gen_random(n, k, C, rng);
            int passes = 0;
            while (edgeFinding(acts, C) && passes < 50) passes++;
            total += passes;
            mx = max(mx, passes);
        }
        // mean max
        cout << fixed;
        cout << (double)total / trials << " " << mx << "\n";

    } else if (mode == "cascade_passes") {
        // ./bench cascade_passes m  → passes for cascading chain of depth m
        // Each stage adds a tight A + near-tight B; total n = 2m+1 activities.
        // Demonstrates O(m) pass growth on structured instances.
        int m = stoi(argv[2]);
        vector<Activity> acts;
        int offset = 0;
        for (int i = 0; i < m; i++) {
            acts.push_back({offset+2, offset+6,  4, 1});
            acts.push_back({offset+4, offset+10, 2, 1});
            offset += 8;
        }
        acts.push_back({0, INF, 3, 1});
        int passes = 0;
        while (edgeFinding(acts, 1) && passes < 500) passes++;
        cout << passes << "\n";

    } else if (mode == "pass_hist") {
        // ./bench pass_hist k n trials  → one pass count per line
        int k      = stoi(argv[2]);
        int n      = stoi(argv[3]);
        int trials = stoi(argv[4]);
        int C      = k + 1;
        mt19937 rng(42);
        for (int t = 0; t < trials; t++) {
            auto acts = gen_random(n, k, C, rng);
            int passes = 0;
            while (edgeFinding(acts, C) && passes < 50) passes++;
            cout << passes << "\n";
        }

    } else if (mode == "diverge") {
        // ./bench diverge n max_passes  → print A.est per pass (buggy)
        int n     = stoi(argv[2]);
        int maxp  = stoi(argv[3]);
        auto acts = gen_tight_chain(n, 2);
        cout << acts[0].est;   // pass 0
        for (int p = 0; p < maxp; p++) {
            bool ch = edgeFinding_buggy(acts, 2);
            cout << " " << acts[0].est;
            if (!ch) break;
        }
        cout << "\n";

    } else if (mode == "converge") {
        // ./bench converge n max_passes  → print A.est per pass (fixed)
        int n     = stoi(argv[2]);
        int maxp  = stoi(argv[3]);
        auto acts = gen_tight_chain(n, 2);
        cout << acts[0].est;
        for (int p = 0; p < maxp; p++) {
            bool ch = edgeFinding(acts, 2);
            cout << " " << acts[0].est;
            if (!ch) break;
        }
        cout << "\n";

    } else {
        cerr << "unknown mode: " << mode << "\n";
        return 1;
    }
    return 0;
}
