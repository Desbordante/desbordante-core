#include "graph_parser.h"

#include <boost/algorithm/string.hpp>
#include <boost/bind/bind.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/function_property_map.hpp>

namespace parser {

namespace {

std::vector<std::string> Split(std::string str, std::string sep) {
    std::vector<std::string> result = {};
    if (str == "") {
        return result;
    }
    size_t pos = 0;
    while ((pos = str.find(sep)) != std::string::npos) {
        result.push_back(str.substr(0, pos));
        str.erase(0, pos + sep.length());
    }
    result.push_back(str);
    return result;
};

std::vector<Literal> ParseLiterals(std::istream& stream) {
    std::vector<Literal> result = {};

    std::string line;
    std::getline(stream, line);
    boost::algorithm::trim(line);
    auto tokens = Split(line, " ");
    for (auto token : tokens) {
        auto custom_names = Split(token, "=");
        auto names1 = Split(custom_names.at(0), ".");
        int index1 = names1.size() == 1 ? -1 : stoi(names1.at(0));
        std::string name1 = *(--names1.end());
        Token t1(index1, name1);

        auto names2 = Split(custom_names.at(1), ".");
        int index2 = names2.size() == 1 ? -1 : stoi(names2.at(0));
        std::string name2 = *(--names2.end());
        Token t2(index2, name2);

        result.push_back(Literal(t1, t2));
    }

    return result;
};

void WriteLiterals(std::ostream& stream, std::vector<Literal> const& literals) {
    for (Literal const& l : literals) {
        std::string token;

        Token fst_token = l.first;
        token = fst_token.first == -1 ? "" : (std::to_string(fst_token.first) + ".");
        token += fst_token.second;
        stream << token;

        stream << "=";

        Token snd_token = l.second;
        token = snd_token.first == -1 ? "" : (std::to_string(snd_token.first) + ".");
        token += snd_token.second;
        stream << token;

        stream << " ";
    }
    stream << std::endl;
};

}  // namespace

namespace graph_parser {

using AMap = boost::property_map<graph_t, std::map<std::string, std::string> Vertex::*>::type;
using RMap = boost::property_map<graph_t, std::string Edge::*>::type;

namespace {
struct NewAttr {
    using Ptr = boost::shared_ptr<boost::dynamic_property_map>;

private:
    template <typename PMap>
    static Ptr MakeDyn(PMap m) {
        using DM = boost::detail::dynamic_property_map_adaptor<PMap>;
        boost::shared_ptr<DM> sp = boost::make_shared<DM>(m);
        return boost::static_pointer_cast<boost::dynamic_property_map>(sp);
    }

public:
    AMap attrs;

    NewAttr(AMap a) : attrs(a) {}

    Ptr operator()(std::string const& name, boost::any const& descr, boost::any const&) const {
        if (typeid(vertex_t) == descr.type())
            return MakeDyn(boost::make_function_property_map<vertex_t>(
                    boost::bind(*this, boost::placeholders::_1, name)));

        return Ptr();
    };

    using result_type = std::string&;

    std::string& operator()(vertex_t v, std::string const& name) const {
        return attrs[v][name];
    }
};
}  // namespace

graph_t ReadGraph(std::istream& stream) {
    graph_t result;
    NewAttr newattr(get(&Vertex::attributes, result));
    boost::dynamic_properties dp(newattr);
    dp.property("label", get(&Edge::label, result));
    dp.property("node_id", get(&Vertex::node_id, result));
    read_graphviz(stream, result, dp);
    return result;
};

graph_t ReadGraph(std::filesystem::path const& path) {
    std::ifstream f(path);
    graph_t result = ReadGraph(f);
    f.close();
    return result;
};

void WriteGraph(std::ostream& stream, graph_t& result) {
    boost::attributes_writer<AMap> vw(get(&Vertex::attributes, result));
    boost::label_writer<RMap> ew(get(&Edge::label, result));
    write_graphviz(stream, result, vw, ew);
};

void WriteGraph(std::filesystem::path const& path, graph_t& result) {
    std::ofstream f(path);
    WriteGraph(f, result);
    f.close();
};

Gfd ReadGfd(std::istream& stream) {
    std::vector<Literal> premises = ParseLiterals(stream);
    std::vector<Literal> conclusion = ParseLiterals(stream);
    graph_t pattern = ReadGraph(stream);
    Gfd result = Gfd();
    result.SetPattern(pattern);
    result.SetPremises(premises);
    result.SetConclusion(conclusion);
    return result;
};

Gfd ReadGfd(std::filesystem::path const& path) {
    std::ifstream f(path);
    Gfd result = ReadGfd(f);
    f.close();
    return result;
};

void WriteGfd(std::ostream& stream, Gfd& result) {
    WriteLiterals(stream, result.GetPremises());
    WriteLiterals(stream, result.GetConclusion());
    graph_t pattern = result.GetPattern();
    WriteGraph(stream, pattern);
};

void WriteGfd(std::filesystem::path const& path, Gfd& result) {
    std::ofstream f(path);
    WriteGfd(f, result);
    f.close();
};

}  // namespace graph_parser

}  // namespace parser
