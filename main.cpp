#include "edge_finding.h"
#include <iostream>
#include <vector>
#include <string>

static void run(const std::string& label, std::vector<Activity> acts,
                const std::vector<std::string>& names, int C) {
    std::cout << label << " (C=" << C << ")\n";
    std::cout << "  Before:";
    for (int i = 0; i < (int)acts.size(); i++)
        std::cout << "  " << names[i] << ".est=" << acts[i].est;
    std::cout << "\n";

    edgeFindingFixpoint(acts, C);

    std::cout << "  After: ";
    for (int i = 0; i < (int)acts.size(); i++)
        std::cout << "  " << names[i] << ".est=" << acts[i].est;
    std::cout << "\n\n";
}

int main() {
    // Figure 1: A(est=0,lct=5,p=1,c=3), B(est=2,lct=5,p=3,c=1),
    //           C(est=2,lct=5,p=2,c=2), D(est=0,lct=INF,p=3,c=2), C=3
    // Expected: est_D -> 4
    run("Figure 1",
        {{0, 5, 1, 3}, {2, 5, 3, 1}, {2, 5, 2, 2}, {0, INF, 3, 2}},
        {"A", "B", "C", "D"}, 3);

    // Figure 4: M(est=2,lct=5,p=1,c=1), N(est=2,lct=5,p=1,c=1),
    //           O(est=0,lct=INF,p=5,c=3), C=3
    // {M,N} << O but EF1 can't detect it; improving detection handles it.
    // Expected: est_O -> 5 (since est_O + p_O = 5 = lct_M = lct_N)
    run("Figure 4",
        {{2, 5, 1, 1}, {2, 5, 1, 1}, {0, INF, 5, 3}},
        {"M", "N", "O"}, 3);

    // Figure 5: W(est=0,lct=7,p=2,c=1), X(est=0,lct=7,p=6,c=1),
    //           Y(est=6,lct=7,p=1,c=1), Z(est=0,lct=INF,p=1,c=1), C=2
    // Expected: est_Z -> 2
    run("Figure 5",
        {{0, 7, 2, 1}, {0, 7, 6, 1}, {6, 7, 1, 1}, {0, INF, 1, 1}},
        {"W", "X", "Y", "Z"}, 2);

    return 0;
}
