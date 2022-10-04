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
    std::cout << "Hash64: " << cr.Hash64Calculate() << std::endl << "Hash32: " << cr.HashCalculate() << std::endl;
}

int main(int argc, char const *argv[])
{
    thc::ChessEvaluation cr;
    cr.Forsyth("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3");
    printf("%lx\n", zobristHash64Calculate(cr));
    cr.Forsyth("rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4");
    printf("%lx\n", zobristHash64Calculate(cr));
    
    return 0;
}