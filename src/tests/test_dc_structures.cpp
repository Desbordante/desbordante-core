#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "all_csv_configs.h"
#include "create_type.h"
#include "csv_parser/csv_parser.h"
#include "dc/FastADC/misc/misc.h"
#include "dc/FastADC/model/denial_constraint.h"
#include "dc/FastADC/model/operator.h"
#include "dc/FastADC/model/pli_shard.h"
#include "dc/FastADC/util/approximate_evidence_inverter.h"
#include "dc/FastADC/util/clue_set_builder.h"
#include "dc/FastADC/util/evidence_set_builder.h"
#include "dc/FastADC/util/predicate_builder.h"
#include "dc/FastADC/util/predicate_organizer.h"
#include "dc/FastADC/util/single_clue_set_builder.h"
#include "int_type.h"
#include "string_type.h"
#include "table/column_layout_typed_relation_data.h"
#include "test_dc_structures_correct_results.h"

namespace tests {

using namespace algos::fastadc;

class TestOperatorInt : public ::testing::Test {
private:
    std::unique_ptr<std::byte[]> left_owner_;
    std::unique_ptr<std::byte[]> right_owner_;

protected:
    std::unique_ptr<model::IntType> type_;
    std::byte* left_ptr_;
    std::byte* right_ptr_;
    std::array<Operator, 6> all_operators_ = {
            {Operator(OperatorType::kEqual), Operator(OperatorType::kUnequal),
             Operator(OperatorType::kGreater), Operator(OperatorType::kLess),
             Operator(OperatorType::kGreaterEqual), Operator(OperatorType::kLessEqual)}};

    void SetUp() override {
        type_ = std::make_unique<model::IntType>();
        left_owner_.reset(type_->MakeValue(0));
        right_owner_.reset(type_->MakeValue(0));
        left_ptr_ = left_owner_.get();
        right_ptr_ = right_owner_.get();
    }

    void SetVals(int left, int right) {
        model::Type::GetValue<int>(left_ptr_) = left;
        model::Type::GetValue<int>(right_ptr_) = right;
    }
};

TEST_F(TestOperatorInt, Equal) {
    Operator op(OperatorType::kEqual);

    auto test = [&](int l, int r, bool res) {
        this->SetVals(l, r);

        EXPECT_EQ(res, op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_)));
    };

    test(1, 1, true);
    test(1, 2, false);
}

TEST_F(TestOperatorInt, Greater) {
    Operator op(OperatorType::kGreater);

    auto test = [&](int l, int r, bool res) {
        this->SetVals(l, r);

        EXPECT_EQ(res, op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_)));
    };

    test(1, 1, false);
    test(1, 2, false);
    test(-10, -20, true);
}

TEST_F(TestOperatorInt, Implication) {
    auto test = [&](int l, int r) {
        this->SetVals(l, r);

        for (auto& op : all_operators_) {
            auto implications = op.GetImplications();
            bool op_result = op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));

            for (auto& imp_op : implications) {
                if (op_result) {
                    EXPECT_TRUE(imp_op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_)));
                }
            }
        }
    };

    test(0, 0);
    test(5, 4);
    test(5, -5);
    test(5, 5);
    test(10, 4);
    test(12313, -10);
}

TEST_F(TestOperatorInt, Inverse) {
    auto test = [&](int l, int r) {
        this->SetVals(l, r);

        for (auto& op : all_operators_) {
            auto inverse_op = op.GetInverse();
            bool op_result = op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));
            bool inverse_result =
                    inverse_op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));

            // The result of an operation and its inverse should be opposite
            EXPECT_NE(op_result, inverse_result);
        }
    };

    test(0, 0);
    test(5, 4);
    test(5, -5);
    test(5, 5);
    test(10, 4);
    test(12313, -10);
}

TEST_F(TestOperatorInt, Symmetric) {
    auto test = [&](int l, int r) {
        this->SetVals(l, r);

        for (auto& op : all_operators_) {
            auto symmetric_op = op.GetSymmetric();
            bool op_result = op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));

            bool symmetric_result =
                    symmetric_op.Eval(this->right_ptr_, this->left_ptr_, *(this->type_));

            EXPECT_EQ(op_result, symmetric_result);
        }
    };

    test(0, 0);
    test(5, 4);
    test(5, -5);
    test(5, 5);
    test(10, 4);
    test(12313, -10);
}

