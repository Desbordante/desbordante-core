#pragma once

#include <bitset>
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

std::vector<std::pair<size_t, size_t>> expected_column_indices = {
        {5, 5}, {6, 6}, {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {0, 2}, {0, 3}, {2, 3}};

std::vector<std::vector<size_t>> expected_eq_masks = {{0}, {1},  {2},  {4},  {6},
                                                      {8}, {10}, {12}, {14}, {16}};

std::vector<std::vector<size_t>> expected_gt_masks = {{},  {},   {3},  {5},  {7},
                                                      {9}, {11}, {13}, {15}, {17}};

std::vector<std::vector<size_t>> expected_correction_map = {
        {48, 49},         {50, 51},         {0, 1, 3, 4},     {2, 3, 4, 5},     {18, 19, 21, 22},
        {20, 21, 22, 23}, {24, 25, 27, 28}, {26, 27, 28, 29}, {36, 37, 39, 40}, {38, 39, 40, 41},
        {42, 43, 45, 46}, {44, 45, 46, 47}, {6, 7, 9, 10},    {8, 9, 10, 11},   {12, 13, 15, 16},
        {14, 15, 16, 17}, {30, 31, 33, 34}, {32, 33, 34, 35}};

std::bitset<64> VectorToBitset(std::vector<size_t> const& positions) {
    std::bitset<64> bset;
    for (auto pos : positions) {
        bset.set(pos);
    }
    return bset;
}
