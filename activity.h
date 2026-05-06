#pragma once
#include <climits>
#include <vector>
#include <algorithm>

static constexpr int INF = INT_MAX / 2;

struct Activity {
    int est, lct, p, c;
    int e() const { return c * p; }
};
