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
    cr.Forsyth("r2q1r1k/pppb1p1n/3p1BpQ/3Pp3/2P1P1P1/2N3P1/PP2B3/R3K2R b KQ - 0 2");
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

    
    std::vector<thc::Move> legalMoves;
    cr.GenLegalMoveList(legalMoves);
    std::cout << legalMoves.size() << std::endl;
    
    return 0;
}