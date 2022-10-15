#include <iostream>
#include <inttypes.h>
#include "thc.h"
#include "hashing.h"

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
    cr.Forsyth("rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3");
    displayPosition(cr, "");
    uint64_t hash = montezuma::zobristHash64Calculate(cr);
    printf("%lx\n", hash);
    thc::Move mv;


    mv.TerseIn(&cr, "b4c3");
    uint64_t new_hash = montezuma::zobristHash64Update(hash, cr, mv);
    cr.PushMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", montezuma::zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);

    mv.TerseIn(&cr, "a1a3");
    new_hash = montezuma::zobristHash64Update(new_hash, cr, mv);
    cr.PushMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", montezuma::zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);
        
    return 0;
}