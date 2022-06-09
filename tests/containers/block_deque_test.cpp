#include "containers/block_deque.h"

#include <gtest/gtest.h>

#include <latch>
#include <thread>

using namespace ws;


class BlockDequeTest : public testing::Test {
protected:
    static constexpr std::size_t capacity_ {3};

    void SetUp() override {
        deq1_.PushBack(1);
        deq1_.PushBack(2);
        // The data should be `{1, 2}`.
    }

    BlockDeque<int> deq0_ {capacity_};
    BlockDeque<int> deq1_ {capacity_};
};

TEST_F(BlockDequeTest, Construction) {
    EXPECT_TRUE(deq0_.Empty());
    EXPECT_FALSE(deq0_.Full());
    EXPECT_EQ(deq0_.Size(), 0);
    EXPECT_EQ(deq0_.Capacity(), capacity_);

    EXPECT_EQ(deq0_.Pop(std::chrono::milliseconds {0}), std::nullopt);
}

TEST_F(BlockDequeTest, SingleThreadPushPop) {
    EXPECT_EQ(deq1_.Size(), 2);
    EXPECT_FALSE(deq1_.Empty());

    // `{1, 2}` → `{0, 1, 2}`.
    deq1_.PushFront(0);
    EXPECT_EQ(deq1_.Size(), 3);
    EXPECT_EQ(deq1_.Front(), 0);
    EXPECT_EQ(deq1_.Back(), 2);

    if (capacity_ == deq1_.Size()) {
        EXPECT_TRUE(deq1_.Full());
    }

    // `{0, 1, 2}` → `{1, 2}`
    EXPECT_EQ(deq1_.Pop(std::chrono::milliseconds {0}), 0);
    // `{1, 2}` → `{2}`
    EXPECT_EQ(deq1_.Pop(std::chrono::milliseconds {0}), 1);
    // `{2}` → `{}`
    EXPECT_EQ(deq1_.Pop(std::chrono::milliseconds {0}), 2);

    EXPECT_TRUE(deq1_.Empty());
}

TEST_F(BlockDequeTest, MultiThreadPushPop) {
    std::latch done {4};

    const auto push {[&done, this]() {
        deq1_.PushBack(0);
        done.count_down();
    }};

    const auto pop {[&done, this]() {
        deq1_.Pop(std::chrono::milliseconds {5000});
        done.count_down();
    }};

    EXPECT_EQ(deq1_.Size(), 2);
    std::thread {push}.detach();
    std::thread {pop}.detach();
    std::thread {pop}.detach();
    std::thread {pop}.detach();
    done.wait();
    EXPECT_TRUE(deq1_.Empty());
}

TEST_F(BlockDequeTest, Close) {
    EXPECT_FALSE(deq1_.Empty());
    deq1_.Close();
    EXPECT_TRUE(deq1_.Empty());
}