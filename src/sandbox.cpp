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
    cr.Forsyth("r1bqkbnr/ppp1pppp/2n5/1B2P3/2Pp4/8/PP1P1PPP/RNBQK1NR b KQkq c3 0 4");
    displayPosition(cr, "");
    uint64_t hash = zobristHash64Calculate(cr);
    printf("%lx\n", hash);
    thc::Move mv;
    mv.TerseIn(&cr, "d4c3");
    uint64_t new_hash = zobristHash64Update(hash, cr, mv);
    cr.PlayMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);
    
    
    return 0;
}