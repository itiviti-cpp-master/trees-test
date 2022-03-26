#include "TestedTreeType.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>

namespace {

std::string double_insert_error_message(int value)
{
    return "inserting an already contained value, the return value must be false;\n"
           "invalid insert for value " + std::to_string(value);
}

std::string double_remove_error_message(int value)
{
    return "inserting not contained value, the return value must be false;\n"
           "invalid remove for value " + std::to_string(value);
}

std::mt19937 & get_random_generator()
{
    // Default seed (5489u) so that tests are deterministic
    static std::mt19937 random; // NOLINT
    return random;
}

int get_random_number()
{
    std::uniform_int_distribution<int> dist(-1e6, 1e6);
    return dist(get_random_generator());
}

} // namespace

/**
 * General tests
 */

TEST(TreeTest, empty)
{
    const Tree tree{};

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
    EXPECT_FALSE(tree.contains(1));
    EXPECT_EQ(tree.values(), std::vector<int>{});
}

TEST(TreeTest, insert)
{
    Tree tree;

    EXPECT_TRUE(tree.insert(1));

    EXPECT_TRUE(tree.contains(1));
    EXPECT_FALSE(tree.contains(0));
    EXPECT_FALSE(tree.contains(2));

    EXPECT_FALSE(tree.empty());
    EXPECT_EQ(tree.size(), 1);
}

TEST(TreeTest, multiple_insert)
{
    Tree tree;
    int value = 1;

    ASSERT_TRUE(tree.insert(value));

    for (std::size_t i = 0; i < 5; ++i) {
        ASSERT_FALSE(tree.insert(value)) << double_insert_error_message(value);
        ASSERT_TRUE(tree.contains(value));
        ASSERT_EQ(tree.size(), 1);
    }
}

TEST(TreeTest, insert_and_check_order)
{
    std::vector<int> values(10);
    std::iota(values.begin(), values.end(), 1);

    std::vector<int> shuffled_values(values.begin(), values.end());
    std::shuffle(shuffled_values.begin(), shuffled_values.end(), get_random_generator());

    Tree tree;

    for (int value : shuffled_values) {
        ASSERT_TRUE(tree.insert(value));
    }

    for (int value : shuffled_values) {
        ASSERT_FALSE(tree.insert(value)) << double_insert_error_message(value);
    }

    for (int value : shuffled_values) {
        ASSERT_TRUE(tree.contains(value));
    }

    ASSERT_EQ(tree.size(), values.size());
    ASSERT_EQ(tree.values(), values);
}

TEST(TreeTest, remove)
{
    Tree tree;

    EXPECT_TRUE(tree.insert(1));

    EXPECT_TRUE(tree.remove(1));

    EXPECT_FALSE(tree.contains(1));
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
}

TEST(TreeTest, remove_from_empty_tree)
{
    Tree tree;

    EXPECT_FALSE(tree.remove(1));
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
}

TEST(TreeTest, multiple_remove)
{
    Tree tree;
    int value = 1;

    ASSERT_TRUE(tree.insert(value));
    ASSERT_TRUE(tree.remove(value));

    for (std::size_t i = 0; i < 5; ++i) {
        ASSERT_FALSE(tree.remove(value)) << double_remove_error_message(value);
        ASSERT_FALSE(tree.contains(value));
        ASSERT_TRUE(tree.empty());
        ASSERT_EQ(tree.size(), 0);
    }
}

TEST(TreeTest, remove_and_check_order)
{
    std::vector<int> values(10);
    std::iota(values.begin(), values.end(), 1);

    std::shuffle(values.begin(), values.end(), get_random_generator());

    Tree tree;

    for (int value : values) {
        ASSERT_TRUE(tree.insert(value));
    }

    std::shuffle(values.begin(), values.end(), get_random_generator());
    auto middle_it = values.begin() + values.size() / 2;

    for (auto it = middle_it; it != values.end(); ++it) {
        ASSERT_TRUE(tree.remove(*it));
    }

    for (auto it = middle_it; it != values.end(); ++it) {
        ASSERT_FALSE(tree.remove(*it)) << double_remove_error_message(*it);
    }

    for (auto it = values.begin(); it != values.end(); ++it) {
        ASSERT_EQ(tree.contains(*it), it < middle_it);
    }

    values.erase(middle_it, values.end());
    std::sort(values.begin(), values.end());

    ASSERT_EQ(tree.size(), values.size());
    ASSERT_EQ(tree.values(), values);
}

/**
 * Performance testing on large input data
 */
class PerformanceTest : public ::testing::TestWithParam<std::size_t>
{
protected:
    void insert_and_remove_random(double insert_likelihood)
    {
        Tree tree;
        std::set<int> set;

        std::size_t number_of_values = GetParam();
        std::bernoulli_distribution op_dist(insert_likelihood);

        for (std::size_t i = 0; i < number_of_values; ++i) {
            int value = get_random_number();
            if (op_dist(get_random_generator())) {
                ASSERT_EQ(tree.insert(value), set.insert(value).second);
            }
            else {
                ASSERT_EQ(tree.remove(value), set.erase(value) == 1);
            }
            ASSERT_EQ(tree.size(), set.size());
        }

        for (const int value : set) {
            ASSERT_TRUE(tree.contains(value));
        }

        for (std::size_t i = 0; i < number_of_values; ++i) {
            int value = get_random_number();
            ASSERT_EQ(tree.contains(value), set.find(value) != set.end());
        }

        ASSERT_EQ(tree.values(), std::vector(set.begin(), set.end()));
    }
};

TEST_P(PerformanceTest, insert_ascending_order)
{
    Tree tree;
    std::set<int> set;

    std::size_t number_of_values = GetParam();

    for (std::size_t i = 0; i < number_of_values; ++i) {
        ASSERT_EQ(tree.insert(i), set.insert(i).second);
        ASSERT_EQ(tree.size(), set.size());
    }

    for (const int value : set) {
        ASSERT_TRUE(tree.contains(value));
    }

    for (std::size_t i = 0; i < number_of_values; ++i) {
        int value = get_random_number();
        ASSERT_EQ(tree.contains(value), set.find(value) != set.end());
    }

    ASSERT_EQ(tree.values(), std::vector(set.begin(), set.end()));
}

TEST_P(PerformanceTest, insert_random)
{
    insert_and_remove_random(1);
}

TEST_P(PerformanceTest, insert_and_remove_random_balanced)
{
    insert_and_remove_random(.5);
}

TEST_P(PerformanceTest, insert_and_remove_random_unbalanced)
{
    insert_and_remove_random(.9);
}

INSTANTIATE_TEST_SUITE_P(TreeTest, PerformanceTest, ::testing::Values(1e3, 1e4, 2e5));
