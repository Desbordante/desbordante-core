#include <numeric>

#include <gtest/gtest.h>

#include "core/util/lttree.h"

class LTTreeTest : public ::testing::Test {
protected:
    utils::LTTree<int> tree;
};

TEST_F(LTTreeTest, Insert) {
    tree.Insert(10, 0);
    tree.Insert(20, 1);
    tree.Insert(5, 2);

    EXPECT_EQ(tree.Size(), 3);  // 5 и 10 должны быть меньше 15
}

TEST_F(LTTreeTest, Size) {
    EXPECT_EQ(tree.Size(), 0);
    tree.Insert(10, 0);
    EXPECT_EQ(tree.Size(), 1);
    tree.Insert(20, 1);
    EXPECT_EQ(tree.Size(), 2);
}

// Тест вставки элементов
TEST_F(LTTreeTest, InsertAndSize) {
    EXPECT_EQ(tree.Size(), 0);

    tree.Insert(10, 0);
    EXPECT_EQ(tree.Size(), 1);

    tree.Insert(5, 1);
    EXPECT_EQ(tree.Size(), 2);

    tree.Insert(15, 2);
    EXPECT_EQ(tree.Size(), 3);

    tree.Insert(10, 3);  // Вставляем еще один элемент с тем же значением
    EXPECT_EQ(tree.Size(), 4);
}

// Тест поиска элементов
TEST_F(LTTreeTest, FindLess) {
    tree.Insert(10, 0);
    tree.Insert(5, 1);
    tree.Insert(15, 2);
    tree.Insert(3, 3);
    tree.Insert(7, 4);
    tree.Insert(12, 5);
    tree.Insert(20, 6);

    roaring::Roaring result = tree.FindLess(8);
    EXPECT_TRUE(result.cardinality() > 0);
}

// Тест баланса дерева
TEST_F(LTTreeTest, IsBalanced) {
    tree.Insert(10, 0);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(5, 1);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(15, 2);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(3, 3);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(7, 4);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(12, 5);
    EXPECT_TRUE(tree.IsBalanced());

    tree.Insert(20, 6);
    EXPECT_TRUE(tree.IsBalanced());
}

TEST_F(LTTreeTest, Height) {
    int const DATA_SIZE = 6;
    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    size_t height = tree.Height();
    EXPECT_EQ(height, 3);
}

// Тест вставки дубликатов
TEST_F(LTTreeTest, DuplicateInsert) {
    tree.Insert(10, 0);
    tree.Insert(10, 1);
    tree.Insert(10, 2);

    EXPECT_EQ(tree.Size(), 3);

    // Все индексы должны быть связаны с одним значением
    roaring::Roaring vec = tree.FindLess(11);
    EXPECT_EQ(vec.cardinality(), 3);
}

TEST_F(LTTreeTest, FindLessEquivalent) {
    for (size_t i = 0; i < 20; ++i) {
        tree.Insert(i, i);
    }

    roaring::Roaring result_less = tree.FindLess(10);
    EXPECT_TRUE(result_less.cardinality() > 0);

    roaring::Roaring result_eq = tree.FindLess(15);
    EXPECT_TRUE(result_eq.cardinality() - result_less.cardinality() > 0);
}

TEST_F(LTTreeTest, Contains) {
    int const DATA_SIZE = 100;

    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    for (int i = DATA_SIZE - 1; i >= 0; --i) {
        EXPECT_TRUE(tree.Contains(i));
        EXPECT_FALSE(tree.Contains(i + DATA_SIZE));
    }
}

TEST_F(LTTreeTest, Remove) {
    int const DATA_SIZE = 100;

    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    std::vector<size_t> expected_indices(DATA_SIZE);
    std::iota(expected_indices.begin(), expected_indices.end(), 0);

    for (int i = DATA_SIZE - 1; i >= 0; --i) {
        tree.Remove(i, i);
        EXPECT_TRUE(tree.IsBalanced());
        EXPECT_FALSE(tree.Contains(i, i));
        EXPECT_EQ(tree.Size(), i);

        for (int j = 0; j < i; ++j) {
            EXPECT_TRUE(tree.Contains(j, j));
        }

        expected_indices.pop_back();
        auto indices = utils::LTTree<int>::RoaringToVector(tree.FindLess(DATA_SIZE));

        EXPECT_EQ(indices, expected_indices);
    }
}

TEST_F(LTTreeTest, IteratorEmpty) {
    auto it = tree.begin();
    EXPECT_EQ(it, tree.end());
}

TEST_F(LTTreeTest, IteratorForward) {
    int const DATA_SIZE = 6;
    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    std::vector<size_t> res;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        res.push_back(it->val());
    }

    std::vector<size_t> expected_indices(DATA_SIZE);
    std::iota(expected_indices.begin(), expected_indices.end(), 0);
    EXPECT_EQ(res, expected_indices);
}

TEST_F(LTTreeTest, IteratorBackward) {
    int const DATA_SIZE = 100;
    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    std::vector<size_t> res;
    auto it = std::prev(tree.end());
    for (; it != tree.begin(); --it) {
        res.push_back(it->val());
    }
    res.push_back(it->val());

    std::vector<size_t> expected_indices;
    for (int i = DATA_SIZE - 1; i >= 0; --i) {
        expected_indices.push_back(static_cast<size_t>(i));
    }
    EXPECT_EQ(res, expected_indices);
}

TEST_F(LTTreeTest, IteratorSorted) {
    int const DATA_SIZE = 100;
    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i, i);
    }

    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end()));
}

TEST_F(LTTreeTest, IteratorDuplicates) {
    int const DATA_SIZE = 100;
    for (int i = 0; i < DATA_SIZE; ++i) {
        tree.Insert(i % 50, i);
    }

    std::vector<int> values;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        values.push_back(it->val());
    }

    EXPECT_EQ(values.size(), DATA_SIZE / 2);
}

TEST_F(LTTreeTest, Salaries) {
    std::vector<int> salaries = {3000, 4000, 5000, 5000, 6000, 4000, 1000, 2000, 3000, 3000};
    size_t index = 2;
    for (auto x : salaries) {
        tree.Insert(x, index++);
    }
    auto res = tree.FindLess(3100);
    roaring::Roaring expected = {2, 8, 9, 10, 11};
    EXPECT_EQ(res, expected);

    tree.Remove(3000, 11);
    tree.Insert(3100, 11);

    res = tree.FindLess(3100);
    expected = {2, 8, 9, 10};
    EXPECT_EQ(res, expected);
}