#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include "tpool.h"

// --- Test: create pool, add 10 work items, assert all run ---

static std::atomic<int> g_work_counter{0};

static void* increment_counter(void* arg) {
    (void)arg;
    g_work_counter.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

TEST(TpoolTest, AllWorkItemsRun) {
    g_work_counter.store(0);
    constexpr int kPoolSize = 4;
    constexpr int kWorkItems = 10;

    ASSERT_EQ(tpool_create(kPoolSize), 0);

    for (int i = 0; i < kWorkItems; i++) {
        ASSERT_EQ(tpool_add_work(increment_counter, nullptr), 0);
    }

    // Wait for all work items to complete (with timeout)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (g_work_counter.load() < kWorkItems) {
        if (std::chrono::steady_clock::now() > deadline) {
            FAIL() << "Timed out waiting for work items. Got " << g_work_counter.load()
                   << " of " << kWorkItems;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(g_work_counter.load(), kWorkItems);
    tpool_destroy();
}

// --- Test: destroy pool correctly joins threads ---

static std::atomic<int> g_slow_counter{0};

static void* slow_work(void* arg) {
    (void)arg;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    g_slow_counter.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

TEST(TpoolTest, DestroyJoinsThreads) {
    g_slow_counter.store(0);
    constexpr int kPoolSize = 2;
    constexpr int kWorkItems = 4;

    ASSERT_EQ(tpool_create(kPoolSize), 0);

    for (int i = 0; i < kWorkItems; i++) {
        ASSERT_EQ(tpool_add_work(slow_work, nullptr), 0);
    }

    // Small delay to let some work start
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Destroy should block until all current work finishes
    tpool_destroy();

    // After destroy, at least the items that were in-progress should have completed.
    // Note: tpool_destroy sets shutdown flag, so pending items may not all run.
    // At minimum, threads that were running should have finished.
    EXPECT_GE(g_slow_counter.load(), kPoolSize);
}

// --- Test: strtok parsing with missing delimiter returns safely ---

TEST(ParsingTest, StrtokMissingDelimiter) {
    char buf[] = "no_delimiter_here";
    char delims[] = "$";
    std::vector<char*> resvec;

    char* res = strtok(buf, delims);
    while (res != nullptr) {
        resvec.push_back(res);
        res = strtok(nullptr, delims);
    }

    // With no '$' delimiter, strtok returns the whole string as one token
    ASSERT_EQ(resvec.size(), 1u);
    EXPECT_STREQ(resvec[0], "no_delimiter_here");
}

TEST(ParsingTest, StrtokNormalDelimiter) {
    char buf[] = "select * from test$mydb";
    char delims[] = "$";
    std::vector<char*> resvec;

    char* res = strtok(buf, delims);
    while (res != nullptr) {
        resvec.push_back(res);
        res = strtok(nullptr, delims);
    }

    ASSERT_EQ(resvec.size(), 2u);
    EXPECT_STREQ(resvec[0], "select * from test");
    EXPECT_STREQ(resvec[1], "mydb");
}

TEST(ParsingTest, StrtokEmptyString) {
    char buf[] = "";
    char delims[] = "$";
    std::vector<char*> resvec;

    char* res = strtok(buf, delims);
    while (res != nullptr) {
        resvec.push_back(res);
        res = strtok(nullptr, delims);
    }

    EXPECT_TRUE(resvec.empty());
}

// --- Test: tpool_add_work rejects null routine ---

TEST(TpoolTest, RejectsNullRoutine) {
    ASSERT_EQ(tpool_create(2), 0);
    EXPECT_EQ(tpool_add_work(nullptr, nullptr), -1);
    tpool_destroy();
}
