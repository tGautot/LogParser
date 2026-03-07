#include <catch2/catch_test_macros.hpp>
#include "cyclic_deque.hpp"

#include <stdexcept>
#include <string>

// ──────────────────────────────────────────────
// Basic state
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - initial state") {
  cyclic_deque<int> dq(5);
  REQUIRE(dq.empty());
  REQUIRE(dq.size() == 0);
  REQUIRE(dq.capacity() == 5);
  REQUIRE_FALSE(dq.full());
}

// ──────────────────────────────────────────────
// push_back
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - push_back and access") {
  cyclic_deque<int> dq(4);

  dq.push_back(10);
  REQUIRE(dq.size() == 1);
  REQUIRE_FALSE(dq.empty());
  REQUIRE(dq.front() == 10);
  REQUIRE(dq.back()  == 10);
  REQUIRE(dq[0]      == 10);

  dq.push_back(20);
  dq.push_back(30);
  REQUIRE(dq.size()  == 3);
  REQUIRE(dq.front() == 10);
  REQUIRE(dq.back()  == 30);
  REQUIRE(dq[0] == 10);
  REQUIRE(dq[1] == 20);
  REQUIRE(dq[2] == 30);
}

TEST_CASE("cyclic_deque - full flag") {
  cyclic_deque<int> dq(3);
  REQUIRE_FALSE(dq.full());
  dq.push_back(1); dq.push_back(2);
  REQUIRE_FALSE(dq.full());
  dq.push_back(3);
  REQUIRE(dq.full());
}

TEST_CASE("cyclic_deque - push_back auto-evicts front when full") {
  cyclic_deque<int> dq(3);
  dq.push_back(1);
  dq.push_back(2);
  dq.push_back(3);

  dq.push_back(4); // evicts 1
  REQUIRE(dq.size()  == 3);
  REQUIRE(dq.front() == 2);
  REQUIRE(dq.back()  == 4);
  REQUIRE(dq[0] == 2);
  REQUIRE(dq[1] == 3);
  REQUIRE(dq[2] == 4);

  dq.push_back(5); // evicts 2
  REQUIRE(dq[0] == 3);
  REQUIRE(dq[1] == 4);
  REQUIRE(dq[2] == 5);
}

TEST_CASE("cyclic_deque - wrap-around over many push_backs") {
  cyclic_deque<int> dq(4);
  for (int i = 0; i < 20; i++)
    dq.push_back(i);

  REQUIRE(dq.size()  == 4);
  REQUIRE(dq.front() == 16);
  REQUIRE(dq[0] == 16);
  REQUIRE(dq[1] == 17);
  REQUIRE(dq[2] == 18);
  REQUIRE(dq[3] == 19);
}

// ──────────────────────────────────────────────
// push_front
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - push_front and access") {
  cyclic_deque<int> dq(4);
  dq.push_back(20);
  dq.push_front(10);
  REQUIRE(dq.size()  == 2);
  REQUIRE(dq.front() == 10);
  REQUIRE(dq.back()  == 20);
  REQUIRE(dq[0] == 10);
  REQUIRE(dq[1] == 20);

  dq.push_front(5);
  REQUIRE(dq.size() == 3);
  REQUIRE(dq[0] == 5);
  REQUIRE(dq[1] == 10);
  REQUIRE(dq[2] == 20);
}

TEST_CASE("cyclic_deque - push_front auto-evicts back when full") {
  // Eviction order: [1,2,3] + push_front(0) → [0,1,2]
  cyclic_deque<int> dq(3);
  dq.push_back(1);
  dq.push_back(2);
  dq.push_back(3);

  dq.push_front(0); // should evict 3
  REQUIRE(dq.size()  == 3);
  REQUIRE(dq.front() == 0);
  REQUIRE(dq[0] == 0);
  REQUIRE(dq[1] == 1);
  REQUIRE(dq[2] == 2);
  REQUIRE(dq.back() == 2);
}

// ──────────────────────────────────────────────
// pop_back / pop_front
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - pop_back") {
  cyclic_deque<int> dq(4);
  dq.push_back(1);
  dq.push_back(2);
  dq.push_back(3);
  dq.pop_back();
  REQUIRE(dq.size()  == 2);
  REQUIRE(dq.back()  == 2);
  REQUIRE(dq[0] == 1);
  REQUIRE(dq[1] == 2);
}

