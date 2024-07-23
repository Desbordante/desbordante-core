#include <set>
#include <string>
template <typename T>
double JaccardIndex(std::set<T> const& set1, std::set<T> const& set2);

double JaccardIndex(std::string const& s1, std::string const& s2);