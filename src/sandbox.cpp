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
    cr.Forsyth("k7/1p1rr1pp/R2p1p2/Q1pq4/P7/8/2P3PP/1R4K1 b - - 0 1");
    displayPosition(cr, "");
    
    thc::TERMINAL terminalScore;
    cr.Evaluate(terminalScore);    // Evaluates if position is legal, and if it is terminal
    if( terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE ){ // White is checkmated
        std::cout << "White is mated";
    } else if( terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE ){ // Black is checkmated
        std::cout << "Black is mated";
    } else if (terminalScore == thc::TERMINAL::TERMINAL_WSTALEMATE || terminalScore == thc::TERMINAL::TERMINAL_BSTALEMATE)
        std::cout << "Draw";
    std::cout << std::endl;

    return 0;
}