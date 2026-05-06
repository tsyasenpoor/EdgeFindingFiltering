#pragma once
#include "activity.h"
#include <vector>

// One pass of Edge Finding (EST update only, Vilim 2009).
// Updates acts[i].est in-place. Returns true if any est changed.
bool edgeFinding(std::vector<Activity>& acts, int C);

// Run until fixpoint (repeatedly apply edgeFinding).
void edgeFindingFixpoint(std::vector<Activity>& acts, int C);