TEST_CASE("cyclic_deque - pop_front") {
  cyclic_deque<int> dq(4);
  dq.push_back(1);
  dq.push_back(2);
  dq.push_back(3);
  dq.pop_front();
  REQUIRE(dq.size()  == 2);
  REQUIRE(dq.front() == 2);
  REQUIRE(dq[0] == 2);
  REQUIRE(dq[1] == 3);
}

TEST_CASE("cyclic_deque - pop on empty is no-op") {
  cyclic_deque<int> dq(4);
  dq.pop_back();
  dq.pop_front();
  REQUIRE(dq.empty());
}

TEST_CASE("cyclic_deque - alternating push and pop maintains order") {
  cyclic_deque<int> dq(4);
  dq.push_back(1);
  dq.push_back(2);
  dq.pop_front();       // removes 1
  dq.push_back(3);
  dq.pop_front();       // removes 2
  dq.push_back(4);
  REQUIRE(dq.size() == 2);
  REQUIRE(dq[0] == 3);
  REQUIRE(dq[1] == 4);
}

// ──────────────────────────────────────────────
// clear
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - clear") {
  cyclic_deque<int> dq(4);
  dq.push_back(1);
  dq.push_back(2);
  dq.clear();
  REQUIRE(dq.empty());
  REQUIRE(dq.size() == 0);

  dq.push_back(5);
  REQUIRE(dq.size()  == 1);
  REQUIRE(dq.front() == 5);
  REQUIRE(dq[0]      == 5);
}

// ──────────────────────────────────────────────
// emplace_back / emplace_front
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - emplace_back with non-trivial type") {
  cyclic_deque<std::string> dq(4);
  dq.emplace_back("hello");
  dq.emplace_back("world");
  REQUIRE(dq.size()  == 2);
  REQUIRE(dq.front() == "hello");
  REQUIRE(dq.back()  == "world");
  REQUIRE(dq[0] == "hello");
  REQUIRE(dq[1] == "world");
}

TEST_CASE("cyclic_deque - emplace_front with non-trivial type") {
  cyclic_deque<std::string> dq(4);
  dq.emplace_back("world");
  dq.emplace_front("hello");
  REQUIRE(dq.size()  == 2);
  REQUIRE(dq[0] == "hello");
  REQUIRE(dq[1] == "world");
}

TEST_CASE("cyclic_deque - emplace_back eviction calls destructor") {
  // Ensures non-trivial element is properly destroyed on eviction
  cyclic_deque<std::string> dq(2);
  dq.emplace_back("first");
  dq.emplace_back("second");
  dq.emplace_back("third"); // evicts "first"
  REQUIRE(dq.size() == 2);
  REQUIRE(dq[0] == "second");
  REQUIRE(dq[1] == "third");
}

TEST_CASE("cyclic_deque - emplace_front auto-evicts back when full") {
  cyclic_deque<std::string> dq(3);
  dq.emplace_back("a");
  dq.emplace_back("b");
  dq.emplace_back("c");

  dq.emplace_front("z"); // should evict "c"
  REQUIRE(dq.size()  == 3);
  REQUIRE(dq.front() == "z");
  REQUIRE(dq[0] == "z");
  REQUIRE(dq[1] == "a");
  REQUIRE(dq[2] == "b");
  REQUIRE(dq.back() == "b");
}

// ──────────────────────────────────────────────
// Move constructor
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - move constructor") {
  cyclic_deque<int> dq(4);
  dq.push_back(10);
  dq.push_back(20);
  dq.push_back(30);

  cyclic_deque<int> moved(std::move(dq));
  REQUIRE(moved.size()  == 3);
  REQUIRE(moved.front() == 10);
  REQUIRE(moved.back()  == 30);
  REQUIRE(moved[0] == 10);
  REQUIRE(moved[1] == 20);
  REQUIRE(moved[2] == 30);
}

// ──────────────────────────────────────────────
// operator[] bounds check
// ──────────────────────────────────────────────

TEST_CASE("cyclic_deque - operator[] out of bounds throws") {
  cyclic_deque<int> dq(4);
  dq.push_back(1);
  REQUIRE_THROWS_AS(dq[1], std::runtime_error);
  REQUIRE_THROWS_AS(dq[5], std::runtime_error);
}
