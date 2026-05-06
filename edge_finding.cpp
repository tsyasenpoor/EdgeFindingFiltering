#include "edge_finding.h"
#include "tl_tree.h"
#include "t_tree.h"
#include <numeric>
#include <algorithm>
#include <map>
#include <set>

using namespace std;

// Build sorted-by-est leaf ordering.
static void buildOrder(const vector<Activity>& acts, int n,
                       vector<int>& est_order, vector<int>& leaf_pos) {
    est_order.resize(n); iota(est_order.begin(), est_order.end(), 0);
    sort(est_order.begin(), est_order.end(),
         [&](int a, int b){ return acts[a].est < acts[b].est; });
    leaf_pos.resize(n);
    for (int pos = 0; pos < n; pos++) leaf_pos[est_order[pos]] = pos;
}

// Algorithm 1.1: Detection phase. Populates prec[i] = lct_j when LCut(T,j) << i.
static void detection(const vector<Activity>& acts, int n, int C,
                      const vector<int>& est_order, const vector<int>& leaf_pos,
                      vector<int>& prec) {
    ThetaLambdaTree tl(n, C);
    for (int pos = 0; pos < n; pos++) {
        int i = est_order[pos];
        tl.setTheta(pos, acts[i].est, acts[i].e());
    }

    vector<int> by_lct(n); iota(by_lct.begin(), by_lct.end(), 0);
    sort(by_lct.begin(), by_lct.end(),
         [&](int a, int b){ return acts[a].lct > acts[b].lct; });

    for (int j : by_lct) {
        while (tl.rootEnvL() > (long long)C * acts[j].lct) {
            int pos = tl.findResponsible();
            int i   = est_order[pos];
            prec[i] = acts[j].lct;
            tl.unset(pos);
        }
        tl.setLambda(leaf_pos[j], acts[j].est, acts[j].e());
    }
}

// ceiling division for positive numerator and divisor
static long long ceildiv(long long num, int den) {
    return (num + den - 1) / den;
}

// Algorithms 1.2 + 1.3: Adjustment phase. Applies EF2 to update est values.
static void adjustment(vector<Activity>& acts, int n, int C,
                       const vector<int>& est_order, const vector<int>& leaf_pos,
                       const vector<int>& prec) {
    set<int> cap_set;
    for (auto& a : acts) cap_set.insert(a.c);

    vector<int> by_lct_asc(n); iota(by_lct_asc.begin(), by_lct_asc.end(), 0);
    sort(by_lct_asc.begin(), by_lct_asc.end(),
         [&](int a, int b){ return acts[a].lct < acts[b].lct; });

    // update_table[c] = sorted (lct_j, cumulative_upd) pairs
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
                long long Env_jc = eb + ea;            // eq (15)
                long long num = Env_jc - (long long)(C - c) * acts[j].lct;
                if (num > 0) upd = max(upd, ceildiv(num, c));  // eq (13)
            }
            uvec.push_back({acts[j].lct, upd});
        }
    }

    // EF2: apply updates
    for (int i = 0; i < n; i++) {
        int pi = prec[i];
        auto it_tbl = utbl.find(acts[i].c);
        if (it_tbl == utbl.end()) continue;
        auto& vec = it_tbl->second;
        // largest lct_j <= pi
        auto it = upper_bound(vec.begin(), vec.end(),
                              make_pair(pi, (long long)INF));
        if (it == vec.begin()) continue;
        --it;
        long long nv = it->second;
        if (nv > (long long)acts[i].est) acts[i].est = (int)nv;
    }
}

bool edgeFinding(vector<Activity>& acts, int C) {
    int n = (int)acts.size();
    if (n == 0) return false;

    vector<int> est_order, leaf_pos;
    buildOrder(acts, n, est_order, leaf_pos);

    vector<int> old_est(n);
    for (int i = 0; i < n; i++) old_est[i] = acts[i].est;

    // Detection
    vector<int> prec(n, -INF);
    detection(acts, n, C, est_order, leaf_pos, prec);

    // Improving detection §6.2
    for (int i = 0; i < n; i++)
        prec[i] = max(prec[i], acts[i].est + acts[i].p);

    // Adjustment
    adjustment(acts, n, C, est_order, leaf_pos, prec);

    for (int i = 0; i < n; i++)
        if (acts[i].est != old_est[i]) return true;
    return false;
}

void edgeFindingFixpoint(vector<Activity>& acts, int C) {
    while (edgeFinding(acts, C)) {}
}
