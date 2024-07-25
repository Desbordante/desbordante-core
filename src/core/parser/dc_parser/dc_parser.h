#include <string>

// Should probably be better named tokenizer?
class DCParser {
    std::string dc_;

    DCParser(std::string dc) : dc_(dc) {}
    bool HasNextPredicate();
}
