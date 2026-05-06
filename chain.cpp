#include "edge_finding.h"
#include <iostream>
#include <vector>
using namespace std;

// Build a cascading chain of depth m:
// Each "stage" i has a tight activity A_i and a near-tight B_i.
// Each B_i's update depends on A_i's updated est, which itself depends
// on the previous stage — so propagation cascades one stage per pass.
static vector<Activity> gen_cascade(int m) {
    // Stage pattern from tests Section 4:
    //   A(2,6,4,1), B(4,10,2,1) → 2 passes with follower
    // Extend: each stage is offset by 8 (the final X.est of the previous)
    vector<Activity> acts;
    int offset = 0;
    for (int i = 0; i < m; i++) {
        acts.push_back({offset+2, offset+6, 4, 1});   // tight A
        acts.push_back({offset+4, offset+10, 2, 1});  // near-tight B
        offset += 8;
    }
    acts.push_back({0, INF, 3, 1});  // follower X
    return acts;
}

int main() {
    cout << "depth passes\n";
    for (int m = 1; m <= 10; m++) {
        auto acts = gen_cascade(m);
        int passes = 0;
        while (edgeFinding(acts, 1) && passes < 100) passes++;
        cout << m << " " << passes << "\n";
    }
}
