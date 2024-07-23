#pragma once
#include <string>

double NormalizedSmithWatermanGotoh(std::string const& s, std::string const& t,
                                    double gapValue = -0.5);
