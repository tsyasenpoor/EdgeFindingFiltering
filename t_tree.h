#pragma once
// Theta tree for Edge Finding adjustment phase.
// Implements Algorithms 1.2 and 1.3 from Vilim (2009).
// Stores e, Env (coefficient C), and Envc (coefficient C-c) per node.
#include "activity.h"
#include <vector>
#include <algorithm>

struct TNode { long long e, Env, Envc; };

inline TNode tn_empty() { return {0, -INF, -INF}; }

inline TNode tn_combine(const TNode& L, const TNode& R) {
    return {
        L.e + R.e,
        std::max(L.Env  + R.e, R.Env),
        std::max(L.Envc + R.e, R.Envc)
    };
}

class ThetaTree {
public:
    int n, C;
    std::vector<TNode>     t;
    std::vector<long long> est_at;  // est_at[pos] = est of added activity, else -INF

    ThetaTree(int n_, int C_) : n(n_), C(C_), t(4*n_, tn_empty()), est_at(n_, -INF) {}

    void reset() {
        std::fill(t.begin(), t.end(), tn_empty());
        std::fill(est_at.begin(), est_at.end(), (long long)-INF);
    }

    // Activate leaf pos with given est, energy ei, and current test-capacity c.
    void add(int pos, long long est, long long ei, int c) {
        est_at[pos] = est;
        upd(1, 0, n-1, pos, {ei, C * est + ei, (long long)(C - c) * est + ei});
    }

    // Algorithm 1.2: returns leaf position (0..n-1) of the boundary activity,
    // or -1 if Envc(root) <= (C-c)*lctj (no valid Omega exists).
    int maxestPos(long long lctj, int c) const {
        long long thr = (long long)(C - c) * lctj;
        if (t[1].Envc <= thr) return -1;
        int v = 1, lo = 0, hi = n - 1;
        long long E = 0;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (t[2*v+1].Envc + E > thr) { v = 2*v+1; lo = mid + 1; }
            else                          { E += t[2*v+1].e; v = 2*v; hi = mid; }
        }
        return lo;  // lo == hi
    }

    // Env(prefix [0..pos]) — returns -INF if no active leaf in range.
    long long queryPrefixEnv(int pos) const {
        if (pos < 0)    return -INF;
        if (pos >= n-1) return t[1].Env;
        return qPrefixEnv(1, 0, n-1, pos);
    }

    // e_sum(suffix [from_pos..n-1])
    long long querySuffixE(int from_pos) const {
        if (from_pos > n-1) return 0;
        if (from_pos <= 0)  return t[1].e;
        return qSuffixE(1, 0, n-1, from_pos);
    }

private:
    void upd(int v, int lo, int hi, int pos, TNode val) {
        if (lo == hi) { t[v] = val; return; }
        int mid = (lo + hi) / 2;
        if (pos <= mid) upd(2*v,   lo,    mid, pos, val);
        else            upd(2*v+1, mid+1, hi,  pos, val);
        t[v] = tn_combine(t[2*v], t[2*v+1]);
    }

    long long qPrefixEnv(int v, int lo, int hi, int pos) const {
        if (hi <= pos) return t[v].Env;
        if (lo > pos)  return -INF;
        int mid = (lo + hi) / 2;
        long long eR = qPrefixE(2*v+1, mid+1, hi, pos);
        long long envL = qPrefixEnv(2*v, lo, mid, pos);
        long long envR = qPrefixEnv(2*v+1, mid+1, hi, pos);
        return std::max(envL + eR, envR);
    }

    long long qPrefixE(int v, int lo, int hi, int pos) const {
        if (hi <= pos) return t[v].e;
        if (lo > pos)  return 0;
        int mid = (lo + hi) / 2;
        return qPrefixE(2*v, lo, mid, pos) + qPrefixE(2*v+1, mid+1, hi, pos);
    }

    long long qSuffixE(int v, int lo, int hi, int from) const {
        if (lo >= from) return t[v].e;
        if (hi < from)  return 0;
        int mid = (lo + hi) / 2;
        return qSuffixE(2*v, lo, mid, from) + qSuffixE(2*v+1, mid+1, hi, from);
    }
};
