#include "typed_column_data.h"

#include "column_layout_typed_relation_data.h"
#include "create_type.h"

namespace {

size_t GetNextAlignedOffset(size_t cur_offset, size_t align) {
    // alignment should be power of 2
    assert(align != 0 && (align & (align - 1)) == 0);
    if (size_t remainder = cur_offset % align; remainder != 0) {
        cur_offset += align - remainder;
    }
    return cur_offset;
}

}  // namespace

namespace model {

TypedColumnDataFactory::TypeMap TypedColumnDataFactory::CreateTypeMap() const {
    TypeMap type_map;
    auto const match = [&type_map](std::string const& val, size_t const row) {
        bool matched = false;
        for (auto const& [type_id, regex] : type_id_to_regex_) {
            if (std::regex_match(val, regex)) {
                type_map[type_id].insert(row);
                matched = true;
                break;
            }
        }
        if (!matched) {
            type_map[TypeId::kString].insert(row);
        }
    };

    for (size_t i = 0; i != unparsed_.size(); ++i) {
        match(unparsed_[i], i);
    }

    if (type_map.count(TypeId::kBigInt) && type_map.count(TypeId::kInt)) {
        std::unordered_set<size_t>& big_ints = type_map[TypeId::kBigInt];
        std::unordered_set<size_t> ints = std::move(type_map.extract(TypeId::kInt).mapped());
        big_ints.insert(ints.begin(), ints.end());
    }

    return type_map;
}

std::vector<TypeId> TypedColumnDataFactory::GetTypesLayout(TypeMap const& tm) const {
    std::vector<TypeId> types_layout(unparsed_.size(), TypeId::kString);

    for (auto const& [type_id, indices] : tm) {
        for (size_t const index : indices) {
            types_layout[index] = type_id;
        }
    }

    return types_layout;
}

TypedColumnDataFactory::TypeIdToType TypedColumnDataFactory::MapTypeIdsToTypes(
    TypeMap const& tm) const {
    std::unordered_map<TypeId, std::unique_ptr<Type>> type_id_to_type;
    for (auto const& [type_id, indices] : tm) {
        type_id_to_type.emplace(type_id, CreateType(type_id, is_null_equal_null_));
    }
    return type_id_to_type;
}

size_t TypedColumnDataFactory::CalculateMixedBufSize(
        std::vector<TypeId> const& types_layout,
        TypeIdToType const& type_id_to_type) const noexcept {
    size_t buf_size = 0;
    for (TypeId const type_id : types_layout) {
        size_t align = MixedType::GetAlignment(type_id);
        buf_size = GetNextAlignedOffset(buf_size, align);
        buf_size += MixedType::GetMixedValueSize(type_id_to_type.at(type_id).get());
    }
    return buf_size;
}

TypedColumnData TypedColumnDataFactory::CreateMixedFromTypeMap(std::unique_ptr<Type const> type,
                                                               TypeMap type_map) {
    assert(type->GetTypeId() == +TypeId::kMixed);
    MixedType const* mixed_type = static_cast<MixedType const*>(type.get());
    std::vector<std::byte const*> data;
    data.reserve(unparsed_.size());

    std::unordered_set<size_t> const& nulls = type_map[TypeId::kNull];
    std::unordered_set<size_t> const& empties = type_map[TypeId::kEmpty];
    size_t const rows_num = unparsed_.size();
    size_t const nulls_num = nulls.size();
    size_t const empties_num = empties.size();

    TypeIdToType type_id_to_type = MapTypeIdsToTypes(type_map);
    std::vector<TypeId> types_layout = GetTypesLayout(type_map);
    size_t const buf_size = CalculateMixedBufSize(types_layout, type_id_to_type);
    static_assert(kTypesMaxAlignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                  "Overaligned types lead to a missaligned accesses to values in the current "
                  "implementation, which is UB");
    std::unique_ptr<std::byte[]> buf(new std::byte[buf_size]);
    type_map.clear(); /* type_map is no longer needed, so saving space */

    size_t buf_index = 0;
    for (size_t i = 0; i != types_layout.size(); ++i) {
        TypeId const type_id = types_layout[i];
        Type const* concrete_type = type_id_to_type.at(type_id).get();
        size_t const value_size = mixed_type->GetMixedValueSize(concrete_type);

        buf_index = GetNextAlignedOffset(buf_index, mixed_type->GetAlignment(type_id));
        std::byte* next = buf.get() + buf_index;

        assert(next + value_size <= buf.get() + buf_size);

        mixed_type->ValueFromStr(next, std::move(unparsed_[i]), concrete_type);

        data.push_back(next);
        buf_index += value_size;
    }

    return TypedColumnData(column_, std::move(type), rows_num, nulls_num, empties_num,
                           std::move(buf), std::move(data), {}, {});
}

TypedColumnData TypedColumnDataFactory::CreateConcreteFromTypeMap(std::unique_ptr<Type const> type,
                                                                  TypeMap type_map) {
    TypeId const type_id = type->GetTypeId();

    if (type_id == +TypeId::kMixed) {
        /* For mixed type use CreateMixedFromTypeMap. */
        assert(0);
    }

    std::unordered_set<size_t> nulls = std::move(type_map[TypeId::kNull]);
    std::unordered_set<size_t> empties = std::move(type_map[TypeId::kEmpty]);
    size_t const rows_num = unparsed_.size();
    size_t const nulls_num = nulls.size();
    size_t const empties_num = empties.size();
    assert(rows_num >= nulls_num + empties_num);

    std::vector<std::byte const*> data(unparsed_.size());

    if (type_id == +TypeId::kUndefined) {
        return TypedColumnData(column_, std::move(type), rows_num, nulls_num, empties_num, nullptr,
                               std::move(data), std::move(nulls), std::move(empties));
    }

    std::unique_ptr<std::byte[]> buf(type->Allocate(rows_num - nulls_num - empties_num));

    size_t buf_index = 0;
    size_t const value_size = type->GetSize();
    for (size_t i : type_map.at(type_id)) {
        assert(buf_index <= type->GetSize() * type_map.at(type_id).size());
        std::byte* next = buf.get() + buf_index;
        type->ValueFromStr(next, std::move(unparsed_[i]));
        data[i] = next;
        buf_index += value_size;
    }

    return TypedColumnData(column_, std::move(type), rows_num, nulls_num, empties_num,
                           std::move(buf), std::move(data), std::move(nulls), std::move(empties));
}

TypedColumnData TypedColumnDataFactory::CreateFromTypeMap(std::unique_ptr<Type const> type,
                                                          TypeMap type_map) {
    if (type->GetTypeId() == +TypeId::kMixed) {
        return CreateMixedFromTypeMap(std::move(type), std::move(type_map));
    } else {
        return CreateConcreteFromTypeMap(std::move(type), std::move(type_map));
    }
}

TypedColumnData TypedColumnDataFactory::CreateFrom() {
    TypeMap type_map = CreateTypeMap();

    decltype(type_map)::node_type null_node = type_map.extract(TypeId::kNull);
    decltype(type_map)::node_type empty_node = type_map.extract(TypeId::kEmpty);

    TypeId type_id = TypeId::kMixed;

    if (type_map.empty()) {
        type_id = TypeId::kUndefined;
    } else if (type_map.size() == 1) {
        type_id = type_map.begin()->first;
    }

    type_map.insert(std::move(null_node));
    type_map.insert(std::move(empty_node));

    return CreateFromTypeMap(CreateType(type_id, is_null_equal_null_), std::move(type_map));
}

std::vector<TypedColumnData> CreateTypedColumnData(IDatasetStream& dataset_stream,
                                                   bool is_null_equal_null) {
    std::unique_ptr<model::ColumnLayoutTypedRelationData> relation_data =
            model::ColumnLayoutTypedRelationData::CreateFrom(dataset_stream, is_null_equal_null);
    std::vector<model::TypedColumnData> col_data = std::move(relation_data->GetColumnData());
    return col_data;
}

}  // namespace model
