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
    cr.Forsyth("r3k2r/4pppp/8/8/8/8/4PPPP/R3K2R w - - 0 1");
    displayPosition(cr, "");
    uint64_t hash = zobristHash64Calculate(cr);
    printf("%lx\n", hash);
    thc::Move mv;
    mv.TerseIn(&cr, "e1d1");
    uint64_t new_hash = zobristHash64Update(hash, cr, mv);
    cr.PushMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);
    
    cr.PopMove(mv);
    new_hash = zobristHash64Update(hash, cr, mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);
    
    return 0;
}