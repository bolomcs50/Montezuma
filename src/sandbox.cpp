#include <iostream>
#include <inttypes.h>
#include "thc.h"
#include "hashing.h"
#include "book.h"

void displayPosition( thc::ChessRules &cr, const std::string &description)
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN = %s", fen.c_str() );
    printf( "%s", s.c_str() );
}

int main(int argc, char const *argv[])
{
    thc::ChessEvaluation cr;
    cr.Forsyth("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    displayPosition(cr, "");
    uint64_t hash = montezuma::zobristHash64Calculate(cr);
    printf("%lx\n", hash);
    thc::Move mv;
//
//
//    mv.TerseIn(&cr, "b4c3");
//    uint64_t new_hash = montezuma::zobristHash64Update(hash, cr, mv);
//    cr.PushMove(mv);
//    displayPosition(cr, ""); 
//    printf("Calculated:\t%lx\n", montezuma::zobristHash64Calculate(cr));
//    printf("Updated:\t%lx\n", new_hash);
//
//    mv.TerseIn(&cr, "a1a3");
//    new_hash = montezuma::zobristHash64Update(new_hash, cr, mv);
//    cr.PushMove(mv);
//    displayPosition(cr, ""); 
//    printf("Calculated:\t%lx\n", montezuma::zobristHash64Calculate(cr));
//    printf("Updated:\t%lx\n", new_hash);
    
    montezuma::Book openingBook;
    openingBook.initialize("Titans.bin");
    thc::Move bestMove;

    return 0;
}