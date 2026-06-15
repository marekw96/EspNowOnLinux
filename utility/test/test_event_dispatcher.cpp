#include <gtest/gtest.h>
#include "event_dispatcher.hpp"
#include <string>
#include <vector>

// Define test event types
struct IntEvent {
    using argument_type = int;
};

struct StringEvent {
    using argument_type = std::string;
};

TEST(EventDispatcherTest, SingleSubscriber) {
    event_dispatcher dispatcher;
    int received_value = 0;
    int call_count = 0;

    auto guard = dispatcher.subscribe<IntEvent>([&](int val) {
        received_value = val;
        call_count++;
    });

    dispatcher.publish<IntEvent>(42);

    EXPECT_EQ(received_value, 42);
    EXPECT_EQ(call_count, 1);
}

TEST(EventDispatcherTest, MultipleSubscribers) {
    event_dispatcher dispatcher;
    std::vector<std::string> received_values;

    auto guard1 = dispatcher.subscribe<StringEvent>([&](const std::string& val) {
        received_values.push_back("Sub1: " + val);
    });

    auto guard2 = dispatcher.subscribe<StringEvent>([&](const std::string& val) {
        received_values.push_back("Sub2: " + val);
    });

    dispatcher.publish<StringEvent>("hello");

    ASSERT_EQ(received_values.size(), 2);
    EXPECT_EQ(received_values[0], "Sub1: hello");
    EXPECT_EQ(received_values[1], "Sub2: hello");
}

TEST(EventDispatcherTest, UnsubscribeViaGuardReset) {
    event_dispatcher dispatcher;
    int call_count_1 = 0;
    int call_count_2 = 0;

    auto guard1 = dispatcher.subscribe<IntEvent>([&](int) {
        call_count_1++;
    });

    auto guard2 = dispatcher.subscribe<IntEvent>([&](int) {
        call_count_2++;
    });

    dispatcher.publish<IntEvent>(10);
    EXPECT_EQ(call_count_1, 1);
    EXPECT_EQ(call_count_2, 1);

    // Unsubscribe the first handler by resetting its guard
    guard1.reset();

    dispatcher.publish<IntEvent>(20);
    EXPECT_EQ(call_count_1, 1); // Should not increase
    EXPECT_EQ(call_count_2, 2); // Should increase

    // Unsubscribe the second handler by resetting its guard
    guard2.reset();

    dispatcher.publish<IntEvent>(30);
    EXPECT_EQ(call_count_1, 1);
    EXPECT_EQ(call_count_2, 2);
}

TEST(EventDispatcherTest, MultipleEventTypes) {
    event_dispatcher dispatcher;
    int int_val = 0;
    std::string string_val = "";

    auto guard1 = dispatcher.subscribe<IntEvent>([&](int val) {
        int_val = val;
    });

    auto guard2 = dispatcher.subscribe<StringEvent>([&](const std::string& val) {
        string_val = val;
    });

    dispatcher.publish<IntEvent>(100);
    dispatcher.publish<StringEvent>("world");

    EXPECT_EQ(int_val, 100);
    EXPECT_EQ(string_val, "world");
}

TEST(EventDispatcherTest, NoSubscribersIsSafe) {
    event_dispatcher dispatcher;
    // Publishing an event with no subscribers should not crash or error.
    EXPECT_NO_THROW(dispatcher.publish<IntEvent>(999));
}

TEST(EventDispatcherTest, UnsubscribeNonExistentIsSafe) {
    event_dispatcher dispatcher;
    // Unsubscribing a non-existent ID should be a safe no-op.
    EXPECT_NO_THROW(dispatcher.unsubscribe(999));
}

TEST(EventDispatcherTest, SubscriptionGuardRAII) {
    event_dispatcher dispatcher;
    int call_count = 0;

    {
        auto guard = dispatcher.subscribe<IntEvent>([&](int) {
            call_count++;
        });

        dispatcher.publish<IntEvent>(1);
        EXPECT_EQ(call_count, 1);
    } // guard goes out of scope here -> should unsubscribe

    dispatcher.publish<IntEvent>(2);
    EXPECT_EQ(call_count, 1); // Should remain 1
}

TEST(EventDispatcherTest, SubscriptionGuardMove) {
    event_dispatcher dispatcher;
    int call_count = 0;

    subscription_guard outer_guard;

    {
        auto inner_guard = dispatcher.subscribe<IntEvent>([&](int) {
            call_count++;
        });

        dispatcher.publish<IntEvent>(1);
        EXPECT_EQ(call_count, 1);

        outer_guard = std::move(inner_guard);
    } // inner_guard goes out of scope (moved from, should not unsubscribe)

    dispatcher.publish<IntEvent>(2);
    EXPECT_EQ(call_count, 2); // Should increase to 2

    outer_guard.reset(); // Manually unsubscribe

    dispatcher.publish<IntEvent>(3);
    EXPECT_EQ(call_count, 2); // Should remain 2
}

TEST(EventDispatcherTest, SubscriptionGuardReassignment) {
    event_dispatcher dispatcher;
    int call_count_1 = 0;
    int call_count_2 = 0;

    auto guard = dispatcher.subscribe<IntEvent>([&](int) {
        call_count_1++;
    });

    dispatcher.publish<IntEvent>(1);
    EXPECT_EQ(call_count_1, 1);

    // Reassigning should unsubscribe the first callback
    guard = dispatcher.subscribe<IntEvent>([&](int) {
        call_count_2++;
    });

    dispatcher.publish<IntEvent>(2);
    EXPECT_EQ(call_count_1, 1); // Should remain 1
    EXPECT_EQ(call_count_2, 1); // Should be 1
}