TEST_F(TestOperatorInt, Transitives) {
    auto test = [&](int a, int b, int c) {
        for (auto& op : all_operators_) {
            this->SetVals(a, b);
            bool ab_result = op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));

            auto transitives = op.GetTransitives();
            for (auto& trans_op : transitives) {
                this->SetVals(b, c);
                bool bc_trans_result =
                        trans_op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_));

                if (ab_result && bc_trans_result) {
                    this->SetVals(a, c);
                    // If 'a op b' and 'b trans_op c', then 'a op c' should also be true
                    EXPECT_TRUE(op.Eval(this->left_ptr_, this->right_ptr_, *(this->type_)));
                }
            }
        }
    };

    test(1, 2, 3);
    test(3, 2, 1);
    test(2, 2, 2);
    test(-2, 0, 2);
    test(-3, -2, 1);
    test(-1, -5, -4);
    test(2, 2, 3);
    test(-1, -1, 0);
    test(1, 3, 3);
    test(-4, -2, -2);
    test(5, 3, 4);
    test(0, -1, 1);
    test(4, 2, 2);
    test(1, 0, 0);
    test(1000, 2000, 3000);
    test(-1000, -500, 0);
    test(10000, -5000, 5000);
    test(3, 3, 2);
    test(-1, -1, -2);
    test(2, 3, 1);
    test(-3, -1, -4);
    test(3, 1, 2);
    test(0, -2, -1);
}

TEST(TestOperatorString, Compare) {
    std::unique_ptr<model::Type> type(CreateType(model::TypeId::kString, true));
    model::StringType const* s = static_cast<model::StringType const*>(type.get());

    Operator eq(OperatorType::kEqual);
    Operator neq(OperatorType::kUnequal);

    auto test = [&](std::string l, std::string r, bool res, Operator& op) {
        using Owner = std::unique_ptr<std::byte[], model::StringTypeDeleter>;

        Owner l_owner(s->MakeValue(l), s->GetDeleter());
        Owner r_owner(s->MakeValue(r), s->GetDeleter());

        std::byte* l_val = l_owner.get();
        std::byte* r_val = r_owner.get();

        EXPECT_EQ(res, op.Eval(l_val, r_val, *s));
    };

    test("sfsdf", "sfsdf", true, eq);
    test("sfsdf", "sfsdf", false, neq);
    test("abc", "cba", false, eq);
    test("abc", "cba", true, neq);
}

TEST(Predicate, PredicateCreatesCorrectly) {
    CSVParser parser{kTmpDC};
    std::unique_ptr<model::ColumnLayoutTypedRelationData> table =
            model::ColumnLayoutTypedRelationData::CreateFrom(parser, true);
    std::vector<model::TypedColumnData> col_data = std::move(table->GetColumnData());
    PredicateProvider pred_provider;
    Column const *first = col_data[0].GetColumn(), *second = col_data[1].GetColumn();

    PredicatePtr s_a_less_t_b =
            pred_provider.GetPredicate(Operator(OperatorType::kLess), ColumnOperand(first, true),
                                       ColumnOperand(second, false));

    EXPECT_TRUE(s_a_less_t_b->Satisfies(col_data, 0, 1));
    EXPECT_TRUE(s_a_less_t_b->Satisfies(col_data, 1, 0));

    PredicatePtr s_a_neq_t_a =
            pred_provider.GetPredicate(Operator(OperatorType::kUnequal), ColumnOperand(first, true),
                                       ColumnOperand(first, false));

    EXPECT_FALSE(s_a_neq_t_a->Satisfies(col_data, 0, 1));
    EXPECT_FALSE(s_a_neq_t_a->Satisfies(col_data, 1, 0));
}

class FastADC : public ::testing::Test {
protected:
    std::unique_ptr<CSVParser> parser_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> table_;
    std::vector<model::TypedColumnData> col_data_;

    PredicateIndexProvider pred_index_provider_;
    PredicateProvider pred_provider_;
    IntIndexProvider int_prov_;
    DoubleIndexProvider double_prov_;
    StringIndexProvider string_prov_;

    PredicateBuilder* predicate_builder_ = nullptr;
    PliShardBuilder* pli_shard_builder_ = nullptr;
    EvidenceSetBuilder* evidence_set_builder_ = nullptr;

    CSVConfig const csv_file_ = kTestDC;
    bool allow_cross_columns_ = true;
    bool has_header_ = true;

    void SetUp() override {
        parser_ = std::make_unique<CSVParser>(csv_file_);
        table_ = model::ColumnLayoutTypedRelationData::CreateFrom(*parser_, has_header_);
        col_data_ = std::move(table_->GetColumnData());
    }

    void TearDown() override {
        delete predicate_builder_;
        delete pli_shard_builder_;
        delete evidence_set_builder_;
    }

    void CreatePredicateBuilder() {
        predicate_builder_ =
                new PredicateBuilder(&pred_provider_, &pred_index_provider_, allow_cross_columns_);
    }

    void CreatePliShardBuilder() {
        pli_shard_builder_ = new PliShardBuilder(&int_prov_, &double_prov_, &string_prov_);
    }

