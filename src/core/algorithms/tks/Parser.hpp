#pragma once

#include "Bitmap.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

using Item = int;

std::unordered_map<Item, Bitmap>
parseSPMF(const std::string& filename,
          std::vector<int>& outSeqStarts,
          int& outTotalItemsets,
          const std::set<Item>& mustAppearItems = {});