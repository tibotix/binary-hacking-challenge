#include <catch2/catch_all.hpp>


#include "common.h"
#include "fifo.h"

using namespace CPUE;

TEST_CASE("try_enqueue", "[fifo]") {
    InplaceFIFO<int, 5> fifo;
    REQUIRE(fifo.try_enqueue(1));
    REQUIRE(fifo.try_enqueue(2));
    REQUIRE(fifo.try_enqueue(3));
    REQUIRE(fifo.try_enqueue(4));
    REQUIRE(fifo.try_enqueue(5));
    REQUIRE(!fifo.try_enqueue(6));
    REQUIRE(!fifo.try_enqueue(7));
}

TEST_CASE("take_first", "[fifo]") {
    InplaceFIFO<int, 5> fifo;
    REQUIRE(fifo.try_enqueue(1));
    REQUIRE(fifo.try_enqueue(2));
    REQUIRE(fifo.take_first().value() == 1);
    REQUIRE(fifo.try_enqueue(3));
    REQUIRE(fifo.try_enqueue(4));
    REQUIRE(fifo.try_enqueue(5));
    REQUIRE(fifo.try_enqueue(6));
    REQUIRE(!fifo.try_enqueue(7));
    REQUIRE(fifo.take_first().value() == 2);
    REQUIRE(fifo.try_enqueue(7));
    REQUIRE(fifo.take_first().value() == 3);
    REQUIRE(fifo.take_first().value() == 4);
    REQUIRE(fifo.take_first().value() == 5);
    REQUIRE(fifo.take_first().value() == 6);
    REQUIRE(fifo.take_first().value() == 7);
    REQUIRE(!fifo.take_first().has_value());
}


TEST_CASE("clear", "[fifo]") {
    InplaceFIFO<int, 5> fifo;
    REQUIRE(fifo.try_enqueue(1));
    REQUIRE(fifo.try_enqueue(2));
    REQUIRE(fifo.try_enqueue(3));
    REQUIRE(fifo.try_enqueue(4));
    REQUIRE(fifo.try_enqueue(5));
    REQUIRE(!fifo.try_enqueue(6));
    fifo.clear();
    REQUIRE(fifo.try_enqueue(6));
    REQUIRE(fifo.try_enqueue(7));
    REQUIRE(fifo.try_enqueue(8));
    REQUIRE(fifo.try_enqueue(9));
    REQUIRE(fifo.try_enqueue(10));
    REQUIRE(!fifo.try_enqueue(11));
}


TEST_CASE("size", "[fifo]") {
    InplaceFIFO<int, 5> fifo;
    REQUIRE(fifo.size() == 0);
    REQUIRE(fifo.try_enqueue(1));
    REQUIRE(fifo.try_enqueue(2));
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 1);
    REQUIRE(fifo.try_enqueue(3));
    REQUIRE(fifo.try_enqueue(4));
    REQUIRE(fifo.try_enqueue(5));
    REQUIRE(fifo.try_enqueue(6));
    REQUIRE(fifo.size() == 5);
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 4);
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 3);
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.try_enqueue(7));
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.try_enqueue(8));
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.try_enqueue(9));
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.try_enqueue(10));
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.try_enqueue(11));
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 2);
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.take_first().has_value());
    REQUIRE(fifo.size() == 0);
    REQUIRE(!fifo.take_first().has_value());
}
