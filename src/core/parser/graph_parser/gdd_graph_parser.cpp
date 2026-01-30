#include <fstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/function_property_map.hpp>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace parser::graph_parser::gdd {

namespace {

using AMap =
        boost::property_map<model::gdd::graph_t, std::unordered_map<std::string, std::string>
                                                         model::gdd::VertexProperties::*>::type;

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

    explicit NewAttr(AMap a) : attrs(a) {}

    Ptr operator()(std::string const& name, boost::any const& descr, boost::any const&) const {
        if (typeid(model::gdd::vertex_t) == descr.type()) {
            return MakeDyn(boost::make_function_property_map<model::gdd::vertex_t>(
                    [this, name](model::gdd::vertex_t v) -> std::string& {
                        return attrs[v][name];
                    }));
        }
        return Ptr();
    }
};

}  // namespace

model::gdd::graph_t ReadGraph(std::istream& stream) {
    model::gdd::graph_t result;

    std::unordered_map<model::gdd::vertex_t, std::string> node_ids;
    auto const node_id_pm = boost::make_assoc_property_map(node_ids);

    NewAttr const newattr(boost::get(&model::gdd::VertexProperties::attributes, result));
    boost::dynamic_properties dp(newattr);

    dp.property("node_id", node_id_pm);
    dp.property("label", boost::get(&model::gdd::EdgeProperties::label, result));

    boost::read_graphviz(stream, result, dp, "node_id");

    for (auto v : boost::make_iterator_range(boost::vertices(result))) {
        auto it = node_ids.find(v);
        if (it == node_ids.end()) {
            throw std::runtime_error("DOT parser: missing node_id for a vertex");
        }

        try {
            result[v].id = std::stoi(it->second);
        } catch (...) {
            throw std::runtime_error("DOT parser: node_id is not a valid integer: " + it->second);
        }

        auto& attrs = result[v].attributes;
        if (auto lbl = attrs.find("label"); lbl != attrs.end()) {
            result[v].label = lbl->second;
            attrs.erase(lbl);
        } else {
            result[v].label.clear();
        }
    }

    return result;
}

model::gdd::graph_t ReadGraph(std::filesystem::path const& path) {
    std::ifstream f(path);
    return ReadGraph(f);
}

}  // namespace parser::graph_parser::gdd
