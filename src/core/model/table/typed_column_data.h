#pragma once

#include <bitset>
#include <string>
#include <vector>

#include <boost/regex.hpp>
#include <magic_enum/magic_enum.hpp>

#include "core/model/table/abstract_column_data.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/table/relation_data.h"
#include "core/model/types/types.h"

namespace model {

class TypedColumnData : public model::AbstractColumnData {
private:
    std::unique_ptr<Type const> type_;
    size_t rows_num_;
    size_t nulls_num_;
    size_t empties_num_;
    std::unique_ptr<std::byte[]> buffer_;
    std::vector<std::byte const*> data_;
    /* For non-mixed type only */
    std::unordered_set<size_t> nulls_;
    std::unordered_set<size_t> empties_;

    TypedColumnData(Column const* column, std::unique_ptr<Type const> type, size_t const rows_num,
                    size_t nulls_num, size_t empties_num, std::unique_ptr<std::byte[]> buffer,
                    std::vector<std::byte const*> data, std::unordered_set<size_t> nulls,
                    std::unordered_set<size_t> empties) noexcept
        : AbstractColumnData(column),
          type_(std::move(type)),
          rows_num_(rows_num),
          nulls_num_(nulls_num),
          empties_num_(empties_num),
          buffer_(std::move(buffer)),
          data_(std::move(data)),
          nulls_(std::move(nulls)),
          empties_(std::move(empties)) {}

    friend class TypedColumnDataFactory;

public:
    TypedColumnData(TypedColumnData const& other) = delete;
    TypedColumnData& operator=(TypedColumnData const& other) = delete;
    TypedColumnData(TypedColumnData&& other) noexcept = default;
    TypedColumnData& operator=(TypedColumnData&& other) noexcept = default;

    ~TypedColumnData() {
        if (type_ == nullptr) {
            return;
        }

        MixedType const* mixed = GetIfMixed();
        for (size_t i = 0; i != data_.size(); ++i) {
            if (GetValueTypeId(i) == TypeId::kString || GetValueTypeId(i) == TypeId::kBigInt ||
                GetValueTypeId(i) == TypeId::kDate) {
                std::byte const* value = (mixed) ? mixed->RetrieveValue(data_[i]) : data_[i];
                if (GetValueTypeId(i) == TypeId::kDate) {
                    DateType::Destruct(value);
                } else {
                    StringType::Destruct(value);
                }
            }
        }
    }

    TypeId GetTypeId() const noexcept {
        return type_->GetTypeId();
    }

    Type const& GetType() const noexcept {
        return *type_;
    }

    std::vector<std::byte const*> const& GetData() const noexcept {
        return data_;
    }

    std::byte const* GetValue(size_t index) const noexcept {
        return data_[index];
    }

    std::string GetDataAsString(size_t index) const {
        if (IsNull(index)) {
            NullType null_type(true);
            return null_type.ValueToString(GetValue(index));
        }
        if (IsEmpty(index)) {
            EmptyType empty_type;
            return empty_type.ValueToString(GetValue(index));
        }
        return type_->ValueToString(GetValue(index));
    }

    size_t GetNumNulls() const noexcept {
        return nulls_num_;
    }

    size_t GetNumEmpties() const noexcept {
        return empties_num_;
    }

    size_t GetNumRows() const noexcept {
        return rows_num_;
    }

    bool IsNull(size_t index) const noexcept {
        MixedType const* mixed = GetIfMixed();
        if (mixed != nullptr) {
            return mixed->RetrieveTypeId(data_[index]) == TypeId::kNull;
        } else {
            return nulls_.find(index) != nulls_.end();
        }
    }

    bool IsEmpty(size_t index) const noexcept {
        MixedType const* mixed = GetIfMixed();
        if (mixed != nullptr) {
            return mixed->RetrieveTypeId(data_[index]) == TypeId::kEmpty;
        } else {
            return empties_.find(index) != empties_.end();
        }
    }

    bool IsNullOrEmpty(size_t index) const noexcept {
        return IsNull(index) || IsEmpty(index);
    }

    TypeId GetValueTypeId(size_t index) const noexcept {
        TypeId const type_id = type_->GetTypeId();
        if (type_id == TypeId::kMixed) {
            return static_cast<MixedType const*>(type_.get())->RetrieveTypeId(data_[index]);
        }

        if (IsNull(index)) {
            return TypeId::kNull;
        }
        if (IsEmpty(index)) {
            return TypeId::kEmpty;
        }

        return type_id;
    }

    bool IsNumeric() const noexcept {
        TypeId type_id = GetTypeId();
        return type_id == TypeId::kInt || /* type_id == ColumnTypeId::kBigInt || */
               type_id == TypeId::kDouble;
    }

    bool IsMixed() const noexcept {
        return GetTypeId() == TypeId::kMixed;
    }

    MixedType const* GetIfMixed() const noexcept {
        return dynamic_cast<MixedType const*>(type_.get());
    }

