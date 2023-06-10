#include "engine.h"
#include <iostream>
#include <sstream>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Engine initializes correctly"){
    std::stringstream iStream, oStream;
    montezuma::Engine engine(iStream, oStream);
    iStream << "uci\nquit\n";
    engine.protocolLoop();

    std::string s;
    std::getline(oStream, s);
    REQUIRE(s.compare("id name Montezuma") == 0);

    std::getline(oStream, s);
    REQUIRE(s.compare("id author Michele Bolognini") == 0);

    std::getline(oStream, s);
    REQUIRE(s.compare("uciok") == 0);
}

TEST_CASE("Engine reads positions correctly"){
}

TEST_CASE("Engine hashes positions correctly"){
}


// TEST_CASE("Example test with sections", "[Optional tag]"){
//     // Init code
//     REQUIRE(1 == 1);

//     SECTION("First section"){
//         REQUIRE(2 == 2);
//     }
// }
