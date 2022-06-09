#include "containers/heap_timer.h"
#include "test_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ws;
using namespace ws::test;


class HeapTimerTest : public testing::Test {
protected:
    class Handler {
    public:
        MOCK_METHOD(void, OnTimeOut, (const int&), (const, noexcept));
    };

    using Clock = HeapTimer<int>::Clock;

    static inline const std::vector<int> init_vals_ {1, 2, 3, 4, 5};

    void SetUp() override {
        std::vector<int> vals {init_vals_.cbegin(), init_vals_.cend()};
        std::random_shuffle(vals.begin(), vals.end());

        std::ranges::for_each(vals, [this, &vals](const auto val) noexcept {
            // The nodes with large values will have a later expiry time.
            heap1_.Push(val, Clock::time_point {Clock::duration {val}},
                        [this](const int& key) { handler_.OnTimeOut(key); });

            // All nodes have the same expiry time.
            // Throw an exception in the callback.
            heap2_.Push(val, Clock::duration::zero(),
                        [this](const int&) { throw std::runtime_error {""}; });
        });
    }

    Handler handler_;
    HeapTimer<int> heap0_ {TestLogger()};
    HeapTimer<int> heap1_ {TestLogger()};
    HeapTimer<int> heap2_ {TestLogger()};
};

TEST_F(HeapTimerTest, Construction) {
    EXPECT_TRUE(heap0_.Empty());
    EXPECT_EQ(heap0_.Size(), 0);
}

TEST_F(HeapTimerTest, PushPop) {
    EXPECT_FALSE(heap1_.Empty());
    EXPECT_EQ(heap1_.Size(), init_vals_.size());

    // The timer system must contain all pushed nodes.
    for (const auto val : init_vals_) {
        EXPECT_TRUE(heap1_.Contain(val));
    }

    std::vector<int> vals;
    while (!heap1_.Empty()) {
        vals.push_back(heap1_.Pop());
    }

    EXPECT_EQ(vals, init_vals_);
}

TEST_F(HeapTimerTest, Adjust) {
    // Give the node `2` the longest expiry time.
    // `{1, 2, 3, 4, 5}` → `{1, 3, 4, 5, 2}`
    heap1_.Adjust(2, Clock::duration {100});
    std::vector<int> vals;
    while (!heap1_.Empty()) {
        vals.push_back(heap1_.Pop());
    }

    EXPECT_EQ(vals, (std::vector {1, 3, 4, 5, 2}));

    // Give the node `3` the shortest expiry time.
    heap2_.Adjust(3, Clock::time_point {Clock::duration {0}});
    EXPECT_EQ(heap2_.Pop(), 3);

    // Throw an exception if a node is not in the timer system.
    EXPECT_FALSE(heap0_.Contain(1));
    EXPECT_THROW((heap0_.Adjust(1, Clock::now())), std::out_of_range);
}

TEST_F(HeapTimerTest, Remove) {
    // Remove the node `2` from a timer system that all nodes have different and increasing expiry times.
    // `{1, 2, 3, 4, 5}` → `{1, 3, 4, 5}`
    EXPECT_TRUE(heap1_.Remove(2));
    std::vector<int> vals;
    while (!heap1_.Empty()) {
        vals.push_back(heap1_.Pop());
    }

    EXPECT_EQ(vals, (std::vector {1, 3, 4, 5}));

    // Remove the node `2` from a timer system that all nodes have the same expiry time.
    EXPECT_TRUE(heap2_.Remove(2));
    EXPECT_FALSE(heap2_.Contain(2));

    // Remove a non-existing node from a timer system.
    EXPECT_FALSE(heap0_.Contain(0));
    EXPECT_FALSE(heap0_.Remove(0));
}

TEST_F(HeapTimerTest, Invoke) {
    auto size {heap1_.Size()};
    EXPECT_CALL(handler_, OnTimeOut(1)).Times(1);
    heap1_.Invoke(1);
    size -= 1;
    EXPECT_EQ(heap1_.Size(), size);

    EXPECT_CALL(handler_, OnTimeOut(2)).Times(1);
    heap1_.Invoke(2);
    size -= 1;
    EXPECT_EQ(heap1_.Size(), size);

    // Throw an exception if a node is not in the timer system.
    EXPECT_THROW(heap0_.Invoke(1), std::out_of_range);
    EXPECT_THROW(heap1_.Invoke(1), std::out_of_range);

    // The exception in the callback will not be rethrown.
    EXPECT_NO_THROW(heap2_.Invoke(1));
}

TEST_F(HeapTimerTest, Tick) {
    using namespace testing;

    {
        // Callbacks should be invoked in order of expiration time.
        InSequence seq;
        for (const auto val : init_vals_) {
            EXPECT_CALL(handler_, OnTimeOut(val)).Times(1);
        }
    }

    heap1_.Tick();
    EXPECT_TRUE(heap1_.Empty());

    // The exception in the callback will not be rethrown.
    EXPECT_NO_THROW(heap2_.Tick());
    EXPECT_TRUE(heap2_.Empty());
}

TEST_F(HeapTimerTest, ToNextTick) {
    using namespace testing;

    {
        // Callbacks should be invoked in order of expiration time.
        InSequence seq;
        for (const auto val : init_vals_) {
            EXPECT_CALL(handler_, OnTimeOut(val)).Times(1);
        }
    }

    // All nodes have expired, so the interval from now to the next tick should be zero.
    EXPECT_EQ(heap1_.ToNextTick(), Clock::duration::zero());
}

TEST_F(HeapTimerTest, Clear) {
    EXPECT_FALSE(heap1_.Empty());
    heap1_.Clear();
    EXPECT_TRUE(heap1_.Empty());
}