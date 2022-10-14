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
    cr.Forsyth("r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1");
    displayPosition(cr, "");
    uint64_t hash = zobristHash64Calculate(cr);
    printf("%lx\n", hash);
    thc::Move mv;


    mv.TerseIn(&cr, "b4c3");
    uint64_t new_hash = zobristHash64Update(hash, cr, mv);
    cr.PushMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);

    mv.TerseIn(&cr, "a1a3");
    new_hash = zobristHash64Update(new_hash, cr, mv);
    cr.PushMove(mv);
    displayPosition(cr, ""); 
    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
    printf("Updated:\t%lx\n", new_hash);

//    mv.TerseIn(&cr, "h1e1");
//    new_hash = zobristHash64Update(new_hash, cr, mv);
//    cr.PushMove(mv);
//    displayPosition(cr, ""); 
//    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
//    printf("Updated:\t%lx\n", new_hash);     
////     
//    mv.TerseIn(&cr, "e6e5");
//    new_hash = zobristHash64Update(new_hash, cr, mv);
//    cr.PushMove(mv);
//    displayPosition(cr, ""); 
//    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
//    printf("Updated:\t%lx\n", new_hash);
//
//    mv.TerseIn(&cr, "h2h1");
//    new_hash = zobristHash64Update(new_hash, cr, mv);
//    cr.PushMove(mv);
//    displayPosition(cr, ""); 
//    printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
//    printf("Updated:\t%lx\n", new_hash);

    
//     cr.PopMove(mv);
//     new_hash = zobristHash64Update(hash, cr, mv);
//     displayPosition(cr, ""); 
//     printf("Calculated:\t%lx\n", zobristHash64Calculate(cr));
//     printf("Updated:\t%lx\n", new_hash);
    
    return 0;
}