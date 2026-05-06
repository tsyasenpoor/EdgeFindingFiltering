#pragma once
// Theta-Lambda tree for Edge Finding detection phase.
// Implements Proposition 2 and Algorithm 1.1 from Vilim (2009).
#include "activity.h"
#include <vector>
#include <algorithm>

struct TLNode { long long e, eL, Env, EnvL; };

inline TLNode tl_empty() { return {0, -INF, -INF, -INF}; }

inline TLNode tl_combine(const TLNode& L, const TLNode& R) {
    return {
        L.e + R.e,
        std::max(L.eL + R.e, L.e + R.eL),
        std::max(L.Env + R.e, R.Env),
        std::max({L.EnvL + R.e, L.Env + R.eL, R.EnvL})
    };
}

class ThetaLambdaTree {
public:
    int n, C;
    std::vector<TLNode> t;

    ThetaLambdaTree(int n_, int C_) : n(n_), C(C_), t(4 * n_, tl_empty()) {}

    void setTheta(int pos, long long est, long long ei) {
        upd(1, 0, n-1, pos, {ei, -INF, C * est + ei, -INF});
    }

    void setLambda(int pos, long long est, long long ei) {
        upd(1, 0, n-1, pos, {0, ei, -INF, C * est + ei});
    }

    void unset(int pos) { upd(1, 0, n-1, pos, tl_empty()); }

    long long rootEnvL() const { return t[1].EnvL; }

    // Returns 0..n-1 leaf position of activity responsible for root's EnvL.
    int findResponsible() const { return searchEnvL(1, 0, n-1); }

private:
    void upd(int v, int lo, int hi, int pos, TLNode val) {
        if (lo == hi) { t[v] = val; return; }
        int mid = (lo + hi) / 2;
        if (pos <= mid) upd(2*v,   lo,    mid, pos, val);
        else            upd(2*v+1, mid+1, hi,  pos, val);
        t[v] = tl_combine(t[2*v], t[2*v+1]);
    }

    // responsible_EnvL: find leaf responsible for EnvL in subtree v
    int searchEnvL(int v, int lo, int hi) const {
        if (lo == hi) return lo;
        int mid = (lo + hi) / 2;
        auto& L = t[2*v]; auto& R = t[2*v+1];
        if (t[v].EnvL == R.EnvL)
            return searchEnvL(2*v+1, mid+1, hi);      // case: EnvL_right
        if (t[v].EnvL == L.Env + R.eL)
            return searchEL(2*v+1, mid+1, hi);         // case: Env_left + eL_right
        return searchEnvL(2*v, lo, mid);               // case: EnvL_left + e_right
    }

    // responsible_eL: find leaf responsible for eL in subtree v
    int searchEL(int v, int lo, int hi) const {
        if (lo == hi) return lo;
        int mid = (lo + hi) / 2;
        auto& L = t[2*v]; auto& R = t[2*v+1];
        if (t[v].eL == L.e + R.eL)
            return searchEL(2*v+1, mid+1, hi);
        return searchEL(2*v, lo, mid);
    }
};
