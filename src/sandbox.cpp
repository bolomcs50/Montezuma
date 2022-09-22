#include <iostream>
#include "thc.h"

int main(int argc, char const *argv[])
{
    
//    // DRAW DETECTION DEMO
//    thc::ChessEvaluation cr;
//    cr.Forsyth("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//    //cr.Forsyth("5r2/q6k/r5pP/4P2P/4Q3/1P3PK1/P7/R1N4R w - - 1 78");
//    std::vector<std::string> moves {"d2d4", "d7d5", "d1d2", "d8d7", "d2d1", "d7d8", "d1d2", "d8d7", "d2d1"};
//    // std::vector<std::string> moves {"g3g4", "a7d7", "g4g3", "d7a7", "g3g4", "a7d7", "g4g3"};
//
//    for (int i = 0; i < moves.size(); i++){
//            thc::Move mv;
//            mv.TerseIn(&cr, moves[i].c_str());
//            cr.PushMove(mv);
//    }
//
//
//    printf( "Position = %s", cr.ToDebugStr().c_str() );
//    printf( "RepCount %d\n\n", cr.GetRepetitionCount());
//
//    thc::Move mv;
//    mv.TerseIn(&cr, "d7d8");
//    cr.PushMove(mv);
//    printf( "Position = %s", cr.ToDebugStr().c_str() );
//    printf( "RepCount %d\n\n", cr.GetRepetitionCount());
    
    // HASH CORRECTNESS TEST
    thc::ChessEvaluation cr;
    cr.Forsyth("7k/p1p2bp1/3q1N1p/4rP2/4pQ2/2P4R/P2r2PP/4R2K w - - 1 1");
    std::vector<std::string> moves {"h3h6"};
    uint64_t currentHash = cr.Hash64Calculate();
    std::cout << "Starting Position:" << cr.ToDebugStr().c_str() << std::endl;
    std::cout << "Hash64Calculate(): " << currentHash << std::endl;
    

    thc::Move mv;
    mv.TerseIn(&cr, moves[0].c_str());
    cr.PushMove(mv);
    // Push the move
    std::cout << "Position after " << mv.TerseOut() << ": " <<cr.ToDebugStr().c_str();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    currentHash = cr.Hash64Update(currentHash, mv);
    std::cout << "Updated Hash: " << currentHash << std::endl;
    // Pop the move
    cr.PopMove(mv);
    std::cout << "Position after popping " << mv.TerseOut() << ": " <<cr.ToDebugStr().c_str();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    currentHash = cr.Hash64Update(currentHash, mv);
    std::cout << "Updated Hash: " << currentHash << std::endl;
    
    

    

    return 0;
}