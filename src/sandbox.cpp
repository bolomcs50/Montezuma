#include <iostream>
#include "thc.h"

void displayPosition( thc::ChessRules &cr, const std::string &description)
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN = %s", fen.c_str() );
    printf( "%s", s.c_str() );
    std::cout << "Hash64: " << cr.Hash64Calculate() << std::endl << "Hash32: " << cr.HashCalculate() << std::endl;
}

int main(int argc, char const *argv[])
{
   
    // HASH CORRECTNESS TEST
    thc::ChessEvaluation cr;
//    cr.Forsyth("1k1r4/3b1p2/1P1b3p/1p1p4/Q2P2pN/1R4P1/KPPq1PP1/4r2R b - - 2 1");
    cr.Forsyth("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    displayPosition(cr, "");
//    cr.Forsyth("1k1r4/3b1p2/1P1b3p/1p1p4/Q2P2pN/1R4P1/KPPq1PP1/4r2R w - - 2 1");
    cr.Forsyth("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    displayPosition(cr, "");

    return 0;
}