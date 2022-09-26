#include <iostream>
#include "thc.h"

int main(int argc, char const *argv[])
{
   
    // HASH CORRECTNESS TEST
    thc::ChessEvaluation cr;
    cr.Forsyth("r4br1/3b1kpp/1q1P4/1pp1RP1N/p7/6Q1/PPB3PP/2KR4 w - - 1 1");
    
    std::vector<std::string> moves {"d2d4", "d7d5", "d1d2", "d8d7", "d2d1", "d7d8", "d1d2", "d8d7", "d2d1"};
    
    thc::Move mv1;
    mv1.TerseIn(&cr, "g3g6");
    
    uint64_t currentHash = cr.Hash64Calculate();
    std::cout << "Starting Position:" << cr.ToDebugStr();
    std::cout << "Hash64Calculate(): " << currentHash << std::endl;
    
    currentHash = cr.Hash64Update(currentHash, mv1);
    cr.PushMove(mv1);
    // Push the move
    std::cout << "\nPosition after " << mv1.TerseOut() << ": " <<  cr.ToDebugStr();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    std::cout << "Updated Hash: " << currentHash << std::endl;
    
    thc::Move mv2;
    mv2.TerseIn(&cr, "h7g6");
    currentHash = cr.Hash64Update(currentHash, mv2);
    cr.PushMove(mv2);
    // Push the move
    std::cout << "\nPosition after " << mv2.TerseOut() << ": " <<  cr.ToDebugStr();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    std::cout << "Updated Hash: " << currentHash << std::endl;
    
    // Pop the move
    cr.PopMove(mv2);
    currentHash = cr.Hash64Update(currentHash, mv2);
    std::cout << "\nPosition after popping " << mv2.TerseOut() << ": " <<cr.ToDebugStr();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    std::cout << "Updated Hash: " << currentHash << std::endl;
    
    // Pop the move
    cr.PopMove(mv1);
    currentHash = cr.Hash64Update(currentHash, mv1);
    std::cout << "\nPosition after popping " << mv1.TerseOut() << ": " <<cr.ToDebugStr();
    std::cout << "Hash64Calculate(): " << cr.Hash64Calculate() << std::endl;
    std::cout << "Updated Hash: " << currentHash << std::endl;
    
    for (int i=0; i< 8; i++){
        for (int j = 0; j< 8; j++){
            std::cout << cr.squares[i*8+j];
        }
        std::cout << std::endl;
    }
    std::cout << (cr.squares[14] == 'p') << (cr.squares[15] == 'p') << (cr.squares[22] == 'p');
    
    
    

    

    return 0;
}