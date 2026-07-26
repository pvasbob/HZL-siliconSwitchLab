#pragma once

#include <iostream>
#include <string_view>

namespace silicon_switch::test {

class TestSuite {
public:
    template<typename Actual, typename Expected>
    void expect_equal(const Actual& actual,
                      const Expected& expected,
                      const std::string_view test_name) {
        if (actual == expected) {
            pass(test_name);
            return;
        }

        fail(test_name);
    }

    void expect_true(const bool condition, const std::string_view test_name) {
        if (condition) {
            pass(test_name);
            return;
        }

        fail(test_name);
    }

    void expect_false(const bool condition, const std::string_view test_name) {
        expect_true(!condition, test_name);
    }

    [[nodiscard]] int exit_code() const {
        if (failure_count_ == 0) {
            std::cout << "All " << test_count_ << " tests passed\n";
            return 0;
        }

        std::cerr << failure_count_ << " of " << test_count_
                  << " tests failed\n";
        return 1;
    }

private:
    void pass(const std::string_view test_name) {
        ++test_count_;
        std::cout << "[PASS] " << test_name << '\n';
    }

    void fail(const std::string_view test_name) {
        ++test_count_;
        ++failure_count_;
        std::cerr << "[FAIL] " << test_name << '\n';
    }

    int test_count_{0};
    int failure_count_{0};
};

}  // namespace silicon_switch::test
