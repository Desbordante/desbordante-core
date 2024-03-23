#include "gfd.h"

#include <sstream>

#include "parser/graph_parser/graph_parser.h"

std::string Gfd::ToString() {
    std::stringstream gfd_stream;
    parser::graph_parser::WriteGfd(gfd_stream, *this);
    return gfd_stream.str();
}
