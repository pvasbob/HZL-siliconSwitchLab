#include "asic/bounded_queue_test.hpp"
#include "silicon_switch/asic/bounded_queue.hpp"
#include "test_support.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>

namespace silicon_switch::test {
void run_bounded_queue_tests(TestSuite& suite) {
    asic::BoundedQueue<int> queue{2U};
    suite.expect_equal(queue.capacity(),std::size_t{2U},"store bounded queue capacity");
    suite.expect_true(queue.empty(),"start with empty bounded queue");
    suite.expect_equal(queue.try_enqueue(10),asic::QueueEnqueueResult::enqueued,"enqueue first packet");
    suite.expect_equal(queue.try_enqueue(20),asic::QueueEnqueueResult::enqueued,"enqueue second packet");
    suite.expect_true(queue.full(),"report full bounded queue");
    suite.expect_equal(queue.try_enqueue(30),asic::QueueEnqueueResult::full,"report congestion queue drop");
    suite.expect_equal(queue.size(),std::size_t{2U},"full enqueue preserves queue size");
    suite.expect_equal(queue.try_dequeue().value(),10,"dequeue packets in FIFO order");
    suite.expect_equal(queue.wait_dequeue().value(),20,"wait dequeue available packet");
    suite.expect_false(queue.try_dequeue().has_value(),"report empty nonblocking dequeue");
    suite.expect_true(queue.close(),"close bounded queue");
    suite.expect_false(queue.close(),"bounded queue close is idempotent");
    suite.expect_true(queue.closed(),"report closed bounded queue");
    suite.expect_equal(queue.try_enqueue(40),asic::QueueEnqueueResult::closed,"reject enqueue after close");
    suite.expect_false(queue.wait_dequeue().has_value(),"closed empty queue ends wait");

    asic::BoundedQueue<int> draining{2U};
    suite.expect_equal(draining.try_enqueue(7),asic::QueueEnqueueResult::enqueued,"enqueue before close");
    suite.expect_true(draining.close(),"close queue containing packet");
    suite.expect_equal(draining.wait_dequeue().value(),7,"drain packet after queue close");
    suite.expect_false(draining.wait_dequeue().has_value(),"finish after draining closed queue");

    asic::BoundedQueue<int> waiting{1U};
    std::optional<int> waited_value{99};
    std::thread waiter{[&waiting,&waited_value] { waited_value=waiting.wait_dequeue(); }};
    suite.expect_true(waiting.close(),"close queue with waiting consumer");
    waiter.join();
    suite.expect_false(waited_value.has_value(),"close wakes waiting consumer");

    asic::BoundedQueue<std::unique_ptr<int>> move_only{1U};
    suite.expect_equal(move_only.try_enqueue(std::make_unique<int>(42)),
                       asic::QueueEnqueueResult::enqueued,"enqueue move-only queue value");
    const auto moved=move_only.try_dequeue();
    suite.expect_equal(**moved,42,"dequeue move-only queue value");

    bool rejected=false;
    try { const asic::BoundedQueue<int> invalid{0U}; static_cast<void>(invalid); }
    catch (const std::invalid_argument&) { rejected=true; }
    suite.expect_true(rejected,"reject zero queue capacity");
} }
