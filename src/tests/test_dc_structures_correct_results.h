#pragma once

#include <string>
#include <vector>

std::vector<std::string> different_column_predicates_expected = {
        "t.A == s.A", "t.A != s.A", "t.A > s.A",  "t.A < s.A",  "t.A >= s.A", "t.A <= s.A",
        "t.A == s.C", "t.A != s.C", "t.A > s.C",  "t.A < s.C",  "t.A >= s.C", "t.A <= s.C",
        "t.A == s.D", "t.A != s.D", "t.A > s.D",  "t.A < s.D",  "t.A >= s.D", "t.A <= s.D",
        "t.B == s.B", "t.B != s.B", "t.B > s.B",  "t.B < s.B",  "t.B >= s.B", "t.B <= s.B",
        "t.C == s.C", "t.C != s.C", "t.C > s.C",  "t.C < s.C",  "t.C >= s.C", "t.C <= s.C",
        "t.C == s.D", "t.C != s.D", "t.C > s.D",  "t.C < s.D",  "t.C >= s.D", "t.C <= s.D",
        "t.D == s.D", "t.D != s.D", "t.D > s.D",  "t.D < s.D",  "t.D >= s.D", "t.D <= s.D",
        "t.E == s.E", "t.E != s.E", "t.E > s.E",  "t.E < s.E",  "t.E >= s.E", "t.E <= s.E",
        "t.F == s.F", "t.F != s.F", "t.G == s.G", "t.G != s.G",
};

std::vector<std::string> num_single_column_predicate_group_expected = {
        "t.A == s.A", "t.A != s.A", "t.A > s.A", "t.A < s.A", "t.A >= s.A", "t.A <= s.A",
        "t.B == s.B", "t.B != s.B", "t.B > s.B", "t.B < s.B", "t.B >= s.B", "t.B <= s.B",
        "t.C == s.C", "t.C != s.C", "t.C > s.C", "t.C < s.C", "t.C >= s.C", "t.C <= s.C",
        "t.D == s.D", "t.D != s.D", "t.D > s.D", "t.D < s.D", "t.D >= s.D", "t.D <= s.D",
        "t.E == s.E", "t.E != s.E", "t.E > s.E", "t.E < s.E", "t.E >= s.E", "t.E <= s.E"};

std::vector<std::string> num_cross_column_predicate_group_expected = {
        "t.A == s.C", "t.A != s.C", "t.A > s.C", "t.A < s.C", "t.A >= s.C", "t.A <= s.C",
        "t.A == s.D", "t.A != s.D", "t.A > s.D", "t.A < s.D", "t.A >= s.D", "t.A <= s.D",
        "t.C == s.D", "t.C != s.D", "t.C > s.D", "t.C < s.D", "t.C >= s.D", "t.C <= s.D"};

std::vector<std::string> str_single_column_predicate_group_expected = {"t.F == s.F", "t.F != s.F",
                                                                       "t.G == s.G", "t.G != s.G"};

std::vector<std::string> str_cross_column_predicate_group_expected = {};