    void CreateEvidenceSetBuilder() {
        evidence_set_builder_ =
                new EvidenceSetBuilder(*predicate_builder_, pli_shard_builder_->pli_shards);
    }
};

TEST_F(FastADC, DifferentColumnPredicateSpace) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);

    auto check_preds = [](auto const& actual, auto const& expected, std::string const& name) {
        ASSERT_EQ(actual.size(), expected.size())
                << "The number of " << name << " does not match the expected count.";

        for (size_t i = 0; i < actual.size(); ++i) {
            EXPECT_EQ(actual[i]->ToString(), expected[i])
                    << name << " at index " << i << " does not match the expected value.";
        }
    };

    check_preds(predicate_builder_->GetPredicates(), different_column_predicates_expected,
                "all predicates");
    check_preds(predicate_builder_->GetNumSingleColumnPredicates(),
                num_single_column_predicate_group_expected, "numeric single column predicates");
    check_preds(predicate_builder_->GetNumCrossColumnPredicates(),
                num_cross_column_predicate_group_expected, "numeric cross column predicates");
    check_preds(predicate_builder_->GetStrSingleColumnPredicates(),
                str_single_column_predicate_group_expected, "string single column predicates");
    check_preds(predicate_builder_->GetStrCrossColumnPredicates(),
                str_cross_column_predicate_group_expected, "string cross column predicates");
}

TEST_F(FastADC, InverseAndMutexMaps) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);

    auto const& predicates = predicate_builder_->GetPredicates();
    auto const& inverse_map = predicate_builder_->GetInverseMap();
    for (size_t i = 0; i < predicates.size(); ++i) {
        auto const& predicate = predicates[i];
        auto inverse_idx = inverse_map[i];
        auto const& inverse_predicate = predicates[inverse_idx];

        EXPECT_EQ(predicate->GetOperator().GetInverse(), inverse_predicate->GetOperator())
                << "Predicate at index " << i
                << " does not have the correct inverse operator at index " << inverse_idx;
    }

    auto const& mutex_map = predicate_builder_->GetMutexMap();
    for (size_t i = 0; i < predicates.size(); ++i) {
        auto const& predicate = predicates[i];
        auto const& mutex_bits = mutex_map[i];

        for (size_t bit = 0; bit < mutex_bits.size(); ++bit) {
            if (mutex_bits.test(bit)) {
                EXPECT_TRUE(predicate->HasSameOperandsAs(*predicates[bit]))
                        << "Predicate at index " << i << " is not mutex with predicate at index "
                        << bit;
            }
        }
    }
}

template <typename T>
void AssertClusterValues(model::TypedColumnData const& column, std::vector<size_t> const& cluster) {
    ASSERT_FALSE(cluster.empty()) << "Cluster is unexpectedly empty.";

    auto expected = GetValue<T>(column, cluster.front());
    for (auto row_index : cluster) {
        auto actual = GetValue<T>(column, row_index);
        ASSERT_EQ(expected, actual) << "Mismatch in cluster for key at row " << row_index;
    }
}

TEST_F(FastADC, PliShards) {
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);
    auto pli_shards = std::move(pli_shard_builder_->pli_shards);

    for (auto const& shard : pli_shards) {
        for (size_t i = 0; i < shard.plis.size(); ++i) {
            auto const& pli = shard.plis[i];
            auto const& keys = pli.GetKeys();
            auto const& clusters = pli.GetClusters();
            auto const& column = col_data_[i];

            for (auto key : keys) {
                auto const& cluster = clusters[pli.GetClusterIdByKey(key)];
                if (cluster.size() <= 1) continue;

                switch (column.GetTypeId()) {
                    case model::TypeId::kInt:
                        AssertClusterValues<int64_t>(column, cluster);
                        break;
                    case model::TypeId::kDouble:
                        AssertClusterValues<double>(column, cluster);
                        break;
                    case model::TypeId::kString:
                        AssertClusterValues<std::string>(column, cluster);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

TEST_F(FastADC, ClueSetPredicatePacksAndCorrectionMap) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);

    // won't be used, just to build some ClueSetBuilder to check generic static fields
    auto dummy_pli_shard = PliShard({}, 0, 0);
    SingleClueSetBuilder builder(*predicate_builder_, dummy_pli_shard);

    ASSERT_EQ(builder.GetNumberOfBitsInClue(), 18);
    auto packs = builder.GetPredicatePacks();
    auto correction_map = builder.GetCorrectionMap();

    for (size_t i = 0; i < packs.size(); ++i) {
        EXPECT_EQ(packs[i].left_idx, expected_column_indices[i].first);
        EXPECT_EQ(packs[i].right_idx, expected_column_indices[i].second);
        EXPECT_EQ(packs[i].eq_mask, VectorToBitset(expected_eq_masks[i]));
        if (!expected_gt_masks[i].empty()) {
            EXPECT_EQ(packs[i].gt_mask, VectorToBitset(expected_gt_masks[i]));
        }
    }

    for (size_t i = 0; i < correction_map.size(); ++i) {
        EXPECT_EQ(correction_map[i], VectorToBitset(expected_correction_map[i]));
    }
}

TEST_F(FastADC, ClueSet) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);

    ClueSetBuilder cluebuilder(*predicate_builder_);

    ClueSet clue_set = cluebuilder.BuildClueSet(pli_shard_builder_->pli_shards);

    for (auto const& [expected_clue, expected_count] : expected_clue_set) {
        auto found = clue_set.find(PredicateBitset(expected_clue));
        ASSERT_NE(found, clue_set.end()) << "Expected clue " << expected_clue << " not found!";
        ASSERT_EQ(found->second, expected_count) << "Count mismatch for clue " << expected_clue;
    }

    // Check that no additional clues are present
    for (auto const& [generated_clue, count] : clue_set) {
        uint64_t clue_value = generated_clue.to_ullong();
        ASSERT_NE(expected_clue_set.find(clue_value), expected_clue_set.end())
                << "Unexpected clue " << clue_value << " found!";
    }
}