    std::string ToString() const final {
        return "Data for column " + column_->ToString() + " of type " +
               std::string(magic_enum::enum_name(GetTypeId()));
    }
};

class TypedColumnDataFactory {
private:
    using TypeMap = std::unordered_map<TypeId, std::unordered_set<size_t>>;
    using TypeIdToType = std::unordered_map<TypeId, std::unique_ptr<Type>>;

    Column const* column_;
    std::vector<std::string> unparsed_;
    bool is_null_equal_null_;
    bool treat_mixed_as_string_;

    inline static std::vector<TypeId> const kAllCandidateTypes = {
            TypeId::kDate, TypeId::kInt, TypeId::kBigInt, TypeId::kDouble, TypeId::kString};
    inline static std::unordered_map<TypeId, boost::regex> const kTypeIdToRegex = {
            {TypeId::kDate,
             boost::regex(
                     R"(^(\d{4})([-.\/]?)(1[0-2]|0[1-9]|[1-9])\2(3[0-1]|0[1-9]|[1-9]|[1-2][0-9])$)")},
            {TypeId::kDouble,
             boost::regex(
                     R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
                     R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
                     R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)")},
            {TypeId::kBigInt, boost::regex(R"(^(\+|-)?\d{20,}$)")},
            {TypeId::kInt, boost::regex(R"(^(\+|-)?\d{1,19}$)")},
            {TypeId::kNull, boost::regex(Null::kValue.data())},
            {TypeId::kEmpty, boost::regex(R"(^$)")}};
    inline static auto const kNullCheck = [](std::string const& val) {
        return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kNull));
    };
    inline static auto const kEmptyCheck = [](std::string const& val) {
        return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kEmpty));
    };
    inline static std::function<bool(std::string const&)> const kUndelimitedDateCheck =
            [](std::string const& val) {
                bool is_undelimited_date = false;
                try {
                    boost::gregorian::from_undelimited_string(val);
                    is_undelimited_date = true;
                } catch (...) {
                }
                return is_undelimited_date;
            };
    inline static std::function<bool(std::string const&)> const kDelimitedDateCheck =
            [](std::string const& val) {
                bool is_simple_date = false;
                try {
                    boost::gregorian::from_simple_string(val);
                    is_simple_date = true;
                } catch (...) {
                }
                return is_simple_date;
            };
    inline static std::unordered_map<TypeId, std::function<bool(std::string const&)>> const
            kTypeIdToChecker = {
                    {TypeId::kDouble,
                     [](std::string const& val) {
                         return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kDouble));
                     }},
                    {TypeId::kBigInt,
                     [](std::string const& val) {
                         return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kBigInt));
                     }},
                    {TypeId::kInt,
                     [](std::string const& val) {
                         return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kInt));
                     }},
                    {TypeId::kDate, [](std::string const& val) {
                         return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kDate)) &&
                                (kDelimitedDateCheck(val) || kUndelimitedDateCheck(val));
                     }}};
    // each 1 represents a possible type from kAllCandidateTypes
    inline static std::unordered_map<TypeId, std::bitset<5>> const kTypeIdToBitset = {
            {TypeId::kDate, std::bitset<5>("00001")},  // bitset for delimited dates
            {TypeId::kInt, std::bitset<5>("01110")},
            {TypeId::kBigInt, std::bitset<5>("01100")},
            {TypeId::kDouble, std::bitset<5>("01000")},
            {TypeId::kString, std::bitset<5>("10000")}};

    size_t CalculateMixedBufSize(std::vector<TypeId> const& types_layout,
                                 TypeIdToType const& type_id_to_type) const noexcept;
    std::vector<TypeId> GetTypesLayout(TypeMap const& tm) const;
    TypeIdToType MapTypeIdsToTypes(TypeMap const& tm) const;
    TypeId DeduceColumnType() const;
    TypeMap CreateTypeMap(TypeId const type_id) const;
    TypedColumnData CreateMixedFromTypeMap(std::unique_ptr<Type const> type, TypeMap type_map);
    TypedColumnData CreateConcreteFromTypeMap(std::unique_ptr<Type const> type, TypeMap type_map);
    TypedColumnData CreateFromTypeMap(std::unique_ptr<Type const> type, TypeMap type_map);
    TypedColumnData CreateFrom();

    TypedColumnDataFactory(Column const* col, std::vector<std::string> unparsed,
                           bool is_null_equal_null, bool treat_mixed_as_string)
        : column_(col),
          unparsed_(std::move(unparsed)),
          is_null_equal_null_(is_null_equal_null),
          treat_mixed_as_string_(treat_mixed_as_string) {}

public:
    static TypedColumnData CreateFrom(Column const* col, std::vector<std::string> unparsed,
                                      bool is_null_equal_null, bool treat_mixed_as_string) {
        TypedColumnDataFactory f(col, std::move(unparsed), is_null_equal_null,
                                 treat_mixed_as_string);
        return f.CreateFrom();
    }
};

std::vector<TypedColumnData> CreateTypedColumnData(IDatasetStream& dataset_stream,
                                                   bool is_null_equal_null);

}  // namespace model
