#pragma once

#include <utility>

#include "../../model/relation_data.h"
#include "cfd_column_data.h"
#include "../../model/cfd_types.h"
#include "idataset_stream.h"

class CFDRelationData : public AbstractRelationData<CFDColumnData>
{
    public:
	unsigned size() const;
	unsigned GetAttrsNumber() const;
    unsigned GetItemsNumber() const;
	size_t GetNumRows() const override;
    const Transaction& GetRow(int) const;
    std::string GetStringFormat(char delim= ' ') const;
    std::string GetStringFormat(const SimpleTidList& subset, char delim= ' ') const;
    [[maybe_unused]] void Sort();
    [[maybe_unused]] void ToFront(const SimpleTidList&);
    [[maybe_unused]] void SetRow(int r, const Transaction& row);
	int GetAttrIndex(int t) const;
	int Frequency(int i) const;
	const std::string& GetValue(int i) const;
	// GetDomain просто выводит values атрибута. GetDomainOfItem смотрит на атрибут item и делает то же самое.
    [[maybe_unused]] const std::vector<int>& GetDomainOfItem(int) const;
	const std::vector<int>& GetDomain(int attr) const;
	// Функция выводит номера атрибутов элементов айтемсета
	std::vector<int> GetAttrVector(const Itemset&) const;
    std::vector<int> GetAttrVectorItems(const Itemset&) const;
    std::string GetAttrName(int index) const;
    // Функция не нужна для fds-first
    // static std::vector<int> getDiffs(const Database&, const Database&);
    // Выводит индекс атрибута по имени.
    [[maybe_unused]] int GetAttr(const std::string&) const;
	// Выводит индекс элемента в items_ (т.е. значение unique_elems_number_) для пары (ключ в item_dictionary_)
    [[maybe_unused]] int GetItem(int, const std::string&) const;
	// Функция не нужна для fds-first
    // static std::vector<int> getDiffs(const Database&, const Database&);
    struct ItemInfo {
		ItemInfo() = default;
		ItemInfo(std::string  val, int attr):value(std::move(val)), attribute(attr), frequency(0) { }
		std::string value;
		int attribute;
		int frequency;
	};
    static std::unique_ptr<CFDRelationData> CreateFrom(model::IDatasetStream& file_input,
                                                            bool is_null_eq_null,
                                                            double c_sample = 1 /* ,
                                                            double rSample = 1 */);
    static std::unique_ptr<CFDRelationData> CreateFrom(model::IDatasetStream& file_input,
                                                            bool is_null_eq_null,
															unsigned columns_number,
															unsigned tuples_number,
                                                            double c_sample = 1 /*,
                                                            double rSample = 1 */);

	CFDRelationData(std::unique_ptr<RelationalSchema> schema, std::vector<CFDColumnData> column_data,
	std::vector<Transaction> data, std::unordered_map<std::pair<int, std::string>, int, pairhash> item_dict,
	std::vector<ItemInfo> items) : AbstractRelationData(std::move(schema), std::move(column_data)),
          data_rows_(std::move(data)),
          item_dictionary_(std::move(item_dict)),
          items_(std::move(items)) {}

    private:
    std::vector<Transaction> data_rows_; //array of data represented as rows of integers
	// По атрибуту элемента и самому элементу содержит значение unique_elems_number_ в конкретный момент.
	std::unordered_map<std::pair<int, std::string>, int, pairhash> item_dictionary_; // maps a value string for the given attribute index to corresponding item id
	// Сначала мы конструируем имена columns_data_. А в values хранятся значения unique_elems_number_ в разные моменты.
	// Их всего будет столько же, сколько и уникальных значений в этом атрибуте
	// Просто список значений типа ItemInfo. По сути, список троек вида: атрибут, значение, сколько раз повторялся в атрибуте.
	// Индексы items_ это значения в базе данных - 1. По сути, каждое значение в базе данных кодируется индексом в items_.
	std::vector<ItemInfo> items_;
};