TEST_F(FastADC, CardinalityMask) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);
    CreateEvidenceSetBuilder();

    EXPECT_EQ(evidence_set_builder_->GetCardinalityMask(),
              VectorToBitset(expected_cardinality_mask));
}

TEST_F(FastADC, EvidenceSet) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);
    CreateEvidenceSetBuilder();
    evidence_set_builder_->BuildEvidenceSet();

    auto evidence_set = std::move(evidence_set_builder_->evidence_set);

    std::unordered_set<PredicateBitset> expected_set;
    for (auto const& expected_vec : expected_evidence_set) {
        expected_set.insert(VectorToBitset(expected_vec));
    }

    for (size_t i = 0; i < evidence_set.Size(); ++i) {
        auto const& actual_evidence = evidence_set[i].evidence;
        EXPECT_TRUE(expected_set.find(actual_evidence) != expected_set.end())
                << "Unexpected evidence: " << actual_evidence;
    }

    EXPECT_EQ(evidence_set.Size(), expected_set.size())
            << "Size mismatch: evidence_set has extra elements.";
}

TEST_F(FastADC, TransformedEvidenceSetAndMutexMap) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);
    CreateEvidenceSetBuilder();
    evidence_set_builder_->BuildEvidenceSet();

    auto&& evidence_set = std::move(evidence_set_builder_->evidence_set);

    PredicateOrganizer organizer(predicate_builder_->PredicateCount(), std::move(evidence_set),
                                 std::move(predicate_builder_->GetMutexMap()));

    std::vector<Evidence> transfromed_evidence_set = organizer.TransformEvidenceSet();

    std::unordered_set<PredicateBitset> expected;
    for (auto const& expected_vec : expected_transformed_evidence_set) {
        expected.insert(VectorToBitset(expected_vec));
    }

    for (auto const& evidence : transfromed_evidence_set) {
        auto const& actual_evidence = evidence.evidence;
        EXPECT_TRUE(expected.find(actual_evidence) != expected.end())
                << "Unexpected evidence: " << actual_evidence;
    }

    std::vector<PredicateBitset> transfromed_mutex_map = organizer.TransformMutexMap();
    for (size_t i = 0; i < transfromed_mutex_map.size(); i++) {
        EXPECT_EQ(transfromed_mutex_map[i], VectorToBitset(expected_mutex_map[i]));
    }
}

struct ToStringComparator {
    bool operator()(DenialConstraint const& a, DenialConstraint const& b) const {
        return a.ToString() < b.ToString();
    }
};

TEST_F(FastADC, DenialConstraints) {
    CreatePredicateBuilder();
    predicate_builder_->BuildPredicateSpace(col_data_);
    CreatePliShardBuilder();
    pli_shard_builder_->BuildPliShards(col_data_);
    CreateEvidenceSetBuilder();
    evidence_set_builder_->BuildEvidenceSet();

    auto&& evidence_set = std::move(evidence_set_builder_->evidence_set);

    ApproxEvidenceInverter dcbuilder(*predicate_builder_, 0.01, std::move(evidence_set));
    auto dcs = dcbuilder.BuildDenialConstraints();

    std::vector<DenialConstraint> result = std::move(dcs.GetResult());
    std::set<DenialConstraint, ToStringComparator> ordered_result(
            std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));

    for (size_t i = 0; i < expected_denial_constraints.size(); i++) {
        std::string dc = std::next(ordered_result.begin(), i)->ToString();
        EXPECT_EQ(dc, expected_denial_constraints[i]) << "Unexpected denial constraint: " << dc;
    }
}

}  // namespace tests
