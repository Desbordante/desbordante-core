#include "core/model/table/typed_column_data.h"

#include <bitset>
#include <cstddef>

#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/create_type.h"

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

TypeId TypedColumnDataFactory::DeduceColumnType() const {
    bool is_undefined = true;
    std::bitset<5> candidate_types_bitset("11111");
    TypeId first_type_id = +TypeId::kUndefined;
    for (std::size_t i = 0; i != unparsed_.size(); ++i) {
        if (!kNullCheck(unparsed_[i]) && !kEmptyCheck(unparsed_[i])) {
            is_undefined = false;
            if (first_type_id != +TypeId::kUndefined) {
                auto& type_check = kTypeIdToChecker.at(first_type_id);
                if (type_check(unparsed_[i])) {
                    // undelimited and delimited dates have different bitsets
                    if (first_type_id == +TypeId::kDate && kDelimitedDateCheck(unparsed_[i])) {
                        candidate_types_bitset &= kTypeIdToBitset.at(first_type_id);
                    }
                    continue;
                }
            }

            std::bitset<5> new_candidate_types_bitset("00000");
            bool matched = false;
            for (auto const& [type_id, type_check] : kTypeIdToChecker) {
                if (type_id != first_type_id && type_check(unparsed_[i])) {
                    if (first_type_id == +TypeId::kUndefined && !matched) {
                        first_type_id = type_id;
                    }
                    matched = true;
                    new_candidate_types_bitset |= kTypeIdToBitset.at(type_id);
                    // possible value types are known at the first match except for dates
                    // (undelimited dates could be ints or doubles and delimited couldn't)
                    if (type_id == +TypeId::kDate && kUndelimitedDateCheck(unparsed_[i])) {
                        new_candidate_types_bitset |= kTypeIdToBitset.at(+TypeId::kInt);
                    }
                    break;
                }
            }
            if (!matched) {
                new_candidate_types_bitset = kTypeIdToBitset.at(+TypeId::kString);
            }

            candidate_types_bitset &= new_candidate_types_bitset;
            if (candidate_types_bitset.none()) {
                if (treat_mixed_as_string_) {
                    candidate_types_bitset = kTypeIdToBitset.at(+TypeId::kString);
                } else {
                    return +TypeId::kMixed;
                }
            }
        }
    }

    if (is_undefined) {
        return +TypeId::kUndefined;
    }

    for (std::size_t i = 0; i < 5; i++) {
        if (candidate_types_bitset[i]) {
            return kAllCandidateTypes[i];
        }
    }

    return +TypeId::kMixed;
}

TypedColumnDataFactory::TypeMap TypedColumnDataFactory::CreateTypeMap(TypeId const type_id) const {
    TypeMap type_map;
    auto const match = [&type_map, type_id](std::string const& val, size_t const row) {
        if (kNullCheck(val)) {
            type_map[+TypeId::kNull].insert(row);
        } else if (kEmptyCheck(val)) {
            type_map[+TypeId::kEmpty].insert(row);
        } else if (type_id != +TypeId::kMixed) {
            type_map[type_id].insert(row);
        } else {
            bool matched = false;
            for (auto const& [type_id, type_check] : kTypeIdToChecker) {
                if (type_check(val)) {
                    type_map[type_id].insert(row);
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                type_map[TypeId::kString].insert(row);
            }
        }
    };

    for (std::size_t i = 0; i != unparsed_.size(); ++i) {
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
    TypeId const type_id = DeduceColumnType();
    TypeMap type_map = CreateTypeMap(type_id);

    return CreateFromTypeMap(CreateType(type_id, is_null_equal_null_), std::move(type_map));
}

std::vector<TypedColumnData> CreateTypedColumnData(IDatasetStream& dataset_stream,
                                                   bool is_null_equal_null) {
    std::unique_ptr<model::ColumnLayoutTypedRelationData> relation_data =
            model::ColumnLayoutTypedRelationData::CreateFrom(dataset_stream, is_null_equal_null,
                                                             false);
    std::vector<model::TypedColumnData> col_data = std::move(relation_data->GetColumnData());
    return col_data;
}

}  // namespace model
