#include <numeric>

#include <gtest/gtest.h>

#include "core/util/lttree.h"

class LTTreeTest : public ::testing::Test {
protected:
    utils::LTTree<int> tree_;
};

TEST_F(LTTreeTest, Insert) {
    tree_.Insert(10, 0);
    tree_.Insert(20, 1);
    tree_.Insert(5, 2);

    EXPECT_EQ(tree_.Size(), 3);  // 5 и 10 должны быть меньше 15
}

TEST_F(LTTreeTest, Size) {
    EXPECT_EQ(tree_.Size(), 0);
    tree_.Insert(10, 0);
    EXPECT_EQ(tree_.Size(), 1);
    tree_.Insert(20, 1);
    EXPECT_EQ(tree_.Size(), 2);
}

// Тест вставки элементов
TEST_F(LTTreeTest, InsertAndSize) {
    EXPECT_EQ(tree_.Size(), 0);

    tree_.Insert(10, 0);
    EXPECT_EQ(tree_.Size(), 1);

    tree_.Insert(5, 1);
    EXPECT_EQ(tree_.Size(), 2);

    tree_.Insert(15, 2);
    EXPECT_EQ(tree_.Size(), 3);

    tree_.Insert(10, 3);  // Вставляем еще один элемент с тем же значением
    EXPECT_EQ(tree_.Size(), 4);
}

// Тест поиска элементов
TEST_F(LTTreeTest, FindLess) {
    tree_.Insert(10, 0);
    tree_.Insert(5, 1);
    tree_.Insert(15, 2);
    tree_.Insert(3, 3);
    tree_.Insert(7, 4);
    tree_.Insert(12, 5);
    tree_.Insert(20, 6);

    roaring::Roaring result = tree_.FindLess(8);
    EXPECT_TRUE(result.cardinality() > 0);
}

// Тест баланса дерева
TEST_F(LTTreeTest, IsBalanced) {
    tree_.Insert(10, 0);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(5, 1);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(15, 2);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(3, 3);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(7, 4);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(12, 5);
    EXPECT_TRUE(tree_.IsBalanced());

    tree_.Insert(20, 6);
    EXPECT_TRUE(tree_.IsBalanced());
}

TEST_F(LTTreeTest, Height) {
    int const data_size = 6;
    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    size_t height = tree_.Height();
    EXPECT_EQ(height, 3);
}

// Тест вставки дубликатов
TEST_F(LTTreeTest, DuplicateInsert) {
    tree_.Insert(10, 0);
    tree_.Insert(10, 1);
    tree_.Insert(10, 2);

    EXPECT_EQ(tree_.Size(), 3);

    // Все индексы должны быть связаны с одним значением
    roaring::Roaring vec = tree_.FindLess(11);
    EXPECT_EQ(vec.cardinality(), 3);
}

TEST_F(LTTreeTest, FindLessEquivalent) {
    for (size_t i = 0; i < 20; ++i) {
        tree_.Insert(i, i);
    }

    roaring::Roaring result_less = tree_.FindLess(10);
    EXPECT_TRUE(result_less.cardinality() > 0);

    roaring::Roaring result_eq = tree_.FindLess(15);
    EXPECT_TRUE(result_eq.cardinality() - result_less.cardinality() > 0);
}

TEST_F(LTTreeTest, Contains) {
    int const data_size = 100;

    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    for (int i = data_size - 1; i >= 0; --i) {
        EXPECT_TRUE(tree_.Contains(i));
        EXPECT_FALSE(tree_.Contains(i + data_size));
    }
}

TEST_F(LTTreeTest, Remove) {
    int const data_size = 100;

    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    std::vector<size_t> expected_indices(data_size);
    std::iota(expected_indices.begin(), expected_indices.end(), 0);

    for (int i = data_size - 1; i >= 0; --i) {
        tree_.Remove(i, i);
        EXPECT_TRUE(tree_.IsBalanced());
        EXPECT_FALSE(tree_.Contains(i, i));
        EXPECT_EQ(tree_.Size(), i);

        for (int j = 0; j < i; ++j) {
            EXPECT_TRUE(tree_.Contains(j, j));
        }

        expected_indices.pop_back();
        auto indices = utils::LTTree<int>::RoaringToVector(tree_.FindLess(data_size));

        EXPECT_EQ(indices, expected_indices);
    }
}

TEST_F(LTTreeTest, IteratorEmpty) {
    auto it = tree_.begin();
    EXPECT_EQ(it, tree_.end());
}

TEST_F(LTTreeTest, IteratorForward) {
    int const data_size = 6;
    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    std::vector<size_t> res;
    for (auto it = tree_.begin(); it != tree_.end(); ++it) {
        res.push_back(it->val());
    }

    std::vector<size_t> expected_indices(data_size);
    std::iota(expected_indices.begin(), expected_indices.end(), 0);
    EXPECT_EQ(res, expected_indices);
}

TEST_F(LTTreeTest, IteratorBackward) {
    int const data_size = 100;
    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    std::vector<size_t> res;
    auto it = std::prev(tree_.end());
    for (; it != tree_.begin(); --it) {
        res.push_back(it->val());
    }
    res.push_back(it->val());

    std::vector<size_t> expected_indices;
    for (int i = data_size - 1; i >= 0; --i) {
        expected_indices.push_back(static_cast<size_t>(i));
    }
    EXPECT_EQ(res, expected_indices);
}

TEST_F(LTTreeTest, IteratorSorted) {
    int const data_size = 100;
    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i, i);
    }

    EXPECT_TRUE(std::is_sorted(tree_.begin(), tree_.end()));
}

TEST_F(LTTreeTest, IteratorDuplicates) {
    int const data_size = 100;
    for (int i = 0; i < data_size; ++i) {
        tree_.Insert(i % 50, i);
    }

    std::vector<int> values;
    for (auto it = tree_.begin(); it != tree_.end(); ++it) {
        values.push_back(it->val());
    }

    EXPECT_EQ(values.size(), data_size / 2);
}

TEST_F(LTTreeTest, Salaries) {
    std::vector<int> salaries = {3000, 4000, 5000, 5000, 6000, 4000, 1000, 2000, 3000, 3000};
    size_t index = 2;
    for (auto x : salaries) {
        tree_.Insert(x, index++);
    }
    auto res = tree_.FindLess(3100);
    roaring::Roaring expected = {2, 8, 9, 10, 11};
    EXPECT_EQ(res, expected);

    tree_.Remove(3000, 11);
    tree_.Insert(3100, 11);

    res = tree_.FindLess(3100);
    expected = {2, 8, 9, 10};
    EXPECT_EQ(res, expected);
}