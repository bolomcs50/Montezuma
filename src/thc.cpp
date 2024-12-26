/****************************************************************************
 * Triple Happy Chess library = thc library
 *  This is thc rendered as a single thc.h header + thc.cpp source file to
 *  avoid the complications of libraries - Inspired by sqlite.c
 *
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

/*
    thc.cpp The basic idea is to concatenate the following into one .cpp file;

        Portability.cpp
        PrivateChessDefs.h
        HashLookup.h
        ChessPosition.cpp
        ChessRules.cpp
        ChessEvaluation.cpp
        Move.cpp
        PrivateChessDefs.cpp
         nested inline expansion of -> GeneratedLookupTables.h
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <algorithm>
#include "thc.h"
using namespace std;
using namespace thc;
/****************************************************************************
 * Portability.cpp Simple definitions to aid platform portability
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

// return 0 if case insensitive match
int strcmp_ignore( const char *s, const char *t )
{
    bool same=true;
    while( *s && *t && same )
    {
        char c = *s++;
        char d = *t++;
        same = (c==d) || (isascii(c) && isascii(d) && toupper(c)==toupper(d));
    }
    if( *s || *t )
        same = false;
    return same?0:1;
}

/****************************************************************************
 * PrivateChessDefs.h Chess classes - Internal implementation details
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#ifndef PRIVATE_CHESS_DEFS_H_INCLUDED
#define PRIVATE_CHESS_DEFS_H_INCLUDED

// TripleHappyChess
namespace thc
{

// Check whether a piece is black, white or an empty square, should really make
//  these and most other macros into inline functions
#define IsEmptySquare(p) ((p)==' ')
#define IsBlack(p) ((p)>'a')              // all lower case pieces
#define IsWhite(p) ((p)<'a' && (p)!=' ')  // all upper case pieces, and not empty

// Allow easy iteration through squares
inline Square& operator++ ( Square& sq )
{
    sq = (Square)(sq+1);
    return sq;
}

// Macro to convert chess notation to Square convention,   
//  eg SQ('c','5') -> c5
//  (We didn't always have such a sensible Square convention. SQ() remains
//  useful for cases like SQ(file,rank), but you may actually see examples
//  like the hardwired SQ('c','5') which can safely be changed to simply
//  c5).
#define SQ(f,r)  ( (Square) ( ('8'-(r))*8 + ((f)-'a') )   )

// More Square macros
#define FILE(sq)    ( (char) (  ((sq)&0x07) + 'a' ) )           // eg c5->'c'
#define RANK(sq)    ( (char) (  '8' - (((sq)>>3) & 0x07) ) )    // eg c5->'5'
#define IFILE(sq)   (  (int)(sq) & 0x07 )                       // eg c5->2
#define IRANK(sq)   (  7 - ((((int)(sq)) >>3) & 0x07) )         // eg c5->4
#define SOUTH(sq)   (  (Square)((sq) + 8) )                     // eg c5->c4
#define NORTH(sq)   (  (Square)((sq) - 8) )                     // eg c5->c6
#define SW(sq)      (  (Square)((sq) + 7) )                     // eg c5->b4
#define SE(sq)      (  (Square)((sq) + 9) )                     // eg c5->d4
#define NW(sq)      (  (Square)((sq) - 9) )                     // eg c5->b6
#define NE(sq)      (  (Square)((sq) - 7) )                     // eg c5->d6

// Utility macro
#ifndef nbrof
    #define nbrof(array) (sizeof((array))/sizeof((array)[0]))
#endif

/* DETAIL is shorthand for the section of type ChessPosition that looks
   like this;

    Square enpassant_target : 8;
    Square wking_square     : 8;
    Square bking_square     : 8;
    int  wking              : 1;
    int  wqueen             : 1;
    int  bking              : 1;
    int  bqueen             : 1;

  We assume it is located in the last 4 bytes of ChessPosition,
  hence the definition of typedef DETAIL as unsigned long, and
  of DETAIL_ADDR below. We assume that ANDing the unsigned
  character at this address + 3, with ~WKING, where WKING
  is defined as unsigned char 0x01, will clear wking. See the
  definition of DETAIL_CASTLING and castling_prohibited_table[].
  These assumptions are likely not portable and are tested in
  TestInternals(). If porting this code, step through that code
  first and make any adjustments necessary */

#define DETAIL_ADDR         ( (DETAIL*) ((char *)this + sizeof(ChessPosition) - sizeof(DETAIL))  )
#define DETAIL_SAVE         DETAIL tmp = *DETAIL_ADDR
#define DETAIL_RESTORE      *DETAIL_ADDR = tmp
#define DETAIL_EQ_ALL               ( (*DETAIL_ADDR&0x0fffffff) == (tmp&0x0fffffff) )
#define DETAIL_EQ_CASTLING          ( (*DETAIL_ADDR&0x0f000000) == (tmp&0x0f000000) )
#define DETAIL_EQ_KING_POSITIONS    ( (*DETAIL_ADDR&0x00ffff00) == (tmp&0x00ffff00) )
#define DETAIL_EQ_EN_PASSANT        ( (*DETAIL_ADDR&0x000000ff) == (tmp&0x000000ff) )
#define DETAIL_PUSH         detail_stack[detail_idx++] = *DETAIL_ADDR
#define DETAIL_POP          *DETAIL_ADDR = detail_stack[--detail_idx]
#define DETAIL_CASTLING(sq) *( 3 + (unsigned char*)DETAIL_ADDR ) &= castling_prohibited_table[sq]

// Bits corresponding to detail bits wking, wqueen, bking, bqueen for
//  DETAIL_CASTLING
#define WKING   0x01    
#define WQUEEN  0x02    
#define BKING   0x04    
#define BQUEEN  0x08


// Convert piece, eg 'N' to bitmask in lookup tables. See automatically
//  PrivateChessDefs.cpp and GeneratedLookupTables.h for format of
//  lookup tables
extern lte to_mask[];

// Lookup squares a queen can move to
extern const lte *queen_lookup[];

// Lookup squares a rook can move to
extern const lte *rook_lookup[];

// Lookup squares a bishop can move to
extern const lte *bishop_lookup[];

// Lookup squares a knight can move to
extern const lte *knight_lookup[];

// Lookup squares a king can move to
extern const lte *king_lookup[];

// Lookup squares a white pawn can move to
extern const lte *pawn_white_lookup[];

// Lookup squares a black pawn can move to
extern const lte *pawn_black_lookup[];

// Lookup good squares for enemy king when a king is on a square in an endgame
extern const lte *good_king_position_lookup[];

// Lookup squares from which an enemy pawn attacks white
extern const lte *pawn_attacks_white_lookup[];

// Lookup squares from which an enemy pawn attacks black
extern const lte *pawn_attacks_black_lookup[];

// Lookup squares from which enemy pieces attack white
extern const lte *attacks_white_lookup[];

// Lookup squares from which enemy pieces attack black
extern const lte *attacks_black_lookup[];

} //namespace thc

#endif // PRIVATE_CHESS_DEFS_H_INCLUDED


/****************************************************************************
 * ChessPosition.cpp Chess classes - Representation of the position on the board
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

/* Some project notes */
/*====================*/

/****************************************************************************
 * Class Hierarchy:
 *
 *  The Chess classes are intended to provide a reusable chess subsystem. To
 *  understand the hierarchy of classes, think in terms of the Java concept
 *  of derived classes "extending" base classes.
 *
 *  So
 *    ChessPositionRaw
 *      ;A simple C style structure that holds a chess position
 *    ChessPosition   extends ChessPositionRaw
 *      ;ChessPosition adds constructors, and other simple facilities
 *    ChessRules      extends ChessPosition
 *      ;ChessRules adds all the rules of standard chess
 *    ChessEvaluation extends ChessRules
 *      ;ChessEvaluation is a simple scoring function for a chess engine
 *      ;it attempts to calculate a score (more +ve numbers for better white
 *      ;positions, more -ve numbers for better black positions) for a
 *      ;position. Tarrasch GUI uses this only to attempt to list the most
 *      ;plausible moves first when you click on a square that can receive
 *      ;multiple moves
 *    ChessEngine     extends ChessEvaluation
 *      ;ChessEngine adds a search tree to ChessEvaluation to make a
 *      ;complete simple engine (the Tarrasch Toy Engine)
 *      ;ChessEngine is now split off from the rest of the thc chess library
 *      ;and is part of the Tarrasch Toy Engine project instead
 ****************************************************************************/

/****************************************************************************
 * Explanation of namespace thc:
 *
 *  The Chess classes use the C++ namespace facility to ensure they can be
 *  used without name conflicts with other 3rd party code. A short, but
 *  highly likely to be unique namespace name is best, we choose "thc" which
 *  is short for TripleHappyChess.
 ****************************************************************************/

/****************************************************************************
 * My original license, now replaced by MIT license full text of which is
 * in file LICENSE in project's root directory.
 *
 * Licensing provisions for all TripleHappy Chess source code;
 *
 * Start Date: 15 February 2003.
 * This software is licensed to be used freely. The following licensing
 * provisions apply;
 *
 * 1) The 'author' is asserted to be the original author of the code, Bill
 *    Forster of Wellington, New Zealand.
 * 2) The 'licensee' is anyone else who wishes to use the software.
 * 3) The 'licensee' is entitled to do anything they wish with the software
 *    EXCEPT for any action that attempts to restrict in any way the rights
 *    granted in 4) below.
 * 4) The 'author' is entitled to do anything he wishes with the software.
 *
 * The intent of this license is to allow the licensees wide freedom to
 * incorporate, modify and sell the software, with the single caveat that
 * they cannot prevent the author from either further development or future
 * commercial use of the software.
 ****************************************************************************/


// Return true if ChessPositions are the same (including counts)
bool ChessPosition::CmpStrict( const ChessPosition &other ) const
{
    return( *this == other  &&
            half_move_clock == other.half_move_clock &&
            full_move_count == other.full_move_count
         );
}

/****************************************************************************
 * Set up position on board from Forsyth string with extensions
 *   return bool okay
 ****************************************************************************/
bool ChessPosition::Forsyth( const char *txt )
{
    int   file, rank, skip, store, temp;
    int   count_wking=0, count_bking=0;
    char  c, cross;
    char  p;
    bool okay, done;
    const char *reset = txt;

    // When store==0 validate txt without storing results
    for( store=0, okay=true; store<2 && okay; store++ )
    {
        txt = reset;

        // Clear the board
        if( store )
        {
            for( Square square=a8; square<=h1; ++square )
            {
                squares[square]   = ' ';
            }

            // Clear the extension fields
            wking  = false;
            wqueen = false;
            bking  = false;
            bqueen = false;
            enpassant_target = SQUARE_INVALID;
            half_move_clock = 0;
            full_move_count = 1;
        }

        // Loop through the main Forsyth field
        for( file=0, rank=7, done=false; *txt && okay && !done; )
        {
            skip = 1;
            c = *txt++;
            p = ' ';
            cross = ' ';
            switch(c)
            {
                case 'x':   cross = 'x';
                            skip = 1;
                            break;
                case ' ':
                case '\t':  done = true;            break;
                case 'k':   p = 'k';
                            count_bking++;          break;
                case 'K':   p = 'K';
                            count_wking++;          break;
                case 'p':
                case 'r':
                case 'n':
                case 'b':
                case 'q':
                case 'P':
                case 'R':
                case 'N':
                case 'B':
                case 'Q':   p = c;   break;
                case '1':   case '2':   case '3':   case '4':
                case '5':   case '6':   case '7':   case '8':
                            skip = c-'0';       break;
                case '/':   // official separator
                case '|':   //  .. we'll allow this
                case '\\':  //  .. or this
                {
                    if( file == 0 )
                        skip = 0;        // eg after "rn6/", '/' does nothing
                    else
                        skip = (8-file); // eg after "rn/", '/' skips 6 squares
                    while( *txt == '/'   // allow consecutive '/' characters
                      ||   *txt == '|'   //     (or '|' or '\')
                      ||   *txt == '\\' )
                    {
                        txt++;
                        skip += 8;
                    }
                    break;
                }
                default:    okay=false;
            }

            // Store single piece or loop over multiple empty squares
            for( int i=0; i<skip && okay && !done; i++ )
            {
                Square sq = SQ('a'+file, '1'+rank);
                if( store )
                {
                    squares[sq] = p;
                    if( p == 'K' )
                        wking_square = sq;
                    else if( p == 'k' )
                        bking_square = sq;
                }
                file++;
                if( file == 8 )
                {
                    file = 0;
                    rank--;
                }
                if( sq == h1 )
                    done = true;        // last square, we're done !
            }
        }

        // In validation pass make sure there's 1 white king and 1 black king
   /*   if( store==0 )  // disabled to allow ILLEGAL_NOT_ONE_KING_EACH message
        {
            if( count_wking!=1 || count_bking!=1 )
                okay = false;
        }  */

        // Now support standard FEN notation with extensions. See appendix to
        // .PGN standard on the Internet (and in project documentation)
        //
        // Briefly there are a total of 6 fields, the normal Forsyth encoding
        // (as above) plus 5 extension fields. Format of the 5 extension
        // fields is illustrated by example.
        //
        // Example extension fields; "w KQq e6 0 2"
        //
        // White to move,
        // All types of castling available, except black king side (so black's
        //  king rook has moved, other king and rook pieces unmoved). This
        //  whole field is replaced by '-' character if no castling available.
        // The last move played was a double pawn advance, and hence the
        //  'en-passant target square' field is NOT empty. The field is the
        //  square a potential capturing pawn would end up on (can deduce last
        //  move was e7e5)
        // Number of half-moves or ply since a pawn move or capture (for 50
        //  move rule)
        // Total (full-move) count (starts at 1, increments after black moves)

        // Who to move
        if( okay )
        {
            if( *txt == '/'
             || *txt == '|'
             || *txt == '\\' )
                txt++;
            while( *txt==' ' || *txt=='\t' )
                txt++;
            if( *txt=='W' || *txt=='w' )
            {
                if( store )
                    white = true;
                txt++;
            }
            else if( *txt=='B' || *txt=='b' )
            {
                if( store )
                    white = false;
                txt++;
            }
            else
                okay = false;
        }

        // Castling flags
        if( okay )
        {
            while( *txt==' ' || *txt=='\t' )
                txt++;
            if( *txt == '-' )
                txt++;
            else
            {
                for( int i=0; i<4 && okay; i++, txt++ )
                {
                    if( *txt == 'K' )
                    {
                        if( store )
                            wking = true;
                    }
                    else if( *txt == 'Q' )
                    {
                        if( store )
                            wqueen = true;
                    }
                    else if( *txt == 'k' )
                    {
                        if( store )
                            bking = true;
                    }
                    else if( *txt == 'q' )
                    {
                        if( store )
                            bqueen = true;
                    }
                    else if( *txt == '-' )               // allow say "KQ-q "
                        ;
                    else if( *txt == ' ' || *txt=='\t' ) // or "KQq "
                        break;
                    else
                        okay = false;
                }
            }
        }

        // Enpassant target
        if( okay )
        {
            while( *txt==' ' || *txt=='\t' )
                txt++;
            if( *txt == '-' )
                txt++;
            else
            {
                char f='a', r='1';
                if( 'a'<=*txt && *txt<='h' )
                    f = *txt++;
                else if( 'A'<=*txt && *txt<='H' )
                {
                    f = *txt++;
                    f = f-'A'+'a';
                }
                else
                    okay = false;
                if( okay )
                {
                    if( '1'<=*txt && *txt<='8' )
                        r = *txt++;
                    else
                        okay = false;
                }
                if( okay && store )
                    enpassant_target = SQ(f,r);
            }
        }

        // Half move clock
        if( okay )
        {
            okay=false;
            while( *txt==' ' || *txt=='\t' )
                txt++;
            temp = atoi(txt);
            if( temp >= 0 )
            {
                while( isascii(*txt) && isdigit(*txt) )
                {
                    okay = true;
                    txt++;
                }
                if( okay && store )
                    half_move_clock = temp;
            }
        }

        // Full move count
        if( okay )
        {
            while( *txt==' ' || *txt=='\t' )
                txt++;
            temp = atoi(txt);
            if( temp < 0 )
                okay = false;
            else if( store )
                full_move_count = temp;
        }
    }
    return( okay );
}

/****************************************************************************
 * ChessRules.cpp Chess classes - Rules of chess
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

// Table indexed by Square, gives mask for DETAIL_CASTLING, such that a move
//  to (or from) that square results in castling being prohibited, eg a move
//  to e8 means that subsequently black kingside and black queenside castling
//  is prohibited
static unsigned char castling_prohibited_table[] =
{
    (unsigned char)(~BQUEEN), 0xff, 0xff, 0xff,                             // a8-d8
    (unsigned char)(~(BQUEEN+BKING)), 0xff, 0xff, (unsigned char)(~BKING),  // e8-h8
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a7-h7
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a6-h6
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a5-h5
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a4-h4
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a3-h3
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,                         // a2-h2
    (unsigned char)(~WQUEEN), 0xff, 0xff, 0xff,                             // a1-d1
    (unsigned char)(~(WQUEEN+WKING)),  0xff, 0xff, (unsigned char)(~WKING)  // e1-h1
};

/****************************************************************************
 * Test internals, for porting to new environments etc
 *   For the moment at least, this is best used by stepping through it
 *   thoughtfully with a debugger. It is not set up to automatically
 *   check whether THC is going to work in the new environment
 ****************************************************************************/
static int log_discard( const char *, ... ) { return 0; }


/****************************************************************************
 * Play a move
 ****************************************************************************/
void ChessRules::PlayMove( Move imove )
{
    // Legal move - save it in history
    history[history_idx++] = imove; // ring array

    // Update full move count
    if( !white )
        full_move_count++;

    // Update half move clock
    if( squares[imove.src] == 'P' || squares[imove.src] == 'p' )
        half_move_clock=0;   // pawn move
    else if( !IsEmptySquare(imove.capture) )
        half_move_clock=0;   // capture
    else
        half_move_clock++;   // neither pawn move or capture

    // Actually play the move
    PushMove( imove );
}


/****************************************************************************
 * Create a list of all legal moves in this position
 ****************************************************************************/
void ChessRules::GenLegalMoveList( vector<Move> &moves )
{
    MOVELIST movelist;
    GenLegalMoveList( &movelist );
    for( int i=0; i<movelist.count; i++ )
        moves.push_back( movelist.moves[i] );
}

/****************************************************************************
 * Create a list of all legal moves in this position, with extra info
 ****************************************************************************/
void ChessRules::GenLegalMoveList( vector<Move>  &moves,
                                   vector<bool>  &check,
                                   vector<bool>  &mate,
                                   vector<bool>  &stalemate )
{
    MOVELIST movelist;
    bool bcheck[MAXMOVES];
    bool bmate[MAXMOVES];
    bool bstalemate[MAXMOVES];
    GenLegalMoveList( &movelist, bcheck, bmate, bstalemate  );
    for( int i=0; i<movelist.count; i++ )
    {
        moves.push_back    ( movelist.moves[i] );
        check.push_back    ( bcheck[i] );
        mate.push_back     ( bmate[i] );
        stalemate.push_back( bstalemate[i] );
    }
}

/****************************************************************************
 * Create a list of all legal moves in this position
 ****************************************************************************/
void ChessRules::GenLegalMoveList( MOVELIST *list )
{
    int i, j;
    bool okay;
    MOVELIST list2;

    // Generate all moves, including illegal (eg put king in check) moves
    GenMoveList( &list2 );

    // Loop copying the proven good ones
    for( i=j=0; i<list2.count; i++ )
    {
        PushMove( list2.moves[i] );
        okay = Evaluate();
        PopMove( list2.moves[i] );
        if( okay )
            list->moves[j++] = list2.moves[i];
    }
    list->count  = j;
}

/****************************************************************************
 * Create a list of all legal moves in this position, with extra info
 ****************************************************************************/
void ChessRules::GenLegalMoveList( MOVELIST *list, bool check[MAXMOVES],
                                                    bool mate[MAXMOVES],
                                                    bool stalemate[MAXMOVES] )
{
    int i, j;
    bool okay;
    TERMINAL terminal_score;
    MOVELIST list2;

    // Generate all moves, including illegal (eg put king in check) moves
    GenMoveList( &list2 );

    // Loop copying the proven good ones
    for( i=j=0; i<list2.count; i++ )
    {
        PushMove( list2.moves[i] );
        okay = Evaluate(terminal_score);
        Square king_to_move = (Square)(white ? wking_square : bking_square );
        bool bcheck = false;
        if( AttackedPiece(king_to_move) )
            bcheck = true;
        PopMove( list2.moves[i] );
        if( okay )
        {
            stalemate[j] = (terminal_score==TERMINAL_WSTALEMATE ||
                            terminal_score==TERMINAL_BSTALEMATE);
            mate[j]      = (terminal_score==TERMINAL_WCHECKMATE ||
                            terminal_score==TERMINAL_BCHECKMATE);
            check[j]     = mate[j] ? false : bcheck;
            list->moves[j++] = list2.moves[i];
        }
    }
    list->count  = j;
}

/****************************************************************************
 * Check draw rules (50 move rule etc.)
 ****************************************************************************/
bool ChessRules::IsDraw( bool white_asks, DRAWTYPE &result )
{
    bool   draw=false;

    // Insufficient mating material
    draw =  IsInsufficientDraw( white_asks, result );

    // 50 move rule
    if( !draw && half_move_clock>=100 )
    {
        result = DRAWTYPE_50MOVE;
        draw = true;
    }

    // 3 times repetition,
    if( !draw && GetRepetitionCount()>=3 )
    {
        result = DRAWTYPE_REPITITION;
        draw = true;
    }

    if( !draw )
        result = NOT_DRAW;
    return( draw );
}

/****************************************************************************
 * Get number of times position has been repeated
 ****************************************************************************/
int ChessRules::GetRepetitionCount()
{
    int matches=0;

    //  Save those aspects of current position that are changed by multiple
    //  PopMove() calls as we search backwards (i.e. squares, white,
    //  detail, detail_idx)
    char save_squares[sizeof(squares)];
    memcpy( save_squares, squares, sizeof(save_squares) );
    unsigned char save_detail_idx = detail_idx;  // must be unsigned char
    bool          save_white      = white;
    unsigned char idx             = history_idx; // must be unsigned char
    DETAIL_SAVE;

    // Search backwards ....
    int nbr_half_moves = (full_move_count-1)*2 + (!white?1:0);
    if( nbr_half_moves > nbrof(history)-1 )
        nbr_half_moves = nbrof(history)-1;
    if( nbr_half_moves > nbrof(detail_stack)-1 )
        nbr_half_moves = nbrof(detail_stack)-1;
    for( int i=0; i<nbr_half_moves; i++ )
    {
        Move m = history[--idx];
        if( m.src == m.dst )
            break;  // unused history is set to zeroed memory
        PopMove(m);

        // ... looking for matching positions
        if( white    == save_white      && // quick ones first!
            DETAIL_EQ_KING_POSITIONS    &&
            0 == memcmp(squares,save_squares,sizeof(squares) )
            )
        {
            matches++;
            if( !DETAIL_EQ_ALL )    // Castling flags and/or enpassant target different?
            {
                // It might not be a match (but it could be - we have to unpack what the differences
                //  really mean)
                bool revoke_match = false;

                // Revoke match if different value of en-passant target square means different
                //  en passant possibilities
                if( !DETAIL_EQ_EN_PASSANT )
                {
                    int ep_saved = (int)(tmp&0xff);
                    int ep_now   = (int)(*DETAIL_ADDR&0xff);

                    // Work out whether each en_passant is a real one, i.e. is there an opposition
                    //  pawn in place to capture (if not it's just a double pawn advance with no
                    //  actual enpassant consequences)
                    bool real=false;
                    int ep = ep_saved;
                    char const *squ = save_squares;
                    for( int j=0; j<2; j++ )
                    {
                        if( ep == 0x10 )    // 0x10 = a6
                        {
                             real = (squ[SE(ep)] == 'P');
                        }
                        else if( 0x10<ep && ep<0x17 )   // 0x10 = h6
                        {
                             real = (squ[SW(ep)] == 'P' || squ[SE(ep)] == 'P');
                        }
                        else if( ep==0x17 )
                        {
                             real = (squ[SW(ep)] == 'P');
                        }
                        else if( 0x28==ep )   // 0x28 = a3
                        {
                             real = (squ[NE(ep)] == 'p');
                        }
                        else if( 0x28<ep && ep<0x2f )   // 0x2f = h3
                        {
                             real = (squ[NE(ep)] == 'p' || squ[NW(ep)] == 'p');
                        }
                        else if( ep==0x2f )
                        {
                             real = (squ[NW(ep)] == 'p' );
                        }
                        if( j > 0 )
                            ep_now = real?ep:0x40;      // evaluate second time through
                        else
                        {
                            ep_saved = real?ep:0x40;    // evaluate first time through
                            ep = ep_now;                // setup second time through
                            squ = squares;
                            real = false;
                        }
                    }

                    // If for example one en_passant is real and the other not, it's not a real match
                    if( ep_saved != ep_now )
                        revoke_match = true;
                }

                // Revoke match if different value of castling flags means different
                //  castling possibilities
                if( !revoke_match && !DETAIL_EQ_CASTLING )
                {
                    bool wking_saved  = save_squares[e1]=='K' && save_squares[h1]=='R' && (int)(tmp&(WKING<<24));
                    bool wking_now    = squares[e1]=='K' && squares[h1]=='R' && (int)(*DETAIL_ADDR&(WKING<<24));
                    bool bking_saved  = save_squares[e8]=='k' && save_squares[h8]=='r' && (int)(tmp&(BKING<<24));
                    bool bking_now    = squares[e8]=='k' && squares[h8]=='r' && (int)(*DETAIL_ADDR&(BKING<<24));
                    bool wqueen_saved = save_squares[e1]=='K' && save_squares[a1]=='R' && (int)(tmp&(WQUEEN<<24));
                    bool wqueen_now   = squares[e1]=='K' && squares[a1]=='R' && (int)(*DETAIL_ADDR&(WQUEEN<<24));
                    bool bqueen_saved = save_squares[e8]=='k' && save_squares[a8]=='r' && (int)(tmp&(BQUEEN<<24));
                    bool bqueen_now   = squares[e8]=='k' && squares[a8]=='r' && (int)(*DETAIL_ADDR&(BQUEEN<<24));
                    revoke_match = ( wking_saved != wking_now ||
                                     bking_saved != bking_now ||
                                     wqueen_saved != wqueen_now ||
                                     bqueen_saved != bqueen_now );
                }

                // If the real castling or enpassant possibilities differ, it's not a match
                //  At one stage we just did a naive binary match of the details - not good enough. For example
                //  a rook moving away from h1 doesn't affect the WKING flag, but does disallow white king side
                //  castling
                if( revoke_match )
                     matches--;
            }
        }

        // For performance reasons, abandon search early if pawn move
        //  or capture
        if( squares[m.src]=='P' || squares[m.src]=='p' || !IsEmptySquare(m.capture) )
            break;
    }

    // Restore current position
    memcpy( squares, save_squares, sizeof(squares) );
    white      = save_white;
    detail_idx = save_detail_idx;
    DETAIL_RESTORE;
    return( matches+1 );  // +1 counts original position
}

/****************************************************************************
 * Check insufficient material draw rule
 ****************************************************************************/
bool ChessRules::IsInsufficientDraw( bool white_asks, DRAWTYPE &result )
{
    char   piece;
    int    piece_count=0;
    bool   bishop_or_knight=false, lone_wking=true, lone_bking=true;
    bool   draw=false;

    // Loop through the board
    for( Square square=a8; square<=h1; ++square )
    {
        piece = squares[square];
        switch( piece )
        {
            case 'B':
            case 'b':
            case 'N':
            case 'n':       bishop_or_knight=true;  // and fall through
            case 'Q':
            case 'q':
            case 'R':
            case 'r':
            case 'P':
            case 'p':       piece_count++;
                            if( isupper(piece) )
                                lone_wking = false;
                            else
                                lone_bking = false;
                            break;
        }
        if( !lone_wking && !lone_bking )
            break;  // quit early for performance
    }

    // Automatic draw if K v K or K v K+N or K v K+B
    //  (note that K+B v K+N etc. is not auto granted due to
    //   selfmates in the corner)
    if( piece_count==0 ||
        (piece_count==1 && bishop_or_knight)
      )
    {
        draw = true;
        result = DRAWTYPE_INSUFFICIENT_AUTO;
    }
    else
    {

        // Otherwise side playing against lone K can claim a draw
        if( white_asks && lone_bking )
        {
            draw   = true;
            result = DRAWTYPE_INSUFFICIENT;
        }
        else if( !white_asks && lone_wking )
        {
            draw   = true;
            result = DRAWTYPE_INSUFFICIENT;
        }
    }
    return( draw );
}

/****************************************************************************
 * Generate a list of all possible moves in a position
 ****************************************************************************/
void ChessRules::GenMoveList( MOVELIST *l )
{
    Square square;

    // Convenient spot for some asserts
    //  Have a look at TestInternals() for this,
    //   A ChessPositionRaw should finish with 32 bits of detail information
    //   (see DETAIL macros, this assert() checks that)
    assert( sizeof(ChessPositionRaw) ==
               (offsetof(ChessPositionRaw,full_move_count) + sizeof(full_move_count) + sizeof(DETAIL))
          );

    // We also rely on Moves being 32 bits for the implementation of Move
    //  bitwise == and != operators
    assert( sizeof(Move) == sizeof(int32_t) );

    // Clear move list
    l->count  = 0;   // set each field for each move

    // Loop through all squares
    for( square=a8; square<=h1; ++square )
    {

        // If square occupied by a piece of the right colour
        char piece=squares[square];
        if( (white&&IsWhite(piece)) || (!white&&IsBlack(piece)) )
        {

            // Generate moves according to the occupying piece
            switch( piece )
            {
                case 'P':
                {
                    WhitePawnMoves( l, square );
                    break;
                }
                case 'p':
                {
                    BlackPawnMoves( l, square );
                    break;
                }
                case 'N':
                case 'n':
                {
                    const lte *ptr = knight_lookup[square];
                    ShortMoves( l, square, ptr, NOT_SPECIAL );
                    break;
                }
                case 'B':
                case 'b':
                {
                    const lte *ptr = bishop_lookup[square];
                    LongMoves( l, square, ptr );
                    break;
                }
                case 'R':
                case 'r':
                {
                    const lte *ptr = rook_lookup[square];
                    LongMoves( l, square, ptr );
                    break;
                }
                case 'Q':
                case 'q':
                {
                    const lte *ptr = queen_lookup[square];
                    LongMoves( l, square, ptr );
                    break;
                }
                case 'K':
                case 'k':
                {
                    KingMoves( l, square );
                    break;
                }
            }
        }
    }
}

/****************************************************************************
 * Generate moves for pieces that move along multi-move rays (B,R,Q)
 ****************************************************************************/
void ChessRules::LongMoves( MOVELIST *l, Square square, const lte *ptr )
{
    Move *m=&l->moves[l->count];
    Square dst;
    lte nbr_rays = *ptr++;
    while( nbr_rays-- )
    {
        lte ray_len = *ptr++;
        while( ray_len-- )
        {
            dst = (Square)*ptr++;
            char piece=squares[dst];

            // If square not occupied (empty), add move to list
            if( IsEmptySquare(piece) )
            {
                m->src     = square;
                m->dst     = dst;
                m->capture = ' ';
                m->special = NOT_SPECIAL;
                m++;
                l->count++;
            }

            // Else must move to end of ray
            else
            {
                ptr += ray_len;
                ray_len = 0;

                // If not occupied by our man add a capture
                if( (white&&IsBlack(piece)) || (!white&&IsWhite(piece)) )
                {
                    m->src     = square;
                    m->dst     = dst;
                    m->special = NOT_SPECIAL;
                    m->capture = piece;
                    l->count++;
                    m++;
                }
            }
        }
    }
}

/****************************************************************************
 * Generate moves for pieces that move along single move rays (N,K)
 ****************************************************************************/
void ChessRules::ShortMoves( MOVELIST *l, Square square,
                                         const lte *ptr, SPECIAL special  )
{
    Move *m=&l->moves[l->count];
    Square dst;
    lte nbr_moves = *ptr++;
    while( nbr_moves-- )
    {
        dst = (Square)*ptr++;
        char piece = squares[dst];

        // If square not occupied (empty), add move to list
        if( IsEmptySquare(piece) )
        {
            m->src     = square;
            m->dst     = dst;
            m->special = special;
            m->capture = ' ';
            m++;
            l->count++;
        }

        // Else if occupied by enemy man, add move to list as a capture
        else if( (white&&IsBlack(piece)) || (!white&&IsWhite(piece)) )
        {
            m->src     = square;
            m->dst     = dst;
            m->special = special;
            m->capture = piece;
            m++;
            l->count++;
        }
    }
}

/****************************************************************************
 * Generate list of king moves
 ****************************************************************************/
void ChessRules::KingMoves( MOVELIST *l, Square square )
{
    const lte *ptr = king_lookup[square];
    ShortMoves( l, square, ptr, SPECIAL_KING_MOVE );

    // Generate castling king moves
    Move *m;
    m = &l->moves[l->count];

    // White castling
    if( square == e1 )   // king on e1 ?
    {

        // King side castling
        if(
            squares[g1] == ' '   &&
            squares[f1] == ' '   &&
            squares[h1] == 'R'   &&
            (wking)            &&
            !AttackedSquare(e1,false) &&
            !AttackedSquare(f1,false) &&
            !AttackedSquare(g1,false)
          )
        {
            m->src     = e1;
            m->dst     = g1;
            m->special = SPECIAL_WK_CASTLING;
            m->capture = ' ';
            m++;
            l->count++;
        }

        // Queen side castling
        if(
            squares[b1] == ' '         &&
            squares[c1] == ' '         &&
            squares[d1] == ' '         &&
            squares[a1] == 'R'         &&
            (wqueen)                 &&
            !AttackedSquare(e1,false)  &&
            !AttackedSquare(d1,false)  &&
            !AttackedSquare(c1,false)
          )
        {
            m->src     = e1;
            m->dst     = c1;
            m->special = SPECIAL_WQ_CASTLING;
            m->capture = ' ';
            m++;
            l->count++;
        }
    }

    // Black castling
    if( square == e8 )   // king on e8 ?
    {

        // King side castling
        if(
            squares[g8] == ' '         &&
            squares[f8] == ' '         &&
            squares[h8] == 'r'         &&
            (bking)                  &&
            !AttackedSquare(e8,true) &&
            !AttackedSquare(f8,true) &&
            !AttackedSquare(g8,true)
          )
        {
            m->src     = e8;
            m->dst     = g8;
            m->special = SPECIAL_BK_CASTLING;
            m->capture = ' ';
            m++;
            l->count++;
        }

        // Queen side castling
        if(
            squares[b8] == ' '         &&
            squares[c8] == ' '         &&
            squares[d8] == ' '         &&
            squares[a8] == 'r'         &&
            (bqueen)                 &&
            !AttackedSquare(e8,true) &&
            !AttackedSquare(d8,true) &&
            !AttackedSquare(c8,true)
          )
        {
            m->src     = e8;
            m->dst     = c8;
            m->special = SPECIAL_BQ_CASTLING;
            m->capture = ' ';
            m++;
            l->count++;
        }
    }
}

/****************************************************************************
 * Generate list of white pawn moves
 ****************************************************************************/
void ChessRules::WhitePawnMoves( MOVELIST *l,  Square square )
{
    Move *m = &l->moves[l->count];
    const lte *ptr = pawn_white_lookup[square];
    bool promotion = (RANK(square) == '7');

    // Capture ray
    lte nbr_moves = *ptr++;
    while( nbr_moves-- )
    {
        Square dst = (Square)*ptr++;
        if( dst == enpassant_target )
        {
            m->src     = square;
            m->dst     = dst;
            m->special = SPECIAL_WEN_PASSANT;
            m->capture = 'p';
            m++;
            l->count++;
        }
        else if( IsBlack(squares[dst]) )
        {
            m->src    = square;
            m->dst    = dst;
            m->capture = squares[dst];
            if( !promotion )
            {
                m->special = NOT_SPECIAL;
                m++;
                l->count++;
            }
            else
            {

                // Generate (under)promotions in the order (Q),N,B,R
                //  but we no longer rely on this elsewhere as it
                //  stops us reordering moves
                m->special   = SPECIAL_PROMOTION_QUEEN;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_KNIGHT;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_BISHOP;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_ROOK;
                m++;
                l->count++;
            }
        }
    }

    // Advance ray
    nbr_moves = *ptr++;
    for( lte i=0; i<nbr_moves; i++ )
    {
        Square dst = (Square)*ptr++;

        // If square occupied, end now
        if( !IsEmptySquare(squares[dst]) )
            break;
        m->src     = square;
        m->dst     = dst;
        m->capture = ' ';
        if( !promotion )
        {
            m->special  =  (i==0 ? NOT_SPECIAL : SPECIAL_WPAWN_2SQUARES);
            m++;
            l->count++;
        }
        else
        {

            // Generate (under)promotions in the order (Q),N,B,R
            //  but we no longer rely on this elsewhere as it
            //  stops us reordering moves
            m->special   = SPECIAL_PROMOTION_QUEEN;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_KNIGHT;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_BISHOP;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_ROOK;
            m++;
            l->count++;
        }
    }
}

/****************************************************************************
 * Generate list of black pawn moves
 ****************************************************************************/
void ChessRules::BlackPawnMoves( MOVELIST *l, Square square )
{
    Move *m = &l->moves[l->count];
    const lte *ptr = pawn_black_lookup[square];
    bool promotion = (RANK(square) == '2');

    // Capture ray
    lte nbr_moves = *ptr++;
    while( nbr_moves-- )
    {
        Square dst = (Square)*ptr++;
        if( dst == enpassant_target )
        {
            m->src     = square;
            m->dst     = dst;
            m->special = SPECIAL_BEN_PASSANT;
            m->capture = 'P';
            m++;
            l->count++;
        }
        else if( IsWhite(squares[dst]) )
        {
            m->src  = square;
            m->dst    = dst;
            m->capture = squares[dst];
            if( !promotion )
            {
                m->special = NOT_SPECIAL;
                m++;
                l->count++;
            }
            else
            {

                // Generate (under)promotions in the order (Q),N,B,R
                //  but we no longer rely on this elsewhere as it
                //  stops us reordering moves
                m->special   = SPECIAL_PROMOTION_QUEEN;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_KNIGHT;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_BISHOP;
                m++;
                l->count++;
                m->src       = square;
                m->dst       = dst;
                m->capture   = squares[dst];
                m->special   = SPECIAL_PROMOTION_ROOK;
                m++;
                l->count++;
            }
        }
    }

    // Advance ray
    nbr_moves = *ptr++;
    for( int i=0; i<nbr_moves; i++ )
    {
        Square dst = (Square)*ptr++;

        // If square occupied, end now
        if( !IsEmptySquare(squares[dst]) )
            break;
        m->src  = square;
        m->dst  = dst;
        m->capture = ' ';
        if( !promotion )
        {
            m->special  =  (i==0 ? NOT_SPECIAL : SPECIAL_BPAWN_2SQUARES);
            m++;
            l->count++;
        }
        else
        {

            // Generate (under)promotions in the order (Q),N,B,R
            //  but we no longer rely on this elsewhere as it
            //  stops us reordering moves
            m->special   = SPECIAL_PROMOTION_QUEEN;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_KNIGHT;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_BISHOP;
            m++;
            l->count++;
            m->src       = square;
            m->dst       = dst;
            m->capture   = ' ';
            m->special   = SPECIAL_PROMOTION_ROOK;
            m++;
            l->count++;
        }
    }
}

/****************************************************************************
 * Make a move (with the potential to undo)
 ****************************************************************************/
void ChessRules::PushMove( Move& m )
{
    // Push old details onto stack
    DETAIL_PUSH;

    // Update castling prohibited flags for destination square, eg h8 -> bking
    DETAIL_CASTLING(m.dst);
                    // IMPORTANT - only dst is required since we also qualify
                    //  castling with presence of rook and king on right squares.
                    //  (I.E. if a rook or king leaves its original square, the
                    //  castling prohibited flag is unaffected, but it doesn't
                    //  matter since we won't castle unless rook and king are
                    //  present on the right squares. If subsequently a king or
                    //  rook returns, that's okay too because the  castling flag
                    //  is cleared by its arrival on the m.dst square, so
                    //  castling remains prohibited).
    enpassant_target = SQUARE_INVALID;

    // Special handling might be required
    switch( m.special )
    {
        default:
        squares[m.dst] = squares[m.src];
        squares[m.src] = ' ';
        break;

        // King move updates king position in details field
        case SPECIAL_KING_MOVE:
        squares[m.dst] = squares[m.src];
        squares[m.src] = ' ';
        if( white )
            wking_square = m.dst;
        else
            bking_square = m.dst;
        break;

        // In promotion case, dst piece doesn't equal src piece
        case SPECIAL_PROMOTION_QUEEN:
        squares[m.src] = ' ';
        squares[m.dst] = (white?'Q':'q');
        break;

        // In promotion case, dst piece doesn't equal src piece
        case SPECIAL_PROMOTION_ROOK:
        squares[m.src] = ' ';
        squares[m.dst] = (white?'R':'r');
        break;

        // In promotion case, dst piece doesn't equal src piece
        case SPECIAL_PROMOTION_BISHOP:
        squares[m.src] = ' ';
        squares[m.dst] = (white?'B':'b');
        break;

        // In promotion case, dst piece doesn't equal src piece
        case SPECIAL_PROMOTION_KNIGHT:
        squares[m.src] = ' ';
        squares[m.dst] = (white?'N':'n');
        break;

        // White enpassant removes pawn south of destination
        case SPECIAL_WEN_PASSANT:
        squares[m.src] = ' ';
        squares[m.dst] = 'P';
        squares[ SOUTH(m.dst) ] = ' ';
        break;

        // Black enpassant removes pawn north of destination
        case SPECIAL_BEN_PASSANT:
        squares[m.src] = ' ';
        squares[m.dst] = 'p';
        squares[ NORTH(m.dst) ] = ' ';
        break;

        // White pawn advances 2 squares sets an enpassant target
        case SPECIAL_WPAWN_2SQUARES:
        squares[m.src] = ' ';
        squares[m.dst] = 'P';
        enpassant_target = SOUTH(m.dst);
        break;

        // Black pawn advances 2 squares sets an enpassant target
        case SPECIAL_BPAWN_2SQUARES:
        squares[m.src] = ' ';
        squares[m.dst] = 'p';
        enpassant_target = NORTH(m.dst);
        break;

        // Castling moves update 4 squares each
        case SPECIAL_WK_CASTLING:
        squares[e1] = ' ';
        squares[f1] = 'R';
        squares[g1] = 'K';
        squares[h1] = ' ';
        wking_square = g1;
        break;
        case SPECIAL_WQ_CASTLING:
        squares[e1] = ' ';
        squares[d1] = 'R';
        squares[c1] = 'K';
        squares[a1] = ' ';
        wking_square = c1;
        break;
        case SPECIAL_BK_CASTLING:
        squares[e8] = ' ';
        squares[f8] = 'r';
        squares[g8] = 'k';
        squares[h8] = ' ';
        bking_square = g8;
        break;
        case SPECIAL_BQ_CASTLING:
        squares[e8] = ' ';
        squares[d8] = 'r';
        squares[c8] = 'k';
        squares[a8] = ' ';
        bking_square = c8;
        break;
    }

    // Toggle who-to-move
    Toggle();
}

/****************************************************************************
 * Undo a move
 ****************************************************************************/
void ChessRules::PopMove( Move& m )
{
    // Previous detail field
    DETAIL_POP;

    // Toggle who-to-move
    Toggle();

    // Special handling might be required
    switch( m.special )
    {
        default:
        squares[m.src] = squares[m.dst];
        squares[m.dst] = m.capture;
        break;

        // For promotion, src piece was a pawn
        case SPECIAL_PROMOTION_QUEEN:
        case SPECIAL_PROMOTION_ROOK:
        case SPECIAL_PROMOTION_BISHOP:
        case SPECIAL_PROMOTION_KNIGHT:
        if( white )
            squares[m.src] = 'P';
        else
            squares[m.src] = 'p';
        squares[m.dst] = m.capture;
        break;

        // White enpassant re-insert black pawn south of destination
        case SPECIAL_WEN_PASSANT:
        squares[m.src] = 'P';
        squares[m.dst] = ' ';
        squares[SOUTH(m.dst)] = 'p';
        break;

        // Black enpassant re-insert white pawn north of destination
        case SPECIAL_BEN_PASSANT:
        squares[m.src] = 'p';
        squares[m.dst] = ' ';
        squares[NORTH(m.dst)] = 'P';
        break;

        // Castling moves update 4 squares each
        case SPECIAL_WK_CASTLING:
        squares[e1] = 'K';
        squares[f1] = ' ';
        squares[g1] = ' ';
        squares[h1] = 'R';
        break;
        case SPECIAL_WQ_CASTLING:
        squares[e1] = 'K';
        squares[d1] = ' ';
        squares[c1] = ' ';
        squares[a1] = 'R';
        break;
        case SPECIAL_BK_CASTLING:
        squares[e8] = 'k';
        squares[f8] = ' ';
        squares[g8] = ' ';
        squares[h8] = 'r';
        break;
        case SPECIAL_BQ_CASTLING:
        squares[e8] = 'k';
        squares[d8] = ' ';
        squares[c8] = ' ';
        squares[a8] = 'r';
        break;
    }
}


/****************************************************************************
 * Determine if an occupied square is attacked
 ****************************************************************************/
bool ChessRules::AttackedPiece( Square square )
{
    bool enemy_is_white  =  IsBlack(squares[square]);
    return( AttackedSquare(square,enemy_is_white) );
}

/****************************************************************************
 * Is a square is attacked by enemy ?
 ****************************************************************************/
bool ChessRules::AttackedSquare( Square square, bool enemy_is_white )
{
    Square dst;
    const lte *ptr = (enemy_is_white ? attacks_black_lookup[square] : attacks_white_lookup[square] );
    lte nbr_rays = *ptr++;
    while( nbr_rays-- )
    {
        lte ray_len = *ptr++;
        while( ray_len-- )
        {
            dst = (Square)*ptr++;
            char piece=squares[dst];

            // If square not occupied (empty), continue
            if( IsEmptySquare(piece) )
                ptr++;  // skip mask

            // Else if occupied
            else
            {
                lte mask = *ptr++;

                // White attacker ?
                if( IsWhite(piece) && enemy_is_white )
                {
                    if( to_mask[piece] & mask )
                        return true;
                }

                // Black attacker ?
                else if( IsBlack(piece) && !enemy_is_white )
                {
                    if( to_mask[piece] & mask )
                        return true;
                }

                // Goto end of ray
                ptr += (2*ray_len);
                ray_len = 0;
            }
        }
    }

    ptr = knight_lookup[square];
    lte nbr_squares = *ptr++;
    while( nbr_squares-- )
    {
        dst = (Square)*ptr++;
        char piece=squares[dst];

        // If occupied by an enemy knight, we have found an attacker
        if( (enemy_is_white&&piece=='N') || (!enemy_is_white&&piece=='n') )
            return true;
    }
    return false;
}

/****************************************************************************
 * Evaluate a position, returns bool okay (not okay means illegal position)
 ****************************************************************************/
bool ChessRules::Evaluate()
{
    Square enemy_king = (Square)(white ? bking_square : wking_square);
    // Enemy king is attacked and our move, position is illegal
    return !AttackedPiece(enemy_king);
}

bool ChessRules::Evaluate( TERMINAL &score_terminal )
{
    return( Evaluate(NULL,score_terminal) );
}

bool ChessRules::Evaluate( MOVELIST *p, TERMINAL &score_terminal )
{
    /* static ;remove for thread safety */ MOVELIST local_list;
    MOVELIST &list = p?*p:local_list;
    int i, any;
    Square my_king, enemy_king;
    bool okay;
    score_terminal=NOT_TERMINAL;

    //DIAG_evaluate_count++;

    // Enemy king is attacked and our move, position is illegal
    enemy_king = (Square)(white ? bking_square : wking_square);
    if( AttackedPiece(enemy_king) )
        okay = false;

    // Else legal position
    else
    {
        okay = true;

        // Work out if the game is over by checking for any legal moves
        GenMoveList( &list );
        for( any=i=0 ; i<list.count && any==0 ; i++ )
        {
            PushMove( list.moves[i] );
            my_king = (Square)(white ? bking_square : wking_square);
            if( !AttackedPiece(my_king) )
                any++;
            PopMove( list.moves[i] );
        }

        // If no legal moves, position is either checkmate or stalemate
        if( any == 0 )
        {
            my_king = (Square)(white ? wking_square : bking_square);
            if( AttackedPiece(my_king) )
                score_terminal = (white ? TERMINAL_WCHECKMATE
                                        : TERMINAL_BCHECKMATE);
            else
                score_terminal = (white ? TERMINAL_WSTALEMATE
                                        : TERMINAL_BSTALEMATE);
        }
    }
    return(okay);
}


/****************************************************************************
 * ChessEvaluation.cpp Chess classes - Simple chess AI, leaf scoring function for position
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

//-- preferences
#define USE_CHASE_PAWNS
//#define USE_WEAKER_CENTRAL
#define USE_STRONG_KING
#define USE_IN_THE_SQUARE
#define USE_LIQUIDATION

// Do we check for mate at a leaf node, and if so how do we do it ?
#define CHECK_FOR_LEAF_MATE
// #define CHECK_FOR_LEAF_MATE_USE_EVALUATE // not the fastest way

// Lookup table for quick calculation of material value of any piece
static int either_colour_material[]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00-0x0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10-0x1f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x20-0x2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x30-0x3f
    0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,500,  0,  0, 30,  0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
   10, 90, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
    0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,500,  0,  0, 30,  0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
   10, 90, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
};

/****************************************************************************
 * Calculate material that side to play can win directly
 *  Fast white to move version
 ****************************************************************************/
int ChessEvaluation::EnpriseWhite()
{

    int best_so_far=0;  // amount of material that can be safely captured

    // Locals
    unsigned char defenders_buf[32];
    unsigned char *defenders;
    unsigned char attackers_buf[32];
    unsigned char *attackers;
    unsigned char *reorder_base, *base;
    unsigned char target;
    unsigned char attacker;
    Square square;
    Square attack_square;
    const lte *ptr;
    lte nbr_rays, nbr_squares;

    // For all squares
    for( square=a8; square<=h1; ++square )
    {

        // If piece on square is black, it's a potential target
        target = squares[square];
        if( IsBlack(target) )
        {
            attackers = attackers_buf;

            // It could be attacked by up to 2 white pawns
            ptr = pawn_attacks_black_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                attack_square = (Square)*ptr++;
                if( (attacker = squares[attack_square]) == 'P' )
                    *attackers++ = attacker;
            }

            // It could be attacked by up to 8 white knights
            ptr = knight_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                attack_square = (Square)*ptr++;
                if( (attacker = squares[attack_square]) == 'N' )
                    *attackers++ = attacker;
            }

            // From here on we may need to reorder the attackers
            reorder_base = base = attackers;

            // Move along each queen ray from the square being evaluated to
            //  each possible attacking square (looking for white attackers
            //  of a black piece)
            ptr = attacks_black_lookup[square];
            nbr_rays = *ptr++;
            while( nbr_rays-- )
            {
                nbr_squares = *ptr++;
                while( nbr_squares-- )
                {
                    attack_square = (Square)*ptr++;
                    lte mask      = *ptr++;
                    attacker = squares[attack_square];

                    // If the square is occupied by an attacking (white) piece, and
                    //  it matches a piece that attacks down that ray we have found
                    //  an attacker
                    if( IsWhite(attacker) && (to_mask[attacker]&mask) )
                    {
                        if( attacker != 'P' ) // we've already done pawn
                            *attackers++ = attacker;
                        if( attacker == 'K' ) // don't look beyond a king
                        {
                            ptr += (nbr_squares+nbr_squares);
                            nbr_squares = 0;
                        }
                    }

                    // Any other piece or a defender, must move to end of ray
                    else if( !IsEmptySquare(attacker) )
                    {
                        ptr += (nbr_squares+nbr_squares);
                        nbr_squares = 0;
                    }
                }

                // Do a limited amount of reordering at end of rays
                //  This will not optimally reorder an absurdly large number of
                //  attackers I'm afraid
                // If this ray generated attackers, and there were attackers
                //  before this ray
                if( attackers>base && base>reorder_base)
                {
                    bool swap=false;
                    if( *reorder_base==('K') )
                        swap = true;
                    else if( *reorder_base==('Q') && *base!=('K') )
                        swap = true;
                    else if( *reorder_base==('R') && *base==('B') )
                        swap = true;
                    if( swap )
                    {
                        unsigned char temp[32];
                        unsigned char *src, *dst;

                        // AAAABBBBBBBB
                        // ^   ^       ^
                        // |   |       |
                        // |   base    attackers
                        // reorder_base
                        // stage 1, AAAA -> temp
                        src = reorder_base;
                        dst = temp;
                        while( src != base )
                            *dst++ = *src++;

                        // stage 2, BBBBBBBB -> reorder_base
                        src = base;
                        dst = reorder_base;
                        while( src != attackers )
                            *dst++ = *src++;

                        // stage 3, replace AAAA after BBBBBBBB
                        src = temp;
                        while( dst != attackers )
                            *dst++ = *src++;
                    }
                }
                base = attackers;
            }

            // Any attackers ?
            if( attackers == attackers_buf )
                continue;  // No - continue to next square

            // Locals
            unsigned char defender;
            Square defend_square;

            // The target itself counts as a defender
            defenders = defenders_buf;
            *defenders++ = target;

            // It could be defended by up to 2 black pawns
            ptr = pawn_attacks_white_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                defend_square = (Square)*ptr++;
                if( (defender = squares[defend_square]) == 'p' )
                    *defenders++ = defender;
            }

            // It could be defended by up to 8 black knights
            ptr = knight_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                defend_square = (Square)*ptr++;
                if( (defender = squares[defend_square]) == 'n' )
                    *defenders++ = defender;
            }

            // From here on we may need to reorder the defenders
            reorder_base = base = defenders;

            // Move along each queen ray from the square being evaluated to
            //  each possible defending square (looking for black defenders
            //  of a black piece)
            ptr = attacks_white_lookup[square];
            nbr_rays = *ptr++;
            while( nbr_rays-- )
            {
                nbr_squares = *ptr++;
                while( nbr_squares-- )
                {
                    defend_square = (Square)*ptr++;
                    lte mask      = *ptr++;
                    defender = squares[defend_square];

                    // If the square is occupied by an defending (black) piece, and
                    //  it matches a piece that defends down that ray we have found
                    //  a defender
                    if( IsBlack(defender) && (to_mask[defender]&mask) )
                    {
                        if( defender != 'p' ) // we've already done pawn
                            *defenders++ = defender;
                        if( defender == 'k' ) // don't look beyond a king
                        {
                            ptr += (nbr_squares+nbr_squares);
                            nbr_squares = 0;
                        }
                    }

                    // Any other piece or an attacker, must move to end of ray
                    else if( !IsEmptySquare(defender) )
                    {
                        ptr += (nbr_squares+nbr_squares);
                        nbr_squares = 0;
                    }
                }

                // Do a limited amount of reordering at end of rays
                //  This will not optimally reorder an absurdly large number of
                //  defenders I'm afraid

                // If this ray generated defenders, and there were defenders
                //  before this ray
                if( defenders>base && base>reorder_base)
                {
                    bool swap=false;
                    if( *reorder_base=='k' )
                        swap = true;
                    else if( *reorder_base=='q' && *base!='k' )
                        swap = true;
                    else if( *reorder_base=='r' && *base=='b' )
                        swap = true;
                    if( swap )
                    {
                        unsigned char temp[32];
                        unsigned char *src, *dst;

                        // AAAABBBBBBBB
                        // ^   ^       ^
                        // |   |       |
                        // |   base    defenders
                        // reorder_base
                        // stage 1, AAAA -> temp
                        src = reorder_base;
                        dst = temp;
                        while( src != base )
                            *dst++ = *src++;

                        // stage 2, BBBBBBBB -> reorder_base
                        src = base;
                        dst = reorder_base;
                        while( src != defenders )
                            *dst++ = *src++;

                        // stage 3, replace AAAA after BBBBBBBB
                        src = temp;
                        while( dst != defenders )
                            *dst++ = *src++;
                    }
                }
                base = defenders;
            }

            // Figure out the net effect of the capturing sequence
            const unsigned char *a=attackers_buf;
            const unsigned char *d=defenders_buf;
            #define POS_INFINITY  1000000000
            #define NEG_INFINITY -1000000000
            int min = POS_INFINITY;
            int max = NEG_INFINITY;
            int net=0;  // net gain

            // While there are attackers and defenders
            while( a<attackers && d<defenders )
            {

                // Attacker captures
                net += either_colour_material[*d++];

                // Defender can elect to stop here
                if( net < min )
                    min = net;

                // Can defender recapture ?
                if( d == defenders )
                {
                    if( net > max )
                        max = net;
                    break;  // no
                }

                // Defender recaptures
                net -= either_colour_material[*a++];

                // Attacker can elect to stop here
                if( net > max )
                    max = net;
            }

            // Result is the lowest of best attacker can do and
            //  best defender can do
            int score = min<=max ? min : max;
            if( score > best_so_far )
                best_so_far = score;
        }   // end if black target
    } // end square loop

    // After looking at every square, return the best result
    return best_so_far;
}


/****************************************************************************
 * Calculate material that side to play can win directly
 *  Fast black to move version
 ****************************************************************************/
int ChessEvaluation::EnpriseBlack()
{
    int best_so_far=0;  // amount of material that can be safely captured

    // Locals
    unsigned char defenders_buf[32];
    unsigned char *defenders;
    unsigned char attackers_buf[32];
    unsigned char *attackers;
    unsigned char *reorder_base, *base;
    unsigned char target;
    unsigned char attacker;
    Square square;
    Square attack_square;
    const lte *ptr;
    lte nbr_rays, nbr_squares;

    // For all squares
    for( square=a8; square<=h1; ++square )
    {

        // If piece on square is white, it's a potential target
        target = squares[square];
        if( IsWhite(target) )
        {
            attackers = attackers_buf;

            // It could be attacked by up to 2 black pawns
            ptr = pawn_attacks_white_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                attack_square = (Square)*ptr++;
                if( (attacker = squares[attack_square]) == 'p' )
                    *attackers++ = attacker;
            }

            // It could be attacked by up to 8 black knights
            ptr = knight_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                attack_square = (Square)*ptr++;
                if( (attacker = squares[attack_square]) == 'n' )
                    *attackers++ = attacker;
            }

            // From here on we may need to reorder the attackers
            reorder_base = base = attackers;

            // Move along each queen ray from the square being evaluated to
            //  each possible attacking square (looking for black attackers
            //  of a white piece)
            ptr = attacks_white_lookup[square];
            nbr_rays = *ptr++;
            while( nbr_rays-- )
            {
                nbr_squares = *ptr++;
                while( nbr_squares-- )
                {
                    attack_square = (Square)*ptr++;
                    lte mask      = *ptr++;
                    attacker = squares[attack_square];

                    // If the square is occupied by an attacking (black) piece, and
                    //  it matches a piece that attacks down that ray we have found
                    //  an attacker
                    if( IsBlack(attacker) && (to_mask[attacker]&mask) )
                    {
                        if( attacker != 'p' ) // we've already done pawn
                            *attackers++ = attacker;
                        if( attacker == 'k' ) // don't look beyond a king
                        {
                            ptr += (nbr_squares+nbr_squares);
                            nbr_squares = 0;
                        }
                    }

                    // Any other piece or a defender, must move to end of ray
                    else if( !IsEmptySquare(attacker) )
                    {
                        ptr += (nbr_squares+nbr_squares);
                        nbr_squares = 0;
                    }
                }

                // Do a limited amount of reordering at end of rays
                //  This will not optimally reorder an absurdly large number of
                //  attackers I'm afraid
                // If this ray generated attackers, and there were attackers
                //  before this ray
                if( attackers>base && base>reorder_base)
                {
                    bool swap=false;
                    if( *reorder_base==('k') )
                        swap = true;
                    else if( *reorder_base==('q') && *base!=('k') )
                        swap = true;
                    else if( *reorder_base==('r') && *base==('b') )
                        swap = true;
                    if( swap )
                    {
                        unsigned char temp[32];
                        unsigned char *src, *dst;

                        // AAAABBBBBBBB
                        // ^   ^       ^
                        // |   |       |
                        // |   base    attackers
                        // reorder_base
                        // stage 1, AAAA -> temp
                        src = reorder_base;
                        dst = temp;
                        while( src != base )
                            *dst++ = *src++;

                        // stage 2, BBBBBBBB -> reorder_base
                        src = base;
                        dst = reorder_base;
                        while( src != attackers )
                            *dst++ = *src++;

                        // stage 3, replace AAAA after BBBBBBBB
                        src = temp;
                        while( dst != attackers )
                            *dst++ = *src++;
                    }
                }
                base = attackers;
            }

            // Any attackers ?
            if( attackers == attackers_buf )
                continue;  // No - continue to next square

            // Locals
            unsigned char defender;
            Square defend_square;

            // The target itself counts as a defender
            defenders = defenders_buf;
            *defenders++ = target;

            // It could be defended by up to 2 white pawns
            ptr = pawn_attacks_black_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                defend_square = (Square)*ptr++;
                if( (defender = squares[defend_square]) == 'P' )
                    *defenders++ = defender;
            }

            // It could be defended by up to 8 white knights
            ptr = knight_lookup[square];
            nbr_squares = *ptr++;
            while( nbr_squares-- )
            {
                defend_square = (Square)*ptr++;
                if( (defender = squares[defend_square]) == 'N' )
                    *defenders++ = defender;
            }

            // From here on we may need to reorder the defenders
            reorder_base = base = defenders;

            // Move along each queen ray from the square being evaluated to
            //  each possible defending square (looking for white defenders
            //  of a white piece)
            ptr = attacks_black_lookup[square];
            nbr_rays = *ptr++;
            while( nbr_rays-- )
            {
                nbr_squares = *ptr++;
                while( nbr_squares-- )
                {
                    defend_square = (Square)*ptr++;
                    lte mask      = *ptr++;
                    defender = squares[defend_square];

                    // If the square is occupied by an defending (white) piece, and
                    //  it matches a piece that defends down that ray we have found
                    //  a defender
                    if( IsWhite(defender) && (to_mask[defender]&mask) )
                    {
                        if( defender != 'P' ) // we've already done pawn
                            *defenders++ = defender;
                        if( defender == 'K' ) // don't look beyond a king
                        {
                            ptr += (nbr_squares+nbr_squares);
                            nbr_squares = 0;
                        }
                    }

                    // Any other piece or an attacker, must move to end of ray
                    else if( !IsEmptySquare(defender) )
                    {
                        ptr += (nbr_squares+nbr_squares);
                        nbr_squares = 0;
                    }
                }

                // Do a limited amount of reordering at end of rays
                //  This will not optimally reorder an absurdly large number of
                //  defenders I'm afraid

                // If this ray generated defenders, and there were defenders
                //  before this ray
                if( defenders>base && base>reorder_base)
                {
                    bool swap=false;
                    if( *reorder_base==('K') )
                        swap = true;
                    else if( *reorder_base==('Q') && *base!=('K') )
                        swap = true;
                    else if( *reorder_base==('R') && *base==('B') )
                        swap = true;
                    if( swap )
                    {
                        unsigned char temp[32];
                        unsigned char *src, *dst;

                        // AAAABBBBBBBB
                        // ^   ^       ^
                        // |   |       |
                        // |   base    defenders
                        // reorder_base
                        // stage 1, AAAA -> temp
                        src = reorder_base;
                        dst = temp;
                        while( src != base )
                            *dst++ = *src++;

                        // stage 2, BBBBBBBB -> reorder_base
                        src = base;
                        dst = reorder_base;
                        while( src != defenders )
                            *dst++ = *src++;

                        // stage 3, replace AAAA after BBBBBBBB
                        src = temp;
                        while( dst != defenders )
                            *dst++ = *src++;
                    }
                }
                base = defenders;
            }

            // Figure out the net effect of the capturing sequence
            const unsigned char *a=attackers_buf;
            const unsigned char *d=defenders_buf;
            int min = POS_INFINITY;
            int max = NEG_INFINITY;
            int net=0;  // net gain

            // While there are attackers and defenders
            while( a<attackers && d<defenders )
            {

                // Attacker captures
                net += either_colour_material[*d++];

                // Defender can elect to stop here
                if( net < min )
                    min = net;

                // Can defender recapture ?
                if( d == defenders )
                {
                    if( net > max )
                        max = net;
                    break;  // no
                }

                // Defender recaptures
                net -= either_colour_material[*a++];

                // Attacker can elect to stop here
                if( net > max )
                    max = net;
            }

            // Result is the lowest of best attacker can do and
            //  best defender can do
            int score = min<=max ? min : max;
            if( score > best_so_far )
                best_so_far = score;
        }   // end if white target
    } // end square loop

    // After looking at every square, return the best result
    return best_so_far;
}



//=========== EVALUATION ===============================================

static int king_ending_bonus_static[] =
{
    #if 1
    /*  0x00-0x07 a8-h8 */ -25,-25,-25,-25,-25,-25,-25,-25,
    /*  0x00-0x0f a7-h7 */ -25,  0,  0,  0,  0,  0,  0,-25,
    /*  0x10-0x17 a6-h6 */ -25,  0, 25, 25, 25, 25,  0,-25,
    /*  0x10-0x1f a5-h5 */ -25,  0, 25, 50, 50, 25,  0,-25,
    /*  0x20-0x27 a4-h4 */ -25,  0, 25, 50, 50, 25,  0,-25,
    /*  0x20-0x2f a3-h3 */ -25,  0, 25, 25, 25, 25,  0,-25,
    /*  0x30-0x37 a2-h2 */ -25,  0,  0,  0,  0,  0,  0,-25,
    /*  0x30-0x3f a1-h1 */ -25,-25,-25,-25,-25,-25,-25,-25
    #else
    /*  0x00-0x07 a8-h8 */ -10,-10,-10,-10,-10,-10,-10,-10,
    /*  0x00-0x0f a7-h7 */ -10,  0,  0,  0,  0,  0,  0,-10,
    /*  0x10-0x17 a6-h6 */ -10,  0, 10, 10, 10, 10,  0,-10,
    /*  0x10-0x1f a5-h5 */ -10,  0, 10, 20, 20, 10,  0,-10,
    /*  0x20-0x27 a4-h4 */ -10,  0, 10, 20, 20, 10,  0,-10,
    /*  0x20-0x2f a3-h3 */ -10,  0, 10, 10, 10, 10,  0,-10,
    /*  0x30-0x37 a2-h2 */ -10,  0,  0,  0,  0,  0,  0,-10,
    /*  0x30-0x3f a1-h1 */ -10,-10,-10,-10,-10,-10,-10,-10
    #endif
};

static int king_ending_bonus_dynamic_white[0x80];
static int king_ending_bonus_dynamic_black[0x80];

// Lookup table for quick calculation of material value of white piece
static int white_material[]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00-0x0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10-0x1f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x20-0x2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x30-0x3f
    0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,500,  0,  0, 30,  0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
   10, 90, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
};

// Lookup table for quick calculation of material value of black piece
static int black_material[]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00-0x0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10-0x1f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x20-0x2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x30-0x3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
    0,  0,-31,  0,  0,  0,  0,  0,  0,  0,  0,-500, 0,  0,-30,  0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
  -10,-90,-50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
};

// Lookup table for quick calculation of material value of white piece (not pawn or king)
static int white_pieces[]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00-0x0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10-0x1f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x20-0x2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x30-0x3f
    0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30,  0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
    0, 90, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
};

// Lookup table for quick calculation of material value of black piece (not pawn or king)
static int black_pieces[]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00-0x0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10-0x1f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x20-0x2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x30-0x3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
    0,  0, 31,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30,  0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
    0, 90, 50,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0    // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
};


/****************************************************************************
 * Evaluate a position, leaf node
 *
 *   (this makes a rather ineffectual effort to score positional features
 *    needs a lot of improvement)
 ****************************************************************************/
void ChessEvaluation::EvaluateLeaf( int &material, int &positional )
{
    //DIAG_evaluate_leaf_count++;
    char   piece;
    int file;
    int bonus = 0;
    int score_black_material = 0;
    int score_white_material = 0;
    int black_connected=0;
    int white_connected=0;

    int white_king_safety_bonus          =0;
    int white_king_central_bonus         =0;
    int white_queen_central_bonus        =0;
    int white_queen_developed_bonus      =0;
    int white_queen78_bonus              =0;
    int white_undeveloped_minor_bonus    =0;

    int black_king_safety_bonus          =0;
    int black_king_central_bonus         =0;
    int black_queen_central_bonus        =0;
    int black_queen_developed_bonus      =0;
    int black_queen78_bonus              =0;
    int black_undeveloped_minor_bonus    =0;


#define BONUS_WHITE_SWAP_PIECE          60
#define BONUS_BLACK_SWAP_PIECE          -60
#define BONUS_BLACK_CONNECTED_ROOKS     -10
#define BONUS_BLACK_BLOCKED_BISHOP      10
#define BLACK_UNDEVELOPED_MINOR_BONUS   3
#define BONUS_BLACK_KNIGHT_CENTRAL0     -8
#define BONUS_BLACK_KNIGHT_CENTRAL1     -9
#define BONUS_BLACK_KNIGHT_CENTRAL2     -10
#define BONUS_BLACK_KNIGHT_CENTRAL3     -12
#define BONUS_BLACK_KING_SAFETY         -10
#define BONUS_BLACK_KING_CENTRAL0       -8
#define BONUS_BLACK_KING_CENTRAL1       -9
#define BONUS_BLACK_KING_CENTRAL2       -10
#define BONUS_BLACK_KING_CENTRAL3       -12
#define BONUS_BLACK_QUEEN_CENTRAL       -10
#define BONUS_BLACK_QUEEN_DEVELOPED     -10
#define BONUS_BLACK_QUEEN78             -5
#define BONUS_BLACK_ROOK7               -5
#define BONUS_BLACK_PAWN5               -20     // boosted because now must be passed
#define BONUS_BLACK_PAWN6               -30     // boosted because now must be passed
#define BONUS_BLACK_PAWN7               -40     // boosted because now must be passed
#define BONUS_BLACK_PAWN_CENTRAL        -5

#define BONUS_WHITE_CONNECTED_ROOKS      10
#define BONUS_WHITE_BLOCKED_BISHOP       -10
#define WHITE_UNDEVELOPED_MINOR_BONUS    -3
#define BONUS_WHITE_KNIGHT_CENTRAL0      8
#define BONUS_WHITE_KNIGHT_CENTRAL1      9
#define BONUS_WHITE_KNIGHT_CENTRAL2      10
#define BONUS_WHITE_KNIGHT_CENTRAL3      12
#define BONUS_WHITE_KING_SAFETY          10
#define BONUS_WHITE_KING_CENTRAL0        8
#define BONUS_WHITE_KING_CENTRAL1        9
#define BONUS_WHITE_KING_CENTRAL2        10
#define BONUS_WHITE_KING_CENTRAL3        12
#define BONUS_WHITE_QUEEN_CENTRAL        10
#define BONUS_WHITE_QUEEN_DEVELOPED      10
#define BONUS_WHITE_QUEEN78              5
#define BONUS_WHITE_ROOK7                5
#define BONUS_WHITE_PAWN5                20     // boosted because now must be passed
#define BONUS_WHITE_PAWN6                30     // boosted because now must be passed
#define BONUS_WHITE_PAWN7                40     // boosted because now must be passed
#define BONUS_WHITE_PAWN_CENTRAL         5
#define BONUS_STRONG_KING                50

const int MATERIAL_OPENING = (500 + ((8*10+4*30+2*50+90)*2)/3);
const int MATERIAL_MIDDLE  = (500 + ((8*10+4*30+2*50+90)*1)/3);


    Square black_king_square = SQUARE_INVALID;
    Square white_king_square = SQUARE_INVALID;
    Square black_pawns_buf[16];
    Square white_pawns_buf[16];
    Square black_passers_buf[16];
    Square white_passers_buf[16];
    Square *black_passers =  black_passers_buf;
    Square *white_passers =  white_passers_buf;
    Square *black_pawns   =  black_pawns_buf;
    Square *white_pawns   =  white_pawns_buf;
    int score_black_pieces = 0;
    int score_white_pieces = 0;

    // a8->h8
    for( Square square=a8; square<=h8; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                break;
            }

            case 'r':
            {
                black_connected++;
                if( black_connected == 2 )
                    bonus += BONUS_BLACK_CONNECTED_ROOKS;
                break;
            }

            case 'n':
            {
                black_connected=2;
                black_undeveloped_minor_bonus++;
                break;
            }

            case 'b':
            {
                black_connected=2;
                black_undeveloped_minor_bonus++;
                if( a8==square && IsBlack(squares[b7]) )
                    bonus += BONUS_BLACK_BLOCKED_BISHOP;
                else if( h8==square && IsBlack(squares[g7]) )
                    bonus += BONUS_BLACK_BLOCKED_BISHOP;
                else
                {
                    if( IsBlack(squares[SE(square)]) &&
                        IsBlack(squares[SW(square)])
                      )
                    {
                        bonus += BONUS_BLACK_BLOCKED_BISHOP;
                    }
                }
                break;
            }

            case 'q':
            {
                black_connected=2;
                break;
            }

            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                black_connected=2;
                file = IFILE(square);
                if( file<2 || file>5 )
                    black_king_safety_bonus = BONUS_BLACK_KING_SAFETY;
                break;
            }

            case 'Q':
            {
                white_queen78_bonus = BONUS_WHITE_QUEEN78;
                break;
            }
        }
    }

    // a7->h7
    unsigned int next_passer_mask = 0;
    unsigned int passer_mask = 0;
    unsigned int three_files = 0x1c0;   // 1 1100 0000
    for( Square square=a7; square<=h7; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                break;
            }

            case 'b':
            {
                if( a7==square && IsBlack(squares[b6]) )
                    bonus += BONUS_BLACK_BLOCKED_BISHOP;
                else if( h7==square && IsBlack(squares[g6]) )
                    bonus += BONUS_BLACK_BLOCKED_BISHOP;
                else
                {
                    if( IsBlack(squares[SE(square)]) &&
                        IsBlack(squares[SW(square)])
                      )
                    {
                        bonus += BONUS_BLACK_BLOCKED_BISHOP;
                    }
                }
                break;
            }

            case 'q':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    black_queen_developed_bonus = BONUS_BLACK_QUEEN_DEVELOPED;
                break;
            }

            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                file = IFILE(square);
                if( file<2 || file>5 )
                    black_king_safety_bonus = BONUS_BLACK_KING_SAFETY;
                break;
            }

            case 'R':
            {
                bonus += BONUS_WHITE_ROOK7;
                break;
            }

            case 'Q':
            {
                white_queen78_bonus = BONUS_WHITE_QUEEN78;
                break;
            }

            case 'P':
            {
                *white_pawns++   = square;
                *white_passers++ = square;
                bonus += BONUS_WHITE_PAWN7;
                #ifdef USE_STRONG_KING
                Square ahead = NORTH(square);
                if( squares[ahead]=='K' && king_ending_bonus_dynamic_white[ahead]==0 )
                    bonus += BONUS_STRONG_KING;
                #endif
                break;
            }

            case 'p':
            {
                *black_pawns++ = square;
                passer_mask |= three_files;
                break;
            }
        }
        three_files >>= 1;
    }

    // a6->h6
    unsigned int file_mask = 0x80;  // 0 1000 0000
    three_files = 0x1c0;            // 1 1100 0000
    for( Square square=a6; square<=h6; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    black_king_central_bonus = BONUS_BLACK_KING_CENTRAL0;
                break;
            }

            case 'n':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_BLACK_KNIGHT_CENTRAL0;
                break;
            }

            case 'q':
            {
                black_queen_central_bonus = BONUS_BLACK_QUEEN_CENTRAL;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    white_king_central_bonus = BONUS_WHITE_KING_CENTRAL3;
                break;
            }

            case 'N':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_WHITE_KNIGHT_CENTRAL3;
                break;
            }

            case 'Q':
            {
                white_queen_central_bonus = BONUS_WHITE_QUEEN_CENTRAL;
                break;
            }

            case 'P':
            {
                *white_pawns++ = square;
                if( !(passer_mask&file_mask) )
                {
                    *white_passers++ = square;
                    bonus += BONUS_WHITE_PAWN6;
                    #ifdef USE_STRONG_KING
                    Square ahead = NORTH(square);
                    if( squares[ahead]=='K' && king_ending_bonus_dynamic_white[ahead]==0 )
                        bonus += BONUS_STRONG_KING;
                    #endif
                }
                break;
            }

            case 'p':
            {
                *black_pawns++ = square;
                next_passer_mask |= three_files;
                break;
            }
        }
        file_mask   >>= 1;
        three_files >>= 1;
    }
    passer_mask |= next_passer_mask;

    // a5->h5;
    file_mask   = 0x80;             // 0 1000 0000
//  three_files = 0x1c0;            // 1 1100 0000
    for( Square square=a5; square<=h5; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    black_king_central_bonus = BONUS_BLACK_KING_CENTRAL1;
                break;
            }

            case 'n':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_BLACK_KNIGHT_CENTRAL1;
                break;
            }

            case 'q':
            {
                black_queen_central_bonus = BONUS_BLACK_QUEEN_CENTRAL;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    white_king_central_bonus = BONUS_WHITE_KING_CENTRAL2;
                break;
            }

            case 'N':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_WHITE_KNIGHT_CENTRAL2;
                break;
            }

            case 'Q':
            {
                white_queen_central_bonus = BONUS_WHITE_QUEEN_CENTRAL;
                break;
            }

            case 'P':
            {
                *white_pawns++ = square;
                file = IFILE(square);
                if( file==3 || file==4 )
                    bonus += BONUS_WHITE_PAWN_CENTRAL;
                if( !(passer_mask&file_mask) )
                {
                    *white_passers++ = square;
                    bonus += BONUS_WHITE_PAWN5;
                    #ifdef USE_STRONG_KING
                    Square ahead = NORTH(square);
                    if( squares[ahead]=='K' && king_ending_bonus_dynamic_white[ahead]==0 )
                        bonus += BONUS_STRONG_KING;
                    #endif
                }
                break;
            }

            case 'p':
            {
                *black_pawns++ = square;
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_BLACK_PAWN_CENTRAL;
                break;
            }
        }
        file_mask   >>= 1;
        // three_files >>= 1;
    }

    // a2->h2
    next_passer_mask = 0;
    passer_mask = 0;
    three_files = 0x1c0;   // 1 1100 0000
    for( Square square=a2; square<=h2; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                break;
            }

            case 'B':
            {
                if( a2==square && IsWhite(squares[b3]) )
                    bonus += BONUS_WHITE_BLOCKED_BISHOP;
                else if( h2==square && IsWhite(squares[g3]) )
                    bonus += BONUS_WHITE_BLOCKED_BISHOP;
                else
                {
                    if( IsWhite(squares[NW(square)]) &&
                        IsWhite(squares[NE(square)])
                      )
                    {
                        bonus += BONUS_WHITE_BLOCKED_BISHOP;
                    }
                }
                break;
            }

            case 'Q':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    white_queen_developed_bonus = BONUS_WHITE_QUEEN_DEVELOPED;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                file = IFILE(square);
                if( file<2 || file>5 )
                    white_king_safety_bonus = BONUS_WHITE_KING_SAFETY;
                break;
            }

            case 'r':
            {
                bonus += BONUS_BLACK_ROOK7;
                break;
            }

            case 'q':
            {
                black_queen78_bonus = BONUS_BLACK_QUEEN78;
                break;
            }

            case 'p':
            {
                *black_pawns++   = square;
                *black_passers++ = square;
                bonus += BONUS_BLACK_PAWN7;
                #ifdef USE_STRONG_KING
                Square ahead = SOUTH(square);
                if( squares[ahead]=='k' && king_ending_bonus_dynamic_black[ahead]==0 )
                    bonus -= BONUS_STRONG_KING;
                #endif
                break;
            }

            case 'P':
            {
                *white_pawns++   = square;
                passer_mask |= three_files;
                break;
            }
        }
        three_files >>= 1;
    }

    // a3->h3
    file_mask = 0x80;       // 0 1000 0000
    three_files = 0x1c0;    // 1 1100 0000
    for( Square square=a3; square<=h3; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    black_king_central_bonus = BONUS_BLACK_KING_CENTRAL3;
                break;
            }

            case 'n':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_BLACK_KNIGHT_CENTRAL3;
                break;
            }

            case 'q':
            {
                black_queen_central_bonus = BONUS_BLACK_QUEEN_CENTRAL;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    white_king_central_bonus = BONUS_WHITE_KING_CENTRAL0;
                break;
            }

            case 'N':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_WHITE_KNIGHT_CENTRAL0;
                break;
            }

            case 'Q':
            {
                white_queen_central_bonus = BONUS_WHITE_QUEEN_CENTRAL;
                break;
            }

            case 'p':
            {
                *black_pawns++ = square;
                if( !(passer_mask&file_mask) )
                {
                    *black_passers++ = square;
                    bonus += BONUS_BLACK_PAWN6;
                    #ifdef USE_STRONG_KING
                    Square ahead = SOUTH(square);
                    if( squares[ahead]=='k' && king_ending_bonus_dynamic_black[ahead]==0 )
                        bonus -= BONUS_STRONG_KING;
                    #endif
                }
                break;
            }

            case 'P':
            {
                *white_pawns++   = square;
                next_passer_mask |= three_files;
                break;
            }
        }
        file_mask   >>= 1;
        three_files >>= 1;
    }
    passer_mask |= next_passer_mask;

    // a4->h4
    file_mask   = 0x80;             // 0 1000 0000
//  three_files = 0x1c0;            // 1 1100 0000
    for( Square square=a4; square<=h4; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    black_king_central_bonus = BONUS_BLACK_KING_CENTRAL2;
                break;
            }

            case 'n':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_BLACK_KNIGHT_CENTRAL2;
                break;
            }

            case 'q':
            {
                black_queen_central_bonus = BONUS_BLACK_QUEEN_CENTRAL;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    white_king_central_bonus = BONUS_WHITE_KING_CENTRAL1;
                break;
            }

            case 'N':
            {
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_WHITE_KNIGHT_CENTRAL1;
                break;
            }

            case 'Q':
            {
                white_queen_central_bonus = BONUS_WHITE_QUEEN_CENTRAL;
                break;
            }

            case 'p':
            {
                *black_pawns++ = square;
                file = IFILE(square);
                if( file==3 || file==4 )
                    bonus += BONUS_BLACK_PAWN_CENTRAL;
                if( !(passer_mask&file_mask) )
                {
                    *black_passers++ = square;
                    bonus += BONUS_BLACK_PAWN5;
                    #ifdef USE_STRONG_KING
                    Square ahead = SOUTH(square);
                    if( squares[ahead]=='k' && king_ending_bonus_dynamic_black[ahead]==0 )
                        bonus -= BONUS_STRONG_KING;
                    #endif
                }
                break;
            }

            case 'P':
            {
                *white_pawns++   = square;
                file = IFILE(square);
                if( 2<=file && file<=5 )
                    bonus += BONUS_WHITE_PAWN_CENTRAL;
                break;
            }
        }
        file_mask   >>= 1;
        // three_files >>= 1;
    }

    // a1->h1
    for( Square square=a1; square<=h1; ++square )
    {
        piece = squares[square];
        score_black_material += black_material[ piece ];
        score_white_material += white_material[ piece ];
        score_black_pieces   += black_pieces[ piece ];
        score_white_pieces   += white_pieces[ piece ];
        switch( piece )
        {
            case 'k':
            {
                black_king_square = square;
                bonus -= king_ending_bonus_dynamic_black[square];
                break;
            }

            case 'R':
            {
                white_connected++;
                if( white_connected == 2 )
                    bonus += BONUS_WHITE_CONNECTED_ROOKS;
                break;
            }

            case 'N':
            {
                white_connected=2;
                white_undeveloped_minor_bonus++;
                break;
            }

            case 'B':
            {
                white_connected=2;
                white_undeveloped_minor_bonus++;
                if( a1==square && IsWhite(squares[b2]) )
                    bonus += BONUS_WHITE_BLOCKED_BISHOP;
                else if( h1==square && IsWhite(squares[g2]) )
                    bonus += BONUS_WHITE_BLOCKED_BISHOP;
                else
                {
                    if( IsWhite(squares[NW(square)]) &&
                        IsWhite(squares[NE(square)])
                      )
                    {
                        bonus += BONUS_WHITE_BLOCKED_BISHOP;
                    }
                }
                break;
            }

            case 'Q':
            {
                white_connected=2;
                break;
            }

            case 'K':
            {
                white_king_square = square;
                bonus += king_ending_bonus_dynamic_white[square];
                white_connected=2;
                file = IFILE(square);
                if( file<2 || file>5 )
                    white_king_safety_bonus = BONUS_WHITE_KING_SAFETY;
                break;
            }

            case 'q':
            {
                black_queen78_bonus = BONUS_BLACK_QUEEN78;
                break;
            }
        }
    }

    if( score_white_material > MATERIAL_OPENING )
    {
        bonus += white_king_safety_bonus;
        bonus += white_queen_developed_bonus;
        bonus += white_undeveloped_minor_bonus*WHITE_UNDEVELOPED_MINOR_BONUS;
    }
    else if( score_white_material > MATERIAL_MIDDLE )
    {
        bonus += white_king_safety_bonus;
        bonus += white_queen_central_bonus;
    }
    else
    {
        // bonus += white_king_central_bonus;
        bonus += white_queen78_bonus;
    }

    if( score_black_material < -MATERIAL_OPENING )
    {
        bonus += black_king_safety_bonus;
        bonus += black_queen_developed_bonus;
        bonus += black_undeveloped_minor_bonus*BLACK_UNDEVELOPED_MINOR_BONUS;
    }
    else if( score_black_material < -MATERIAL_MIDDLE )
    {
        bonus += black_king_safety_bonus;
        bonus += black_queen_central_bonus;
    }
    else
    {
        // bonus += black_king_central_bonus;
        bonus += black_queen78_bonus;
    }

    material   = score_white_material + score_black_material;
    if( white )
    {
#ifdef CHECK_FOR_LEAF_MATE
        bool mate=false;
        if( AttackedPiece((Square)wking_square) )
 #ifdef CHECK_FOR_LEAF_MATE_USE_EVALUATE
        {
            TERMINAL terminal_score;
            bool okay = Evaluate(terminal_score);
            if( okay && terminal_score==TERMINAL_WCHECKMATE )   // white mated ?
                mate = true;
        }
 #else
        {

            mate = true;
            MOVELIST list;
            GenMoveList( &list );
            for( int i=0; mate && i<list.count; i++ )
            {
                PushMove( list.moves[i] );
                if( !AttackedPiece((Square)wking_square) )
                    mate = false;
                PopMove( list.moves[i] );
            }
        }
 #endif
        if( mate )
            material = -500;    // white is mated
        else
            material += EnpriseWhite();
#else
        material += EnpriseWhite();
#endif
    }
    else
    {
#ifdef CHECK_FOR_LEAF_MATE
        bool mate=false;
        if( AttackedPiece((Square)bking_square) )
 #ifdef CHECK_FOR_LEAF_MATE_USE_EVALUATE
        {
            TERMINAL terminal_score;
            bool okay = Evaluate(terminal_score);
            if( okay && terminal_score==TERMINAL_BCHECKMATE )   // black mated ?
                mate = true;
        }
 #else
        {
            mate = true;
            MOVELIST list;
            GenMoveList( &list );
            for( int i=0; mate && i<list.count; i++ )
            {
                PushMove( list.moves[i] );
                if( !AttackedPiece((Square)bking_square) )
                    mate = false;
                PopMove( list.moves[i] );
            }
        }
 #endif
        if( mate )
            material = 500;    // black is mated
        else
            material -= EnpriseBlack();
#else
        material -= EnpriseBlack();
#endif
    }
    positional = bonus;

    // Reward stronger side with a bonus for swapping pieces not pawns
    if( material>0 && planning_white_piece_pawn_percent ) // if white ahead
                                                          //   and a figure to compare to
    {
        int score_white_pawns = score_white_material - 500 // -500 is king
                                - score_white_pieces;
        int piece_pawn_percent = 1000;
        if( score_white_pawns )
        {
            piece_pawn_percent = (100*score_white_pieces) /
                                      score_white_pawns;
            if( piece_pawn_percent > 1000 )
                piece_pawn_percent = 1000;
        }
        // start of game
        //  piece_pawn_percent = 100* Q+2R+2B+2N/8P = 100 * (190+120)/80
        //                     = 400 (approx)
        // typical endings
        //  piece_pawn_percent = 100* R+B/5P = 100 * (80)/50
        //                     = 160
        //  piece_pawn_percent = 100* R/5P = 100 * 50/50
        //                     = 100  after swapping a bishop
        //  piece_pawn_percent = 100* R+B/P = 100 * (80)/10
        //                     = 800
        //  piece_pawn_percent = 100* R/P = 100 * 50/10
        //                     = 500  after swapping a bishop
        //
        //  Lower numbers are better for the stronger side, calculate
        //  an adjustment as follows;
        //   up to +0.8 pawns for improved ratio for white as stronger side
        //   up to -0.8 pawns for worse ratio for white as stronger side
        int piece_pawn_ratio_adjustment = 8 - (8*piece_pawn_percent)/planning_white_piece_pawn_percent;
        if( piece_pawn_ratio_adjustment < -8 )
            piece_pawn_ratio_adjustment = -8;
        //   eg planning_white_piece_pawn_percent = 160
        //      now            piece_pawn_percent = 160
        //      adjustment = 0
        //   eg planning_white_piece_pawn_percent = 160
        //      now            piece_pawn_percent = 100
        //      adjustment = +3 (i.e. +0.3 pawns)
        //   eg planning_white_piece_pawn_percent = 800
        //      now            piece_pawn_percent = 500
        //      adjustment = +3 (i.e. +0.3 pawns)
        //   eg planning_white_piece_pawn_percent = 100
        //      now            piece_pawn_percent = 160
        //      adjustment = -4 (i.e. -0.4 pawns)
        //   eg planning_white_piece_pawn_percent = 500
        //      now            piece_pawn_percent = 800
        //      adjustment = -4 (i.e. -0.4 pawns)

        // If white is better, positive adjustment increases +ve material advantage
        material += piece_pawn_ratio_adjustment;
    }
    else if( material<0 && planning_black_piece_pawn_percent ) // if black ahead
                                                               //   and a figure to compare to
    {
        int score_black_pawns = (0-score_black_material) - 500 // -500 is king
                                - score_black_pieces;
        int piece_pawn_percent = 1000;
        if( score_black_pawns )
        {
            piece_pawn_percent = (100*score_black_pieces) /
                                      score_black_pawns;
            if( piece_pawn_percent > 1000 )
                piece_pawn_percent = 1000;
        }
        int piece_pawn_ratio_adjustment = 8 - (8*piece_pawn_percent)/planning_black_piece_pawn_percent;
        if( piece_pawn_ratio_adjustment < -8 )
            piece_pawn_ratio_adjustment = -8;

        // If black is better, positive adjustment increases -ve material advantage
        material -= piece_pawn_ratio_adjustment;
    }

    // Check whether white king is in square of black passers
    if( score_white_pieces==0 )
    {
        bool black_will_queen = false;
        #ifdef USE_IN_THE_SQUARE
        while( black_passers>black_passers_buf
                && white_king_square != SQUARE_INVALID
                && !black_will_queen )
        {
            --black_passers;
            Square square = *black_passers;
            int pfile = IFILE(square);
            int prank = IRANK(square);
            int kfile = IFILE(white_king_square);
            int krank = IRANK(white_king_square);

            // Calculations assume it is black to move; so if it is white
            //  shift pawn one more square away from queening square
            if( white )
                prank++;

            // Will queen if eg Pa3, Ka4
            if( prank < krank )
                black_will_queen = true;
            else
            {

                // Will queen if eg Pa3, Kd3
                if( kfile > pfile )
                    black_will_queen = (kfile-pfile > prank); // eg d-a=3 > 2

                // Will queen if eg Ph3, Ke3
                else if( kfile < pfile )
                    black_will_queen = (pfile-kfile > prank); // eg h-e=3 > 2
            }
        }

        // If white has a bare king, a pawn protecting a pawn always wins and a pair of pawns
        //  separated by 5 or 6 files always wins (even if white to move captures one)
        //  Note at the planning stage white had material - so are encouraging liquidation to
        //  easily winning pawn endings
        #ifdef USE_LIQUIDATION
        while( score_white_material==500 && planning_score_white_pieces && black_pawns>black_pawns_buf
                 && !black_will_queen )
        {
            int nbr_separating_files = (white?5:4);
            --black_pawns;
            Square square = *black_pawns;
            int pfile1 = IFILE(square);
            int prank1 = IRANK(square);
            Square *p = black_pawns;
            while( p > black_pawns_buf )
            {
                p--;
                Square square2 = *p;
                int pfile2 = IFILE(square2);
                int prank2 = IRANK(square2);
                if( (prank2==prank1+1 || prank2+1==prank1) &&
                    (pfile2==pfile1+1 || pfile2+1==pfile1)
                  )
                {
                    black_will_queen = true;  // pawn protects pawn
                }
                else if( pfile2>pfile1+nbr_separating_files || pfile1>pfile2+nbr_separating_files )
                {
                    black_will_queen = true;  // pawns separated by 5 or more empty files
                }
            }
        }
        #endif
        if( black_will_queen )
            material -= 65; // almost as good as a black queen (if it's too close we might not actually queen!)
    }

    // Check whether black king is in square of white passers
    if( score_black_pieces==0 )
    {
        bool white_will_queen = false;
        #ifdef USE_IN_THE_SQUARE
        while( white_passers>white_passers_buf
                && black_king_square != SQUARE_INVALID
                && !white_will_queen )
        {
            --white_passers;
            Square square = *white_passers;
            int pfile = IFILE(square);
            int prank = IRANK(square);
            int kfile = IFILE(black_king_square);
            int krank = IRANK(black_king_square);

            // Calculations assume it is white to move; so if it is black
            //  shift pawn one more square away from queening square
            if( !white )
                prank--;

            // Will queen if eg Pa6, Ka5
            if( prank > krank )
                white_will_queen = true;
            else
            {

                // Will queen if eg Pa6, Kd6
                if( kfile > pfile )
                    white_will_queen = (kfile-pfile > 7-prank); // eg d-a=3 > 7-5=2

                // Will queen if eg Ph3, Ke3
                else if( kfile < pfile )
                    white_will_queen = (pfile-kfile > 7-prank); // eg h-e=3 > 7-5=2
            }
        }
        #endif

        // If black has a bare king, a pawn protecting a pawn always wins and a pair of pawns
        //  separated by 5 or 6 files always wins (even if black to move captures one)
        //  Note at the planning stage black had material - so are encouraging liquidation to
        //  easily winning pawn endings
        #ifdef USE_LIQUIDATION
        while( score_black_material==-500 && planning_score_black_pieces && white_pawns>white_pawns_buf
                 && !white_will_queen )
        {
            int nbr_separating_files = (!white?5:4);
            --white_pawns;
            Square square = *white_pawns;
            int pfile1 = IFILE(square);
            int prank1 = IRANK(square);
            Square *p = white_pawns;
            while( p > white_pawns_buf )
            {
                p--;
                Square square2 = *p;
                int pfile2 = IFILE(square2);
                int prank2 = IRANK(square2);
                if( (prank2==prank1+1 || prank2+1==prank1) &&
                    (pfile2==pfile1+1 || pfile2+1==pfile1)
                  )
                {
                    white_will_queen = true;  // pawn protects pawn
                }
                else if( pfile2>pfile1+nbr_separating_files || pfile1>pfile2+nbr_separating_files )
                {
                    white_will_queen = true;  // pawns separated by 5 or more empty files
                }
            }
        }
        #endif
        if( white_will_queen )
            material += 65; // almost as good as a white queen (if it's too close we might not actually queen!)
    }
    #endif
//  int score_test = material*4 /*balance=4*/ + positional;
//  int score_test_cp = (score_test*10)/4;
//  if( score_test_cp > 4000 )
//      cprintf( "too much" );   // Problem fen "k7/8/PPK5/8/8/8/8/8 w - - 0 1"
}

/****************************************************************************
 * Create a list of all legal moves (sorted strongest first)
 ****************************************************************************/
// Sort moves according to their score
struct MOVE_IDX
{
    int score;
    int idx;
    bool operator <  (const MOVE_IDX& arg) const { return score <  arg.score; }
    bool operator >  (const MOVE_IDX& arg) const { return score >  arg.score; }
    bool operator == (const MOVE_IDX& arg) const { return score == arg.score; }
};

/****************************************************************************
 * Move.cpp Chess classes - Move
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

/****************************************************************************
 * Read natural string move eg "Nf3"
 *  return bool okay
 ****************************************************************************/
bool Move::NaturalIn( ChessRules *cr, const char *natural_in )
{
    MOVELIST list;
    int  i, len=0;
    char src_file='\0', src_rank='\0', dst_file='\0', dst_rank='\0';
    char promotion='\0';
    bool enpassant=false;
    bool kcastling=false;
    bool qcastling=false;
    Square dst_=a8;
    Move *m, *found=NULL;
    char *s;
    char  move[10];
    bool  white=cr->white;
    char  piece=(white?'P':'p');
    bool  default_piece=true;

    // Indicate no move found (yet)
    bool okay=true;

    // Copy to read-write variable
    okay = false;
    for( i=0; i<sizeof(move); i++ )
    {
        move[i] = natural_in[i];
        if( move[i]=='\0' || move[i]==' ' || move[i]=='\t' ||
            move[i]=='\r' || move[i]=='\n' )
        {
            move[i] = '\0';
            okay = true;
            break;
        }
    }
    if( okay )
    {

        // Trim string from end
        s = strchr(move,'\0') - 1;
        while( s>=move && !(isascii(*s) && isalnum(*s)) )
            *s-- = '\0';

        // Trim string from start
        s = move;
        while( *s==' ' || *s=='\t' )
            s++;
        len = (int)strlen(s);
        for( i=0; i<len+1; i++ )  // +1 gets '\0' at end
            move[i] = *s++;  // if no leading space this does
                            //  nothing, but no harm either

        // Trim enpassant
        if( len>=2 && move[len-1]=='p' )
        {
            if( 0 == strcmp(&move[len-2],"ep") )
            {
                move[len-2] = '\0';
                enpassant = true;
            }
            else if( len>=3 && 0==strcmp(&move[len-3],"e.p") )
            {
                move[len-3] = '\0';
                enpassant = true;
            }

            // Trim string from end, again
            s = strchr(move,'\0') - 1;
            while( s>=move && !(isascii(*s) && isalnum(*s)) )
                *s-- = '\0';
            len = (int)strlen(move);
        }

        // Promotion
        if( len>2 )  // We are supporting "ab" to mean Pawn a5xb6 (say), and this test makes sure we don't
        {            // mix that up with a lower case bishop promotion, and that we don't reject "ef" say
                     // on the basis that 'F' is not a promotion indication. We've never supported "abQ" say
                     // as a7xb8=Q, and we still don't so "abb" as a bishop promotion doesn't work, but we
                     // continue to support "ab=Q", and even "ab=b".
                     // The test also ensures we can access move[len-2] below
                     // These comments added when we changed the logic to support "b8Q" and "a7xb8Q", the
                     // '=' can optionally be omitted in such cases, the first change in this code for many,
                     // many years.
            char last = move[len-1];
            bool is_file = ('1'<=last && last<='8');
            if( !is_file )
            {
                switch( last )
                {
                    case 'O':
                    case 'o':   break;  // Allow castling!
                    case 'q':
                    case 'Q':   promotion='Q';  break;
                    case 'r':
                    case 'R':   promotion='R';  break;
                    case 'b':   if( len==3 && '2'<=move[1] && move[1]<='7' )
                                    break;  // else fall through to promotion - allows say "a5b" as disambiguating
                                            //  version of "ab" if there's more than one "ab" available! Something
                                            //  of an ultra refinement
                    case 'B':   promotion='B';  break;
                    case 'n':
                    case 'N':   promotion='N';  break;
                    default:    okay = false;   break;   // Castling and promotions are the only cases longer than 2
                                                         //  chars where a non-file ends a move. (Note we still accept
                                                         //  2 character pawn captures like "ef").
                }
                if( promotion )
                {
                    switch( move[len-2] )
                    {
                        case '=':
                        case '1':   // we now allow '=' to be omitted, as eg ChessBase mobile seems to (sometimes?)
                        case '8':   break;
                        default:    okay = false;   break;
                    }
                    if( okay )
                    {

                        // Trim string from end, again
                        move[len-1] = '\0';     // Get rid of 'Q', 'N' etc
                        s = move + len-2;
                        while( s>=move && !(isascii(*s) && isalnum(*s)) )
                            *s-- = '\0';    // get rid of '=' but not '1','8'
                        len = (int)strlen(move);
                    }
                }
            }
        }
    }

    // Castling
    if( okay )
    {
        if( 0==strcmp_ignore(move,"oo") || 0==strcmp_ignore(move,"o-o") )
        {
            strcpy( move, (white?"e1g1":"e8g8") );
            len       = 4;
            piece     = (white?'K':'k');
            default_piece = false;
            kcastling = true;
        }
        else if( 0==strcmp_ignore(move,"ooo") || 0==strcmp_ignore(move,"o-o-o") )
        {
            strcpy( move, (white?"e1c1":"e8c8") );
            len       = 4;
            piece     = (white?'K':'k');
            default_piece = false;
            qcastling = true;
        }
    }

    // Destination square for all except pawn takes pawn (eg "ef")
    if( okay )
    {
        if( len==2 && 'a'<=move[0] && move[0]<='h'
                   && 'a'<=move[1] && move[1]<='h' )
        {
            src_file = move[0]; // eg "ab" pawn takes pawn
            dst_file = move[1];
        }
        else if( len==3 && 'a'<=move[0] && move[0]<='h'
                        && '2'<=move[1] && move[1]<='7'
                        && 'a'<=move[2] && move[2]<='h' )
        {
            src_file = move[0]; // eg "a3b"  pawn takes pawn
            dst_file = move[2];
        }
        else if( len>=2 && 'a'<=move[len-2] && move[len-2]<='h'
                        && '1'<=move[len-1] && move[len-1]<='8' )
        {
            dst_file = move[len-2];
            dst_rank = move[len-1];
            dst_ = SQ(dst_file,dst_rank);
        }
        else
            okay = false;
    }

    // Source square and or piece
    if( okay )
    {
        if( len > 2 )
        {
            if( 'a'<=move[0] && move[0]<='h' &&
                '1'<=move[1] && move[1]<='8' )
            {
                src_file = move[0];
                src_rank = move[1];
            }
            else
            {
                switch( move[0] )
                {
                    case 'K':   piece = (white?'K':'k');    default_piece=false; break;
                    case 'Q':   piece = (white?'Q':'q');    default_piece=false; break;
                    case 'R':   piece = (white?'R':'r');    default_piece=false; break;
                    case 'N':   piece = (white?'N':'n');    default_piece=false; break;
                    case 'P':   piece = (white?'P':'p');    default_piece=false; break;
                    case 'B':   piece = (white?'B':'b');    default_piece=false; break;
                    default:
                    {
                        if( 'a'<=move[0] && move[0]<='h' )
                            src_file = move[0]; // eg "ef4"
                        else
                            okay = false;
                        break;
                    }
                }
                if( len>3  && src_file=='\0' )  // not eg "ef4" above
                {
                    if( '1'<=move[1] && move[1]<='8' )
                        src_rank = move[1];
                    else if( 'a'<=move[1] && move[1]<='h' )
                    {
                        src_file = move[1];
                        if( len>4 && '1'<=move[2] && move[2]<='8' )
                            src_rank = move[2];
                    }
                }
            }
        }
    }

    // Check against all possible moves
    if( okay )
    {
        cr->GenLegalMoveList( &list );

        // Have source and destination, eg "d2d3"
        if( enpassant )
            src_rank = dst_rank = '\0';
        if( src_file && src_rank && dst_file && dst_rank )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( (default_piece || piece==cr->squares[m->src])  &&
                    src_file  ==   FILE(m->src)       &&
                    src_rank  ==   RANK(m->src)       &&
                    dst_       ==   m->dst
                )
                {
                    if( kcastling )
                    {
                        if( m->special ==
                             (white?SPECIAL_WK_CASTLING:SPECIAL_BK_CASTLING) )
                            found = m;
                    }
                    else if( qcastling )
                    {
                        if( m->special ==
                             (white?SPECIAL_WQ_CASTLING:SPECIAL_BQ_CASTLING) )
                            found = m;
                    }
                    else
                        found = m;
                    break;
                }
            }
        }

        // Have source file only, eg "Rae1"
        else if( src_file && dst_file && dst_rank )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( piece     ==   cr->squares[m->src]  &&
                    src_file  ==   FILE(m->src)         &&
                 /* src_rank  ==   RANK(m->src)  */
                    dst_       ==   m->dst
                )
                {
                    found = m;
                    break;
                }
            }
        }

        // Have source rank only, eg "R2d2"
        else if( src_rank && dst_file && dst_rank )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( piece     ==   cr->squares[m->src]   &&
                 /* src_file  ==   FILE(m->src) */
                    src_rank  ==   RANK(m->src)          &&
                    dst_       ==   m->dst
                )
                {
                    found = m;
                    break;
                }
            }
        }

        // Have destination file only eg e4f (because 2 ef moves are possible)
        else if( src_file && src_rank && dst_file )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( piece     ==   cr->squares[m->src]      &&
                    src_file  ==   FILE(m->src)             &&
                    src_rank  ==   RANK(m->src)             &&
                    dst_file  ==   FILE(m->dst)
                )
                {
                    found = m;
                    break;
                }
            }
        }

        // Have files only, eg "ef"
        else if( src_file && dst_file )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( piece     ==   cr->squares[m->src]      &&
                    src_file  ==   FILE(m->src)             &&
                 /* src_rank  ==   RANK(m->src) */
                    dst_file  ==   FILE(m->dst)
                )
                {
                    if( enpassant )
                    {
                        if( m->special ==
                             (white?SPECIAL_WEN_PASSANT:SPECIAL_BEN_PASSANT) )
                            found = m;
                    }
                    else
                        found = m;
                    break;
                }
            }
        }

        // Have destination square only eg "a4"
        else if( dst_rank && dst_file )
        {
            for( i=0; i<list.count; i++ )
            {
                m = &list.moves[i];
                if( piece     ==   cr->squares[m->src]          &&
                    dst_       ==   m->dst
                )
                {
                    found = m;
                    break;
                }
            }
        }
    }

    // Copy found move
    if( okay && found )
    {
        bool found_promotion =
            ( found->special == SPECIAL_PROMOTION_QUEEN ||
              found->special == SPECIAL_PROMOTION_ROOK ||
              found->special == SPECIAL_PROMOTION_BISHOP ||
              found->special == SPECIAL_PROMOTION_KNIGHT
            );
        if( promotion && !found_promotion )
            okay = false;
        if( found_promotion )
        {
            switch( promotion )
            {
                default:
                case 'Q': found->special = SPECIAL_PROMOTION_QUEEN;   break;
                case 'R': found->special = SPECIAL_PROMOTION_ROOK;    break;
                case 'B': found->special = SPECIAL_PROMOTION_BISHOP;  break;
                case 'N': found->special = SPECIAL_PROMOTION_KNIGHT;  break;
            }
        }
    }
    if( !found )
        okay = false;
    if( okay )
        *this = *found;
    return okay;
}

/****************************************************************************
 * Read natural string move eg "Nf3"
 *  return bool okay
 * Fast alternative for known good input
 ****************************************************************************/
bool Move::NaturalInFast( ChessRules *cr, const char *natural_in )
{
    bool err = false;
    bool found = false;
    bool capture_ = false;
    Move mv;

    /*
     Handles moves of the following type
     exd8=N
     e8=B
     exd8N
     e8B
     exd5
     e4
     Nf3
     Nxf3
     Nef3
     Nexf3
     N2f3
     N2xf3
     O-O
     O-O-O
     */

//
    mv.special = NOT_SPECIAL;
    mv.capture = ' ';
    char f = *natural_in++;

    // WHITE MOVE
    if( cr->white )
    {

        // Pawn move ?
        if( 'a'<=f && f<='h' )
        {
            char r = *natural_in++;
            if( r != 'x')
            {

                // Non capturing, non promoting pawn move
                if( '3'<= r && r<= '7')
                {
                    mv.dst = SQ(f,r);
                    mv.src = SOUTH(mv.dst);
                    if( cr->squares[mv.src]=='P' && cr->squares[mv.dst]==' ')
                        found =true;
                    else if( r=='4' && cr->squares[mv.src]==' ' && cr->squares[mv.dst]==' ')
                    {
                        mv.special = SPECIAL_WPAWN_2SQUARES;
                        mv.src = SOUTH(mv.src);
                        found = (cr->squares[mv.src]=='P');
                    }
                }

                // Non capturing, promoting pawn move
                else if( r=='8' )
                {
                    if( *natural_in == '=' )    // now optional
                        natural_in++;
                    mv.dst = SQ(f,r);
                    mv.src = SOUTH(mv.dst);
                    if( cr->squares[mv.src]=='P' && cr->squares[mv.dst]==' ')
                    {
                        switch( *natural_in++ )
                        {
                            default:
                            {
                                break;
                            }
                            case 'Q':
                            case 'q':
                            {
                                mv.special = SPECIAL_PROMOTION_QUEEN;
                                found = true;
                                break;
                            }
                            case 'R':
                            case 'r':
                            {
                                mv.special = SPECIAL_PROMOTION_ROOK;
                                found = true;
                                break;
                            }
                            case 'N':
                            case 'n':
                            {
                                mv.special = SPECIAL_PROMOTION_KNIGHT;
                                found = true;
                                break;
                            }
                            case 'B':
                            case 'b':
                            {
                                mv.special = SPECIAL_PROMOTION_BISHOP;
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
            else // if ( r == 'x' )
            {
                char g = *natural_in++;
                if( 'a'<=g && g<='h' )
                {
                    r = *natural_in++;

                    // Non promoting, capturing pawn move
                    if( '3'<= r && r<= '7')
                    {
                        mv.dst = SQ(g,r);
                        mv.src = SQ(f,r-1);
                        if( cr->squares[mv.src]=='P' && IsBlack(cr->squares[mv.dst]) )
                        {
                            mv.capture = cr->squares[mv.dst];
                            found = true;
                        }
                        else if( r=='6' && cr->squares[mv.src]=='P' && cr->squares[mv.dst]==' '  && mv.dst==cr->enpassant_target && cr->squares[SOUTH(mv.dst)]=='p' )
                        {
                            mv.capture = 'p';
                            mv.special = SPECIAL_WEN_PASSANT;
                            found = true;
                        }
                    }

                    // Promoting, capturing pawn move
                    else if( r=='8' )
                    {
                        if( *natural_in == '=' )    // now optional
                            natural_in++;
                        mv.dst = SQ(g,r);
                        mv.src = SQ(f,r-1);
                        if( cr->squares[mv.src]=='P' && IsBlack(cr->squares[mv.dst]) )
                        {
                            mv.capture = cr->squares[mv.dst];
                            switch( *natural_in++ )
                            {
                                default:
                                {
                                    break;
                                }
                                case 'Q':
                                case 'q':
                                {
                                    mv.special = SPECIAL_PROMOTION_QUEEN;
                                    found = true;
                                    break;
                                }
                                case 'R':
                                case 'r':
                                {
                                    mv.special = SPECIAL_PROMOTION_ROOK;
                                    found = true;
                                    break;
                                }
                                case 'N':
                                case 'n':
                                {
                                    mv.special = SPECIAL_PROMOTION_KNIGHT;
                                    found = true;
                                    break;
                                }
                                case 'B':
                                case 'b':
                                {
                                    mv.special = SPECIAL_PROMOTION_BISHOP;
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {

            // Piece move
            const lte **ray_lookup = queen_lookup;
            switch( f )
            {
                case 'O':
                {
                    if( 0 == memcmp(natural_in,"-O-O",4) )
                    {
                        natural_in += 4;
                        mv.src = e1;
                        mv.dst = c1;
                        mv.capture = ' ';
                        mv.special = SPECIAL_WQ_CASTLING;
                        found = true;
                    }
                    else if( 0 == memcmp(natural_in,"-O",2) )
                    {
                        natural_in += 2;
                        mv.src = e1;
                        mv.dst = g1;
                        mv.capture = ' ';
                        mv.special = SPECIAL_WK_CASTLING;
                        found = true;
                    }
                    break;
                }

                // King is simple special case - there's only one king so no disambiguation
                case 'K':
                {
                    f = *natural_in++;
                    if( f == 'x' )
                    {
                        capture_ = true;
                        f = *natural_in++;
                    }
                    if( 'a'<= f && f<='h' )
                    {
                        char r = *natural_in++;
                        if( '1'<=r && r<='8' )
                        {
                            mv.src = cr->wking_square;
                            mv.dst = SQ(f,r);
                            mv.special = SPECIAL_KING_MOVE;
                            mv.capture = cr->squares[mv.dst];
                            found = ( capture_ ? IsBlack(mv.capture) : IsEmptySquare(mv.capture) );
                        }
                    }
                    break;
                }

                // Other pieces may need to check legality for disambiguation
                case 'Q':   ray_lookup = queen_lookup;                      // fall through
                case 'R':   if( f=='R' )    ray_lookup = rook_lookup;       // fall through
                case 'B':   if( f=='B' )    ray_lookup = bishop_lookup;     // fall through
                case 'N':
                {
                    char piece = f;
                    f = *natural_in++;
                    char src_file='\0';
                    char src_rank='\0';
                    if( f == 'x' )
                    {
                        capture_ = true;
                        f = *natural_in++;
                    }
                    if( '1'<=f && f<='8' )
                    {
                        src_rank = f;
                    }
                    else if( 'a'<= f && f<='h' )
                    {
                        src_file = f;
                    }
                    else
                        err = true;
                    if( !err )
                    {
                        char g = *natural_in++;
                        if( g == 'x' )
                        {
                            if( capture_ )
                                err = true;
                            else
                            {
                                capture_ = true;
                                g = *natural_in++;
                            }
                        }
                        if( '1'<=g && g<='8' )
                        {
                            if( src_file )
                            {
                                mv.dst = SQ(src_file,g);
                                src_file = '\0';
                            }
                            else
                                err = true;
                        }
                        else if( 'a'<=g && g<='h' )
                        {
                            char dst_rank = *natural_in++;
                            if( '1'<=dst_rank && dst_rank<='8' )
                                mv.dst = SQ(g,dst_rank);
                            else
                                err = true;
                        }
                        else
                            err = true;
                        if( !err )
                        {
                            if( capture_ ? IsBlack(cr->squares[mv.dst]) : cr->squares[mv.dst]==' ' )
                            {
                                mv.capture = cr->squares[mv.dst];
                                if( piece == 'N' )
                                {
                                    int count=0;
                                    for( int probe=0; !found && probe<2; probe++ )
                                    {
                                        const lte *ptr = knight_lookup[mv.dst];
                                        lte nbr_moves = *ptr++;
                                        while( !found && nbr_moves-- )
                                        {
                                            Square src_ = (Square)*ptr++;
                                            if( cr->squares[src_]=='N' )
                                            {
                                                bool src_file_ok = !src_file || FILE(src_)==src_file;
                                                bool src_rank_ok = !src_rank || RANK(src_)==src_rank;
                                                if( src_file_ok && src_rank_ok )
                                                {
                                                    mv.src = src_;
                                                    if( probe == 0 )
                                                        count++;
                                                    else // probe==1 means disambiguate by testing whether move is legal, found will be set if
                                                        // we are not exposing white king to check.
                                                    {
                                                        char temp = cr->squares[mv.dst];
                                                        cr->squares[mv.dst] = 'N';  // temporarily make move
                                                        cr->squares[src_] = ' ';
                                                        found = !cr->AttackedSquare( cr->wking_square, false ); //bool AttackedSquare( Square square, bool enemy_is_white );
                                                        cr->squares[mv.dst] = temp;  // now undo move
                                                        cr->squares[src_] = 'N';
                                                    }
                                                }
                                            }
                                        }
                                        if( probe==0 && count==1 )
                                            found = true; // done, no need for disambiguation by check
                                    }
                                }
                                else // if( rook, bishop, queen )
                                {
                                    int count = 0;
                                    for( int probe=0; !found && probe<2; probe++ )
                                    {
                                        const lte *ptr = ray_lookup[mv.dst];
                                        lte nbr_rays = *ptr++;
                                        while( !found && nbr_rays-- )
                                        {
                                            lte ray_len = *ptr++;
                                            while( !found && ray_len-- )
                                            {
                                                Square src_ = (Square)*ptr++;
                                                if( !IsEmptySquare(cr->squares[src_]) )
                                                {
                                                    // Any piece, friend or enemy must move to end of ray
                                                    ptr += ray_len;
                                                    ray_len = 0;
                                                    if( cr->squares[src_] == piece )
                                                    {
                                                        bool src_file_ok = !src_file || FILE(src_)==src_file;
                                                        bool src_rank_ok = !src_rank || RANK(src_)==src_rank;
                                                        if( src_file_ok && src_rank_ok )
                                                        {
                                                            mv.src = src_;
                                                            if( probe == 0 )
                                                                count++;
                                                            else // probe==1 means disambiguate by testing whether move is legal, found will be set if
                                                                // we are not exposing white king to check.
                                                            {
                                                                char temp = cr->squares[mv.dst];
                                                                cr->squares[mv.dst] = piece;  // temporarily make move
                                                                cr->squares[mv.src] = ' ';
                                                                found = !cr->AttackedSquare( cr->wking_square, false ); //bool AttackedSquare( Square square, bool enemy_is_white );
                                                                cr->squares[mv.dst] = temp;  // now undo move
                                                                cr->squares[mv.src] = piece;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if( probe==0 && count==1 )
                                            found = true; // done, no need for disambiguation by check
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // BLACK MOVE
    else
    {
        // Pawn move ?
        if( 'a'<=f && f<='h' )
        {
            char r = *natural_in++;
            if( r != 'x')
            {

                // Non capturing, non promoting pawn move
                if( '2'<= r && r<= '6')
                {
                    mv.dst = SQ(f,r);
                    mv.src = NORTH(mv.dst);
                    if( cr->squares[mv.src]=='p' && cr->squares[mv.dst]==' ')
                        found =true;
                    else if( r=='5' && cr->squares[mv.src]==' ' && cr->squares[mv.dst]==' ')
                    {
                        mv.special = SPECIAL_BPAWN_2SQUARES;
                        mv.src = NORTH(mv.src);
                        found = (cr->squares[mv.src]=='p');
                    }
                }

                // Non capturing, promoting pawn move
                else if( r=='1' )
                {
                    if( *natural_in == '=' )    // now optional
                        natural_in++;
                    mv.dst = SQ(f,r);
                    mv.src = NORTH(mv.dst);
                    if( cr->squares[mv.src]=='p' && cr->squares[mv.dst]==' ')
                    {
                        switch( *natural_in++ )
                        {
                            default:
                            {
                                break;
                            }
                            case 'Q':
                            case 'q':
                            {
                                mv.special = SPECIAL_PROMOTION_QUEEN;
                                found = true;
                                break;
                            }
                            case 'R':
                            case 'r':
                            {
                                mv.special = SPECIAL_PROMOTION_ROOK;
                                found = true;
                                break;
                            }
                            case 'N':
                            case 'n':
                            {
                                mv.special = SPECIAL_PROMOTION_KNIGHT;
                                found = true;
                                break;
                            }
                            case 'B':
                            case 'b':
                            {
                                mv.special = SPECIAL_PROMOTION_BISHOP;
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
            else // if ( r == 'x' )
            {
                char g = *natural_in++;
                if( 'a'<=g && g<='h' )
                {
                    r = *natural_in++;

                    // Non promoting, capturing pawn move
                    if( '2'<= r && r<= '6')
                    {
                        mv.dst = SQ(g,r);
                        mv.src = SQ(f,r+1);
                        if( cr->squares[mv.src]=='p' && IsWhite(cr->squares[mv.dst]) )
                        {
                            mv.capture = cr->squares[mv.dst];
                            found = true;
                        }
                        else if( r=='3' && cr->squares[mv.src]=='p' && cr->squares[mv.dst]==' '  && mv.dst==cr->enpassant_target && cr->squares[NORTH(mv.dst)]=='P' )
                        {
                            mv.capture = 'P';
                            mv.special = SPECIAL_BEN_PASSANT;
                            found = true;
                        }
                    }

                    // Promoting, capturing pawn move
                    else if( r=='1' )
                    {
                        if( *natural_in == '=' )    // now optional
                            natural_in++;
                        mv.dst = SQ(g,r);
                        mv.src = SQ(f,r+1);
                        if( cr->squares[mv.src]=='p' && IsWhite(cr->squares[mv.dst]) )
                        {
                            mv.capture = cr->squares[mv.dst];
                            switch( *natural_in++ )
                            {
                                default:
                                {
                                    break;
                                }
                                case 'Q':
                                case 'q':
                                {
                                    mv.special = SPECIAL_PROMOTION_QUEEN;
                                    found = true;
                                    break;
                                }
                                case 'R':
                                case 'r':
                                {
                                    mv.special = SPECIAL_PROMOTION_ROOK;
                                    found = true;
                                    break;
                                }
                                case 'N':
                                case 'n':
                                {
                                    mv.special = SPECIAL_PROMOTION_KNIGHT;
                                    found = true;
                                    break;
                                }
                                case 'B':
                                case 'b':
                                {
                                    mv.special = SPECIAL_PROMOTION_BISHOP;
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {

            // Piece move
            const lte **ray_lookup=queen_lookup;
            switch( f )
            {
                case 'O':
                {
                    if( 0 == memcmp(natural_in,"-O-O",4) )
                    {
                        natural_in += 4;
                        mv.src = e8;
                        mv.dst = c8;
                        mv.capture = ' ';
                        mv.special = SPECIAL_BQ_CASTLING;
                        found = true;
                    }
                    else if( 0 == memcmp(natural_in,"-O",2) )
                    {
                        natural_in += 2;
                        mv.src = e8;
                        mv.dst = g8;
                        mv.capture = ' ';
                        mv.special = SPECIAL_BK_CASTLING;
                        found = true;
                    }
                    break;
                }

                // King is simple special case - there's only one king so no disambiguation
                case 'K':
                {
                    f = *natural_in++;
                    if( f == 'x' )
                    {
                        capture_ = true;
                        f = *natural_in++;
                    }
                    if( 'a'<= f && f<='h' )
                    {
                        char r = *natural_in++;
                        if( '1'<=r && r<='8' )
                        {
                            mv.src = cr->bking_square;
                            mv.dst = SQ(f,r);
                            mv.special = SPECIAL_KING_MOVE;
                            mv.capture = cr->squares[mv.dst];
                            found = ( capture_ ? IsWhite(mv.capture) : IsEmptySquare(mv.capture) );
                        }
                    }
                    break;
                }

                // Other pieces may need to check legality for disambiguation
                case 'Q':   ray_lookup = queen_lookup;                      // fall through
                case 'R':   if( f=='R' )    ray_lookup = rook_lookup;       // fall through
                case 'B':   if( f=='B' )    ray_lookup = bishop_lookup;     // fall through
                case 'N':
                {
                    char piece = static_cast<char>(tolower(f));
                    f = *natural_in++;
                    char src_file='\0';
                    char src_rank='\0';
                    if( f == 'x' )
                    {
                        capture_ = true;
                        f = *natural_in++;
                    }
                    if( '1'<=f && f<='8' )
                    {
                        src_rank = f;
                    }
                    else if( 'a'<= f && f<='h' )
                    {
                        src_file = f;
                    }
                    else
                        err = true;
                    if( !err )
                    {
                        char g = *natural_in++;
                        if( g == 'x' )
                        {
                            if( capture_ )
                                err = true;
                            else
                            {
                                capture_ = true;
                                g = *natural_in++;
                            }
                        }
                        if( '1'<=g && g<='8' )
                        {
                            if( src_file )
                            {
                                mv.dst = SQ(src_file,g);
                                src_file = '\0';
                            }
                            else
                                err = true;
                        }
                        else if( 'a'<=g && g<='h' )
                        {
                            char dst_rank = *natural_in++;
                            if( '1'<=dst_rank && dst_rank<='8' )
                                mv.dst = SQ(g,dst_rank);
                            else
                                err = true;
                        }
                        else
                            err = true;
                        if( !err )
                        {
                            if( capture_ ? IsWhite(cr->squares[mv.dst]) : cr->squares[mv.dst]==' ' )
                            {
                                mv.capture = cr->squares[mv.dst];
                                if( piece == 'n' )
                                {
                                    int count=0;
                                    for( int probe=0; !found && probe<2; probe++ )
                                    {
                                        const lte *ptr = knight_lookup[mv.dst];
                                        lte nbr_moves = *ptr++;
                                        while( !found && nbr_moves-- )
                                        {
                                            Square src_ = (Square)*ptr++;
                                            if( cr->squares[src_]=='n' )
                                            {
                                                bool src_file_ok = !src_file || FILE(src_)==src_file;
                                                bool src_rank_ok = !src_rank || RANK(src_)==src_rank;
                                                if( src_file_ok && src_rank_ok )
                                                {
                                                    mv.src = src_;
                                                    if( probe == 0 )
                                                        count++;
                                                    else // probe==1 means disambiguate by testing whether move is legal, found will be set if
                                                        // we are not exposing black king to check.
                                                    {
                                                        char temp = cr->squares[mv.dst];
                                                        cr->squares[mv.dst] = 'n';  // temporarily make move
                                                        cr->squares[mv.src] = ' ';
                                                        found = !cr->AttackedSquare( cr->bking_square, true ); //bool AttackedSquare( Square square, bool enemy_is_white );
                                                        cr->squares[mv.dst] = temp;  // now undo move
                                                        cr->squares[mv.src] = 'n';
                                                    }
                                                }
                                            }
                                        }
                                        if( probe==0 && count==1 )
                                            found = true; // done, no need for disambiguation by check
                                    }
                                }
                                else // if( rook, bishop, queen )
                                {
                                    int count = 0;
                                    for( int probe=0; !found && probe<2; probe++ )
                                    {
                                        const lte *ptr = ray_lookup[mv.dst];
                                        lte nbr_rays = *ptr++;
                                        while( !found && nbr_rays-- )
                                        {
                                            lte ray_len = *ptr++;
                                            while( !found && ray_len-- )
                                            {
                                                Square src_ = (Square)*ptr++;
                                                if( !IsEmptySquare(cr->squares[src_]) )
                                                {
                                                    // Any piece, friend or enemy must move to end of ray
                                                    ptr += ray_len;
                                                    ray_len = 0;
                                                    if( cr->squares[src_] == piece )
                                                    {
                                                        bool src_file_ok = !src_file || FILE(src_)==src_file;
                                                        bool src_rank_ok = !src_rank || RANK(src_)==src_rank;
                                                        if( src_file_ok && src_rank_ok )
                                                        {
                                                            mv.src = src_;
                                                            if( probe == 0 )
                                                                count++;
                                                            else // probe==1 means disambiguate by testing whether move is legal, found will be set if
                                                                // we are not exposing black king to check.
                                                            {
                                                                char temp = cr->squares[mv.dst];
                                                                cr->squares[mv.dst] = piece;  // temporarily make move
                                                                cr->squares[mv.src] = ' ';
                                                                found = !cr->AttackedSquare( cr->bking_square, true ); //bool AttackedSquare( Square square, bool enemy_is_white );
                                                                cr->squares[mv.dst] = temp;  // now undo move
                                                                cr->squares[mv.src] = piece;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if( probe==0 && count==1 )
                                            found = true; // done, no need for disambiguation by check
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if( found )
    {
        char c = *natural_in;
        bool problem = (isascii(c) && isalnum(c));
        if( problem )
            found = false;
        else
            *this = mv;
    }
    return found;
}

/****************************************************************************
 * Read terse string move eg "g1f3"
 *  return bool okay
 ****************************************************************************/
bool Move::TerseIn( ChessRules *cr, const char *tmove )
{
    MOVELIST list;
    int i;
    bool okay=false;
    if( strlen(tmove)>=4 && 'a'<=tmove[0] && tmove[0]<='h'
                         && '1'<=tmove[1] && tmove[1]<='8'
                         && 'a'<=tmove[2] && tmove[2]<='h'
                         && '1'<=tmove[3] && tmove[3]<='8' )
    {
        Square src_   = SQ(tmove[0],tmove[1]);
        Square dst_   = SQ(tmove[2],tmove[3]);
        char   expected_promotion_if_any = 'Q';
        if( tmove[4] )
        {
            if( tmove[4]=='n' || tmove[4]=='N' )
                expected_promotion_if_any = 'N';
            else if( tmove[4]=='b' || tmove[4]=='B' )
                expected_promotion_if_any = 'B';
            else if( tmove[4]=='r' || tmove[4]=='R' )
                expected_promotion_if_any = 'R';
        }

        // Generate legal moves, then search for this move
        cr->GenLegalMoveList( &list );
        for( i=0; !okay && i<list.count; i++ )
        {
            if( list.moves[i].dst==dst_ && list.moves[i].src==src_ )
            {
                switch( list.moves[i].special )
                {
                    default:    okay=true;  break;
                    case SPECIAL_PROMOTION_QUEEN:
                    {
                        if( expected_promotion_if_any == 'Q' )
                            okay = true;
                        break;
                    }
                    case SPECIAL_PROMOTION_ROOK:
                    {
                        if( expected_promotion_if_any == 'R' )
                            okay = true;
                        break;
                    }
                    case SPECIAL_PROMOTION_BISHOP:
                    {
                        if( expected_promotion_if_any == 'B' )
                            okay = true;
                        break;
                    }
                    case SPECIAL_PROMOTION_KNIGHT:
                    {
                        if( expected_promotion_if_any == 'N' )
                            okay = true;
                        break;
                    }
                }
            }
            if( okay )
                *this = list.moves[i];
        }
    }
    return okay;
}

/****************************************************************************
 * Convert to natural string
 *    eg "Nf3"
 ****************************************************************************/
std::string Move::NaturalOut( ChessRules *cr )
{

// Improved algorithm

    /* basic procedure is run the following algorithms in turn:
        pawn move     ?
        castling      ?
        Nd2 or Nxd2   ? (loop through all legal moves check if unique)
        Nbd2 or Nbxd2 ? (loop through all legal moves check if unique)
        N1d2 or N1xd2 ? (loop through all legal moves check if unique)
        Nb1d2 or Nb1xd2 (fallback if nothing else works)
    */

    char nmove[10];
    nmove[0] = '-';
    nmove[1] = '-';
    nmove[2] = '\0';
    MOVELIST list;
    bool check[MAXMOVES];
    bool mate[MAXMOVES];
    bool stalemate[MAXMOVES];
    enum
    {
        ALG_PAWN_MOVE,
        ALG_CASTLING,
        ALG_ND2,
        ALG_NBD2,
        ALG_N1D2,
        ALG_NB1D2
    };
    bool done=false;
    bool found = false;
    char append='\0';
    cr->GenLegalMoveList( &list, check, mate, stalemate );
    Move mfound = list.moves[0];   // just to prevent a bogus compiler uninitialized var warning
    for( int i=0; !found && i<list.count; i++ )
    {
        mfound = list.moves[i];
        if( mfound == *this )
        {
            found = true;
            if( mate[i] )
                append = '#';
            else if( check[i] )
                append = '+';
        }
    }

    // Loop through algorithms
    for( int alg=ALG_PAWN_MOVE; found && !done && alg<=ALG_NB1D2; alg++ )
    {
        bool do_loop = (alg==ALG_ND2 || alg==ALG_NBD2 || alg==ALG_N1D2);
        int matches=0;
        Move m;

        // Run the algorithm on the input move (i=-1) AND on all legal moves
        //  in a loop if do_loop set for this algorithm (i=0 to i=count-1)
        for( int i=-1; !done && i<(do_loop?list.count:0); i++ )
        {
            char *str_dst;
            char compare[10];
            if( i == -1 )
            {
                m = *this;
                str_dst = nmove;
            }
            else
            {
                m = list.moves[i];
                str_dst = compare;
            }
            Square src_ = m.src;
            Square dst_ = m.dst;
            char t, p = cr->squares[src_];
            if( islower(p) )
                p = (char)toupper(p);
            if( !IsEmptySquare(m.capture) ) // until we did it this way, enpassant was '-' instead of 'x'
                t = 'x';
            else
                t = '-';
            switch( alg )
            {
                // pawn move ? "e4" or "exf6", plus "=Q" etc if promotion
                case ALG_PAWN_MOVE:
                {
                    if( p == 'P' )
                    {
                        done = true;
                        if( t == 'x' )
                            sprintf( nmove, "%cx%c%c", FILE(src_),FILE(dst_),RANK(dst_) );
                        else
                            sprintf( nmove, "%c%c",FILE(dst_),RANK(dst_) );
                        char *s = strchr(nmove,'\0');
                        switch( m.special )
                        {
                            case SPECIAL_PROMOTION_QUEEN:
                                strcpy( s, "=Q" );  break;
                            case SPECIAL_PROMOTION_ROOK:
                                strcpy( s, "=R" );  break;
                            case SPECIAL_PROMOTION_BISHOP:
                                strcpy( s, "=B" );  break;
                            case SPECIAL_PROMOTION_KNIGHT:
                                strcpy( s, "=N" );  break;
                            default:
                                break;
                        }
                    }
                    break;
                }

                // castling ?
                case ALG_CASTLING:
                {
                    if( m.special==SPECIAL_WK_CASTLING || m.special==SPECIAL_BK_CASTLING )
                    {
                        strcpy( nmove, "O-O" );
                        done = true;
                    }
                    else if( m.special==SPECIAL_WQ_CASTLING || m.special==SPECIAL_BQ_CASTLING )
                    {
                        strcpy( nmove, "O-O-O" );
                        done = true;
                    }
                    break;
                }

                // Nd2 or Nxd2
                case ALG_ND2:
                {
                    if( t == 'x' )
                        sprintf( str_dst, "%cx%c%c", p, FILE(dst_), RANK(dst_) );
                    else
                        sprintf( str_dst, "%c%c%c", p, FILE(dst_), RANK(dst_) );
                    if( i >= 0 )
                    {
                        if( 0 == strcmp(nmove,compare) )
                            matches++;
                    }
                    break;
                }

                // Nbd2 or Nbxd2
                case ALG_NBD2:
                {
                    if( t == 'x' )
                        sprintf( str_dst, "%c%cx%c%c", p, FILE(src_), FILE(dst_), RANK(dst_) );
                    else
                        sprintf( str_dst, "%c%c%c%c", p, FILE(src_), FILE(dst_), RANK(dst_) );
                    if( i >= 0 )
                    {
                        if( 0 == strcmp(nmove,compare) )
                            matches++;
                    }
                    break;
                }

                // N1d2 or N1xd2
                case ALG_N1D2:
                {
                    if( t == 'x' )
                        sprintf( str_dst, "%c%cx%c%c", p, RANK(src_), FILE(dst_), RANK(dst_) );
                    else
                        sprintf( str_dst, "%c%c%c%c", p, RANK(src_), FILE(dst_), RANK(dst_) );
                    if( i >= 0 )
                    {
                        if( 0 == strcmp(nmove,compare) )
                            matches++;
                    }
                    break;
                }

                //  Nb1d2 or Nb1xd2
                case ALG_NB1D2:
                {
                    done = true;
                    if( t == 'x' )
                        sprintf( nmove, "%c%c%cx%c%c", p, FILE(src_), RANK(src_), FILE(dst_), RANK(dst_) );
                    else
                        sprintf( nmove, "%c%c%c%c%c", p, FILE(src_), RANK(src_), FILE(dst_), RANK(dst_) );
                    break;
                }
            }
        }   // end loop for all legal moves with given algorithm

        // If it's a looping algorithm and only one move matches nmove, we're done
        if( do_loop && matches==1 )
            done = true;
    }   // end loop for all algorithms
    if( append )
    {
        char *s = strchr(nmove,'\0');
        *s++ = append;
        *s = '\0';
    }
    return nmove;
}

/****************************************************************************
 * Convert to terse string eg "e7e8q"
 ****************************************************************************/
std::string Move::TerseOut()
{
    char tmove[6];
    if( src == dst )   // null move should be "0000" according to UCI spec
    {
        tmove[0] = '0';
        tmove[1] = '0';
        tmove[2] = '0';
        tmove[3] = '0';
        tmove[4] = '\0';
    }
    else
    {
        tmove[0] = FILE(src);
        tmove[1] = RANK(src);
        tmove[2] = FILE(dst);
        tmove[3] = RANK(dst);
        if( special == SPECIAL_PROMOTION_QUEEN )
            tmove[4] = 'q';
        else if( special == SPECIAL_PROMOTION_ROOK )
            tmove[4] = 'r';
        else if( special == SPECIAL_PROMOTION_BISHOP )
            tmove[4] = 'b';
        else if( special == SPECIAL_PROMOTION_KNIGHT )
            tmove[4] = 'n';
        else
            tmove[4] = '\0';
        tmove[5] = '\0';
    }
    return tmove;
}

/****************************************************************************
 * PrivateChessDefs.cpp Complement PrivateChessDefs.h by providing a shared instantation of
 *  the automatically generated lookup tables.
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
namespace thc
{

// All the lookup tables
#define P 1
#define B 2
#define N 4
#define R 8
#define Q 16
#define K 32

// GeneratedLookupTables.h assumes a suitable type lte = lookup tables element
//  plus a bitmask convention for pieces using identifiers P,R,N,B,Q,K is
//  defined
// #include "GeneratedLookupTables.h"
/****************************************************************************
 * GeneratedLookupTables.h These lookup tables are machine generated
 *  They require prior definitions of;
 *   squares (a1,a2..h8)
 *   pieces (P,N,B,N,R,Q,K)
 *   lte (=lookup table element, a type for the lookup tables, eg int or unsigned char)
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2020, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/

// Queen, up to 8 rays
static const lte queen_lookup_a1[] =
{
(lte)3
    ,(lte)7 ,(lte)b1 ,(lte)c1 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)a2 ,(lte)a3 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
    ,(lte)7 ,(lte)b2 ,(lte)c3 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
};
static const lte queen_lookup_a2[] =
{
(lte)5
    ,(lte)7 ,(lte)b2 ,(lte)c2 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)a1
    ,(lte)6 ,(lte)a3 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
    ,(lte)6 ,(lte)b3 ,(lte)c4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)1 ,(lte)b1
};
static const lte queen_lookup_a3[] =
{
(lte)5
    ,(lte)7 ,(lte)b3 ,(lte)c3 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)a2 ,(lte)a1
    ,(lte)5 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
    ,(lte)5 ,(lte)b4 ,(lte)c5 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)2 ,(lte)b2 ,(lte)c1
};
static const lte queen_lookup_a4[] =
{
(lte)5
    ,(lte)7 ,(lte)b4 ,(lte)c4 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
    ,(lte)4 ,(lte)b5 ,(lte)c6 ,(lte)d7 ,(lte)e8
    ,(lte)3 ,(lte)b3 ,(lte)c2 ,(lte)d1
};
static const lte queen_lookup_a5[] =
{
(lte)5
    ,(lte)7 ,(lte)b5 ,(lte)c5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)3 ,(lte)a6 ,(lte)a7 ,(lte)a8
    ,(lte)3 ,(lte)b6 ,(lte)c7 ,(lte)d8
    ,(lte)4 ,(lte)b4 ,(lte)c3 ,(lte)d2 ,(lte)e1
};
static const lte queen_lookup_a6[] =
{
(lte)5
    ,(lte)7 ,(lte)b6 ,(lte)c6 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)2 ,(lte)a7 ,(lte)a8
    ,(lte)2 ,(lte)b7 ,(lte)c8
    ,(lte)5 ,(lte)b5 ,(lte)c4 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte queen_lookup_a7[] =
{
(lte)5
    ,(lte)7 ,(lte)b7 ,(lte)c7 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)a6 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)1 ,(lte)a8
    ,(lte)1 ,(lte)b8
    ,(lte)6 ,(lte)b6 ,(lte)c5 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte queen_lookup_a8[] =
{
(lte)3
    ,(lte)7 ,(lte)b8 ,(lte)c8 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)a7 ,(lte)a6 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)7 ,(lte)b7 ,(lte)c6 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_b1[] =
{
(lte)5
    ,(lte)1 ,(lte)a1
    ,(lte)6 ,(lte)c1 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)b2 ,(lte)b3 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a2
    ,(lte)6 ,(lte)c2 ,(lte)d3 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
};
static const lte queen_lookup_b2[] =
{
(lte)8
    ,(lte)1 ,(lte)a2
    ,(lte)6 ,(lte)c2 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)b1
    ,(lte)6 ,(lte)b3 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a1
    ,(lte)1 ,(lte)a3
    ,(lte)6 ,(lte)c3 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)1 ,(lte)c1
};
static const lte queen_lookup_b3[] =
{
(lte)8
    ,(lte)1 ,(lte)a3
    ,(lte)6 ,(lte)c3 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)b2 ,(lte)b1
    ,(lte)5 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a2
    ,(lte)1 ,(lte)a4
    ,(lte)5 ,(lte)c4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)2 ,(lte)c2 ,(lte)d1
};
static const lte queen_lookup_b4[] =
{
(lte)8
    ,(lte)1 ,(lte)a4
    ,(lte)6 ,(lte)c4 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a3
    ,(lte)1 ,(lte)a5
    ,(lte)4 ,(lte)c5 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)3 ,(lte)c3 ,(lte)d2 ,(lte)e1
};
static const lte queen_lookup_b5[] =
{
(lte)8
    ,(lte)1 ,(lte)a5
    ,(lte)6 ,(lte)c5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)3 ,(lte)b6 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a4
    ,(lte)1 ,(lte)a6
    ,(lte)3 ,(lte)c6 ,(lte)d7 ,(lte)e8
    ,(lte)4 ,(lte)c4 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte queen_lookup_b6[] =
{
(lte)8
    ,(lte)1 ,(lte)a6
    ,(lte)6 ,(lte)c6 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)2 ,(lte)b7 ,(lte)b8
    ,(lte)1 ,(lte)a5
    ,(lte)1 ,(lte)a7
    ,(lte)2 ,(lte)c7 ,(lte)d8
    ,(lte)5 ,(lte)c5 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte queen_lookup_b7[] =
{
(lte)8
    ,(lte)1 ,(lte)a7
    ,(lte)6 ,(lte)c7 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)b6 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)1 ,(lte)b8
    ,(lte)1 ,(lte)a6
    ,(lte)1 ,(lte)a8
    ,(lte)1 ,(lte)c8
    ,(lte)6 ,(lte)c6 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_b8[] =
{
(lte)5
    ,(lte)1 ,(lte)a8
    ,(lte)6 ,(lte)c8 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)b7 ,(lte)b6 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)1 ,(lte)a7
    ,(lte)6 ,(lte)c7 ,(lte)d6 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte queen_lookup_c1[] =
{
(lte)5
    ,(lte)2 ,(lte)b1 ,(lte)a1
    ,(lte)5 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)c2 ,(lte)c3 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
    ,(lte)2 ,(lte)b2 ,(lte)a3
    ,(lte)5 ,(lte)d2 ,(lte)e3 ,(lte)f4 ,(lte)g5 ,(lte)h6
};
static const lte queen_lookup_c2[] =
{
(lte)8
    ,(lte)2 ,(lte)b2 ,(lte)a2
    ,(lte)5 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)c1
    ,(lte)6 ,(lte)c3 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
    ,(lte)1 ,(lte)b1
    ,(lte)2 ,(lte)b3 ,(lte)a4
    ,(lte)5 ,(lte)d3 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)1 ,(lte)d1
};
static const lte queen_lookup_c3[] =
{
(lte)8
    ,(lte)2 ,(lte)b3 ,(lte)a3
    ,(lte)5 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)c2 ,(lte)c1
    ,(lte)5 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
    ,(lte)2 ,(lte)b2 ,(lte)a1
    ,(lte)2 ,(lte)b4 ,(lte)a5
    ,(lte)5 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)2 ,(lte)d2 ,(lte)e1
};
static const lte queen_lookup_c4[] =
{
(lte)8
    ,(lte)2 ,(lte)b4 ,(lte)a4
    ,(lte)5 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
    ,(lte)2 ,(lte)b3 ,(lte)a2
    ,(lte)2 ,(lte)b5 ,(lte)a6
    ,(lte)4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)3 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte queen_lookup_c5[] =
{
(lte)8
    ,(lte)2 ,(lte)b5 ,(lte)a5
    ,(lte)5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)3 ,(lte)c6 ,(lte)c7 ,(lte)c8
    ,(lte)2 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)b6 ,(lte)a7
    ,(lte)3 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)4 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte queen_lookup_c6[] =
{
(lte)8
    ,(lte)2 ,(lte)b6 ,(lte)a6
    ,(lte)5 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)2 ,(lte)c7 ,(lte)c8
    ,(lte)2 ,(lte)b5 ,(lte)a4
    ,(lte)2 ,(lte)b7 ,(lte)a8
    ,(lte)2 ,(lte)d7 ,(lte)e8
    ,(lte)5 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_c7[] =
{
(lte)8
    ,(lte)2 ,(lte)b7 ,(lte)a7
    ,(lte)5 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)c6 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)1 ,(lte)c8
    ,(lte)2 ,(lte)b6 ,(lte)a5
    ,(lte)1 ,(lte)b8
    ,(lte)1 ,(lte)d8
    ,(lte)5 ,(lte)d6 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte queen_lookup_c8[] =
{
(lte)5
    ,(lte)2 ,(lte)b8 ,(lte)a8
    ,(lte)5 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)c7 ,(lte)c6 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)2 ,(lte)b7 ,(lte)a6
    ,(lte)5 ,(lte)d7 ,(lte)e6 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte queen_lookup_d1[] =
{
(lte)5
    ,(lte)3 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)4 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)d2 ,(lte)d3 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
    ,(lte)3 ,(lte)c2 ,(lte)b3 ,(lte)a4
    ,(lte)4 ,(lte)e2 ,(lte)f3 ,(lte)g4 ,(lte)h5
};
static const lte queen_lookup_d2[] =
{
(lte)8
    ,(lte)3 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)4 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)d1
    ,(lte)6 ,(lte)d3 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
    ,(lte)1 ,(lte)c1
    ,(lte)3 ,(lte)c3 ,(lte)b4 ,(lte)a5
    ,(lte)4 ,(lte)e3 ,(lte)f4 ,(lte)g5 ,(lte)h6
    ,(lte)1 ,(lte)e1
};
static const lte queen_lookup_d3[] =
{
(lte)8
    ,(lte)3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)4 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)d2 ,(lte)d1
    ,(lte)5 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
    ,(lte)2 ,(lte)c2 ,(lte)b1
    ,(lte)3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)4 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)2 ,(lte)e2 ,(lte)f1
};
static const lte queen_lookup_d4[] =
{
(lte)8
    ,(lte)3 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
    ,(lte)3 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)3 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)3 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte queen_lookup_d5[] =
{
(lte)8
    ,(lte)3 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)4 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)3 ,(lte)d6 ,(lte)d7 ,(lte)d8
    ,(lte)3 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)3 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)3 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)4 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_d6[] =
{
(lte)8
    ,(lte)3 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)4 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)2 ,(lte)d7 ,(lte)d8
    ,(lte)3 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)c7 ,(lte)b8
    ,(lte)2 ,(lte)e7 ,(lte)f8
    ,(lte)4 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte queen_lookup_d7[] =
{
(lte)8
    ,(lte)3 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)4 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)d6 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)1 ,(lte)d8
    ,(lte)3 ,(lte)c6 ,(lte)b5 ,(lte)a4
    ,(lte)1 ,(lte)c8
    ,(lte)1 ,(lte)e8
    ,(lte)4 ,(lte)e6 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte queen_lookup_d8[] =
{
(lte)5
    ,(lte)3 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)4 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)d7 ,(lte)d6 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)3 ,(lte)c7 ,(lte)b6 ,(lte)a5
    ,(lte)4 ,(lte)e7 ,(lte)f6 ,(lte)g5 ,(lte)h4
};
static const lte queen_lookup_e1[] =
{
(lte)5
    ,(lte)4 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)3 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)e2 ,(lte)e3 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
    ,(lte)4 ,(lte)d2 ,(lte)c3 ,(lte)b4 ,(lte)a5
    ,(lte)3 ,(lte)f2 ,(lte)g3 ,(lte)h4
};
static const lte queen_lookup_e2[] =
{
(lte)8
    ,(lte)4 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)3 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)e1
    ,(lte)6 ,(lte)e3 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
    ,(lte)1 ,(lte)d1
    ,(lte)4 ,(lte)d3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)3 ,(lte)f3 ,(lte)g4 ,(lte)h5
    ,(lte)1 ,(lte)f1
};
static const lte queen_lookup_e3[] =
{
(lte)8
    ,(lte)4 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)e2 ,(lte)e1
    ,(lte)5 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
    ,(lte)2 ,(lte)d2 ,(lte)c1
    ,(lte)4 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)3 ,(lte)f4 ,(lte)g5 ,(lte)h6
    ,(lte)2 ,(lte)f2 ,(lte)g1
};
static const lte queen_lookup_e4[] =
{
(lte)8
    ,(lte)4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)3 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
    ,(lte)3 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)3 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)3 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_e5[] =
{
(lte)8
    ,(lte)4 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)3 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)3 ,(lte)e6 ,(lte)e7 ,(lte)e8
    ,(lte)4 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)3 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)3 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)3 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte queen_lookup_e6[] =
{
(lte)8
    ,(lte)4 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)3 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)2 ,(lte)e7 ,(lte)e8
    ,(lte)4 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)2 ,(lte)d7 ,(lte)c8
    ,(lte)2 ,(lte)f7 ,(lte)g8
    ,(lte)3 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte queen_lookup_e7[] =
{
(lte)8
    ,(lte)4 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)3 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)e6 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)1 ,(lte)e8
    ,(lte)4 ,(lte)d6 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)1 ,(lte)d8
    ,(lte)1 ,(lte)f8
    ,(lte)3 ,(lte)f6 ,(lte)g5 ,(lte)h4
};
static const lte queen_lookup_e8[] =
{
(lte)5
    ,(lte)4 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)3 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)e7 ,(lte)e6 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)4 ,(lte)d7 ,(lte)c6 ,(lte)b5 ,(lte)a4
    ,(lte)3 ,(lte)f7 ,(lte)g6 ,(lte)h5
};
static const lte queen_lookup_f1[] =
{
(lte)5
    ,(lte)5 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)2 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)f2 ,(lte)f3 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
    ,(lte)5 ,(lte)e2 ,(lte)d3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)2 ,(lte)g2 ,(lte)h3
};
static const lte queen_lookup_f2[] =
{
(lte)8
    ,(lte)5 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)f1
    ,(lte)6 ,(lte)f3 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
    ,(lte)1 ,(lte)e1
    ,(lte)5 ,(lte)e3 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)2 ,(lte)g3 ,(lte)h4
    ,(lte)1 ,(lte)g1
};
static const lte queen_lookup_f3[] =
{
(lte)8
    ,(lte)5 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)2 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)f2 ,(lte)f1
    ,(lte)5 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
    ,(lte)2 ,(lte)e2 ,(lte)d1
    ,(lte)5 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)2 ,(lte)g4 ,(lte)h5
    ,(lte)2 ,(lte)g2 ,(lte)h1
};
static const lte queen_lookup_f4[] =
{
(lte)8
    ,(lte)5 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)2 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
    ,(lte)3 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)2 ,(lte)g5 ,(lte)h6
    ,(lte)2 ,(lte)g3 ,(lte)h2
};
static const lte queen_lookup_f5[] =
{
(lte)8
    ,(lte)5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)2 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)3 ,(lte)f6 ,(lte)f7 ,(lte)f8
    ,(lte)4 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)3 ,(lte)e6 ,(lte)d7 ,(lte)c8
    ,(lte)2 ,(lte)g6 ,(lte)h7
    ,(lte)2 ,(lte)g4 ,(lte)h3
};
static const lte queen_lookup_f6[] =
{
(lte)8
    ,(lte)5 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)2 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)2 ,(lte)f7 ,(lte)f8
    ,(lte)5 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)2 ,(lte)e7 ,(lte)d8
    ,(lte)2 ,(lte)g7 ,(lte)h8
    ,(lte)2 ,(lte)g5 ,(lte)h4
};
static const lte queen_lookup_f7[] =
{
(lte)8
    ,(lte)5 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)2 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)f6 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)1 ,(lte)f8
    ,(lte)5 ,(lte)e6 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)1 ,(lte)e8
    ,(lte)1 ,(lte)g8
    ,(lte)2 ,(lte)g6 ,(lte)h5
};
static const lte queen_lookup_f8[] =
{
(lte)5
    ,(lte)5 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)2 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)f7 ,(lte)f6 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)5 ,(lte)e7 ,(lte)d6 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)g7 ,(lte)h6
};
static const lte queen_lookup_g1[] =
{
(lte)5
    ,(lte)6 ,(lte)f1 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)1 ,(lte)h1
    ,(lte)7 ,(lte)g2 ,(lte)g3 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
    ,(lte)6 ,(lte)f2 ,(lte)e3 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)1 ,(lte)h2
};
static const lte queen_lookup_g2[] =
{
(lte)8
    ,(lte)6 ,(lte)f2 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)1 ,(lte)h2
    ,(lte)1 ,(lte)g1
    ,(lte)6 ,(lte)g3 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
    ,(lte)1 ,(lte)f1
    ,(lte)6 ,(lte)f3 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)1 ,(lte)h3
    ,(lte)1 ,(lte)h1
};
static const lte queen_lookup_g3[] =
{
(lte)8
    ,(lte)6 ,(lte)f3 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)1 ,(lte)h3
    ,(lte)2 ,(lte)g2 ,(lte)g1
    ,(lte)5 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
    ,(lte)2 ,(lte)f2 ,(lte)e1
    ,(lte)5 ,(lte)f4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)1 ,(lte)h4
    ,(lte)1 ,(lte)h2
};
static const lte queen_lookup_g4[] =
{
(lte)8
    ,(lte)6 ,(lte)f4 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)1 ,(lte)h4
    ,(lte)3 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
    ,(lte)3 ,(lte)f3 ,(lte)e2 ,(lte)d1
    ,(lte)4 ,(lte)f5 ,(lte)e6 ,(lte)d7 ,(lte)c8
    ,(lte)1 ,(lte)h5
    ,(lte)1 ,(lte)h3
};
static const lte queen_lookup_g5[] =
{
(lte)8
    ,(lte)6 ,(lte)f5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)1 ,(lte)h5
    ,(lte)4 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)3 ,(lte)g6 ,(lte)g7 ,(lte)g8
    ,(lte)4 ,(lte)f4 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)3 ,(lte)f6 ,(lte)e7 ,(lte)d8
    ,(lte)1 ,(lte)h6
    ,(lte)1 ,(lte)h4
};
static const lte queen_lookup_g6[] =
{
(lte)8
    ,(lte)6 ,(lte)f6 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)1 ,(lte)h6
    ,(lte)5 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)2 ,(lte)g7 ,(lte)g8
    ,(lte)5 ,(lte)f5 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)2 ,(lte)f7 ,(lte)e8
    ,(lte)1 ,(lte)h7
    ,(lte)1 ,(lte)h5
};
static const lte queen_lookup_g7[] =
{
(lte)8
    ,(lte)6 ,(lte)f7 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)1 ,(lte)h7
    ,(lte)6 ,(lte)g6 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)1 ,(lte)g8
    ,(lte)6 ,(lte)f6 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)1 ,(lte)f8
    ,(lte)1 ,(lte)h8
    ,(lte)1 ,(lte)h6
};
static const lte queen_lookup_g8[] =
{
(lte)5
    ,(lte)6 ,(lte)f8 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)1 ,(lte)h8
    ,(lte)7 ,(lte)g7 ,(lte)g6 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)6 ,(lte)f7 ,(lte)e6 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)1 ,(lte)h7
};
static const lte queen_lookup_h1[] =
{
(lte)3
    ,(lte)7 ,(lte)g1 ,(lte)f1 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)7 ,(lte)h2 ,(lte)h3 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
    ,(lte)7 ,(lte)g2 ,(lte)f3 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
};
static const lte queen_lookup_h2[] =
{
(lte)5
    ,(lte)7 ,(lte)g2 ,(lte)f2 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)1 ,(lte)h1
    ,(lte)6 ,(lte)h3 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
    ,(lte)1 ,(lte)g1
    ,(lte)6 ,(lte)g3 ,(lte)f4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
};
static const lte queen_lookup_h3[] =
{
(lte)5
    ,(lte)7 ,(lte)g3 ,(lte)f3 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)2 ,(lte)h2 ,(lte)h1
    ,(lte)5 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
    ,(lte)2 ,(lte)g2 ,(lte)f1
    ,(lte)5 ,(lte)g4 ,(lte)f5 ,(lte)e6 ,(lte)d7 ,(lte)c8
};
static const lte queen_lookup_h4[] =
{
(lte)5
    ,(lte)7 ,(lte)g4 ,(lte)f4 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)3 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
    ,(lte)3 ,(lte)g3 ,(lte)f2 ,(lte)e1
    ,(lte)4 ,(lte)g5 ,(lte)f6 ,(lte)e7 ,(lte)d8
};
static const lte queen_lookup_h5[] =
{
(lte)5
    ,(lte)7 ,(lte)g5 ,(lte)f5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)4 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)3 ,(lte)h6 ,(lte)h7 ,(lte)h8
    ,(lte)4 ,(lte)g4 ,(lte)f3 ,(lte)e2 ,(lte)d1
    ,(lte)3 ,(lte)g6 ,(lte)f7 ,(lte)e8
};
static const lte queen_lookup_h6[] =
{
(lte)5
    ,(lte)7 ,(lte)g6 ,(lte)f6 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)5 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)2 ,(lte)h7 ,(lte)h8
    ,(lte)5 ,(lte)g5 ,(lte)f4 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)2 ,(lte)g7 ,(lte)f8
};
static const lte queen_lookup_h7[] =
{
(lte)5
    ,(lte)7 ,(lte)g7 ,(lte)f7 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)6 ,(lte)h6 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)1 ,(lte)h8
    ,(lte)6 ,(lte)g6 ,(lte)f5 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)1 ,(lte)g8
};
static const lte queen_lookup_h8[] =
{
(lte)3
    ,(lte)7 ,(lte)g8 ,(lte)f8 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)7 ,(lte)h7 ,(lte)h6 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)7 ,(lte)g7 ,(lte)f6 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
};

// queen_lookup
const lte *queen_lookup[] =
{
    queen_lookup_a8,
    queen_lookup_b8,
    queen_lookup_c8,
    queen_lookup_d8,
    queen_lookup_e8,
    queen_lookup_f8,
    queen_lookup_g8,
    queen_lookup_h8,
    queen_lookup_a7,
    queen_lookup_b7,
    queen_lookup_c7,
    queen_lookup_d7,
    queen_lookup_e7,
    queen_lookup_f7,
    queen_lookup_g7,
    queen_lookup_h7,
    queen_lookup_a6,
    queen_lookup_b6,
    queen_lookup_c6,
    queen_lookup_d6,
    queen_lookup_e6,
    queen_lookup_f6,
    queen_lookup_g6,
    queen_lookup_h6,
    queen_lookup_a5,
    queen_lookup_b5,
    queen_lookup_c5,
    queen_lookup_d5,
    queen_lookup_e5,
    queen_lookup_f5,
    queen_lookup_g5,
    queen_lookup_h5,
    queen_lookup_a4,
    queen_lookup_b4,
    queen_lookup_c4,
    queen_lookup_d4,
    queen_lookup_e4,
    queen_lookup_f4,
    queen_lookup_g4,
    queen_lookup_h4,
    queen_lookup_a3,
    queen_lookup_b3,
    queen_lookup_c3,
    queen_lookup_d3,
    queen_lookup_e3,
    queen_lookup_f3,
    queen_lookup_g3,
    queen_lookup_h3,
    queen_lookup_a2,
    queen_lookup_b2,
    queen_lookup_c2,
    queen_lookup_d2,
    queen_lookup_e2,
    queen_lookup_f2,
    queen_lookup_g2,
    queen_lookup_h2,
    queen_lookup_a1,
    queen_lookup_b1,
    queen_lookup_c1,
    queen_lookup_d1,
    queen_lookup_e1,
    queen_lookup_f1,
    queen_lookup_g1,
    queen_lookup_h1
};

// Rook, up to 4 rays
static const lte rook_lookup_a1[] =
{
(lte)2
    ,(lte)7 ,(lte)b1 ,(lte)c1 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)a2 ,(lte)a3 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a2[] =
{
(lte)3
    ,(lte)7 ,(lte)b2 ,(lte)c2 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)a1
    ,(lte)6 ,(lte)a3 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a3[] =
{
(lte)3
    ,(lte)7 ,(lte)b3 ,(lte)c3 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)a2 ,(lte)a1
    ,(lte)5 ,(lte)a4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a4[] =
{
(lte)3
    ,(lte)7 ,(lte)b4 ,(lte)c4 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)4 ,(lte)a5 ,(lte)a6 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a5[] =
{
(lte)3
    ,(lte)7 ,(lte)b5 ,(lte)c5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)3 ,(lte)a6 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a6[] =
{
(lte)3
    ,(lte)7 ,(lte)b6 ,(lte)c6 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)2 ,(lte)a7 ,(lte)a8
};
static const lte rook_lookup_a7[] =
{
(lte)3
    ,(lte)7 ,(lte)b7 ,(lte)c7 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)a6 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
    ,(lte)1 ,(lte)a8
};
static const lte rook_lookup_a8[] =
{
(lte)2
    ,(lte)7 ,(lte)b8 ,(lte)c8 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)a7 ,(lte)a6 ,(lte)a5 ,(lte)a4 ,(lte)a3 ,(lte)a2 ,(lte)a1
};
static const lte rook_lookup_b1[] =
{
(lte)3
    ,(lte)1 ,(lte)a1
    ,(lte)6 ,(lte)c1 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)b2 ,(lte)b3 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b2[] =
{
(lte)4
    ,(lte)1 ,(lte)a2
    ,(lte)6 ,(lte)c2 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)b1
    ,(lte)6 ,(lte)b3 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b3[] =
{
(lte)4
    ,(lte)1 ,(lte)a3
    ,(lte)6 ,(lte)c3 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)b2 ,(lte)b1
    ,(lte)5 ,(lte)b4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b4[] =
{
(lte)4
    ,(lte)1 ,(lte)a4
    ,(lte)6 ,(lte)c4 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)4 ,(lte)b5 ,(lte)b6 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b5[] =
{
(lte)4
    ,(lte)1 ,(lte)a5
    ,(lte)6 ,(lte)c5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)3 ,(lte)b6 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b6[] =
{
(lte)4
    ,(lte)1 ,(lte)a6
    ,(lte)6 ,(lte)c6 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)2 ,(lte)b7 ,(lte)b8
};
static const lte rook_lookup_b7[] =
{
(lte)4
    ,(lte)1 ,(lte)a7
    ,(lte)6 ,(lte)c7 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)b6 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
    ,(lte)1 ,(lte)b8
};
static const lte rook_lookup_b8[] =
{
(lte)3
    ,(lte)1 ,(lte)a8
    ,(lte)6 ,(lte)c8 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)b7 ,(lte)b6 ,(lte)b5 ,(lte)b4 ,(lte)b3 ,(lte)b2 ,(lte)b1
};
static const lte rook_lookup_c1[] =
{
(lte)3
    ,(lte)2 ,(lte)b1 ,(lte)a1
    ,(lte)5 ,(lte)d1 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)c2 ,(lte)c3 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c2[] =
{
(lte)4
    ,(lte)2 ,(lte)b2 ,(lte)a2
    ,(lte)5 ,(lte)d2 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)c1
    ,(lte)6 ,(lte)c3 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c3[] =
{
(lte)4
    ,(lte)2 ,(lte)b3 ,(lte)a3
    ,(lte)5 ,(lte)d3 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)c2 ,(lte)c1
    ,(lte)5 ,(lte)c4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c4[] =
{
(lte)4
    ,(lte)2 ,(lte)b4 ,(lte)a4
    ,(lte)5 ,(lte)d4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)4 ,(lte)c5 ,(lte)c6 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c5[] =
{
(lte)4
    ,(lte)2 ,(lte)b5 ,(lte)a5
    ,(lte)5 ,(lte)d5 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)3 ,(lte)c6 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c6[] =
{
(lte)4
    ,(lte)2 ,(lte)b6 ,(lte)a6
    ,(lte)5 ,(lte)d6 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)2 ,(lte)c7 ,(lte)c8
};
static const lte rook_lookup_c7[] =
{
(lte)4
    ,(lte)2 ,(lte)b7 ,(lte)a7
    ,(lte)5 ,(lte)d7 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)c6 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
    ,(lte)1 ,(lte)c8
};
static const lte rook_lookup_c8[] =
{
(lte)3
    ,(lte)2 ,(lte)b8 ,(lte)a8
    ,(lte)5 ,(lte)d8 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)c7 ,(lte)c6 ,(lte)c5 ,(lte)c4 ,(lte)c3 ,(lte)c2 ,(lte)c1
};
static const lte rook_lookup_d1[] =
{
(lte)3
    ,(lte)3 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)4 ,(lte)e1 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)d2 ,(lte)d3 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d2[] =
{
(lte)4
    ,(lte)3 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)4 ,(lte)e2 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)d1
    ,(lte)6 ,(lte)d3 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d3[] =
{
(lte)4
    ,(lte)3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)4 ,(lte)e3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)d2 ,(lte)d1
    ,(lte)5 ,(lte)d4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d4[] =
{
(lte)4
    ,(lte)3 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)4 ,(lte)e4 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)4 ,(lte)d5 ,(lte)d6 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d5[] =
{
(lte)4
    ,(lte)3 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)4 ,(lte)e5 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)3 ,(lte)d6 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d6[] =
{
(lte)4
    ,(lte)3 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)4 ,(lte)e6 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)2 ,(lte)d7 ,(lte)d8
};
static const lte rook_lookup_d7[] =
{
(lte)4
    ,(lte)3 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)4 ,(lte)e7 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)d6 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
    ,(lte)1 ,(lte)d8
};
static const lte rook_lookup_d8[] =
{
(lte)3
    ,(lte)3 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)4 ,(lte)e8 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)d7 ,(lte)d6 ,(lte)d5 ,(lte)d4 ,(lte)d3 ,(lte)d2 ,(lte)d1
};
static const lte rook_lookup_e1[] =
{
(lte)3
    ,(lte)4 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)3 ,(lte)f1 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)e2 ,(lte)e3 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e2[] =
{
(lte)4
    ,(lte)4 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)3 ,(lte)f2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)e1
    ,(lte)6 ,(lte)e3 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e3[] =
{
(lte)4
    ,(lte)4 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)3 ,(lte)f3 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)e2 ,(lte)e1
    ,(lte)5 ,(lte)e4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e4[] =
{
(lte)4
    ,(lte)4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)3 ,(lte)f4 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)4 ,(lte)e5 ,(lte)e6 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e5[] =
{
(lte)4
    ,(lte)4 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)3 ,(lte)f5 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)3 ,(lte)e6 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e6[] =
{
(lte)4
    ,(lte)4 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)3 ,(lte)f6 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)2 ,(lte)e7 ,(lte)e8
};
static const lte rook_lookup_e7[] =
{
(lte)4
    ,(lte)4 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)3 ,(lte)f7 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)e6 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
    ,(lte)1 ,(lte)e8
};
static const lte rook_lookup_e8[] =
{
(lte)3
    ,(lte)4 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)3 ,(lte)f8 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)e7 ,(lte)e6 ,(lte)e5 ,(lte)e4 ,(lte)e3 ,(lte)e2 ,(lte)e1
};
static const lte rook_lookup_f1[] =
{
(lte)3
    ,(lte)5 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)2 ,(lte)g1 ,(lte)h1
    ,(lte)7 ,(lte)f2 ,(lte)f3 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f2[] =
{
(lte)4
    ,(lte)5 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)2 ,(lte)g2 ,(lte)h2
    ,(lte)1 ,(lte)f1
    ,(lte)6 ,(lte)f3 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f3[] =
{
(lte)4
    ,(lte)5 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)2 ,(lte)g3 ,(lte)h3
    ,(lte)2 ,(lte)f2 ,(lte)f1
    ,(lte)5 ,(lte)f4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f4[] =
{
(lte)4
    ,(lte)5 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)2 ,(lte)g4 ,(lte)h4
    ,(lte)3 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)4 ,(lte)f5 ,(lte)f6 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f5[] =
{
(lte)4
    ,(lte)5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)2 ,(lte)g5 ,(lte)h5
    ,(lte)4 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)3 ,(lte)f6 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f6[] =
{
(lte)4
    ,(lte)5 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)2 ,(lte)g6 ,(lte)h6
    ,(lte)5 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)2 ,(lte)f7 ,(lte)f8
};
static const lte rook_lookup_f7[] =
{
(lte)4
    ,(lte)5 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)2 ,(lte)g7 ,(lte)h7
    ,(lte)6 ,(lte)f6 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
    ,(lte)1 ,(lte)f8
};
static const lte rook_lookup_f8[] =
{
(lte)3
    ,(lte)5 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)2 ,(lte)g8 ,(lte)h8
    ,(lte)7 ,(lte)f7 ,(lte)f6 ,(lte)f5 ,(lte)f4 ,(lte)f3 ,(lte)f2 ,(lte)f1
};
static const lte rook_lookup_g1[] =
{
(lte)3
    ,(lte)6 ,(lte)f1 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)1 ,(lte)h1
    ,(lte)7 ,(lte)g2 ,(lte)g3 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g2[] =
{
(lte)4
    ,(lte)6 ,(lte)f2 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)1 ,(lte)h2
    ,(lte)1 ,(lte)g1
    ,(lte)6 ,(lte)g3 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g3[] =
{
(lte)4
    ,(lte)6 ,(lte)f3 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)1 ,(lte)h3
    ,(lte)2 ,(lte)g2 ,(lte)g1
    ,(lte)5 ,(lte)g4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g4[] =
{
(lte)4
    ,(lte)6 ,(lte)f4 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)1 ,(lte)h4
    ,(lte)3 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)4 ,(lte)g5 ,(lte)g6 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g5[] =
{
(lte)4
    ,(lte)6 ,(lte)f5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)1 ,(lte)h5
    ,(lte)4 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)3 ,(lte)g6 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g6[] =
{
(lte)4
    ,(lte)6 ,(lte)f6 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)1 ,(lte)h6
    ,(lte)5 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)2 ,(lte)g7 ,(lte)g8
};
static const lte rook_lookup_g7[] =
{
(lte)4
    ,(lte)6 ,(lte)f7 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)1 ,(lte)h7
    ,(lte)6 ,(lte)g6 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
    ,(lte)1 ,(lte)g8
};
static const lte rook_lookup_g8[] =
{
(lte)3
    ,(lte)6 ,(lte)f8 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)1 ,(lte)h8
    ,(lte)7 ,(lte)g7 ,(lte)g6 ,(lte)g5 ,(lte)g4 ,(lte)g3 ,(lte)g2 ,(lte)g1
};
static const lte rook_lookup_h1[] =
{
(lte)2
    ,(lte)7 ,(lte)g1 ,(lte)f1 ,(lte)e1 ,(lte)d1 ,(lte)c1 ,(lte)b1 ,(lte)a1
    ,(lte)7 ,(lte)h2 ,(lte)h3 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h2[] =
{
(lte)3
    ,(lte)7 ,(lte)g2 ,(lte)f2 ,(lte)e2 ,(lte)d2 ,(lte)c2 ,(lte)b2 ,(lte)a2
    ,(lte)1 ,(lte)h1
    ,(lte)6 ,(lte)h3 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h3[] =
{
(lte)3
    ,(lte)7 ,(lte)g3 ,(lte)f3 ,(lte)e3 ,(lte)d3 ,(lte)c3 ,(lte)b3 ,(lte)a3
    ,(lte)2 ,(lte)h2 ,(lte)h1
    ,(lte)5 ,(lte)h4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h4[] =
{
(lte)3
    ,(lte)7 ,(lte)g4 ,(lte)f4 ,(lte)e4 ,(lte)d4 ,(lte)c4 ,(lte)b4 ,(lte)a4
    ,(lte)3 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)4 ,(lte)h5 ,(lte)h6 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h5[] =
{
(lte)3
    ,(lte)7 ,(lte)g5 ,(lte)f5 ,(lte)e5 ,(lte)d5 ,(lte)c5 ,(lte)b5 ,(lte)a5
    ,(lte)4 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)3 ,(lte)h6 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h6[] =
{
(lte)3
    ,(lte)7 ,(lte)g6 ,(lte)f6 ,(lte)e6 ,(lte)d6 ,(lte)c6 ,(lte)b6 ,(lte)a6
    ,(lte)5 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)2 ,(lte)h7 ,(lte)h8
};
static const lte rook_lookup_h7[] =
{
(lte)3
    ,(lte)7 ,(lte)g7 ,(lte)f7 ,(lte)e7 ,(lte)d7 ,(lte)c7 ,(lte)b7 ,(lte)a7
    ,(lte)6 ,(lte)h6 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
    ,(lte)1 ,(lte)h8
};
static const lte rook_lookup_h8[] =
{
(lte)2
    ,(lte)7 ,(lte)g8 ,(lte)f8 ,(lte)e8 ,(lte)d8 ,(lte)c8 ,(lte)b8 ,(lte)a8
    ,(lte)7 ,(lte)h7 ,(lte)h6 ,(lte)h5 ,(lte)h4 ,(lte)h3 ,(lte)h2 ,(lte)h1
};

// rook_lookup
const lte *rook_lookup[] =
{
    rook_lookup_a8,
    rook_lookup_b8,
    rook_lookup_c8,
    rook_lookup_d8,
    rook_lookup_e8,
    rook_lookup_f8,
    rook_lookup_g8,
    rook_lookup_h8,
    rook_lookup_a7,
    rook_lookup_b7,
    rook_lookup_c7,
    rook_lookup_d7,
    rook_lookup_e7,
    rook_lookup_f7,
    rook_lookup_g7,
    rook_lookup_h7,
    rook_lookup_a6,
    rook_lookup_b6,
    rook_lookup_c6,
    rook_lookup_d6,
    rook_lookup_e6,
    rook_lookup_f6,
    rook_lookup_g6,
    rook_lookup_h6,
    rook_lookup_a5,
    rook_lookup_b5,
    rook_lookup_c5,
    rook_lookup_d5,
    rook_lookup_e5,
    rook_lookup_f5,
    rook_lookup_g5,
    rook_lookup_h5,
    rook_lookup_a4,
    rook_lookup_b4,
    rook_lookup_c4,
    rook_lookup_d4,
    rook_lookup_e4,
    rook_lookup_f4,
    rook_lookup_g4,
    rook_lookup_h4,
    rook_lookup_a3,
    rook_lookup_b3,
    rook_lookup_c3,
    rook_lookup_d3,
    rook_lookup_e3,
    rook_lookup_f3,
    rook_lookup_g3,
    rook_lookup_h3,
    rook_lookup_a2,
    rook_lookup_b2,
    rook_lookup_c2,
    rook_lookup_d2,
    rook_lookup_e2,
    rook_lookup_f2,
    rook_lookup_g2,
    rook_lookup_h2,
    rook_lookup_a1,
    rook_lookup_b1,
    rook_lookup_c1,
    rook_lookup_d1,
    rook_lookup_e1,
    rook_lookup_f1,
    rook_lookup_g1,
    rook_lookup_h1
};

// Bishop, up to 4 rays
static const lte bishop_lookup_a1[] =
{
(lte)1
    ,(lte)7 ,(lte)b2 ,(lte)c3 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
};
static const lte bishop_lookup_a2[] =
{
(lte)2
    ,(lte)6 ,(lte)b3 ,(lte)c4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)1 ,(lte)b1
};
static const lte bishop_lookup_a3[] =
{
(lte)2
    ,(lte)5 ,(lte)b4 ,(lte)c5 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)2 ,(lte)b2 ,(lte)c1
};
static const lte bishop_lookup_a4[] =
{
(lte)2
    ,(lte)4 ,(lte)b5 ,(lte)c6 ,(lte)d7 ,(lte)e8
    ,(lte)3 ,(lte)b3 ,(lte)c2 ,(lte)d1
};
static const lte bishop_lookup_a5[] =
{
(lte)2
    ,(lte)3 ,(lte)b6 ,(lte)c7 ,(lte)d8
    ,(lte)4 ,(lte)b4 ,(lte)c3 ,(lte)d2 ,(lte)e1
};
static const lte bishop_lookup_a6[] =
{
(lte)2
    ,(lte)2 ,(lte)b7 ,(lte)c8
    ,(lte)5 ,(lte)b5 ,(lte)c4 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte bishop_lookup_a7[] =
{
(lte)2
    ,(lte)1 ,(lte)b8
    ,(lte)6 ,(lte)b6 ,(lte)c5 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte bishop_lookup_a8[] =
{
(lte)1
    ,(lte)7 ,(lte)b7 ,(lte)c6 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_b1[] =
{
(lte)2
    ,(lte)1 ,(lte)a2
    ,(lte)6 ,(lte)c2 ,(lte)d3 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
};
static const lte bishop_lookup_b2[] =
{
(lte)4
    ,(lte)1 ,(lte)a1
    ,(lte)1 ,(lte)a3
    ,(lte)6 ,(lte)c3 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)1 ,(lte)c1
};
static const lte bishop_lookup_b3[] =
{
(lte)4
    ,(lte)1 ,(lte)a2
    ,(lte)1 ,(lte)a4
    ,(lte)5 ,(lte)c4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)2 ,(lte)c2 ,(lte)d1
};
static const lte bishop_lookup_b4[] =
{
(lte)4
    ,(lte)1 ,(lte)a3
    ,(lte)1 ,(lte)a5
    ,(lte)4 ,(lte)c5 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)3 ,(lte)c3 ,(lte)d2 ,(lte)e1
};
static const lte bishop_lookup_b5[] =
{
(lte)4
    ,(lte)1 ,(lte)a4
    ,(lte)1 ,(lte)a6
    ,(lte)3 ,(lte)c6 ,(lte)d7 ,(lte)e8
    ,(lte)4 ,(lte)c4 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte bishop_lookup_b6[] =
{
(lte)4
    ,(lte)1 ,(lte)a5
    ,(lte)1 ,(lte)a7
    ,(lte)2 ,(lte)c7 ,(lte)d8
    ,(lte)5 ,(lte)c5 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte bishop_lookup_b7[] =
{
(lte)4
    ,(lte)1 ,(lte)a6
    ,(lte)1 ,(lte)a8
    ,(lte)1 ,(lte)c8
    ,(lte)6 ,(lte)c6 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_b8[] =
{
(lte)2
    ,(lte)1 ,(lte)a7
    ,(lte)6 ,(lte)c7 ,(lte)d6 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte bishop_lookup_c1[] =
{
(lte)2
    ,(lte)2 ,(lte)b2 ,(lte)a3
    ,(lte)5 ,(lte)d2 ,(lte)e3 ,(lte)f4 ,(lte)g5 ,(lte)h6
};
static const lte bishop_lookup_c2[] =
{
(lte)4
    ,(lte)1 ,(lte)b1
    ,(lte)2 ,(lte)b3 ,(lte)a4
    ,(lte)5 ,(lte)d3 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)1 ,(lte)d1
};
static const lte bishop_lookup_c3[] =
{
(lte)4
    ,(lte)2 ,(lte)b2 ,(lte)a1
    ,(lte)2 ,(lte)b4 ,(lte)a5
    ,(lte)5 ,(lte)d4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)2 ,(lte)d2 ,(lte)e1
};
static const lte bishop_lookup_c4[] =
{
(lte)4
    ,(lte)2 ,(lte)b3 ,(lte)a2
    ,(lte)2 ,(lte)b5 ,(lte)a6
    ,(lte)4 ,(lte)d5 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)3 ,(lte)d3 ,(lte)e2 ,(lte)f1
};
static const lte bishop_lookup_c5[] =
{
(lte)4
    ,(lte)2 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)b6 ,(lte)a7
    ,(lte)3 ,(lte)d6 ,(lte)e7 ,(lte)f8
    ,(lte)4 ,(lte)d4 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte bishop_lookup_c6[] =
{
(lte)4
    ,(lte)2 ,(lte)b5 ,(lte)a4
    ,(lte)2 ,(lte)b7 ,(lte)a8
    ,(lte)2 ,(lte)d7 ,(lte)e8
    ,(lte)5 ,(lte)d5 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_c7[] =
{
(lte)4
    ,(lte)2 ,(lte)b6 ,(lte)a5
    ,(lte)1 ,(lte)b8
    ,(lte)1 ,(lte)d8
    ,(lte)5 ,(lte)d6 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte bishop_lookup_c8[] =
{
(lte)2
    ,(lte)2 ,(lte)b7 ,(lte)a6
    ,(lte)5 ,(lte)d7 ,(lte)e6 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte bishop_lookup_d1[] =
{
(lte)2
    ,(lte)3 ,(lte)c2 ,(lte)b3 ,(lte)a4
    ,(lte)4 ,(lte)e2 ,(lte)f3 ,(lte)g4 ,(lte)h5
};
static const lte bishop_lookup_d2[] =
{
(lte)4
    ,(lte)1 ,(lte)c1
    ,(lte)3 ,(lte)c3 ,(lte)b4 ,(lte)a5
    ,(lte)4 ,(lte)e3 ,(lte)f4 ,(lte)g5 ,(lte)h6
    ,(lte)1 ,(lte)e1
};
static const lte bishop_lookup_d3[] =
{
(lte)4
    ,(lte)2 ,(lte)c2 ,(lte)b1
    ,(lte)3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)4 ,(lte)e4 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)2 ,(lte)e2 ,(lte)f1
};
static const lte bishop_lookup_d4[] =
{
(lte)4
    ,(lte)3 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)3 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)4 ,(lte)e5 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)3 ,(lte)e3 ,(lte)f2 ,(lte)g1
};
static const lte bishop_lookup_d5[] =
{
(lte)4
    ,(lte)3 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)3 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)3 ,(lte)e6 ,(lte)f7 ,(lte)g8
    ,(lte)4 ,(lte)e4 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_d6[] =
{
(lte)4
    ,(lte)3 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)c7 ,(lte)b8
    ,(lte)2 ,(lte)e7 ,(lte)f8
    ,(lte)4 ,(lte)e5 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte bishop_lookup_d7[] =
{
(lte)4
    ,(lte)3 ,(lte)c6 ,(lte)b5 ,(lte)a4
    ,(lte)1 ,(lte)c8
    ,(lte)1 ,(lte)e8
    ,(lte)4 ,(lte)e6 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte bishop_lookup_d8[] =
{
(lte)2
    ,(lte)3 ,(lte)c7 ,(lte)b6 ,(lte)a5
    ,(lte)4 ,(lte)e7 ,(lte)f6 ,(lte)g5 ,(lte)h4
};
static const lte bishop_lookup_e1[] =
{
(lte)2
    ,(lte)4 ,(lte)d2 ,(lte)c3 ,(lte)b4 ,(lte)a5
    ,(lte)3 ,(lte)f2 ,(lte)g3 ,(lte)h4
};
static const lte bishop_lookup_e2[] =
{
(lte)4
    ,(lte)1 ,(lte)d1
    ,(lte)4 ,(lte)d3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)3 ,(lte)f3 ,(lte)g4 ,(lte)h5
    ,(lte)1 ,(lte)f1
};
static const lte bishop_lookup_e3[] =
{
(lte)4
    ,(lte)2 ,(lte)d2 ,(lte)c1
    ,(lte)4 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)3 ,(lte)f4 ,(lte)g5 ,(lte)h6
    ,(lte)2 ,(lte)f2 ,(lte)g1
};
static const lte bishop_lookup_e4[] =
{
(lte)4
    ,(lte)3 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)3 ,(lte)f5 ,(lte)g6 ,(lte)h7
    ,(lte)3 ,(lte)f3 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_e5[] =
{
(lte)4
    ,(lte)4 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)3 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)3 ,(lte)f6 ,(lte)g7 ,(lte)h8
    ,(lte)3 ,(lte)f4 ,(lte)g3 ,(lte)h2
};
static const lte bishop_lookup_e6[] =
{
(lte)4
    ,(lte)4 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)2 ,(lte)d7 ,(lte)c8
    ,(lte)2 ,(lte)f7 ,(lte)g8
    ,(lte)3 ,(lte)f5 ,(lte)g4 ,(lte)h3
};
static const lte bishop_lookup_e7[] =
{
(lte)4
    ,(lte)4 ,(lte)d6 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)1 ,(lte)d8
    ,(lte)1 ,(lte)f8
    ,(lte)3 ,(lte)f6 ,(lte)g5 ,(lte)h4
};
static const lte bishop_lookup_e8[] =
{
(lte)2
    ,(lte)4 ,(lte)d7 ,(lte)c6 ,(lte)b5 ,(lte)a4
    ,(lte)3 ,(lte)f7 ,(lte)g6 ,(lte)h5
};
static const lte bishop_lookup_f1[] =
{
(lte)2
    ,(lte)5 ,(lte)e2 ,(lte)d3 ,(lte)c4 ,(lte)b5 ,(lte)a6
    ,(lte)2 ,(lte)g2 ,(lte)h3
};
static const lte bishop_lookup_f2[] =
{
(lte)4
    ,(lte)1 ,(lte)e1
    ,(lte)5 ,(lte)e3 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)2 ,(lte)g3 ,(lte)h4
    ,(lte)1 ,(lte)g1
};
static const lte bishop_lookup_f3[] =
{
(lte)4
    ,(lte)2 ,(lte)e2 ,(lte)d1
    ,(lte)5 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)2 ,(lte)g4 ,(lte)h5
    ,(lte)2 ,(lte)g2 ,(lte)h1
};
static const lte bishop_lookup_f4[] =
{
(lte)4
    ,(lte)3 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)2 ,(lte)g5 ,(lte)h6
    ,(lte)2 ,(lte)g3 ,(lte)h2
};
static const lte bishop_lookup_f5[] =
{
(lte)4
    ,(lte)4 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)3 ,(lte)e6 ,(lte)d7 ,(lte)c8
    ,(lte)2 ,(lte)g6 ,(lte)h7
    ,(lte)2 ,(lte)g4 ,(lte)h3
};
static const lte bishop_lookup_f6[] =
{
(lte)4
    ,(lte)5 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)2 ,(lte)e7 ,(lte)d8
    ,(lte)2 ,(lte)g7 ,(lte)h8
    ,(lte)2 ,(lte)g5 ,(lte)h4
};
static const lte bishop_lookup_f7[] =
{
(lte)4
    ,(lte)5 ,(lte)e6 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)1 ,(lte)e8
    ,(lte)1 ,(lte)g8
    ,(lte)2 ,(lte)g6 ,(lte)h5
};
static const lte bishop_lookup_f8[] =
{
(lte)2
    ,(lte)5 ,(lte)e7 ,(lte)d6 ,(lte)c5 ,(lte)b4 ,(lte)a3
    ,(lte)2 ,(lte)g7 ,(lte)h6
};
static const lte bishop_lookup_g1[] =
{
(lte)2
    ,(lte)6 ,(lte)f2 ,(lte)e3 ,(lte)d4 ,(lte)c5 ,(lte)b6 ,(lte)a7
    ,(lte)1 ,(lte)h2
};
static const lte bishop_lookup_g2[] =
{
(lte)4
    ,(lte)1 ,(lte)f1
    ,(lte)6 ,(lte)f3 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
    ,(lte)1 ,(lte)h3
    ,(lte)1 ,(lte)h1
};
static const lte bishop_lookup_g3[] =
{
(lte)4
    ,(lte)2 ,(lte)f2 ,(lte)e1
    ,(lte)5 ,(lte)f4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
    ,(lte)1 ,(lte)h4
    ,(lte)1 ,(lte)h2
};
static const lte bishop_lookup_g4[] =
{
(lte)4
    ,(lte)3 ,(lte)f3 ,(lte)e2 ,(lte)d1
    ,(lte)4 ,(lte)f5 ,(lte)e6 ,(lte)d7 ,(lte)c8
    ,(lte)1 ,(lte)h5
    ,(lte)1 ,(lte)h3
};
static const lte bishop_lookup_g5[] =
{
(lte)4
    ,(lte)4 ,(lte)f4 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)3 ,(lte)f6 ,(lte)e7 ,(lte)d8
    ,(lte)1 ,(lte)h6
    ,(lte)1 ,(lte)h4
};
static const lte bishop_lookup_g6[] =
{
(lte)4
    ,(lte)5 ,(lte)f5 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)2 ,(lte)f7 ,(lte)e8
    ,(lte)1 ,(lte)h7
    ,(lte)1 ,(lte)h5
};
static const lte bishop_lookup_g7[] =
{
(lte)4
    ,(lte)6 ,(lte)f6 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
    ,(lte)1 ,(lte)f8
    ,(lte)1 ,(lte)h8
    ,(lte)1 ,(lte)h6
};
static const lte bishop_lookup_g8[] =
{
(lte)2
    ,(lte)6 ,(lte)f7 ,(lte)e6 ,(lte)d5 ,(lte)c4 ,(lte)b3 ,(lte)a2
    ,(lte)1 ,(lte)h7
};
static const lte bishop_lookup_h1[] =
{
(lte)1
    ,(lte)7 ,(lte)g2 ,(lte)f3 ,(lte)e4 ,(lte)d5 ,(lte)c6 ,(lte)b7 ,(lte)a8
};
static const lte bishop_lookup_h2[] =
{
(lte)2
    ,(lte)1 ,(lte)g1
    ,(lte)6 ,(lte)g3 ,(lte)f4 ,(lte)e5 ,(lte)d6 ,(lte)c7 ,(lte)b8
};
static const lte bishop_lookup_h3[] =
{
(lte)2
    ,(lte)2 ,(lte)g2 ,(lte)f1
    ,(lte)5 ,(lte)g4 ,(lte)f5 ,(lte)e6 ,(lte)d7 ,(lte)c8
};
static const lte bishop_lookup_h4[] =
{
(lte)2
    ,(lte)3 ,(lte)g3 ,(lte)f2 ,(lte)e1
    ,(lte)4 ,(lte)g5 ,(lte)f6 ,(lte)e7 ,(lte)d8
};
static const lte bishop_lookup_h5[] =
{
(lte)2
    ,(lte)4 ,(lte)g4 ,(lte)f3 ,(lte)e2 ,(lte)d1
    ,(lte)3 ,(lte)g6 ,(lte)f7 ,(lte)e8
};
static const lte bishop_lookup_h6[] =
{
(lte)2
    ,(lte)5 ,(lte)g5 ,(lte)f4 ,(lte)e3 ,(lte)d2 ,(lte)c1
    ,(lte)2 ,(lte)g7 ,(lte)f8
};
static const lte bishop_lookup_h7[] =
{
(lte)2
    ,(lte)6 ,(lte)g6 ,(lte)f5 ,(lte)e4 ,(lte)d3 ,(lte)c2 ,(lte)b1
    ,(lte)1 ,(lte)g8
};
static const lte bishop_lookup_h8[] =
{
(lte)1
    ,(lte)7 ,(lte)g7 ,(lte)f6 ,(lte)e5 ,(lte)d4 ,(lte)c3 ,(lte)b2 ,(lte)a1
};

// bishop_lookup
const lte *bishop_lookup[] =
{
    bishop_lookup_a8,
    bishop_lookup_b8,
    bishop_lookup_c8,
    bishop_lookup_d8,
    bishop_lookup_e8,
    bishop_lookup_f8,
    bishop_lookup_g8,
    bishop_lookup_h8,
    bishop_lookup_a7,
    bishop_lookup_b7,
    bishop_lookup_c7,
    bishop_lookup_d7,
    bishop_lookup_e7,
    bishop_lookup_f7,
    bishop_lookup_g7,
    bishop_lookup_h7,
    bishop_lookup_a6,
    bishop_lookup_b6,
    bishop_lookup_c6,
    bishop_lookup_d6,
    bishop_lookup_e6,
    bishop_lookup_f6,
    bishop_lookup_g6,
    bishop_lookup_h6,
    bishop_lookup_a5,
    bishop_lookup_b5,
    bishop_lookup_c5,
    bishop_lookup_d5,
    bishop_lookup_e5,
    bishop_lookup_f5,
    bishop_lookup_g5,
    bishop_lookup_h5,
    bishop_lookup_a4,
    bishop_lookup_b4,
    bishop_lookup_c4,
    bishop_lookup_d4,
    bishop_lookup_e4,
    bishop_lookup_f4,
    bishop_lookup_g4,
    bishop_lookup_h4,
    bishop_lookup_a3,
    bishop_lookup_b3,
    bishop_lookup_c3,
    bishop_lookup_d3,
    bishop_lookup_e3,
    bishop_lookup_f3,
    bishop_lookup_g3,
    bishop_lookup_h3,
    bishop_lookup_a2,
    bishop_lookup_b2,
    bishop_lookup_c2,
    bishop_lookup_d2,
    bishop_lookup_e2,
    bishop_lookup_f2,
    bishop_lookup_g2,
    bishop_lookup_h2,
    bishop_lookup_a1,
    bishop_lookup_b1,
    bishop_lookup_c1,
    bishop_lookup_d1,
    bishop_lookup_e1,
    bishop_lookup_f1,
    bishop_lookup_g1,
    bishop_lookup_h1
};

// Knight, up to 8 squares
static const lte knight_lookup_a1[] =
{
    (lte)2, (lte)c2, (lte)b3
};
static const lte knight_lookup_a2[] =
{
    (lte)3, (lte)c1, (lte)c3, (lte)b4
};
static const lte knight_lookup_a3[] =
{
    (lte)4, (lte)c2, (lte)c4, (lte)b1, (lte)b5
};
static const lte knight_lookup_a4[] =
{
    (lte)4, (lte)c3, (lte)c5, (lte)b2, (lte)b6
};
static const lte knight_lookup_a5[] =
{
    (lte)4, (lte)c4, (lte)c6, (lte)b3, (lte)b7
};
static const lte knight_lookup_a6[] =
{
    (lte)4, (lte)c5, (lte)c7, (lte)b4, (lte)b8
};
static const lte knight_lookup_a7[] =
{
    (lte)3, (lte)c6, (lte)c8, (lte)b5
};
static const lte knight_lookup_a8[] =
{
    (lte)2, (lte)c7, (lte)b6
};
static const lte knight_lookup_b1[] =
{
    (lte)3, (lte)a3, (lte)d2, (lte)c3
};
static const lte knight_lookup_b2[] =
{
    (lte)4, (lte)a4, (lte)d1, (lte)d3, (lte)c4
};
static const lte knight_lookup_b3[] =
{
    (lte)6, (lte)a1, (lte)a5, (lte)d2, (lte)d4, (lte)c1, (lte)c5
};
static const lte knight_lookup_b4[] =
{
    (lte)6, (lte)a2, (lte)a6, (lte)d3, (lte)d5, (lte)c2, (lte)c6
};
static const lte knight_lookup_b5[] =
{
    (lte)6, (lte)a3, (lte)a7, (lte)d4, (lte)d6, (lte)c3, (lte)c7
};
static const lte knight_lookup_b6[] =
{
    (lte)6, (lte)a4, (lte)a8, (lte)d5, (lte)d7, (lte)c4, (lte)c8
};
static const lte knight_lookup_b7[] =
{
    (lte)4, (lte)a5, (lte)d6, (lte)d8, (lte)c5
};
static const lte knight_lookup_b8[] =
{
    (lte)3, (lte)a6, (lte)d7, (lte)c6
};
static const lte knight_lookup_c1[] =
{
    (lte)4, (lte)a2, (lte)b3, (lte)e2, (lte)d3
};
static const lte knight_lookup_c2[] =
{
    (lte)6, (lte)a1, (lte)a3, (lte)b4, (lte)e1, (lte)e3, (lte)d4
};
static const lte knight_lookup_c3[] =
{
    (lte)8, (lte)a2, (lte)a4, (lte)b1, (lte)b5, (lte)e2, (lte)e4, (lte)d1, (lte)d5
};
static const lte knight_lookup_c4[] =
{
    (lte)8, (lte)a3, (lte)a5, (lte)b2, (lte)b6, (lte)e3, (lte)e5, (lte)d2, (lte)d6
};
static const lte knight_lookup_c5[] =
{
    (lte)8, (lte)a4, (lte)a6, (lte)b3, (lte)b7, (lte)e4, (lte)e6, (lte)d3, (lte)d7
};
static const lte knight_lookup_c6[] =
{
    (lte)8, (lte)a5, (lte)a7, (lte)b4, (lte)b8, (lte)e5, (lte)e7, (lte)d4, (lte)d8
};
static const lte knight_lookup_c7[] =
{
    (lte)6, (lte)a6, (lte)a8, (lte)b5, (lte)e6, (lte)e8, (lte)d5
};
static const lte knight_lookup_c8[] =
{
    (lte)4, (lte)a7, (lte)b6, (lte)e7, (lte)d6
};
static const lte knight_lookup_d1[] =
{
    (lte)4, (lte)b2, (lte)c3, (lte)f2, (lte)e3
};
static const lte knight_lookup_d2[] =
{
    (lte)6, (lte)b1, (lte)b3, (lte)c4, (lte)f1, (lte)f3, (lte)e4
};
static const lte knight_lookup_d3[] =
{
    (lte)8, (lte)b2, (lte)b4, (lte)c1, (lte)c5, (lte)f2, (lte)f4, (lte)e1, (lte)e5
};
static const lte knight_lookup_d4[] =
{
    (lte)8, (lte)b3, (lte)b5, (lte)c2, (lte)c6, (lte)f3, (lte)f5, (lte)e2, (lte)e6
};
static const lte knight_lookup_d5[] =
{
    (lte)8, (lte)b4, (lte)b6, (lte)c3, (lte)c7, (lte)f4, (lte)f6, (lte)e3, (lte)e7
};
static const lte knight_lookup_d6[] =
{
    (lte)8, (lte)b5, (lte)b7, (lte)c4, (lte)c8, (lte)f5, (lte)f7, (lte)e4, (lte)e8
};
static const lte knight_lookup_d7[] =
{
    (lte)6, (lte)b6, (lte)b8, (lte)c5, (lte)f6, (lte)f8, (lte)e5
};
static const lte knight_lookup_d8[] =
{
    (lte)4, (lte)b7, (lte)c6, (lte)f7, (lte)e6
};
static const lte knight_lookup_e1[] =
{
    (lte)4, (lte)c2, (lte)d3, (lte)g2, (lte)f3
};
static const lte knight_lookup_e2[] =
{
    (lte)6, (lte)c1, (lte)c3, (lte)d4, (lte)g1, (lte)g3, (lte)f4
};
static const lte knight_lookup_e3[] =
{
    (lte)8, (lte)c2, (lte)c4, (lte)d1, (lte)d5, (lte)g2, (lte)g4, (lte)f1, (lte)f5
};
static const lte knight_lookup_e4[] =
{
    (lte)8, (lte)c3, (lte)c5, (lte)d2, (lte)d6, (lte)g3, (lte)g5, (lte)f2, (lte)f6
};
static const lte knight_lookup_e5[] =
{
    (lte)8, (lte)c4, (lte)c6, (lte)d3, (lte)d7, (lte)g4, (lte)g6, (lte)f3, (lte)f7
};
static const lte knight_lookup_e6[] =
{
    (lte)8, (lte)c5, (lte)c7, (lte)d4, (lte)d8, (lte)g5, (lte)g7, (lte)f4, (lte)f8
};
static const lte knight_lookup_e7[] =
{
    (lte)6, (lte)c6, (lte)c8, (lte)d5, (lte)g6, (lte)g8, (lte)f5
};
static const lte knight_lookup_e8[] =
{
    (lte)4, (lte)c7, (lte)d6, (lte)g7, (lte)f6
};
static const lte knight_lookup_f1[] =
{
    (lte)4, (lte)d2, (lte)e3, (lte)h2, (lte)g3
};
static const lte knight_lookup_f2[] =
{
    (lte)6, (lte)d1, (lte)d3, (lte)e4, (lte)h1, (lte)h3, (lte)g4
};
static const lte knight_lookup_f3[] =
{
    (lte)8, (lte)d2, (lte)d4, (lte)e1, (lte)e5, (lte)h2, (lte)h4, (lte)g1, (lte)g5
};
static const lte knight_lookup_f4[] =
{
    (lte)8, (lte)d3, (lte)d5, (lte)e2, (lte)e6, (lte)h3, (lte)h5, (lte)g2, (lte)g6
};
static const lte knight_lookup_f5[] =
{
    (lte)8, (lte)d4, (lte)d6, (lte)e3, (lte)e7, (lte)h4, (lte)h6, (lte)g3, (lte)g7
};
static const lte knight_lookup_f6[] =
{
    (lte)8, (lte)d5, (lte)d7, (lte)e4, (lte)e8, (lte)h5, (lte)h7, (lte)g4, (lte)g8
};
static const lte knight_lookup_f7[] =
{
    (lte)6, (lte)d6, (lte)d8, (lte)e5, (lte)h6, (lte)h8, (lte)g5
};
static const lte knight_lookup_f8[] =
{
    (lte)4, (lte)d7, (lte)e6, (lte)h7, (lte)g6
};
static const lte knight_lookup_g1[] =
{
    (lte)3, (lte)e2, (lte)f3, (lte)h3
};
static const lte knight_lookup_g2[] =
{
    (lte)4, (lte)e1, (lte)e3, (lte)f4, (lte)h4
};
static const lte knight_lookup_g3[] =
{
    (lte)6, (lte)e2, (lte)e4, (lte)f1, (lte)f5, (lte)h1, (lte)h5
};
static const lte knight_lookup_g4[] =
{
    (lte)6, (lte)e3, (lte)e5, (lte)f2, (lte)f6, (lte)h2, (lte)h6
};
static const lte knight_lookup_g5[] =
{
    (lte)6, (lte)e4, (lte)e6, (lte)f3, (lte)f7, (lte)h3, (lte)h7
};
static const lte knight_lookup_g6[] =
{
    (lte)6, (lte)e5, (lte)e7, (lte)f4, (lte)f8, (lte)h4, (lte)h8
};
static const lte knight_lookup_g7[] =
{
    (lte)4, (lte)e6, (lte)e8, (lte)f5, (lte)h5
};
static const lte knight_lookup_g8[] =
{
    (lte)3, (lte)e7, (lte)f6, (lte)h6
};
static const lte knight_lookup_h1[] =
{
    (lte)2, (lte)f2, (lte)g3
};
static const lte knight_lookup_h2[] =
{
    (lte)3, (lte)f1, (lte)f3, (lte)g4
};
static const lte knight_lookup_h3[] =
{
    (lte)4, (lte)f2, (lte)f4, (lte)g1, (lte)g5
};
static const lte knight_lookup_h4[] =
{
    (lte)4, (lte)f3, (lte)f5, (lte)g2, (lte)g6
};
static const lte knight_lookup_h5[] =
{
    (lte)4, (lte)f4, (lte)f6, (lte)g3, (lte)g7
};
static const lte knight_lookup_h6[] =
{
    (lte)4, (lte)f5, (lte)f7, (lte)g4, (lte)g8
};
static const lte knight_lookup_h7[] =
{
    (lte)3, (lte)f6, (lte)f8, (lte)g5
};
static const lte knight_lookup_h8[] =
{
    (lte)2, (lte)f7, (lte)g6
};

// knight_lookup
const lte *knight_lookup[] =
{
    knight_lookup_a8,
    knight_lookup_b8,
    knight_lookup_c8,
    knight_lookup_d8,
    knight_lookup_e8,
    knight_lookup_f8,
    knight_lookup_g8,
    knight_lookup_h8,
    knight_lookup_a7,
    knight_lookup_b7,
    knight_lookup_c7,
    knight_lookup_d7,
    knight_lookup_e7,
    knight_lookup_f7,
    knight_lookup_g7,
    knight_lookup_h7,
    knight_lookup_a6,
    knight_lookup_b6,
    knight_lookup_c6,
    knight_lookup_d6,
    knight_lookup_e6,
    knight_lookup_f6,
    knight_lookup_g6,
    knight_lookup_h6,
    knight_lookup_a5,
    knight_lookup_b5,
    knight_lookup_c5,
    knight_lookup_d5,
    knight_lookup_e5,
    knight_lookup_f5,
    knight_lookup_g5,
    knight_lookup_h5,
    knight_lookup_a4,
    knight_lookup_b4,
    knight_lookup_c4,
    knight_lookup_d4,
    knight_lookup_e4,
    knight_lookup_f4,
    knight_lookup_g4,
    knight_lookup_h4,
    knight_lookup_a3,
    knight_lookup_b3,
    knight_lookup_c3,
    knight_lookup_d3,
    knight_lookup_e3,
    knight_lookup_f3,
    knight_lookup_g3,
    knight_lookup_h3,
    knight_lookup_a2,
    knight_lookup_b2,
    knight_lookup_c2,
    knight_lookup_d2,
    knight_lookup_e2,
    knight_lookup_f2,
    knight_lookup_g2,
    knight_lookup_h2,
    knight_lookup_a1,
    knight_lookup_b1,
    knight_lookup_c1,
    knight_lookup_d1,
    knight_lookup_e1,
    knight_lookup_f1,
    knight_lookup_g1,
    knight_lookup_h1
};

// King, up to 8 squares
static const lte king_lookup_a1[] =
{
    (lte)3, (lte)b2, (lte)b1, (lte)a2
};
static const lte king_lookup_a2[] =
{
    (lte)5, (lte)b1, (lte)b3, (lte)b2, (lte)a1, (lte)a3
};
static const lte king_lookup_a3[] =
{
    (lte)5, (lte)b2, (lte)b4, (lte)b3, (lte)a2, (lte)a4
};
static const lte king_lookup_a4[] =
{
    (lte)5, (lte)b3, (lte)b5, (lte)b4, (lte)a3, (lte)a5
};
static const lte king_lookup_a5[] =
{
    (lte)5, (lte)b4, (lte)b6, (lte)b5, (lte)a4, (lte)a6
};
static const lte king_lookup_a6[] =
{
    (lte)5, (lte)b5, (lte)b7, (lte)b6, (lte)a5, (lte)a7
};
static const lte king_lookup_a7[] =
{
    (lte)5, (lte)b6, (lte)b8, (lte)b7, (lte)a6, (lte)a8
};
static const lte king_lookup_a8[] =
{
    (lte)3, (lte)b7, (lte)b8, (lte)a7
};
static const lte king_lookup_b1[] =
{
    (lte)5, (lte)a2, (lte)c2, (lte)a1, (lte)c1, (lte)b2
};
static const lte king_lookup_b2[] =
{
    (lte)8, (lte)a1, (lte)a3, (lte)c1, (lte)c3, (lte)a2, (lte)c2, (lte)b1, (lte)b3
};
static const lte king_lookup_b3[] =
{
    (lte)8, (lte)a2, (lte)a4, (lte)c2, (lte)c4, (lte)a3, (lte)c3, (lte)b2, (lte)b4
};
static const lte king_lookup_b4[] =
{
    (lte)8, (lte)a3, (lte)a5, (lte)c3, (lte)c5, (lte)a4, (lte)c4, (lte)b3, (lte)b5
};
static const lte king_lookup_b5[] =
{
    (lte)8, (lte)a4, (lte)a6, (lte)c4, (lte)c6, (lte)a5, (lte)c5, (lte)b4, (lte)b6
};
static const lte king_lookup_b6[] =
{
    (lte)8, (lte)a5, (lte)a7, (lte)c5, (lte)c7, (lte)a6, (lte)c6, (lte)b5, (lte)b7
};
static const lte king_lookup_b7[] =
{
    (lte)8, (lte)a6, (lte)a8, (lte)c6, (lte)c8, (lte)a7, (lte)c7, (lte)b6, (lte)b8
};
static const lte king_lookup_b8[] =
{
    (lte)5, (lte)a7, (lte)c7, (lte)a8, (lte)c8, (lte)b7
};
static const lte king_lookup_c1[] =
{
    (lte)5, (lte)b2, (lte)d2, (lte)b1, (lte)d1, (lte)c2
};
static const lte king_lookup_c2[] =
{
    (lte)8, (lte)b1, (lte)b3, (lte)d1, (lte)d3, (lte)b2, (lte)d2, (lte)c1, (lte)c3
};
static const lte king_lookup_c3[] =
{
    (lte)8, (lte)b2, (lte)b4, (lte)d2, (lte)d4, (lte)b3, (lte)d3, (lte)c2, (lte)c4
};
static const lte king_lookup_c4[] =
{
    (lte)8, (lte)b3, (lte)b5, (lte)d3, (lte)d5, (lte)b4, (lte)d4, (lte)c3, (lte)c5
};
static const lte king_lookup_c5[] =
{
    (lte)8, (lte)b4, (lte)b6, (lte)d4, (lte)d6, (lte)b5, (lte)d5, (lte)c4, (lte)c6
};
static const lte king_lookup_c6[] =
{
    (lte)8, (lte)b5, (lte)b7, (lte)d5, (lte)d7, (lte)b6, (lte)d6, (lte)c5, (lte)c7
};
static const lte king_lookup_c7[] =
{
    (lte)8, (lte)b6, (lte)b8, (lte)d6, (lte)d8, (lte)b7, (lte)d7, (lte)c6, (lte)c8
};
static const lte king_lookup_c8[] =
{
    (lte)5, (lte)b7, (lte)d7, (lte)b8, (lte)d8, (lte)c7
};
static const lte king_lookup_d1[] =
{
    (lte)5, (lte)c2, (lte)e2, (lte)c1, (lte)e1, (lte)d2
};
static const lte king_lookup_d2[] =
{
    (lte)8, (lte)c1, (lte)c3, (lte)e1, (lte)e3, (lte)c2, (lte)e2, (lte)d1, (lte)d3
};
static const lte king_lookup_d3[] =
{
    (lte)8, (lte)c2, (lte)c4, (lte)e2, (lte)e4, (lte)c3, (lte)e3, (lte)d2, (lte)d4
};
static const lte king_lookup_d4[] =
{
    (lte)8, (lte)c3, (lte)c5, (lte)e3, (lte)e5, (lte)c4, (lte)e4, (lte)d3, (lte)d5
};
static const lte king_lookup_d5[] =
{
    (lte)8, (lte)c4, (lte)c6, (lte)e4, (lte)e6, (lte)c5, (lte)e5, (lte)d4, (lte)d6
};
static const lte king_lookup_d6[] =
{
    (lte)8, (lte)c5, (lte)c7, (lte)e5, (lte)e7, (lte)c6, (lte)e6, (lte)d5, (lte)d7
};
static const lte king_lookup_d7[] =
{
    (lte)8, (lte)c6, (lte)c8, (lte)e6, (lte)e8, (lte)c7, (lte)e7, (lte)d6, (lte)d8
};
static const lte king_lookup_d8[] =
{
    (lte)5, (lte)c7, (lte)e7, (lte)c8, (lte)e8, (lte)d7
};
static const lte king_lookup_e1[] =
{
    (lte)5, (lte)d2, (lte)f2, (lte)d1, (lte)f1, (lte)e2
};
static const lte king_lookup_e2[] =
{
    (lte)8, (lte)d1, (lte)d3, (lte)f1, (lte)f3, (lte)d2, (lte)f2, (lte)e1, (lte)e3
};
static const lte king_lookup_e3[] =
{
    (lte)8, (lte)d2, (lte)d4, (lte)f2, (lte)f4, (lte)d3, (lte)f3, (lte)e2, (lte)e4
};
static const lte king_lookup_e4[] =
{
    (lte)8, (lte)d3, (lte)d5, (lte)f3, (lte)f5, (lte)d4, (lte)f4, (lte)e3, (lte)e5
};
static const lte king_lookup_e5[] =
{
    (lte)8, (lte)d4, (lte)d6, (lte)f4, (lte)f6, (lte)d5, (lte)f5, (lte)e4, (lte)e6
};
static const lte king_lookup_e6[] =
{
    (lte)8, (lte)d5, (lte)d7, (lte)f5, (lte)f7, (lte)d6, (lte)f6, (lte)e5, (lte)e7
};
static const lte king_lookup_e7[] =
{
    (lte)8, (lte)d6, (lte)d8, (lte)f6, (lte)f8, (lte)d7, (lte)f7, (lte)e6, (lte)e8
};
static const lte king_lookup_e8[] =
{
    (lte)5, (lte)d7, (lte)f7, (lte)d8, (lte)f8, (lte)e7
};
static const lte king_lookup_f1[] =
{
    (lte)5, (lte)e2, (lte)g2, (lte)e1, (lte)g1, (lte)f2
};
static const lte king_lookup_f2[] =
{
    (lte)8, (lte)e1, (lte)e3, (lte)g1, (lte)g3, (lte)e2, (lte)g2, (lte)f1, (lte)f3
};
static const lte king_lookup_f3[] =
{
    (lte)8, (lte)e2, (lte)e4, (lte)g2, (lte)g4, (lte)e3, (lte)g3, (lte)f2, (lte)f4
};
static const lte king_lookup_f4[] =
{
    (lte)8, (lte)e3, (lte)e5, (lte)g3, (lte)g5, (lte)e4, (lte)g4, (lte)f3, (lte)f5
};
static const lte king_lookup_f5[] =
{
    (lte)8, (lte)e4, (lte)e6, (lte)g4, (lte)g6, (lte)e5, (lte)g5, (lte)f4, (lte)f6
};
static const lte king_lookup_f6[] =
{
    (lte)8, (lte)e5, (lte)e7, (lte)g5, (lte)g7, (lte)e6, (lte)g6, (lte)f5, (lte)f7
};
static const lte king_lookup_f7[] =
{
    (lte)8, (lte)e6, (lte)e8, (lte)g6, (lte)g8, (lte)e7, (lte)g7, (lte)f6, (lte)f8
};
static const lte king_lookup_f8[] =
{
    (lte)5, (lte)e7, (lte)g7, (lte)e8, (lte)g8, (lte)f7
};
static const lte king_lookup_g1[] =
{
    (lte)5, (lte)f2, (lte)h2, (lte)f1, (lte)h1, (lte)g2
};
static const lte king_lookup_g2[] =
{
    (lte)8, (lte)f1, (lte)f3, (lte)h1, (lte)h3, (lte)f2, (lte)h2, (lte)g1, (lte)g3
};
static const lte king_lookup_g3[] =
{
    (lte)8, (lte)f2, (lte)f4, (lte)h2, (lte)h4, (lte)f3, (lte)h3, (lte)g2, (lte)g4
};
static const lte king_lookup_g4[] =
{
    (lte)8, (lte)f3, (lte)f5, (lte)h3, (lte)h5, (lte)f4, (lte)h4, (lte)g3, (lte)g5
};
static const lte king_lookup_g5[] =
{
    (lte)8, (lte)f4, (lte)f6, (lte)h4, (lte)h6, (lte)f5, (lte)h5, (lte)g4, (lte)g6
};
static const lte king_lookup_g6[] =
{
    (lte)8, (lte)f5, (lte)f7, (lte)h5, (lte)h7, (lte)f6, (lte)h6, (lte)g5, (lte)g7
};
static const lte king_lookup_g7[] =
{
    (lte)8, (lte)f6, (lte)f8, (lte)h6, (lte)h8, (lte)f7, (lte)h7, (lte)g6, (lte)g8
};
static const lte king_lookup_g8[] =
{
    (lte)5, (lte)f7, (lte)h7, (lte)f8, (lte)h8, (lte)g7
};
static const lte king_lookup_h1[] =
{
    (lte)3, (lte)g2, (lte)g1, (lte)h2
};
static const lte king_lookup_h2[] =
{
    (lte)5, (lte)g1, (lte)g3, (lte)g2, (lte)h1, (lte)h3
};
static const lte king_lookup_h3[] =
{
    (lte)5, (lte)g2, (lte)g4, (lte)g3, (lte)h2, (lte)h4
};
static const lte king_lookup_h4[] =
{
    (lte)5, (lte)g3, (lte)g5, (lte)g4, (lte)h3, (lte)h5
};
static const lte king_lookup_h5[] =
{
    (lte)5, (lte)g4, (lte)g6, (lte)g5, (lte)h4, (lte)h6
};
static const lte king_lookup_h6[] =
{
    (lte)5, (lte)g5, (lte)g7, (lte)g6, (lte)h5, (lte)h7
};
static const lte king_lookup_h7[] =
{
    (lte)5, (lte)g6, (lte)g8, (lte)g7, (lte)h6, (lte)h8
};
static const lte king_lookup_h8[] =
{
    (lte)3, (lte)g7, (lte)g8, (lte)h7
};

// king_lookup
const lte *king_lookup[] =
{
    king_lookup_a8,
    king_lookup_b8,
    king_lookup_c8,
    king_lookup_d8,
    king_lookup_e8,
    king_lookup_f8,
    king_lookup_g8,
    king_lookup_h8,
    king_lookup_a7,
    king_lookup_b7,
    king_lookup_c7,
    king_lookup_d7,
    king_lookup_e7,
    king_lookup_f7,
    king_lookup_g7,
    king_lookup_h7,
    king_lookup_a6,
    king_lookup_b6,
    king_lookup_c6,
    king_lookup_d6,
    king_lookup_e6,
    king_lookup_f6,
    king_lookup_g6,
    king_lookup_h6,
    king_lookup_a5,
    king_lookup_b5,
    king_lookup_c5,
    king_lookup_d5,
    king_lookup_e5,
    king_lookup_f5,
    king_lookup_g5,
    king_lookup_h5,
    king_lookup_a4,
    king_lookup_b4,
    king_lookup_c4,
    king_lookup_d4,
    king_lookup_e4,
    king_lookup_f4,
    king_lookup_g4,
    king_lookup_h4,
    king_lookup_a3,
    king_lookup_b3,
    king_lookup_c3,
    king_lookup_d3,
    king_lookup_e3,
    king_lookup_f3,
    king_lookup_g3,
    king_lookup_h3,
    king_lookup_a2,
    king_lookup_b2,
    king_lookup_c2,
    king_lookup_d2,
    king_lookup_e2,
    king_lookup_f2,
    king_lookup_g2,
    king_lookup_h2,
    king_lookup_a1,
    king_lookup_b1,
    king_lookup_c1,
    king_lookup_d1,
    king_lookup_e1,
    king_lookup_f1,
    king_lookup_g1,
    king_lookup_h1
};

// White pawn, capture ray followed by advance ray
static const lte pawn_white_lookup_a1[] =
{
    (lte)1, (lte)b2,
    (lte)1, (lte)a2
};
static const lte pawn_white_lookup_a2[] =
{
    (lte)1, (lte)b3,
    (lte)2, (lte)a3, (lte)a4
};
static const lte pawn_white_lookup_a3[] =
{
    (lte)1, (lte)b4,
    (lte)1, (lte)a4
};
static const lte pawn_white_lookup_a4[] =
{
    (lte)1, (lte)b5,
    (lte)1, (lte)a5
};
static const lte pawn_white_lookup_a5[] =
{
    (lte)1, (lte)b6,
    (lte)1, (lte)a6
};
static const lte pawn_white_lookup_a6[] =
{
    (lte)1, (lte)b7,
    (lte)1, (lte)a7
};
static const lte pawn_white_lookup_a7[] =
{
    (lte)1, (lte)b8,
    (lte)1, (lte)a8
};
static const lte pawn_white_lookup_a8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_b1[] =
{
    (lte)2, (lte)a2, (lte)c2,
    (lte)1, (lte)b2
};
static const lte pawn_white_lookup_b2[] =
{
    (lte)2, (lte)a3, (lte)c3,
    (lte)2, (lte)b3, (lte)b4
};
static const lte pawn_white_lookup_b3[] =
{
    (lte)2, (lte)a4, (lte)c4,
    (lte)1, (lte)b4
};
static const lte pawn_white_lookup_b4[] =
{
    (lte)2, (lte)a5, (lte)c5,
    (lte)1, (lte)b5
};
static const lte pawn_white_lookup_b5[] =
{
    (lte)2, (lte)a6, (lte)c6,
    (lte)1, (lte)b6
};
static const lte pawn_white_lookup_b6[] =
{
    (lte)2, (lte)a7, (lte)c7,
    (lte)1, (lte)b7
};
static const lte pawn_white_lookup_b7[] =
{
    (lte)2, (lte)a8, (lte)c8,
    (lte)1, (lte)b8
};
static const lte pawn_white_lookup_b8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_c1[] =
{
    (lte)2, (lte)b2, (lte)d2,
    (lte)1, (lte)c2
};
static const lte pawn_white_lookup_c2[] =
{
    (lte)2, (lte)b3, (lte)d3,
    (lte)2, (lte)c3, (lte)c4
};
static const lte pawn_white_lookup_c3[] =
{
    (lte)2, (lte)b4, (lte)d4,
    (lte)1, (lte)c4
};
static const lte pawn_white_lookup_c4[] =
{
    (lte)2, (lte)b5, (lte)d5,
    (lte)1, (lte)c5
};
static const lte pawn_white_lookup_c5[] =
{
    (lte)2, (lte)b6, (lte)d6,
    (lte)1, (lte)c6
};
static const lte pawn_white_lookup_c6[] =
{
    (lte)2, (lte)b7, (lte)d7,
    (lte)1, (lte)c7
};
static const lte pawn_white_lookup_c7[] =
{
    (lte)2, (lte)b8, (lte)d8,
    (lte)1, (lte)c8
};
static const lte pawn_white_lookup_c8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_d1[] =
{
    (lte)2, (lte)c2, (lte)e2,
    (lte)1, (lte)d2
};
static const lte pawn_white_lookup_d2[] =
{
    (lte)2, (lte)c3, (lte)e3,
    (lte)2, (lte)d3, (lte)d4
};
static const lte pawn_white_lookup_d3[] =
{
    (lte)2, (lte)c4, (lte)e4,
    (lte)1, (lte)d4
};
static const lte pawn_white_lookup_d4[] =
{
    (lte)2, (lte)c5, (lte)e5,
    (lte)1, (lte)d5
};
static const lte pawn_white_lookup_d5[] =
{
    (lte)2, (lte)c6, (lte)e6,
    (lte)1, (lte)d6
};
static const lte pawn_white_lookup_d6[] =
{
    (lte)2, (lte)c7, (lte)e7,
    (lte)1, (lte)d7
};
static const lte pawn_white_lookup_d7[] =
{
    (lte)2, (lte)c8, (lte)e8,
    (lte)1, (lte)d8
};
static const lte pawn_white_lookup_d8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_e1[] =
{
    (lte)2, (lte)d2, (lte)f2,
    (lte)1, (lte)e2
};
static const lte pawn_white_lookup_e2[] =
{
    (lte)2, (lte)d3, (lte)f3,
    (lte)2, (lte)e3, (lte)e4
};
static const lte pawn_white_lookup_e3[] =
{
    (lte)2, (lte)d4, (lte)f4,
    (lte)1, (lte)e4
};
static const lte pawn_white_lookup_e4[] =
{
    (lte)2, (lte)d5, (lte)f5,
    (lte)1, (lte)e5
};
static const lte pawn_white_lookup_e5[] =
{
    (lte)2, (lte)d6, (lte)f6,
    (lte)1, (lte)e6
};
static const lte pawn_white_lookup_e6[] =
{
    (lte)2, (lte)d7, (lte)f7,
    (lte)1, (lte)e7
};
static const lte pawn_white_lookup_e7[] =
{
    (lte)2, (lte)d8, (lte)f8,
    (lte)1, (lte)e8
};
static const lte pawn_white_lookup_e8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_f1[] =
{
    (lte)2, (lte)e2, (lte)g2,
    (lte)1, (lte)f2
};
static const lte pawn_white_lookup_f2[] =
{
    (lte)2, (lte)e3, (lte)g3,
    (lte)2, (lte)f3, (lte)f4
};
static const lte pawn_white_lookup_f3[] =
{
    (lte)2, (lte)e4, (lte)g4,
    (lte)1, (lte)f4
};
static const lte pawn_white_lookup_f4[] =
{
    (lte)2, (lte)e5, (lte)g5,
    (lte)1, (lte)f5
};
static const lte pawn_white_lookup_f5[] =
{
    (lte)2, (lte)e6, (lte)g6,
    (lte)1, (lte)f6
};
static const lte pawn_white_lookup_f6[] =
{
    (lte)2, (lte)e7, (lte)g7,
    (lte)1, (lte)f7
};
static const lte pawn_white_lookup_f7[] =
{
    (lte)2, (lte)e8, (lte)g8,
    (lte)1, (lte)f8
};
static const lte pawn_white_lookup_f8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_g1[] =
{
    (lte)2, (lte)f2, (lte)h2,
    (lte)1, (lte)g2
};
static const lte pawn_white_lookup_g2[] =
{
    (lte)2, (lte)f3, (lte)h3,
    (lte)2, (lte)g3, (lte)g4
};
static const lte pawn_white_lookup_g3[] =
{
    (lte)2, (lte)f4, (lte)h4,
    (lte)1, (lte)g4
};
static const lte pawn_white_lookup_g4[] =
{
    (lte)2, (lte)f5, (lte)h5,
    (lte)1, (lte)g5
};
static const lte pawn_white_lookup_g5[] =
{
    (lte)2, (lte)f6, (lte)h6,
    (lte)1, (lte)g6
};
static const lte pawn_white_lookup_g6[] =
{
    (lte)2, (lte)f7, (lte)h7,
    (lte)1, (lte)g7
};
static const lte pawn_white_lookup_g7[] =
{
    (lte)2, (lte)f8, (lte)h8,
    (lte)1, (lte)g8
};
static const lte pawn_white_lookup_g8[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_white_lookup_h1[] =
{
    (lte)1, (lte)g2,
    (lte)1, (lte)h2
};
static const lte pawn_white_lookup_h2[] =
{
    (lte)1, (lte)g3,
    (lte)2, (lte)h3, (lte)h4
};
static const lte pawn_white_lookup_h3[] =
{
    (lte)1, (lte)g4,
    (lte)1, (lte)h4
};
static const lte pawn_white_lookup_h4[] =
{
    (lte)1, (lte)g5,
    (lte)1, (lte)h5
};
static const lte pawn_white_lookup_h5[] =
{
    (lte)1, (lte)g6,
    (lte)1, (lte)h6
};
static const lte pawn_white_lookup_h6[] =
{
    (lte)1, (lte)g7,
    (lte)1, (lte)h7
};
static const lte pawn_white_lookup_h7[] =
{
    (lte)1, (lte)g8,
    (lte)1, (lte)h8
};
static const lte pawn_white_lookup_h8[] =
{
    (lte)0,
    (lte)0
};

// pawn_white_lookup
const lte *pawn_white_lookup[] =
{
    pawn_white_lookup_a8,
    pawn_white_lookup_b8,
    pawn_white_lookup_c8,
    pawn_white_lookup_d8,
    pawn_white_lookup_e8,
    pawn_white_lookup_f8,
    pawn_white_lookup_g8,
    pawn_white_lookup_h8,
    pawn_white_lookup_a7,
    pawn_white_lookup_b7,
    pawn_white_lookup_c7,
    pawn_white_lookup_d7,
    pawn_white_lookup_e7,
    pawn_white_lookup_f7,
    pawn_white_lookup_g7,
    pawn_white_lookup_h7,
    pawn_white_lookup_a6,
    pawn_white_lookup_b6,
    pawn_white_lookup_c6,
    pawn_white_lookup_d6,
    pawn_white_lookup_e6,
    pawn_white_lookup_f6,
    pawn_white_lookup_g6,
    pawn_white_lookup_h6,
    pawn_white_lookup_a5,
    pawn_white_lookup_b5,
    pawn_white_lookup_c5,
    pawn_white_lookup_d5,
    pawn_white_lookup_e5,
    pawn_white_lookup_f5,
    pawn_white_lookup_g5,
    pawn_white_lookup_h5,
    pawn_white_lookup_a4,
    pawn_white_lookup_b4,
    pawn_white_lookup_c4,
    pawn_white_lookup_d4,
    pawn_white_lookup_e4,
    pawn_white_lookup_f4,
    pawn_white_lookup_g4,
    pawn_white_lookup_h4,
    pawn_white_lookup_a3,
    pawn_white_lookup_b3,
    pawn_white_lookup_c3,
    pawn_white_lookup_d3,
    pawn_white_lookup_e3,
    pawn_white_lookup_f3,
    pawn_white_lookup_g3,
    pawn_white_lookup_h3,
    pawn_white_lookup_a2,
    pawn_white_lookup_b2,
    pawn_white_lookup_c2,
    pawn_white_lookup_d2,
    pawn_white_lookup_e2,
    pawn_white_lookup_f2,
    pawn_white_lookup_g2,
    pawn_white_lookup_h2,
    pawn_white_lookup_a1,
    pawn_white_lookup_b1,
    pawn_white_lookup_c1,
    pawn_white_lookup_d1,
    pawn_white_lookup_e1,
    pawn_white_lookup_f1,
    pawn_white_lookup_g1,
    pawn_white_lookup_h1
};

// Black pawn, capture ray followed by advance ray
static const lte pawn_black_lookup_a1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_a2[] =
{
    (lte)1, (lte)b1,
    (lte)1, (lte)a1
};
static const lte pawn_black_lookup_a3[] =
{
    (lte)1, (lte)b2,
    (lte)1, (lte)a2
};
static const lte pawn_black_lookup_a4[] =
{
    (lte)1, (lte)b3,
    (lte)1, (lte)a3
};
static const lte pawn_black_lookup_a5[] =
{
    (lte)1, (lte)b4,
    (lte)1, (lte)a4
};
static const lte pawn_black_lookup_a6[] =
{
    (lte)1, (lte)b5,
    (lte)1, (lte)a5
};
static const lte pawn_black_lookup_a7[] =
{
    (lte)1, (lte)b6,
    (lte)2, (lte)a6, (lte)a5
};
static const lte pawn_black_lookup_a8[] =
{
    (lte)1, (lte)b7,
    (lte)1, (lte)a7
};
static const lte pawn_black_lookup_b1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_b2[] =
{
    (lte)2, (lte)a1, (lte)c1,
    (lte)1, (lte)b1
};
static const lte pawn_black_lookup_b3[] =
{
    (lte)2, (lte)a2, (lte)c2,
    (lte)1, (lte)b2
};
static const lte pawn_black_lookup_b4[] =
{
    (lte)2, (lte)a3, (lte)c3,
    (lte)1, (lte)b3
};
static const lte pawn_black_lookup_b5[] =
{
    (lte)2, (lte)a4, (lte)c4,
    (lte)1, (lte)b4
};
static const lte pawn_black_lookup_b6[] =
{
    (lte)2, (lte)a5, (lte)c5,
    (lte)1, (lte)b5
};
static const lte pawn_black_lookup_b7[] =
{
    (lte)2, (lte)a6, (lte)c6,
    (lte)2, (lte)b6, (lte)b5
};
static const lte pawn_black_lookup_b8[] =
{
    (lte)2, (lte)a7, (lte)c7,
    (lte)1, (lte)b7
};
static const lte pawn_black_lookup_c1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_c2[] =
{
    (lte)2, (lte)b1, (lte)d1,
    (lte)1, (lte)c1
};
static const lte pawn_black_lookup_c3[] =
{
    (lte)2, (lte)b2, (lte)d2,
    (lte)1, (lte)c2
};
static const lte pawn_black_lookup_c4[] =
{
    (lte)2, (lte)b3, (lte)d3,
    (lte)1, (lte)c3
};
static const lte pawn_black_lookup_c5[] =
{
    (lte)2, (lte)b4, (lte)d4,
    (lte)1, (lte)c4
};
static const lte pawn_black_lookup_c6[] =
{
    (lte)2, (lte)b5, (lte)d5,
    (lte)1, (lte)c5
};
static const lte pawn_black_lookup_c7[] =
{
    (lte)2, (lte)b6, (lte)d6,
    (lte)2, (lte)c6, (lte)c5
};
static const lte pawn_black_lookup_c8[] =
{
    (lte)2, (lte)b7, (lte)d7,
    (lte)1, (lte)c7
};
static const lte pawn_black_lookup_d1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_d2[] =
{
    (lte)2, (lte)c1, (lte)e1,
    (lte)1, (lte)d1
};
static const lte pawn_black_lookup_d3[] =
{
    (lte)2, (lte)c2, (lte)e2,
    (lte)1, (lte)d2
};
static const lte pawn_black_lookup_d4[] =
{
    (lte)2, (lte)c3, (lte)e3,
    (lte)1, (lte)d3
};
static const lte pawn_black_lookup_d5[] =
{
    (lte)2, (lte)c4, (lte)e4,
    (lte)1, (lte)d4
};
static const lte pawn_black_lookup_d6[] =
{
    (lte)2, (lte)c5, (lte)e5,
    (lte)1, (lte)d5
};
static const lte pawn_black_lookup_d7[] =
{
    (lte)2, (lte)c6, (lte)e6,
    (lte)2, (lte)d6, (lte)d5
};
static const lte pawn_black_lookup_d8[] =
{
    (lte)2, (lte)c7, (lte)e7,
    (lte)1, (lte)d7
};
static const lte pawn_black_lookup_e1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_e2[] =
{
    (lte)2, (lte)d1, (lte)f1,
    (lte)1, (lte)e1
};
static const lte pawn_black_lookup_e3[] =
{
    (lte)2, (lte)d2, (lte)f2,
    (lte)1, (lte)e2
};
static const lte pawn_black_lookup_e4[] =
{
    (lte)2, (lte)d3, (lte)f3,
    (lte)1, (lte)e3
};
static const lte pawn_black_lookup_e5[] =
{
    (lte)2, (lte)d4, (lte)f4,
    (lte)1, (lte)e4
};
static const lte pawn_black_lookup_e6[] =
{
    (lte)2, (lte)d5, (lte)f5,
    (lte)1, (lte)e5
};
static const lte pawn_black_lookup_e7[] =
{
    (lte)2, (lte)d6, (lte)f6,
    (lte)2, (lte)e6, (lte)e5
};
static const lte pawn_black_lookup_e8[] =
{
    (lte)2, (lte)d7, (lte)f7,
    (lte)1, (lte)e7
};
static const lte pawn_black_lookup_f1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_f2[] =
{
    (lte)2, (lte)e1, (lte)g1,
    (lte)1, (lte)f1
};
static const lte pawn_black_lookup_f3[] =
{
    (lte)2, (lte)e2, (lte)g2,
    (lte)1, (lte)f2
};
static const lte pawn_black_lookup_f4[] =
{
    (lte)2, (lte)e3, (lte)g3,
    (lte)1, (lte)f3
};
static const lte pawn_black_lookup_f5[] =
{
    (lte)2, (lte)e4, (lte)g4,
    (lte)1, (lte)f4
};
static const lte pawn_black_lookup_f6[] =
{
    (lte)2, (lte)e5, (lte)g5,
    (lte)1, (lte)f5
};
static const lte pawn_black_lookup_f7[] =
{
    (lte)2, (lte)e6, (lte)g6,
    (lte)2, (lte)f6, (lte)f5
};
static const lte pawn_black_lookup_f8[] =
{
    (lte)2, (lte)e7, (lte)g7,
    (lte)1, (lte)f7
};
static const lte pawn_black_lookup_g1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_g2[] =
{
    (lte)2, (lte)f1, (lte)h1,
    (lte)1, (lte)g1
};
static const lte pawn_black_lookup_g3[] =
{
    (lte)2, (lte)f2, (lte)h2,
    (lte)1, (lte)g2
};
static const lte pawn_black_lookup_g4[] =
{
    (lte)2, (lte)f3, (lte)h3,
    (lte)1, (lte)g3
};
static const lte pawn_black_lookup_g5[] =
{
    (lte)2, (lte)f4, (lte)h4,
    (lte)1, (lte)g4
};
static const lte pawn_black_lookup_g6[] =
{
    (lte)2, (lte)f5, (lte)h5,
    (lte)1, (lte)g5
};
static const lte pawn_black_lookup_g7[] =
{
    (lte)2, (lte)f6, (lte)h6,
    (lte)2, (lte)g6, (lte)g5
};
static const lte pawn_black_lookup_g8[] =
{
    (lte)2, (lte)f7, (lte)h7,
    (lte)1, (lte)g7
};
static const lte pawn_black_lookup_h1[] =
{
    (lte)0,
    (lte)0
};
static const lte pawn_black_lookup_h2[] =
{
    (lte)1, (lte)g1,
    (lte)1, (lte)h1
};
static const lte pawn_black_lookup_h3[] =
{
    (lte)1, (lte)g2,
    (lte)1, (lte)h2
};
static const lte pawn_black_lookup_h4[] =
{
    (lte)1, (lte)g3,
    (lte)1, (lte)h3
};
static const lte pawn_black_lookup_h5[] =
{
    (lte)1, (lte)g4,
    (lte)1, (lte)h4
};
static const lte pawn_black_lookup_h6[] =
{
    (lte)1, (lte)g5,
    (lte)1, (lte)h5
};
static const lte pawn_black_lookup_h7[] =
{
    (lte)1, (lte)g6,
    (lte)2, (lte)h6, (lte)h5
};
static const lte pawn_black_lookup_h8[] =
{
    (lte)1, (lte)g7,
    (lte)1, (lte)h7
};

// pawn_black_lookup
const lte *pawn_black_lookup[] =
{
    pawn_black_lookup_a8,
    pawn_black_lookup_b8,
    pawn_black_lookup_c8,
    pawn_black_lookup_d8,
    pawn_black_lookup_e8,
    pawn_black_lookup_f8,
    pawn_black_lookup_g8,
    pawn_black_lookup_h8,
    pawn_black_lookup_a7,
    pawn_black_lookup_b7,
    pawn_black_lookup_c7,
    pawn_black_lookup_d7,
    pawn_black_lookup_e7,
    pawn_black_lookup_f7,
    pawn_black_lookup_g7,
    pawn_black_lookup_h7,
    pawn_black_lookup_a6,
    pawn_black_lookup_b6,
    pawn_black_lookup_c6,
    pawn_black_lookup_d6,
    pawn_black_lookup_e6,
    pawn_black_lookup_f6,
    pawn_black_lookup_g6,
    pawn_black_lookup_h6,
    pawn_black_lookup_a5,
    pawn_black_lookup_b5,
    pawn_black_lookup_c5,
    pawn_black_lookup_d5,
    pawn_black_lookup_e5,
    pawn_black_lookup_f5,
    pawn_black_lookup_g5,
    pawn_black_lookup_h5,
    pawn_black_lookup_a4,
    pawn_black_lookup_b4,
    pawn_black_lookup_c4,
    pawn_black_lookup_d4,
    pawn_black_lookup_e4,
    pawn_black_lookup_f4,
    pawn_black_lookup_g4,
    pawn_black_lookup_h4,
    pawn_black_lookup_a3,
    pawn_black_lookup_b3,
    pawn_black_lookup_c3,
    pawn_black_lookup_d3,
    pawn_black_lookup_e3,
    pawn_black_lookup_f3,
    pawn_black_lookup_g3,
    pawn_black_lookup_h3,
    pawn_black_lookup_a2,
    pawn_black_lookup_b2,
    pawn_black_lookup_c2,
    pawn_black_lookup_d2,
    pawn_black_lookup_e2,
    pawn_black_lookup_f2,
    pawn_black_lookup_g2,
    pawn_black_lookup_h2,
    pawn_black_lookup_a1,
    pawn_black_lookup_b1,
    pawn_black_lookup_c1,
    pawn_black_lookup_d1,
    pawn_black_lookup_e1,
    pawn_black_lookup_f1,
    pawn_black_lookup_g1,
    pawn_black_lookup_h1
};

// Good king positions in the ending are often a knight move (2-1) or
//  a (2-0) move towards the centre
static const lte good_king_position_lookup_a1[] =
{
    (lte)4, (lte)c1, (lte)c2, (lte)a3, (lte)b3
};
static const lte good_king_position_lookup_a2[] =
{
    (lte)5, (lte)c1, (lte)c2, (lte)c3, (lte)a4, (lte)b4
};
static const lte good_king_position_lookup_a3[] =
{
    (lte)7, (lte)c2, (lte)c3, (lte)c4, (lte)a1, (lte)b1, (lte)a5, (lte)b5
};
static const lte good_king_position_lookup_a4[] =
{
    (lte)7, (lte)c3, (lte)c4, (lte)c5, (lte)a2, (lte)b2, (lte)a6, (lte)b6
};
static const lte good_king_position_lookup_a5[] =
{
    (lte)7, (lte)c4, (lte)c5, (lte)c6, (lte)a3, (lte)b3, (lte)a7, (lte)b7
};
static const lte good_king_position_lookup_a6[] =
{
    (lte)7, (lte)c5, (lte)c6, (lte)c7, (lte)a4, (lte)b4, (lte)a8, (lte)b8
};
static const lte good_king_position_lookup_a7[] =
{
    (lte)5, (lte)c6, (lte)c7, (lte)c8, (lte)a5, (lte)b5
};
static const lte good_king_position_lookup_a8[] =
{
    (lte)4, (lte)c7, (lte)c8, (lte)a6, (lte)b6
};
static const lte good_king_position_lookup_b1[] =
{
    (lte)5, (lte)d1, (lte)d2, (lte)a3, (lte)b3, (lte)c3
};
static const lte good_king_position_lookup_b2[] =
{
    (lte)6, (lte)d1, (lte)d2, (lte)d3, (lte)a4, (lte)b4, (lte)c4
};
static const lte good_king_position_lookup_b3[] =
{
    (lte)9, (lte)d2, (lte)d3, (lte)d4, (lte)a1, (lte)b1, (lte)c1, (lte)a5, (lte)b5, (lte)c5
};
static const lte good_king_position_lookup_b4[] =
{
    (lte)9, (lte)d3, (lte)d4, (lte)d5, (lte)a2, (lte)b2, (lte)c2, (lte)a6, (lte)b6, (lte)c6
};
static const lte good_king_position_lookup_b5[] =
{
    (lte)9, (lte)d4, (lte)d5, (lte)d6, (lte)a3, (lte)b3, (lte)c3, (lte)a7, (lte)b7, (lte)c7
};
static const lte good_king_position_lookup_b6[] =
{
    (lte)9, (lte)d5, (lte)d6, (lte)d7, (lte)a4, (lte)b4, (lte)c4, (lte)a8, (lte)b8, (lte)c8
};
static const lte good_king_position_lookup_b7[] =
{
    (lte)6, (lte)d6, (lte)d7, (lte)d8, (lte)a5, (lte)b5, (lte)c5
};
static const lte good_king_position_lookup_b8[] =
{
    (lte)5, (lte)d7, (lte)d8, (lte)a6, (lte)b6, (lte)c6
};
static const lte good_king_position_lookup_c1[] =
{
    (lte)7, (lte)a1, (lte)a2, (lte)e1, (lte)e2, (lte)b3, (lte)c3, (lte)d3
};
static const lte good_king_position_lookup_c2[] =
{
    (lte)9, (lte)a1, (lte)a2, (lte)a3, (lte)e1, (lte)e2, (lte)e3, (lte)b4, (lte)c4, (lte)d4
};
static const lte good_king_position_lookup_c3[] =
{
    (lte)12, (lte)a2, (lte)a3, (lte)a4, (lte)e2, (lte)e3, (lte)e4, (lte)b1, (lte)c1, (lte)d1, (lte)b5, (lte)c5, (lte)d5
};
static const lte good_king_position_lookup_c4[] =
{
    (lte)12, (lte)a3, (lte)a4, (lte)a5, (lte)e3, (lte)e4, (lte)e5, (lte)b2, (lte)c2, (lte)d2, (lte)b6, (lte)c6, (lte)d6
};
static const lte good_king_position_lookup_c5[] =
{
    (lte)12, (lte)a4, (lte)a5, (lte)a6, (lte)e4, (lte)e5, (lte)e6, (lte)b3, (lte)c3, (lte)d3, (lte)b7, (lte)c7, (lte)d7
};
static const lte good_king_position_lookup_c6[] =
{
    (lte)12, (lte)a5, (lte)a6, (lte)a7, (lte)e5, (lte)e6, (lte)e7, (lte)b4, (lte)c4, (lte)d4, (lte)b8, (lte)c8, (lte)d8
};
static const lte good_king_position_lookup_c7[] =
{
    (lte)9, (lte)a6, (lte)a7, (lte)a8, (lte)e6, (lte)e7, (lte)e8, (lte)b5, (lte)c5, (lte)d5
};
static const lte good_king_position_lookup_c8[] =
{
    (lte)7, (lte)a7, (lte)a8, (lte)e7, (lte)e8, (lte)b6, (lte)c6, (lte)d6
};
static const lte good_king_position_lookup_d1[] =
{
    (lte)7, (lte)b1, (lte)b2, (lte)f1, (lte)f2, (lte)c3, (lte)d3, (lte)e3
};
static const lte good_king_position_lookup_d2[] =
{
    (lte)9, (lte)b1, (lte)b2, (lte)b3, (lte)f1, (lte)f2, (lte)f3, (lte)c4, (lte)d4, (lte)e4
};
static const lte good_king_position_lookup_d3[] =
{
    (lte)12, (lte)b2, (lte)b3, (lte)b4, (lte)f2, (lte)f3, (lte)f4, (lte)c1, (lte)d1, (lte)e1, (lte)c5, (lte)d5, (lte)e5
};
static const lte good_king_position_lookup_d4[] =
{
    (lte)12, (lte)b3, (lte)b4, (lte)b5, (lte)f3, (lte)f4, (lte)f5, (lte)c2, (lte)d2, (lte)e2, (lte)c6, (lte)d6, (lte)e6
};
static const lte good_king_position_lookup_d5[] =
{
    (lte)12, (lte)b4, (lte)b5, (lte)b6, (lte)f4, (lte)f5, (lte)f6, (lte)c3, (lte)d3, (lte)e3, (lte)c7, (lte)d7, (lte)e7
};
static const lte good_king_position_lookup_d6[] =
{
    (lte)12, (lte)b5, (lte)b6, (lte)b7, (lte)f5, (lte)f6, (lte)f7, (lte)c4, (lte)d4, (lte)e4, (lte)c8, (lte)d8, (lte)e8
};
static const lte good_king_position_lookup_d7[] =
{
    (lte)9, (lte)b6, (lte)b7, (lte)b8, (lte)f6, (lte)f7, (lte)f8, (lte)c5, (lte)d5, (lte)e5
};
static const lte good_king_position_lookup_d8[] =
{
    (lte)7, (lte)b7, (lte)b8, (lte)f7, (lte)f8, (lte)c6, (lte)d6, (lte)e6
};
static const lte good_king_position_lookup_e1[] =
{
    (lte)7, (lte)c1, (lte)c2, (lte)g1, (lte)g2, (lte)d3, (lte)e3, (lte)f3
};
static const lte good_king_position_lookup_e2[] =
{
    (lte)9, (lte)c1, (lte)c2, (lte)c3, (lte)g1, (lte)g2, (lte)g3, (lte)d4, (lte)e4, (lte)f4
};
static const lte good_king_position_lookup_e3[] =
{
    (lte)12, (lte)c2, (lte)c3, (lte)c4, (lte)g2, (lte)g3, (lte)g4, (lte)d1, (lte)e1, (lte)f1, (lte)d5, (lte)e5, (lte)f5
};
static const lte good_king_position_lookup_e4[] =
{
    (lte)12, (lte)c3, (lte)c4, (lte)c5, (lte)g3, (lte)g4, (lte)g5, (lte)d2, (lte)e2, (lte)f2, (lte)d6, (lte)e6, (lte)f6
};
static const lte good_king_position_lookup_e5[] =
{
    (lte)12, (lte)c4, (lte)c5, (lte)c6, (lte)g4, (lte)g5, (lte)g6, (lte)d3, (lte)e3, (lte)f3, (lte)d7, (lte)e7, (lte)f7
};
static const lte good_king_position_lookup_e6[] =
{
    (lte)12, (lte)c5, (lte)c6, (lte)c7, (lte)g5, (lte)g6, (lte)g7, (lte)d4, (lte)e4, (lte)f4, (lte)d8, (lte)e8, (lte)f8
};
static const lte good_king_position_lookup_e7[] =
{
    (lte)9, (lte)c6, (lte)c7, (lte)c8, (lte)g6, (lte)g7, (lte)g8, (lte)d5, (lte)e5, (lte)f5
};
static const lte good_king_position_lookup_e8[] =
{
    (lte)7, (lte)c7, (lte)c8, (lte)g7, (lte)g8, (lte)d6, (lte)e6, (lte)f6
};
static const lte good_king_position_lookup_f1[] =
{
    (lte)7, (lte)d1, (lte)d2, (lte)h1, (lte)h2, (lte)e3, (lte)f3, (lte)g3
};
static const lte good_king_position_lookup_f2[] =
{
    (lte)9, (lte)d1, (lte)d2, (lte)d3, (lte)h1, (lte)h2, (lte)h3, (lte)e4, (lte)f4, (lte)g4
};
static const lte good_king_position_lookup_f3[] =
{
    (lte)12, (lte)d2, (lte)d3, (lte)d4, (lte)h2, (lte)h3, (lte)h4, (lte)e1, (lte)f1, (lte)g1, (lte)e5, (lte)f5, (lte)g5
};
static const lte good_king_position_lookup_f4[] =
{
    (lte)12, (lte)d3, (lte)d4, (lte)d5, (lte)h3, (lte)h4, (lte)h5, (lte)e2, (lte)f2, (lte)g2, (lte)e6, (lte)f6, (lte)g6
};
static const lte good_king_position_lookup_f5[] =
{
    (lte)12, (lte)d4, (lte)d5, (lte)d6, (lte)h4, (lte)h5, (lte)h6, (lte)e3, (lte)f3, (lte)g3, (lte)e7, (lte)f7, (lte)g7
};
static const lte good_king_position_lookup_f6[] =
{
    (lte)12, (lte)d5, (lte)d6, (lte)d7, (lte)h5, (lte)h6, (lte)h7, (lte)e4, (lte)f4, (lte)g4, (lte)e8, (lte)f8, (lte)g8
};
static const lte good_king_position_lookup_f7[] =
{
    (lte)9, (lte)d6, (lte)d7, (lte)d8, (lte)h6, (lte)h7, (lte)h8, (lte)e5, (lte)f5, (lte)g5
};
static const lte good_king_position_lookup_f8[] =
{
    (lte)7, (lte)d7, (lte)d8, (lte)h7, (lte)h8, (lte)e6, (lte)f6, (lte)g6
};
static const lte good_king_position_lookup_g1[] =
{
    (lte)5, (lte)e1, (lte)e2, (lte)f3, (lte)g3, (lte)h3
};
static const lte good_king_position_lookup_g2[] =
{
    (lte)6, (lte)e1, (lte)e2, (lte)e3, (lte)f4, (lte)g4, (lte)h4
};
static const lte good_king_position_lookup_g3[] =
{
    (lte)9, (lte)e2, (lte)e3, (lte)e4, (lte)f1, (lte)g1, (lte)h1, (lte)f5, (lte)g5, (lte)h5
};
static const lte good_king_position_lookup_g4[] =
{
    (lte)9, (lte)e3, (lte)e4, (lte)e5, (lte)f2, (lte)g2, (lte)h2, (lte)f6, (lte)g6, (lte)h6
};
static const lte good_king_position_lookup_g5[] =
{
    (lte)9, (lte)e4, (lte)e5, (lte)e6, (lte)f3, (lte)g3, (lte)h3, (lte)f7, (lte)g7, (lte)h7
};
static const lte good_king_position_lookup_g6[] =
{
    (lte)9, (lte)e5, (lte)e6, (lte)e7, (lte)f4, (lte)g4, (lte)h4, (lte)f8, (lte)g8, (lte)h8
};
static const lte good_king_position_lookup_g7[] =
{
    (lte)6, (lte)e6, (lte)e7, (lte)e8, (lte)f5, (lte)g5, (lte)h5
};
static const lte good_king_position_lookup_g8[] =
{
    (lte)5, (lte)e7, (lte)e8, (lte)f6, (lte)g6, (lte)h6
};
static const lte good_king_position_lookup_h1[] =
{
    (lte)4, (lte)f1, (lte)f2, (lte)g3, (lte)h3
};
static const lte good_king_position_lookup_h2[] =
{
    (lte)5, (lte)f1, (lte)f2, (lte)f3, (lte)g4, (lte)h4
};
static const lte good_king_position_lookup_h3[] =
{
    (lte)7, (lte)f2, (lte)f3, (lte)f4, (lte)g1, (lte)h1, (lte)g5, (lte)h5
};
static const lte good_king_position_lookup_h4[] =
{
    (lte)7, (lte)f3, (lte)f4, (lte)f5, (lte)g2, (lte)h2, (lte)g6, (lte)h6
};
static const lte good_king_position_lookup_h5[] =
{
    (lte)7, (lte)f4, (lte)f5, (lte)f6, (lte)g3, (lte)h3, (lte)g7, (lte)h7
};
static const lte good_king_position_lookup_h6[] =
{
    (lte)7, (lte)f5, (lte)f6, (lte)f7, (lte)g4, (lte)h4, (lte)g8, (lte)h8
};
static const lte good_king_position_lookup_h7[] =
{
    (lte)5, (lte)f6, (lte)f7, (lte)f8, (lte)g5, (lte)h5
};
static const lte good_king_position_lookup_h8[] =
{
    (lte)4, (lte)f7, (lte)f8, (lte)g6, (lte)h6
};

// good_king_position_lookup
const lte *good_king_position_lookup[] =
{
    good_king_position_lookup_a8,
    good_king_position_lookup_b8,
    good_king_position_lookup_c8,
    good_king_position_lookup_d8,
    good_king_position_lookup_e8,
    good_king_position_lookup_f8,
    good_king_position_lookup_g8,
    good_king_position_lookup_h8,
    good_king_position_lookup_a7,
    good_king_position_lookup_b7,
    good_king_position_lookup_c7,
    good_king_position_lookup_d7,
    good_king_position_lookup_e7,
    good_king_position_lookup_f7,
    good_king_position_lookup_g7,
    good_king_position_lookup_h7,
    good_king_position_lookup_a6,
    good_king_position_lookup_b6,
    good_king_position_lookup_c6,
    good_king_position_lookup_d6,
    good_king_position_lookup_e6,
    good_king_position_lookup_f6,
    good_king_position_lookup_g6,
    good_king_position_lookup_h6,
    good_king_position_lookup_a5,
    good_king_position_lookup_b5,
    good_king_position_lookup_c5,
    good_king_position_lookup_d5,
    good_king_position_lookup_e5,
    good_king_position_lookup_f5,
    good_king_position_lookup_g5,
    good_king_position_lookup_h5,
    good_king_position_lookup_a4,
    good_king_position_lookup_b4,
    good_king_position_lookup_c4,
    good_king_position_lookup_d4,
    good_king_position_lookup_e4,
    good_king_position_lookup_f4,
    good_king_position_lookup_g4,
    good_king_position_lookup_h4,
    good_king_position_lookup_a3,
    good_king_position_lookup_b3,
    good_king_position_lookup_c3,
    good_king_position_lookup_d3,
    good_king_position_lookup_e3,
    good_king_position_lookup_f3,
    good_king_position_lookup_g3,
    good_king_position_lookup_h3,
    good_king_position_lookup_a2,
    good_king_position_lookup_b2,
    good_king_position_lookup_c2,
    good_king_position_lookup_d2,
    good_king_position_lookup_e2,
    good_king_position_lookup_f2,
    good_king_position_lookup_g2,
    good_king_position_lookup_h2,
    good_king_position_lookup_a1,
    good_king_position_lookup_b1,
    good_king_position_lookup_c1,
    good_king_position_lookup_d1,
    good_king_position_lookup_e1,
    good_king_position_lookup_f1,
    good_king_position_lookup_g1,
    good_king_position_lookup_h1
};

// Attack from up to 2 black pawns on a white piece
static const lte pawn_attacks_white_lookup_a1[] =
{
    (lte)1, (lte)b2
};
static const lte pawn_attacks_white_lookup_a2[] =
{
    (lte)1, (lte)b3
};
static const lte pawn_attacks_white_lookup_a3[] =
{
    (lte)1, (lte)b4
};
static const lte pawn_attacks_white_lookup_a4[] =
{
    (lte)1, (lte)b5
};
static const lte pawn_attacks_white_lookup_a5[] =
{
    (lte)1, (lte)b6
};
static const lte pawn_attacks_white_lookup_a6[] =
{
    (lte)1, (lte)b7
};
static const lte pawn_attacks_white_lookup_a7[] =
{
    (lte)1, (lte)b8
};
static const lte pawn_attacks_white_lookup_a8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_b1[] =
{
    (lte)2, (lte)a2, (lte)c2
};
static const lte pawn_attacks_white_lookup_b2[] =
{
    (lte)2, (lte)a3, (lte)c3
};
static const lte pawn_attacks_white_lookup_b3[] =
{
    (lte)2, (lte)a4, (lte)c4
};
static const lte pawn_attacks_white_lookup_b4[] =
{
    (lte)2, (lte)a5, (lte)c5
};
static const lte pawn_attacks_white_lookup_b5[] =
{
    (lte)2, (lte)a6, (lte)c6
};
static const lte pawn_attacks_white_lookup_b6[] =
{
    (lte)2, (lte)a7, (lte)c7
};
static const lte pawn_attacks_white_lookup_b7[] =
{
    (lte)2, (lte)a8, (lte)c8
};
static const lte pawn_attacks_white_lookup_b8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_c1[] =
{
    (lte)2, (lte)b2, (lte)d2
};
static const lte pawn_attacks_white_lookup_c2[] =
{
    (lte)2, (lte)b3, (lte)d3
};
static const lte pawn_attacks_white_lookup_c3[] =
{
    (lte)2, (lte)b4, (lte)d4
};
static const lte pawn_attacks_white_lookup_c4[] =
{
    (lte)2, (lte)b5, (lte)d5
};
static const lte pawn_attacks_white_lookup_c5[] =
{
    (lte)2, (lte)b6, (lte)d6
};
static const lte pawn_attacks_white_lookup_c6[] =
{
    (lte)2, (lte)b7, (lte)d7
};
static const lte pawn_attacks_white_lookup_c7[] =
{
    (lte)2, (lte)b8, (lte)d8
};
static const lte pawn_attacks_white_lookup_c8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_d1[] =
{
    (lte)2, (lte)c2, (lte)e2
};
static const lte pawn_attacks_white_lookup_d2[] =
{
    (lte)2, (lte)c3, (lte)e3
};
static const lte pawn_attacks_white_lookup_d3[] =
{
    (lte)2, (lte)c4, (lte)e4
};
static const lte pawn_attacks_white_lookup_d4[] =
{
    (lte)2, (lte)c5, (lte)e5
};
static const lte pawn_attacks_white_lookup_d5[] =
{
    (lte)2, (lte)c6, (lte)e6
};
static const lte pawn_attacks_white_lookup_d6[] =
{
    (lte)2, (lte)c7, (lte)e7
};
static const lte pawn_attacks_white_lookup_d7[] =
{
    (lte)2, (lte)c8, (lte)e8
};
static const lte pawn_attacks_white_lookup_d8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_e1[] =
{
    (lte)2, (lte)d2, (lte)f2
};
static const lte pawn_attacks_white_lookup_e2[] =
{
    (lte)2, (lte)d3, (lte)f3
};
static const lte pawn_attacks_white_lookup_e3[] =
{
    (lte)2, (lte)d4, (lte)f4
};
static const lte pawn_attacks_white_lookup_e4[] =
{
    (lte)2, (lte)d5, (lte)f5
};
static const lte pawn_attacks_white_lookup_e5[] =
{
    (lte)2, (lte)d6, (lte)f6
};
static const lte pawn_attacks_white_lookup_e6[] =
{
    (lte)2, (lte)d7, (lte)f7
};
static const lte pawn_attacks_white_lookup_e7[] =
{
    (lte)2, (lte)d8, (lte)f8
};
static const lte pawn_attacks_white_lookup_e8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_f1[] =
{
    (lte)2, (lte)e2, (lte)g2
};
static const lte pawn_attacks_white_lookup_f2[] =
{
    (lte)2, (lte)e3, (lte)g3
};
static const lte pawn_attacks_white_lookup_f3[] =
{
    (lte)2, (lte)e4, (lte)g4
};
static const lte pawn_attacks_white_lookup_f4[] =
{
    (lte)2, (lte)e5, (lte)g5
};
static const lte pawn_attacks_white_lookup_f5[] =
{
    (lte)2, (lte)e6, (lte)g6
};
static const lte pawn_attacks_white_lookup_f6[] =
{
    (lte)2, (lte)e7, (lte)g7
};
static const lte pawn_attacks_white_lookup_f7[] =
{
    (lte)2, (lte)e8, (lte)g8
};
static const lte pawn_attacks_white_lookup_f8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_g1[] =
{
    (lte)2, (lte)f2, (lte)h2
};
static const lte pawn_attacks_white_lookup_g2[] =
{
    (lte)2, (lte)f3, (lte)h3
};
static const lte pawn_attacks_white_lookup_g3[] =
{
    (lte)2, (lte)f4, (lte)h4
};
static const lte pawn_attacks_white_lookup_g4[] =
{
    (lte)2, (lte)f5, (lte)h5
};
static const lte pawn_attacks_white_lookup_g5[] =
{
    (lte)2, (lte)f6, (lte)h6
};
static const lte pawn_attacks_white_lookup_g6[] =
{
    (lte)2, (lte)f7, (lte)h7
};
static const lte pawn_attacks_white_lookup_g7[] =
{
    (lte)2, (lte)f8, (lte)h8
};
static const lte pawn_attacks_white_lookup_g8[] =
{
    (lte)0
};
static const lte pawn_attacks_white_lookup_h1[] =
{
    (lte)1, (lte)g2
};
static const lte pawn_attacks_white_lookup_h2[] =
{
    (lte)1, (lte)g3
};
static const lte pawn_attacks_white_lookup_h3[] =
{
    (lte)1, (lte)g4
};
static const lte pawn_attacks_white_lookup_h4[] =
{
    (lte)1, (lte)g5
};
static const lte pawn_attacks_white_lookup_h5[] =
{
    (lte)1, (lte)g6
};
static const lte pawn_attacks_white_lookup_h6[] =
{
    (lte)1, (lte)g7
};
static const lte pawn_attacks_white_lookup_h7[] =
{
    (lte)1, (lte)g8
};
static const lte pawn_attacks_white_lookup_h8[] =
{
    (lte)0
};

// pawn_attacks_white_lookup
const lte *pawn_attacks_white_lookup[] =
{
    pawn_attacks_white_lookup_a8,
    pawn_attacks_white_lookup_b8,
    pawn_attacks_white_lookup_c8,
    pawn_attacks_white_lookup_d8,
    pawn_attacks_white_lookup_e8,
    pawn_attacks_white_lookup_f8,
    pawn_attacks_white_lookup_g8,
    pawn_attacks_white_lookup_h8,
    pawn_attacks_white_lookup_a7,
    pawn_attacks_white_lookup_b7,
    pawn_attacks_white_lookup_c7,
    pawn_attacks_white_lookup_d7,
    pawn_attacks_white_lookup_e7,
    pawn_attacks_white_lookup_f7,
    pawn_attacks_white_lookup_g7,
    pawn_attacks_white_lookup_h7,
    pawn_attacks_white_lookup_a6,
    pawn_attacks_white_lookup_b6,
    pawn_attacks_white_lookup_c6,
    pawn_attacks_white_lookup_d6,
    pawn_attacks_white_lookup_e6,
    pawn_attacks_white_lookup_f6,
    pawn_attacks_white_lookup_g6,
    pawn_attacks_white_lookup_h6,
    pawn_attacks_white_lookup_a5,
    pawn_attacks_white_lookup_b5,
    pawn_attacks_white_lookup_c5,
    pawn_attacks_white_lookup_d5,
    pawn_attacks_white_lookup_e5,
    pawn_attacks_white_lookup_f5,
    pawn_attacks_white_lookup_g5,
    pawn_attacks_white_lookup_h5,
    pawn_attacks_white_lookup_a4,
    pawn_attacks_white_lookup_b4,
    pawn_attacks_white_lookup_c4,
    pawn_attacks_white_lookup_d4,
    pawn_attacks_white_lookup_e4,
    pawn_attacks_white_lookup_f4,
    pawn_attacks_white_lookup_g4,
    pawn_attacks_white_lookup_h4,
    pawn_attacks_white_lookup_a3,
    pawn_attacks_white_lookup_b3,
    pawn_attacks_white_lookup_c3,
    pawn_attacks_white_lookup_d3,
    pawn_attacks_white_lookup_e3,
    pawn_attacks_white_lookup_f3,
    pawn_attacks_white_lookup_g3,
    pawn_attacks_white_lookup_h3,
    pawn_attacks_white_lookup_a2,
    pawn_attacks_white_lookup_b2,
    pawn_attacks_white_lookup_c2,
    pawn_attacks_white_lookup_d2,
    pawn_attacks_white_lookup_e2,
    pawn_attacks_white_lookup_f2,
    pawn_attacks_white_lookup_g2,
    pawn_attacks_white_lookup_h2,
    pawn_attacks_white_lookup_a1,
    pawn_attacks_white_lookup_b1,
    pawn_attacks_white_lookup_c1,
    pawn_attacks_white_lookup_d1,
    pawn_attacks_white_lookup_e1,
    pawn_attacks_white_lookup_f1,
    pawn_attacks_white_lookup_g1,
    pawn_attacks_white_lookup_h1
};

// Attack from up to 2 white pawns on a black piece
static const lte pawn_attacks_black_lookup_a1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_a2[] =
{
    (lte)1, (lte)b1
};
static const lte pawn_attacks_black_lookup_a3[] =
{
    (lte)1, (lte)b2
};
static const lte pawn_attacks_black_lookup_a4[] =
{
    (lte)1, (lte)b3
};
static const lte pawn_attacks_black_lookup_a5[] =
{
    (lte)1, (lte)b4
};
static const lte pawn_attacks_black_lookup_a6[] =
{
    (lte)1, (lte)b5
};
static const lte pawn_attacks_black_lookup_a7[] =
{
    (lte)1, (lte)b6
};
static const lte pawn_attacks_black_lookup_a8[] =
{
    (lte)1, (lte)b7
};
static const lte pawn_attacks_black_lookup_b1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_b2[] =
{
    (lte)2, (lte)a1, (lte)c1
};
static const lte pawn_attacks_black_lookup_b3[] =
{
    (lte)2, (lte)a2, (lte)c2
};
static const lte pawn_attacks_black_lookup_b4[] =
{
    (lte)2, (lte)a3, (lte)c3
};
static const lte pawn_attacks_black_lookup_b5[] =
{
    (lte)2, (lte)a4, (lte)c4
};
static const lte pawn_attacks_black_lookup_b6[] =
{
    (lte)2, (lte)a5, (lte)c5
};
static const lte pawn_attacks_black_lookup_b7[] =
{
    (lte)2, (lte)a6, (lte)c6
};
static const lte pawn_attacks_black_lookup_b8[] =
{
    (lte)2, (lte)a7, (lte)c7
};
static const lte pawn_attacks_black_lookup_c1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_c2[] =
{
    (lte)2, (lte)b1, (lte)d1
};
static const lte pawn_attacks_black_lookup_c3[] =
{
    (lte)2, (lte)b2, (lte)d2
};
static const lte pawn_attacks_black_lookup_c4[] =
{
    (lte)2, (lte)b3, (lte)d3
};
static const lte pawn_attacks_black_lookup_c5[] =
{
    (lte)2, (lte)b4, (lte)d4
};
static const lte pawn_attacks_black_lookup_c6[] =
{
    (lte)2, (lte)b5, (lte)d5
};
static const lte pawn_attacks_black_lookup_c7[] =
{
    (lte)2, (lte)b6, (lte)d6
};
static const lte pawn_attacks_black_lookup_c8[] =
{
    (lte)2, (lte)b7, (lte)d7
};
static const lte pawn_attacks_black_lookup_d1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_d2[] =
{
    (lte)2, (lte)c1, (lte)e1
};
static const lte pawn_attacks_black_lookup_d3[] =
{
    (lte)2, (lte)c2, (lte)e2
};
static const lte pawn_attacks_black_lookup_d4[] =
{
    (lte)2, (lte)c3, (lte)e3
};
static const lte pawn_attacks_black_lookup_d5[] =
{
    (lte)2, (lte)c4, (lte)e4
};
static const lte pawn_attacks_black_lookup_d6[] =
{
    (lte)2, (lte)c5, (lte)e5
};
static const lte pawn_attacks_black_lookup_d7[] =
{
    (lte)2, (lte)c6, (lte)e6
};
static const lte pawn_attacks_black_lookup_d8[] =
{
    (lte)2, (lte)c7, (lte)e7
};
static const lte pawn_attacks_black_lookup_e1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_e2[] =
{
    (lte)2, (lte)d1, (lte)f1
};
static const lte pawn_attacks_black_lookup_e3[] =
{
    (lte)2, (lte)d2, (lte)f2
};
static const lte pawn_attacks_black_lookup_e4[] =
{
    (lte)2, (lte)d3, (lte)f3
};
static const lte pawn_attacks_black_lookup_e5[] =
{
    (lte)2, (lte)d4, (lte)f4
};
static const lte pawn_attacks_black_lookup_e6[] =
{
    (lte)2, (lte)d5, (lte)f5
};
static const lte pawn_attacks_black_lookup_e7[] =
{
    (lte)2, (lte)d6, (lte)f6
};
static const lte pawn_attacks_black_lookup_e8[] =
{
    (lte)2, (lte)d7, (lte)f7
};
static const lte pawn_attacks_black_lookup_f1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_f2[] =
{
    (lte)2, (lte)e1, (lte)g1
};
static const lte pawn_attacks_black_lookup_f3[] =
{
    (lte)2, (lte)e2, (lte)g2
};
static const lte pawn_attacks_black_lookup_f4[] =
{
    (lte)2, (lte)e3, (lte)g3
};
static const lte pawn_attacks_black_lookup_f5[] =
{
    (lte)2, (lte)e4, (lte)g4
};
static const lte pawn_attacks_black_lookup_f6[] =
{
    (lte)2, (lte)e5, (lte)g5
};
static const lte pawn_attacks_black_lookup_f7[] =
{
    (lte)2, (lte)e6, (lte)g6
};
static const lte pawn_attacks_black_lookup_f8[] =
{
    (lte)2, (lte)e7, (lte)g7
};
static const lte pawn_attacks_black_lookup_g1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_g2[] =
{
    (lte)2, (lte)f1, (lte)h1
};
static const lte pawn_attacks_black_lookup_g3[] =
{
    (lte)2, (lte)f2, (lte)h2
};
static const lte pawn_attacks_black_lookup_g4[] =
{
    (lte)2, (lte)f3, (lte)h3
};
static const lte pawn_attacks_black_lookup_g5[] =
{
    (lte)2, (lte)f4, (lte)h4
};
static const lte pawn_attacks_black_lookup_g6[] =
{
    (lte)2, (lte)f5, (lte)h5
};
static const lte pawn_attacks_black_lookup_g7[] =
{
    (lte)2, (lte)f6, (lte)h6
};
static const lte pawn_attacks_black_lookup_g8[] =
{
    (lte)2, (lte)f7, (lte)h7
};
static const lte pawn_attacks_black_lookup_h1[] =
{
    (lte)0
};
static const lte pawn_attacks_black_lookup_h2[] =
{
    (lte)1, (lte)g1
};
static const lte pawn_attacks_black_lookup_h3[] =
{
    (lte)1, (lte)g2
};
static const lte pawn_attacks_black_lookup_h4[] =
{
    (lte)1, (lte)g3
};
static const lte pawn_attacks_black_lookup_h5[] =
{
    (lte)1, (lte)g4
};
static const lte pawn_attacks_black_lookup_h6[] =
{
    (lte)1, (lte)g5
};
static const lte pawn_attacks_black_lookup_h7[] =
{
    (lte)1, (lte)g6
};
static const lte pawn_attacks_black_lookup_h8[] =
{
    (lte)1, (lte)g7
};

// pawn_attacks_black_lookup
const lte *pawn_attacks_black_lookup[] =
{
    pawn_attacks_black_lookup_a8,
    pawn_attacks_black_lookup_b8,
    pawn_attacks_black_lookup_c8,
    pawn_attacks_black_lookup_d8,
    pawn_attacks_black_lookup_e8,
    pawn_attacks_black_lookup_f8,
    pawn_attacks_black_lookup_g8,
    pawn_attacks_black_lookup_h8,
    pawn_attacks_black_lookup_a7,
    pawn_attacks_black_lookup_b7,
    pawn_attacks_black_lookup_c7,
    pawn_attacks_black_lookup_d7,
    pawn_attacks_black_lookup_e7,
    pawn_attacks_black_lookup_f7,
    pawn_attacks_black_lookup_g7,
    pawn_attacks_black_lookup_h7,
    pawn_attacks_black_lookup_a6,
    pawn_attacks_black_lookup_b6,
    pawn_attacks_black_lookup_c6,
    pawn_attacks_black_lookup_d6,
    pawn_attacks_black_lookup_e6,
    pawn_attacks_black_lookup_f6,
    pawn_attacks_black_lookup_g6,
    pawn_attacks_black_lookup_h6,
    pawn_attacks_black_lookup_a5,
    pawn_attacks_black_lookup_b5,
    pawn_attacks_black_lookup_c5,
    pawn_attacks_black_lookup_d5,
    pawn_attacks_black_lookup_e5,
    pawn_attacks_black_lookup_f5,
    pawn_attacks_black_lookup_g5,
    pawn_attacks_black_lookup_h5,
    pawn_attacks_black_lookup_a4,
    pawn_attacks_black_lookup_b4,
    pawn_attacks_black_lookup_c4,
    pawn_attacks_black_lookup_d4,
    pawn_attacks_black_lookup_e4,
    pawn_attacks_black_lookup_f4,
    pawn_attacks_black_lookup_g4,
    pawn_attacks_black_lookup_h4,
    pawn_attacks_black_lookup_a3,
    pawn_attacks_black_lookup_b3,
    pawn_attacks_black_lookup_c3,
    pawn_attacks_black_lookup_d3,
    pawn_attacks_black_lookup_e3,
    pawn_attacks_black_lookup_f3,
    pawn_attacks_black_lookup_g3,
    pawn_attacks_black_lookup_h3,
    pawn_attacks_black_lookup_a2,
    pawn_attacks_black_lookup_b2,
    pawn_attacks_black_lookup_c2,
    pawn_attacks_black_lookup_d2,
    pawn_attacks_black_lookup_e2,
    pawn_attacks_black_lookup_f2,
    pawn_attacks_black_lookup_g2,
    pawn_attacks_black_lookup_h2,
    pawn_attacks_black_lookup_a1,
    pawn_attacks_black_lookup_b1,
    pawn_attacks_black_lookup_c1,
    pawn_attacks_black_lookup_d1,
    pawn_attacks_black_lookup_e1,
    pawn_attacks_black_lookup_f1,
    pawn_attacks_black_lookup_g1,
    pawn_attacks_black_lookup_h1
};

// Attack from up to 8 rays on a white piece
static const lte attacks_white_lookup_a1[] =
{
(lte)3
    ,(lte)7 ,(lte)b1,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)a2,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)7 ,(lte)b2,(lte)(K|P|B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
};
static const lte attacks_white_lookup_a2[] =
{
(lte)5
    ,(lte)7 ,(lte)b2,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)a1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)a3,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)6 ,(lte)b3,(lte)(K|P|B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_a3[] =
{
(lte)5
    ,(lte)7 ,(lte)b3,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)a2,(lte)(K|R|Q)   ,(lte)a1,(lte)(R|Q)
    ,(lte)5 ,(lte)a4,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)5 ,(lte)b4,(lte)(K|P|B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|B|Q)   ,(lte)c1,(lte)(B|Q)
};
static const lte attacks_white_lookup_a4[] =
{
(lte)5
    ,(lte)7 ,(lte)b4,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)a3,(lte)(K|R|Q)   ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)4 ,(lte)a5,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)4 ,(lte)b5,(lte)(K|P|B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)3 ,(lte)b3,(lte)(K|B|Q)   ,(lte)c2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
};
static const lte attacks_white_lookup_a5[] =
{
(lte)5
    ,(lte)7 ,(lte)b5,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)a4,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)3 ,(lte)a6,(lte)(K|R|Q)   ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)3 ,(lte)b6,(lte)(K|P|B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)4 ,(lte)b4,(lte)(K|B|Q)   ,(lte)c3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_white_lookup_a6[] =
{
(lte)5
    ,(lte)7 ,(lte)b6,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)a5,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)2 ,(lte)a7,(lte)(K|R|Q)   ,(lte)a8,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|P|B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)5 ,(lte)b5,(lte)(K|B|Q)   ,(lte)c4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_white_lookup_a7[] =
{
(lte)5
    ,(lte)7 ,(lte)b7,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)a6,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)1 ,(lte)a8,(lte)(K|R|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)b6,(lte)(K|B|Q)   ,(lte)c5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_white_lookup_a8[] =
{
(lte)3
    ,(lte)7 ,(lte)b8,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)a7,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)7 ,(lte)b7,(lte)(K|B|Q)   ,(lte)c6,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b1[] =
{
(lte)5
    ,(lte)1 ,(lte)a1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c1,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)b2,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a2,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)c2,(lte)(K|P|B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
};
static const lte attacks_white_lookup_b2[] =
{
(lte)8
    ,(lte)1 ,(lte)a2,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c2,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)b3,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a1,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a3,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|P|B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_b3[] =
{
(lte)8
    ,(lte)1 ,(lte)a3,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|R|Q)   ,(lte)b1,(lte)(R|Q)
    ,(lte)5 ,(lte)b4,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a2,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a4,(lte)(K|P|B|Q)
    ,(lte)5 ,(lte)c4,(lte)(K|P|B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|B|Q)   ,(lte)d1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b4[] =
{
(lte)8
    ,(lte)1 ,(lte)a4,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c4,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)b3,(lte)(K|R|Q)   ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)4 ,(lte)b5,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a3,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a5,(lte)(K|P|B|Q)
    ,(lte)4 ,(lte)c5,(lte)(K|P|B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|B|Q)   ,(lte)d2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b5[] =
{
(lte)8
    ,(lte)1 ,(lte)a5,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c5,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)b4,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)3 ,(lte)b6,(lte)(K|R|Q)   ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a4,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a6,(lte)(K|P|B|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|P|B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)4 ,(lte)c4,(lte)(K|B|Q)   ,(lte)d3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b6[] =
{
(lte)8
    ,(lte)1 ,(lte)a6,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)b5,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|R|Q)   ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a5,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a7,(lte)(K|P|B|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|P|B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)5 ,(lte)c5,(lte)(K|B|Q)   ,(lte)d4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b7[] =
{
(lte)8
    ,(lte)1 ,(lte)a7,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c7,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)b6,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|R|Q)
    ,(lte)1 ,(lte)a6,(lte)(K|B|Q)
    ,(lte)1 ,(lte)a8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|B|Q)   ,(lte)d5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_b8[] =
{
(lte)5
    ,(lte)1 ,(lte)a8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c8,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)b7,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)1 ,(lte)a7,(lte)(K|B|Q)
    ,(lte)6 ,(lte)c7,(lte)(K|B|Q)   ,(lte)d6,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_white_lookup_c1[] =
{
(lte)5
    ,(lte)2 ,(lte)b1,(lte)(K|R|Q)   ,(lte)a1,(lte)(R|Q)
    ,(lte)5 ,(lte)d1,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)c2,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|P|B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)5 ,(lte)d2,(lte)(K|P|B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
};
static const lte attacks_white_lookup_c2[] =
{
(lte)8
    ,(lte)2 ,(lte)b2,(lte)(K|R|Q)   ,(lte)a2,(lte)(R|Q)
    ,(lte)5 ,(lte)d2,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|B|Q)
    ,(lte)2 ,(lte)b3,(lte)(K|P|B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)5 ,(lte)d3,(lte)(K|P|B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_c3[] =
{
(lte)8
    ,(lte)2 ,(lte)b3,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q)
    ,(lte)5 ,(lte)d3,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q)
    ,(lte)5 ,(lte)c4,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|B|Q)   ,(lte)a1,(lte)(B|Q)
    ,(lte)2 ,(lte)b4,(lte)(K|P|B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|P|B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|B|Q)   ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_white_lookup_c4[] =
{
(lte)8
    ,(lte)2 ,(lte)b4,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)4 ,(lte)c5,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b3,(lte)(K|B|Q)   ,(lte)a2,(lte)(B|Q)
    ,(lte)2 ,(lte)b5,(lte)(K|P|B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|P|B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|B|Q)   ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_white_lookup_c5[] =
{
(lte)8
    ,(lte)2 ,(lte)b5,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)c4,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b4,(lte)(K|B|Q)   ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)b6,(lte)(K|P|B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|P|B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|B|Q)   ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_white_lookup_c6[] =
{
(lte)8
    ,(lte)2 ,(lte)b6,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q)
    ,(lte)5 ,(lte)d6,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)c5,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b5,(lte)(K|B|Q)   ,(lte)a4,(lte)(B|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|P|B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|P|B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|B|Q)   ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_c7[] =
{
(lte)8
    ,(lte)2 ,(lte)b7,(lte)(K|R|Q)   ,(lte)a7,(lte)(R|Q)
    ,(lte)5 ,(lte)d7,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|R|Q)
    ,(lte)2 ,(lte)b6,(lte)(K|B|Q)   ,(lte)a5,(lte)(B|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|P|B|Q)
    ,(lte)5 ,(lte)d6,(lte)(K|B|Q)   ,(lte)e5,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_white_lookup_c8[] =
{
(lte)5
    ,(lte)2 ,(lte)b8,(lte)(K|R|Q)   ,(lte)a8,(lte)(R|Q)
    ,(lte)5 ,(lte)d8,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)c7,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|B|Q)   ,(lte)a6,(lte)(B|Q)
    ,(lte)5 ,(lte)d7,(lte)(K|B|Q)   ,(lte)e6,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_white_lookup_d1[] =
{
(lte)5
    ,(lte)3 ,(lte)c1,(lte)(K|R|Q)   ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)4 ,(lte)e1,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)d2,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c2,(lte)(K|P|B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)4 ,(lte)e2,(lte)(K|P|B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_white_lookup_d2[] =
{
(lte)8
    ,(lte)3 ,(lte)c2,(lte)(K|R|Q)   ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)4 ,(lte)e2,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)d3,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|B|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|P|B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)4 ,(lte)e3,(lte)(K|P|B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_d3[] =
{
(lte)8
    ,(lte)3 ,(lte)c3,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)4 ,(lte)e3,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|B|Q)   ,(lte)b1,(lte)(B|Q)
    ,(lte)3 ,(lte)c4,(lte)(K|P|B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|P|B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|B|Q)   ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_white_lookup_d4[] =
{
(lte)8
    ,(lte)3 ,(lte)c4,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|B|Q)   ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)3 ,(lte)c5,(lte)(K|P|B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|P|B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|B|Q)   ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_white_lookup_d5[] =
{
(lte)8
    ,(lte)3 ,(lte)c5,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c4,(lte)(K|B|Q)   ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|P|B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|P|B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|B|Q)   ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_d6[] =
{
(lte)8
    ,(lte)3 ,(lte)c6,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)4 ,(lte)e6,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c5,(lte)(K|B|Q)   ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|P|B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|P|B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|B|Q)   ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_white_lookup_d7[] =
{
(lte)8
    ,(lte)3 ,(lte)c7,(lte)(K|R|Q)   ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)4 ,(lte)e7,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)d6,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|R|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|B|Q)   ,(lte)b5,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|P|B|Q)
    ,(lte)4 ,(lte)e6,(lte)(K|B|Q)   ,(lte)f5,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_white_lookup_d8[] =
{
(lte)5
    ,(lte)3 ,(lte)c8,(lte)(K|R|Q)   ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)4 ,(lte)e8,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)d7,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)3 ,(lte)c7,(lte)(K|B|Q)   ,(lte)b6,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)4 ,(lte)e7,(lte)(K|B|Q)   ,(lte)f6,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_white_lookup_e1[] =
{
(lte)5
    ,(lte)4 ,(lte)d1,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)3 ,(lte)f1,(lte)(K|R|Q)   ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)e2,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d2,(lte)(K|P|B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)3 ,(lte)f2,(lte)(K|P|B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_white_lookup_e2[] =
{
(lte)8
    ,(lte)4 ,(lte)d2,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)3 ,(lte)f2,(lte)(K|R|Q)   ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)e3,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|B|Q)
    ,(lte)4 ,(lte)d3,(lte)(K|P|B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|P|B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_e3[] =
{
(lte)8
    ,(lte)4 ,(lte)d3,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q)
    ,(lte)5 ,(lte)e4,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|B|Q)   ,(lte)c1,(lte)(B|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|P|B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|P|B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|B|Q)   ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_white_lookup_e4[] =
{
(lte)8
    ,(lte)4 ,(lte)d4,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|B|Q)   ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|P|B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|P|B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|B|Q)   ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_e5[] =
{
(lte)8
    ,(lte)4 ,(lte)d5,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|B|Q)   ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|P|B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|P|B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|B|Q)   ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_white_lookup_e6[] =
{
(lte)8
    ,(lte)4 ,(lte)d6,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)e5,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|B|Q)   ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|P|B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|P|B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|B|Q)   ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_white_lookup_e7[] =
{
(lte)8
    ,(lte)4 ,(lte)d7,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)3 ,(lte)f7,(lte)(K|R|Q)   ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)e6,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|R|Q)
    ,(lte)4 ,(lte)d6,(lte)(K|B|Q)   ,(lte)c5,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|P|B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|B|Q)   ,(lte)g5,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_white_lookup_e8[] =
{
(lte)5
    ,(lte)4 ,(lte)d8,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)3 ,(lte)f8,(lte)(K|R|Q)   ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)e7,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)4 ,(lte)d7,(lte)(K|B|Q)   ,(lte)c6,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)3 ,(lte)f7,(lte)(K|B|Q)   ,(lte)g6,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_white_lookup_f1[] =
{
(lte)5
    ,(lte)5 ,(lte)e1,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)2 ,(lte)g1,(lte)(K|R|Q)   ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)f2,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)5 ,(lte)e2,(lte)(K|P|B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|P|B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_white_lookup_f2[] =
{
(lte)8
    ,(lte)5 ,(lte)e2,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|R|Q)   ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)f3,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|B|Q)
    ,(lte)5 ,(lte)e3,(lte)(K|P|B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|P|B|Q) ,(lte)h4,(lte)(B|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_f3[] =
{
(lte)8
    ,(lte)5 ,(lte)e3,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q)
    ,(lte)5 ,(lte)f4,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|B|Q)   ,(lte)d1,(lte)(B|Q)
    ,(lte)5 ,(lte)e4,(lte)(K|P|B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|P|B|Q) ,(lte)h5,(lte)(B|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|B|Q)   ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_white_lookup_f4[] =
{
(lte)8
    ,(lte)5 ,(lte)e4,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)4 ,(lte)f5,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|B|Q)   ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|P|B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|P|B|Q) ,(lte)h6,(lte)(B|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|B|Q)   ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_white_lookup_f5[] =
{
(lte)8
    ,(lte)5 ,(lte)e5,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)f4,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|B|Q)   ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|P|B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|P|B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|B|Q)   ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_white_lookup_f6[] =
{
(lte)8
    ,(lte)5 ,(lte)e6,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)f5,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q)
    ,(lte)5 ,(lte)e5,(lte)(K|B|Q)   ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|P|B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|P|B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|B|Q)   ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_white_lookup_f7[] =
{
(lte)8
    ,(lte)5 ,(lte)e7,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|R|Q)   ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)f6,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|R|Q)
    ,(lte)5 ,(lte)e6,(lte)(K|B|Q)   ,(lte)d5,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|P|B|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|B|Q)   ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_white_lookup_f8[] =
{
(lte)5
    ,(lte)5 ,(lte)e8,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)2 ,(lte)g8,(lte)(K|R|Q)   ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)f7,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)5 ,(lte)e7,(lte)(K|B|Q)   ,(lte)d6,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|B|Q)   ,(lte)h6,(lte)(B|Q)
};
static const lte attacks_white_lookup_g1[] =
{
(lte)5
    ,(lte)6 ,(lte)f1,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|R|Q)
    ,(lte)7 ,(lte)g2,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)6 ,(lte)f2,(lte)(K|P|B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|P|B|Q)
};
static const lte attacks_white_lookup_g2[] =
{
(lte)8
    ,(lte)6 ,(lte)f2,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|R|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g3,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|B|Q)
    ,(lte)6 ,(lte)f3,(lte)(K|P|B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g3[] =
{
(lte)8
    ,(lte)6 ,(lte)f3,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|R|Q)   ,(lte)g1,(lte)(R|Q)
    ,(lte)5 ,(lte)g4,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|B|Q)   ,(lte)e1,(lte)(B|Q)
    ,(lte)5 ,(lte)f4,(lte)(K|P|B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g4[] =
{
(lte)8
    ,(lte)6 ,(lte)f4,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|R|Q)
    ,(lte)3 ,(lte)g3,(lte)(K|R|Q)   ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)4 ,(lte)g5,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|B|Q)   ,(lte)e2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
    ,(lte)4 ,(lte)f5,(lte)(K|P|B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g5[] =
{
(lte)8
    ,(lte)6 ,(lte)f5,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|R|Q)
    ,(lte)4 ,(lte)g4,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)3 ,(lte)g6,(lte)(K|R|Q)   ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)4 ,(lte)f4,(lte)(K|B|Q)   ,(lte)e3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|P|B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g6[] =
{
(lte)8
    ,(lte)6 ,(lte)f6,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|R|Q)
    ,(lte)5 ,(lte)g5,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|R|Q)   ,(lte)g8,(lte)(R|Q)
    ,(lte)5 ,(lte)f5,(lte)(K|B|Q)   ,(lte)e4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|P|B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g7[] =
{
(lte)8
    ,(lte)6 ,(lte)f7,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g6,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)f6,(lte)(K|B|Q)   ,(lte)e5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_g8[] =
{
(lte)5
    ,(lte)6 ,(lte)f8,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|R|Q)
    ,(lte)7 ,(lte)g7,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)6 ,(lte)f7,(lte)(K|B|Q)   ,(lte)e6,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|B|Q)
};
static const lte attacks_white_lookup_h1[] =
{
(lte)3
    ,(lte)7 ,(lte)g1,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)7 ,(lte)h2,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)g2,(lte)(K|P|B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h2[] =
{
(lte)5
    ,(lte)7 ,(lte)g2,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)h3,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|B|Q)
    ,(lte)6 ,(lte)g3,(lte)(K|P|B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h3[] =
{
(lte)5
    ,(lte)7 ,(lte)g3,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)2 ,(lte)h2,(lte)(K|R|Q)   ,(lte)h1,(lte)(R|Q)
    ,(lte)5 ,(lte)h4,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|B|Q)   ,(lte)f1,(lte)(B|Q)
    ,(lte)5 ,(lte)g4,(lte)(K|P|B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h4[] =
{
(lte)5
    ,(lte)7 ,(lte)g4,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)3 ,(lte)h3,(lte)(K|R|Q)   ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)4 ,(lte)h5,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)3 ,(lte)g3,(lte)(K|B|Q)   ,(lte)f2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
    ,(lte)4 ,(lte)g5,(lte)(K|P|B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h5[] =
{
(lte)5
    ,(lte)7 ,(lte)g5,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)4 ,(lte)h4,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)3 ,(lte)h6,(lte)(K|R|Q)   ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)4 ,(lte)g4,(lte)(K|B|Q)   ,(lte)f3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
    ,(lte)3 ,(lte)g6,(lte)(K|P|B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h6[] =
{
(lte)5
    ,(lte)7 ,(lte)g6,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)5 ,(lte)h5,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)2 ,(lte)h7,(lte)(K|R|Q)   ,(lte)h8,(lte)(R|Q)
    ,(lte)5 ,(lte)g5,(lte)(K|B|Q)   ,(lte)f4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|P|B|Q) ,(lte)f8,(lte)(B|Q)
};
static const lte attacks_white_lookup_h7[] =
{
(lte)5
    ,(lte)7 ,(lte)g7,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)6 ,(lte)h6,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g6,(lte)(K|B|Q)   ,(lte)f5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|P|B|Q)
};
static const lte attacks_white_lookup_h8[] =
{
(lte)3
    ,(lte)7 ,(lte)g8,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)7 ,(lte)h7,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)g7,(lte)(K|B|Q)   ,(lte)f6,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
};

// attacks_white_lookup
const lte *attacks_white_lookup[] =
{
    attacks_white_lookup_a8,
    attacks_white_lookup_b8,
    attacks_white_lookup_c8,
    attacks_white_lookup_d8,
    attacks_white_lookup_e8,
    attacks_white_lookup_f8,
    attacks_white_lookup_g8,
    attacks_white_lookup_h8,
    attacks_white_lookup_a7,
    attacks_white_lookup_b7,
    attacks_white_lookup_c7,
    attacks_white_lookup_d7,
    attacks_white_lookup_e7,
    attacks_white_lookup_f7,
    attacks_white_lookup_g7,
    attacks_white_lookup_h7,
    attacks_white_lookup_a6,
    attacks_white_lookup_b6,
    attacks_white_lookup_c6,
    attacks_white_lookup_d6,
    attacks_white_lookup_e6,
    attacks_white_lookup_f6,
    attacks_white_lookup_g6,
    attacks_white_lookup_h6,
    attacks_white_lookup_a5,
    attacks_white_lookup_b5,
    attacks_white_lookup_c5,
    attacks_white_lookup_d5,
    attacks_white_lookup_e5,
    attacks_white_lookup_f5,
    attacks_white_lookup_g5,
    attacks_white_lookup_h5,
    attacks_white_lookup_a4,
    attacks_white_lookup_b4,
    attacks_white_lookup_c4,
    attacks_white_lookup_d4,
    attacks_white_lookup_e4,
    attacks_white_lookup_f4,
    attacks_white_lookup_g4,
    attacks_white_lookup_h4,
    attacks_white_lookup_a3,
    attacks_white_lookup_b3,
    attacks_white_lookup_c3,
    attacks_white_lookup_d3,
    attacks_white_lookup_e3,
    attacks_white_lookup_f3,
    attacks_white_lookup_g3,
    attacks_white_lookup_h3,
    attacks_white_lookup_a2,
    attacks_white_lookup_b2,
    attacks_white_lookup_c2,
    attacks_white_lookup_d2,
    attacks_white_lookup_e2,
    attacks_white_lookup_f2,
    attacks_white_lookup_g2,
    attacks_white_lookup_h2,
    attacks_white_lookup_a1,
    attacks_white_lookup_b1,
    attacks_white_lookup_c1,
    attacks_white_lookup_d1,
    attacks_white_lookup_e1,
    attacks_white_lookup_f1,
    attacks_white_lookup_g1,
    attacks_white_lookup_h1
};

// Attack from up to 8 rays on a black piece
static const lte attacks_black_lookup_a1[] =
{
(lte)3
    ,(lte)7 ,(lte)b1,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)a2,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)7 ,(lte)b2,(lte)(K|B|Q)   ,(lte)c3,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
};
static const lte attacks_black_lookup_a2[] =
{
(lte)5
    ,(lte)7 ,(lte)b2,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)a1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)a3,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)6 ,(lte)b3,(lte)(K|B|Q)   ,(lte)c4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_a3[] =
{
(lte)5
    ,(lte)7 ,(lte)b3,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)a2,(lte)(K|R|Q)   ,(lte)a1,(lte)(R|Q)
    ,(lte)5 ,(lte)a4,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q) ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)5 ,(lte)b4,(lte)(K|B|Q)   ,(lte)c5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|P|B|Q) ,(lte)c1,(lte)(B|Q)
};
static const lte attacks_black_lookup_a4[] =
{
(lte)5
    ,(lte)7 ,(lte)b4,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)a3,(lte)(K|R|Q)   ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)4 ,(lte)a5,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q) ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)4 ,(lte)b5,(lte)(K|B|Q)   ,(lte)c6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)3 ,(lte)b3,(lte)(K|P|B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
};
static const lte attacks_black_lookup_a5[] =
{
(lte)5
    ,(lte)7 ,(lte)b5,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)a4,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)3 ,(lte)a6,(lte)(K|R|Q)   ,(lte)a7,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)3 ,(lte)b6,(lte)(K|B|Q)   ,(lte)c7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)4 ,(lte)b4,(lte)(K|P|B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_black_lookup_a6[] =
{
(lte)5
    ,(lte)7 ,(lte)b6,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)a5,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)2 ,(lte)a7,(lte)(K|R|Q)   ,(lte)a8,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|B|Q)   ,(lte)c8,(lte)(B|Q)
    ,(lte)5 ,(lte)b5,(lte)(K|P|B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_black_lookup_a7[] =
{
(lte)5
    ,(lte)7 ,(lte)b7,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)a6,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)1 ,(lte)a8,(lte)(K|R|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|B|Q)
    ,(lte)6 ,(lte)b6,(lte)(K|P|B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_black_lookup_a8[] =
{
(lte)3
    ,(lte)7 ,(lte)b8,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)a7,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q) ,(lte)a5,(lte)(R|Q) ,(lte)a4,(lte)(R|Q) ,(lte)a3,(lte)(R|Q) ,(lte)a2,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)7 ,(lte)b7,(lte)(K|P|B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b1[] =
{
(lte)5
    ,(lte)1 ,(lte)a1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c1,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)b2,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a2,(lte)(K|B|Q)
    ,(lte)6 ,(lte)c2,(lte)(K|B|Q)   ,(lte)d3,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
};
static const lte attacks_black_lookup_b2[] =
{
(lte)8
    ,(lte)1 ,(lte)a2,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c2,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)b3,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a1,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a3,(lte)(K|B|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|B|Q)   ,(lte)d4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_b3[] =
{
(lte)8
    ,(lte)1 ,(lte)a3,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|R|Q)   ,(lte)b1,(lte)(R|Q)
    ,(lte)5 ,(lte)b4,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a2,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a4,(lte)(K|B|Q)
    ,(lte)5 ,(lte)c4,(lte)(K|B|Q)   ,(lte)d5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|P|B|Q) ,(lte)d1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b4[] =
{
(lte)8
    ,(lte)1 ,(lte)a4,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c4,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)b3,(lte)(K|R|Q)   ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)4 ,(lte)b5,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a3,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a5,(lte)(K|B|Q)
    ,(lte)4 ,(lte)c5,(lte)(K|B|Q)   ,(lte)d6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|P|B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b5[] =
{
(lte)8
    ,(lte)1 ,(lte)a5,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c5,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)b4,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)3 ,(lte)b6,(lte)(K|R|Q)   ,(lte)b7,(lte)(R|Q) ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a4,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a6,(lte)(K|B|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|B|Q)   ,(lte)d7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
    ,(lte)4 ,(lte)c4,(lte)(K|P|B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b6[] =
{
(lte)8
    ,(lte)1 ,(lte)a6,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)b5,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|R|Q)   ,(lte)b8,(lte)(R|Q)
    ,(lte)1 ,(lte)a5,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a7,(lte)(K|B|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|B|Q)   ,(lte)d8,(lte)(B|Q)
    ,(lte)5 ,(lte)c5,(lte)(K|P|B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b7[] =
{
(lte)8
    ,(lte)1 ,(lte)a7,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c7,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)b6,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|R|Q)
    ,(lte)1 ,(lte)a6,(lte)(K|P|B|Q)
    ,(lte)1 ,(lte)a8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|B|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|P|B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_b8[] =
{
(lte)5
    ,(lte)1 ,(lte)a8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c8,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)b7,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)b1,(lte)(R|Q)
    ,(lte)1 ,(lte)a7,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)c7,(lte)(K|P|B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_black_lookup_c1[] =
{
(lte)5
    ,(lte)2 ,(lte)b1,(lte)(K|R|Q)   ,(lte)a1,(lte)(R|Q)
    ,(lte)5 ,(lte)d1,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q) ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)c2,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|B|Q)   ,(lte)a3,(lte)(B|Q)
    ,(lte)5 ,(lte)d2,(lte)(K|B|Q)   ,(lte)e3,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
};
static const lte attacks_black_lookup_c2[] =
{
(lte)8
    ,(lte)2 ,(lte)b2,(lte)(K|R|Q)   ,(lte)a2,(lte)(R|Q)
    ,(lte)5 ,(lte)d2,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)c3,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)1 ,(lte)b1,(lte)(K|P|B|Q)
    ,(lte)2 ,(lte)b3,(lte)(K|B|Q)   ,(lte)a4,(lte)(B|Q)
    ,(lte)5 ,(lte)d3,(lte)(K|B|Q)   ,(lte)e4,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_c3[] =
{
(lte)8
    ,(lte)2 ,(lte)b3,(lte)(K|R|Q)   ,(lte)a3,(lte)(R|Q)
    ,(lte)5 ,(lte)d3,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q)
    ,(lte)5 ,(lte)c4,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b2,(lte)(K|P|B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)2 ,(lte)b4,(lte)(K|B|Q)   ,(lte)a5,(lte)(B|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|B|Q)   ,(lte)e5,(lte)(B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|P|B|Q) ,(lte)e1,(lte)(B|Q)
};
static const lte attacks_black_lookup_c4[] =
{
(lte)8
    ,(lte)2 ,(lte)b4,(lte)(K|R|Q)   ,(lte)a4,(lte)(R|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)4 ,(lte)c5,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b3,(lte)(K|P|B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)2 ,(lte)b5,(lte)(K|B|Q)   ,(lte)a6,(lte)(B|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|B|Q)   ,(lte)e6,(lte)(B|Q) ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|P|B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_black_lookup_c5[] =
{
(lte)8
    ,(lte)2 ,(lte)b5,(lte)(K|R|Q)   ,(lte)a5,(lte)(R|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)c4,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b4,(lte)(K|P|B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)b6,(lte)(K|B|Q)   ,(lte)a7,(lte)(B|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|B|Q)   ,(lte)e7,(lte)(B|Q) ,(lte)f8,(lte)(B|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|P|B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_black_lookup_c6[] =
{
(lte)8
    ,(lte)2 ,(lte)b6,(lte)(K|R|Q)   ,(lte)a6,(lte)(R|Q)
    ,(lte)5 ,(lte)d6,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)c5,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q)
    ,(lte)2 ,(lte)b5,(lte)(K|P|B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|B|Q)   ,(lte)a8,(lte)(B|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|B|Q)   ,(lte)e8,(lte)(B|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|P|B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_c7[] =
{
(lte)8
    ,(lte)2 ,(lte)b7,(lte)(K|R|Q)   ,(lte)a7,(lte)(R|Q)
    ,(lte)5 ,(lte)d7,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)c6,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|R|Q)
    ,(lte)2 ,(lte)b6,(lte)(K|P|B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)1 ,(lte)b8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|B|Q)
    ,(lte)5 ,(lte)d6,(lte)(K|P|B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_black_lookup_c8[] =
{
(lte)5
    ,(lte)2 ,(lte)b8,(lte)(K|R|Q)   ,(lte)a8,(lte)(R|Q)
    ,(lte)5 ,(lte)d8,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q) ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)c7,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)c1,(lte)(R|Q)
    ,(lte)2 ,(lte)b7,(lte)(K|P|B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)5 ,(lte)d7,(lte)(K|P|B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_black_lookup_d1[] =
{
(lte)5
    ,(lte)3 ,(lte)c1,(lte)(K|R|Q)   ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)4 ,(lte)e1,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q) ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)d2,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c2,(lte)(K|B|Q)   ,(lte)b3,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)4 ,(lte)e2,(lte)(K|B|Q)   ,(lte)f3,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_black_lookup_d2[] =
{
(lte)8
    ,(lte)3 ,(lte)c2,(lte)(K|R|Q)   ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)4 ,(lte)e2,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)d3,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)1 ,(lte)c1,(lte)(K|P|B|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|B|Q)   ,(lte)b4,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)4 ,(lte)e3,(lte)(K|B|Q)   ,(lte)f4,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_d3[] =
{
(lte)8
    ,(lte)3 ,(lte)c3,(lte)(K|R|Q)   ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)4 ,(lte)e3,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q)
    ,(lte)5 ,(lte)d4,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)2 ,(lte)c2,(lte)(K|P|B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)3 ,(lte)c4,(lte)(K|B|Q)   ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|B|Q)   ,(lte)f5,(lte)(B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|P|B|Q) ,(lte)f1,(lte)(B|Q)
};
static const lte attacks_black_lookup_d4[] =
{
(lte)8
    ,(lte)3 ,(lte)c4,(lte)(K|R|Q)   ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c3,(lte)(K|P|B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)3 ,(lte)c5,(lte)(K|B|Q)   ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|B|Q)   ,(lte)f6,(lte)(B|Q) ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|P|B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_black_lookup_d5[] =
{
(lte)8
    ,(lte)3 ,(lte)c5,(lte)(K|R|Q)   ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c4,(lte)(K|P|B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|B|Q)   ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|B|Q)   ,(lte)f7,(lte)(B|Q) ,(lte)g8,(lte)(B|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|P|B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_d6[] =
{
(lte)8
    ,(lte)3 ,(lte)c6,(lte)(K|R|Q)   ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)4 ,(lte)e6,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)d5,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q)
    ,(lte)3 ,(lte)c5,(lte)(K|P|B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)c7,(lte)(K|B|Q)   ,(lte)b8,(lte)(B|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|B|Q)   ,(lte)f8,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|P|B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_black_lookup_d7[] =
{
(lte)8
    ,(lte)3 ,(lte)c7,(lte)(K|R|Q)   ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)4 ,(lte)e7,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)d6,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|R|Q)
    ,(lte)3 ,(lte)c6,(lte)(K|P|B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)1 ,(lte)c8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|B|Q)
    ,(lte)4 ,(lte)e6,(lte)(K|P|B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_black_lookup_d8[] =
{
(lte)5
    ,(lte)3 ,(lte)c8,(lte)(K|R|Q)   ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)4 ,(lte)e8,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q) ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)d7,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)d1,(lte)(R|Q)
    ,(lte)3 ,(lte)c7,(lte)(K|P|B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)4 ,(lte)e7,(lte)(K|P|B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_black_lookup_e1[] =
{
(lte)5
    ,(lte)4 ,(lte)d1,(lte)(K|R|Q)   ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)3 ,(lte)f1,(lte)(K|R|Q)   ,(lte)g1,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)e2,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d2,(lte)(K|B|Q)   ,(lte)c3,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a5,(lte)(B|Q)
    ,(lte)3 ,(lte)f2,(lte)(K|B|Q)   ,(lte)g3,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_black_lookup_e2[] =
{
(lte)8
    ,(lte)4 ,(lte)d2,(lte)(K|R|Q)   ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)3 ,(lte)f2,(lte)(K|R|Q)   ,(lte)g2,(lte)(R|Q) ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)e3,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)1 ,(lte)d1,(lte)(K|P|B|Q)
    ,(lte)4 ,(lte)d3,(lte)(K|B|Q)   ,(lte)c4,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|B|Q)   ,(lte)g4,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_e3[] =
{
(lte)8
    ,(lte)4 ,(lte)d3,(lte)(K|R|Q)   ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q)
    ,(lte)5 ,(lte)e4,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)2 ,(lte)d2,(lte)(K|P|B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|B|Q)   ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|B|Q)   ,(lte)g5,(lte)(B|Q) ,(lte)h6,(lte)(B|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|P|B|Q) ,(lte)g1,(lte)(B|Q)
};
static const lte attacks_black_lookup_e4[] =
{
(lte)8
    ,(lte)4 ,(lte)d4,(lte)(K|R|Q)   ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)3 ,(lte)d3,(lte)(K|P|B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|B|Q)   ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|B|Q)   ,(lte)g6,(lte)(B|Q) ,(lte)h7,(lte)(B|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|P|B|Q) ,(lte)g2,(lte)(B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_e5[] =
{
(lte)8
    ,(lte)4 ,(lte)d5,(lte)(K|R|Q)   ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d4,(lte)(K|P|B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)3 ,(lte)d6,(lte)(K|B|Q)   ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|B|Q)   ,(lte)g7,(lte)(B|Q) ,(lte)h8,(lte)(B|Q)
    ,(lte)3 ,(lte)f4,(lte)(K|P|B|Q) ,(lte)g3,(lte)(B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_black_lookup_e6[] =
{
(lte)8
    ,(lte)4 ,(lte)d6,(lte)(K|R|Q)   ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)e5,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q)
    ,(lte)4 ,(lte)d5,(lte)(K|P|B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)2 ,(lte)d7,(lte)(K|B|Q)   ,(lte)c8,(lte)(B|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|B|Q)   ,(lte)g8,(lte)(B|Q)
    ,(lte)3 ,(lte)f5,(lte)(K|P|B|Q) ,(lte)g4,(lte)(B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_black_lookup_e7[] =
{
(lte)8
    ,(lte)4 ,(lte)d7,(lte)(K|R|Q)   ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)3 ,(lte)f7,(lte)(K|R|Q)   ,(lte)g7,(lte)(R|Q) ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)e6,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|R|Q)
    ,(lte)4 ,(lte)d6,(lte)(K|P|B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)1 ,(lte)d8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|P|B|Q) ,(lte)g5,(lte)(B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_black_lookup_e8[] =
{
(lte)5
    ,(lte)4 ,(lte)d8,(lte)(K|R|Q)   ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)3 ,(lte)f8,(lte)(K|R|Q)   ,(lte)g8,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)e7,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)e1,(lte)(R|Q)
    ,(lte)4 ,(lte)d7,(lte)(K|P|B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a4,(lte)(B|Q)
    ,(lte)3 ,(lte)f7,(lte)(K|P|B|Q) ,(lte)g6,(lte)(B|Q) ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_black_lookup_f1[] =
{
(lte)5
    ,(lte)5 ,(lte)e1,(lte)(K|R|Q)   ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)2 ,(lte)g1,(lte)(K|R|Q)   ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)f2,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)5 ,(lte)e2,(lte)(K|B|Q)   ,(lte)d3,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b5,(lte)(B|Q) ,(lte)a6,(lte)(B|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|B|Q)   ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_black_lookup_f2[] =
{
(lte)8
    ,(lte)5 ,(lte)e2,(lte)(K|R|Q)   ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|R|Q)   ,(lte)h2,(lte)(R|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)f3,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)1 ,(lte)e1,(lte)(K|P|B|Q)
    ,(lte)5 ,(lte)e3,(lte)(K|B|Q)   ,(lte)d4,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|B|Q)   ,(lte)h4,(lte)(B|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_f3[] =
{
(lte)8
    ,(lte)5 ,(lte)e3,(lte)(K|R|Q)   ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q)
    ,(lte)5 ,(lte)f4,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)2 ,(lte)e2,(lte)(K|P|B|Q) ,(lte)d1,(lte)(B|Q)
    ,(lte)5 ,(lte)e4,(lte)(K|B|Q)   ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|B|Q)   ,(lte)h5,(lte)(B|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|P|B|Q) ,(lte)h1,(lte)(B|Q)
};
static const lte attacks_black_lookup_f4[] =
{
(lte)8
    ,(lte)5 ,(lte)e4,(lte)(K|R|Q)   ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)4 ,(lte)f5,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)3 ,(lte)e3,(lte)(K|P|B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)4 ,(lte)e5,(lte)(K|B|Q)   ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|B|Q)   ,(lte)h6,(lte)(B|Q)
    ,(lte)2 ,(lte)g3,(lte)(K|P|B|Q) ,(lte)h2,(lte)(B|Q)
};
static const lte attacks_black_lookup_f5[] =
{
(lte)8
    ,(lte)5 ,(lte)e5,(lte)(K|R|Q)   ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q)
    ,(lte)4 ,(lte)f4,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)f8,(lte)(R|Q)
    ,(lte)4 ,(lte)e4,(lte)(K|P|B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)3 ,(lte)e6,(lte)(K|B|Q)   ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|B|Q)   ,(lte)h7,(lte)(B|Q)
    ,(lte)2 ,(lte)g4,(lte)(K|P|B|Q) ,(lte)h3,(lte)(B|Q)
};
static const lte attacks_black_lookup_f6[] =
{
(lte)8
    ,(lte)5 ,(lte)e6,(lte)(K|R|Q)   ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q)
    ,(lte)5 ,(lte)f5,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q)
    ,(lte)5 ,(lte)e5,(lte)(K|P|B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)2 ,(lte)e7,(lte)(K|B|Q)   ,(lte)d8,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|B|Q)   ,(lte)h8,(lte)(B|Q)
    ,(lte)2 ,(lte)g5,(lte)(K|P|B|Q) ,(lte)h4,(lte)(B|Q)
};
static const lte attacks_black_lookup_f7[] =
{
(lte)8
    ,(lte)5 ,(lte)e7,(lte)(K|R|Q)   ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|R|Q)   ,(lte)h7,(lte)(R|Q)
    ,(lte)6 ,(lte)f6,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|R|Q)
    ,(lte)5 ,(lte)e6,(lte)(K|P|B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)1 ,(lte)e8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|B|Q)
    ,(lte)2 ,(lte)g6,(lte)(K|P|B|Q) ,(lte)h5,(lte)(B|Q)
};
static const lte attacks_black_lookup_f8[] =
{
(lte)5
    ,(lte)5 ,(lte)e8,(lte)(K|R|Q)   ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)2 ,(lte)g8,(lte)(K|R|Q)   ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)f7,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)f5,(lte)(R|Q) ,(lte)f4,(lte)(R|Q) ,(lte)f3,(lte)(R|Q) ,(lte)f2,(lte)(R|Q) ,(lte)f1,(lte)(R|Q)
    ,(lte)5 ,(lte)e7,(lte)(K|P|B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b4,(lte)(B|Q) ,(lte)a3,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|P|B|Q) ,(lte)h6,(lte)(B|Q)
};
static const lte attacks_black_lookup_g1[] =
{
(lte)5
    ,(lte)6 ,(lte)f1,(lte)(K|R|Q)   ,(lte)e1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|R|Q)
    ,(lte)7 ,(lte)g2,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)6 ,(lte)f2,(lte)(K|B|Q)   ,(lte)e3,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c5,(lte)(B|Q) ,(lte)b6,(lte)(B|Q) ,(lte)a7,(lte)(B|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|B|Q)
};
static const lte attacks_black_lookup_g2[] =
{
(lte)8
    ,(lte)6 ,(lte)f2,(lte)(K|R|Q)   ,(lte)e2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|R|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g3,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)1 ,(lte)f1,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)f3,(lte)(K|B|Q)   ,(lte)e4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g3[] =
{
(lte)8
    ,(lte)6 ,(lte)f3,(lte)(K|R|Q)   ,(lte)e3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|R|Q)   ,(lte)g1,(lte)(R|Q)
    ,(lte)5 ,(lte)g4,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)2 ,(lte)f2,(lte)(K|P|B|Q) ,(lte)e1,(lte)(B|Q)
    ,(lte)5 ,(lte)f4,(lte)(K|B|Q)   ,(lte)e5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h2,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g4[] =
{
(lte)8
    ,(lte)6 ,(lte)f4,(lte)(K|R|Q)   ,(lte)e4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|R|Q)
    ,(lte)3 ,(lte)g3,(lte)(K|R|Q)   ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)4 ,(lte)g5,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)3 ,(lte)f3,(lte)(K|P|B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
    ,(lte)4 ,(lte)f5,(lte)(K|B|Q)   ,(lte)e6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h3,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g5[] =
{
(lte)8
    ,(lte)6 ,(lte)f5,(lte)(K|R|Q)   ,(lte)e5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|R|Q)
    ,(lte)4 ,(lte)g4,(lte)(K|R|Q)   ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)3 ,(lte)g6,(lte)(K|R|Q)   ,(lte)g7,(lte)(R|Q) ,(lte)g8,(lte)(R|Q)
    ,(lte)4 ,(lte)f4,(lte)(K|P|B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)3 ,(lte)f6,(lte)(K|B|Q)   ,(lte)e7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h4,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g6[] =
{
(lte)8
    ,(lte)6 ,(lte)f6,(lte)(K|R|Q)   ,(lte)e6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|R|Q)
    ,(lte)5 ,(lte)g5,(lte)(K|R|Q)   ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|R|Q)   ,(lte)g8,(lte)(R|Q)
    ,(lte)5 ,(lte)f5,(lte)(K|P|B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)2 ,(lte)f7,(lte)(K|B|Q)   ,(lte)e8,(lte)(B|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h5,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g7[] =
{
(lte)8
    ,(lte)6 ,(lte)f7,(lte)(K|R|Q)   ,(lte)e7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g6,(lte)(K|R|Q)   ,(lte)g5,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)f6,(lte)(K|P|B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
    ,(lte)1 ,(lte)f8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|B|Q)
    ,(lte)1 ,(lte)h6,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_g8[] =
{
(lte)5
    ,(lte)6 ,(lte)f8,(lte)(K|R|Q)   ,(lte)e8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|R|Q)
    ,(lte)7 ,(lte)g7,(lte)(K|R|Q)   ,(lte)g6,(lte)(R|Q) ,(lte)g5,(lte)(R|Q) ,(lte)g4,(lte)(R|Q) ,(lte)g3,(lte)(R|Q) ,(lte)g2,(lte)(R|Q) ,(lte)g1,(lte)(R|Q)
    ,(lte)6 ,(lte)f7,(lte)(K|P|B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c4,(lte)(B|Q) ,(lte)b3,(lte)(B|Q) ,(lte)a2,(lte)(B|Q)
    ,(lte)1 ,(lte)h7,(lte)(K|P|B|Q)
};
static const lte attacks_black_lookup_h1[] =
{
(lte)3
    ,(lte)7 ,(lte)g1,(lte)(K|R|Q)   ,(lte)f1,(lte)(R|Q) ,(lte)e1,(lte)(R|Q) ,(lte)d1,(lte)(R|Q) ,(lte)c1,(lte)(R|Q) ,(lte)b1,(lte)(R|Q) ,(lte)a1,(lte)(R|Q)
    ,(lte)7 ,(lte)h2,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)7 ,(lte)g2,(lte)(K|B|Q)   ,(lte)f3,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d5,(lte)(B|Q) ,(lte)c6,(lte)(B|Q) ,(lte)b7,(lte)(B|Q) ,(lte)a8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h2[] =
{
(lte)5
    ,(lte)7 ,(lte)g2,(lte)(K|R|Q)   ,(lte)f2,(lte)(R|Q) ,(lte)e2,(lte)(R|Q) ,(lte)d2,(lte)(R|Q) ,(lte)c2,(lte)(R|Q) ,(lte)b2,(lte)(R|Q) ,(lte)a2,(lte)(R|Q)
    ,(lte)1 ,(lte)h1,(lte)(K|R|Q)
    ,(lte)6 ,(lte)h3,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)1 ,(lte)g1,(lte)(K|P|B|Q)
    ,(lte)6 ,(lte)g3,(lte)(K|B|Q)   ,(lte)f4,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d6,(lte)(B|Q) ,(lte)c7,(lte)(B|Q) ,(lte)b8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h3[] =
{
(lte)5
    ,(lte)7 ,(lte)g3,(lte)(K|R|Q)   ,(lte)f3,(lte)(R|Q) ,(lte)e3,(lte)(R|Q) ,(lte)d3,(lte)(R|Q) ,(lte)c3,(lte)(R|Q) ,(lte)b3,(lte)(R|Q) ,(lte)a3,(lte)(R|Q)
    ,(lte)2 ,(lte)h2,(lte)(K|R|Q)   ,(lte)h1,(lte)(R|Q)
    ,(lte)5 ,(lte)h4,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q) ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)2 ,(lte)g2,(lte)(K|P|B|Q) ,(lte)f1,(lte)(B|Q)
    ,(lte)5 ,(lte)g4,(lte)(K|B|Q)   ,(lte)f5,(lte)(B|Q) ,(lte)e6,(lte)(B|Q) ,(lte)d7,(lte)(B|Q) ,(lte)c8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h4[] =
{
(lte)5
    ,(lte)7 ,(lte)g4,(lte)(K|R|Q)   ,(lte)f4,(lte)(R|Q) ,(lte)e4,(lte)(R|Q) ,(lte)d4,(lte)(R|Q) ,(lte)c4,(lte)(R|Q) ,(lte)b4,(lte)(R|Q) ,(lte)a4,(lte)(R|Q)
    ,(lte)3 ,(lte)h3,(lte)(K|R|Q)   ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)4 ,(lte)h5,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q) ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)3 ,(lte)g3,(lte)(K|P|B|Q) ,(lte)f2,(lte)(B|Q) ,(lte)e1,(lte)(B|Q)
    ,(lte)4 ,(lte)g5,(lte)(K|B|Q)   ,(lte)f6,(lte)(B|Q) ,(lte)e7,(lte)(B|Q) ,(lte)d8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h5[] =
{
(lte)5
    ,(lte)7 ,(lte)g5,(lte)(K|R|Q)   ,(lte)f5,(lte)(R|Q) ,(lte)e5,(lte)(R|Q) ,(lte)d5,(lte)(R|Q) ,(lte)c5,(lte)(R|Q) ,(lte)b5,(lte)(R|Q) ,(lte)a5,(lte)(R|Q)
    ,(lte)4 ,(lte)h4,(lte)(K|R|Q)   ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)3 ,(lte)h6,(lte)(K|R|Q)   ,(lte)h7,(lte)(R|Q) ,(lte)h8,(lte)(R|Q)
    ,(lte)4 ,(lte)g4,(lte)(K|P|B|Q) ,(lte)f3,(lte)(B|Q) ,(lte)e2,(lte)(B|Q) ,(lte)d1,(lte)(B|Q)
    ,(lte)3 ,(lte)g6,(lte)(K|B|Q)   ,(lte)f7,(lte)(B|Q) ,(lte)e8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h6[] =
{
(lte)5
    ,(lte)7 ,(lte)g6,(lte)(K|R|Q)   ,(lte)f6,(lte)(R|Q) ,(lte)e6,(lte)(R|Q) ,(lte)d6,(lte)(R|Q) ,(lte)c6,(lte)(R|Q) ,(lte)b6,(lte)(R|Q) ,(lte)a6,(lte)(R|Q)
    ,(lte)5 ,(lte)h5,(lte)(K|R|Q)   ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)2 ,(lte)h7,(lte)(K|R|Q)   ,(lte)h8,(lte)(R|Q)
    ,(lte)5 ,(lte)g5,(lte)(K|P|B|Q) ,(lte)f4,(lte)(B|Q) ,(lte)e3,(lte)(B|Q) ,(lte)d2,(lte)(B|Q) ,(lte)c1,(lte)(B|Q)
    ,(lte)2 ,(lte)g7,(lte)(K|B|Q)   ,(lte)f8,(lte)(B|Q)
};
static const lte attacks_black_lookup_h7[] =
{
(lte)5
    ,(lte)7 ,(lte)g7,(lte)(K|R|Q)   ,(lte)f7,(lte)(R|Q) ,(lte)e7,(lte)(R|Q) ,(lte)d7,(lte)(R|Q) ,(lte)c7,(lte)(R|Q) ,(lte)b7,(lte)(R|Q) ,(lte)a7,(lte)(R|Q)
    ,(lte)6 ,(lte)h6,(lte)(K|R|Q)   ,(lte)h5,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)1 ,(lte)h8,(lte)(K|R|Q)
    ,(lte)6 ,(lte)g6,(lte)(K|P|B|Q) ,(lte)f5,(lte)(B|Q) ,(lte)e4,(lte)(B|Q) ,(lte)d3,(lte)(B|Q) ,(lte)c2,(lte)(B|Q) ,(lte)b1,(lte)(B|Q)
    ,(lte)1 ,(lte)g8,(lte)(K|B|Q)
};
static const lte attacks_black_lookup_h8[] =
{
(lte)3
    ,(lte)7 ,(lte)g8,(lte)(K|R|Q)   ,(lte)f8,(lte)(R|Q) ,(lte)e8,(lte)(R|Q) ,(lte)d8,(lte)(R|Q) ,(lte)c8,(lte)(R|Q) ,(lte)b8,(lte)(R|Q) ,(lte)a8,(lte)(R|Q)
    ,(lte)7 ,(lte)h7,(lte)(K|R|Q)   ,(lte)h6,(lte)(R|Q) ,(lte)h5,(lte)(R|Q) ,(lte)h4,(lte)(R|Q) ,(lte)h3,(lte)(R|Q) ,(lte)h2,(lte)(R|Q) ,(lte)h1,(lte)(R|Q)
    ,(lte)7 ,(lte)g7,(lte)(K|P|B|Q) ,(lte)f6,(lte)(B|Q) ,(lte)e5,(lte)(B|Q) ,(lte)d4,(lte)(B|Q) ,(lte)c3,(lte)(B|Q) ,(lte)b2,(lte)(B|Q) ,(lte)a1,(lte)(B|Q)
};

// attacks_black_lookup
const lte *attacks_black_lookup[] =
{
    attacks_black_lookup_a8,
    attacks_black_lookup_b8,
    attacks_black_lookup_c8,
    attacks_black_lookup_d8,
    attacks_black_lookup_e8,
    attacks_black_lookup_f8,
    attacks_black_lookup_g8,
    attacks_black_lookup_h8,
    attacks_black_lookup_a7,
    attacks_black_lookup_b7,
    attacks_black_lookup_c7,
    attacks_black_lookup_d7,
    attacks_black_lookup_e7,
    attacks_black_lookup_f7,
    attacks_black_lookup_g7,
    attacks_black_lookup_h7,
    attacks_black_lookup_a6,
    attacks_black_lookup_b6,
    attacks_black_lookup_c6,
    attacks_black_lookup_d6,
    attacks_black_lookup_e6,
    attacks_black_lookup_f6,
    attacks_black_lookup_g6,
    attacks_black_lookup_h6,
    attacks_black_lookup_a5,
    attacks_black_lookup_b5,
    attacks_black_lookup_c5,
    attacks_black_lookup_d5,
    attacks_black_lookup_e5,
    attacks_black_lookup_f5,
    attacks_black_lookup_g5,
    attacks_black_lookup_h5,
    attacks_black_lookup_a4,
    attacks_black_lookup_b4,
    attacks_black_lookup_c4,
    attacks_black_lookup_d4,
    attacks_black_lookup_e4,
    attacks_black_lookup_f4,
    attacks_black_lookup_g4,
    attacks_black_lookup_h4,
    attacks_black_lookup_a3,
    attacks_black_lookup_b3,
    attacks_black_lookup_c3,
    attacks_black_lookup_d3,
    attacks_black_lookup_e3,
    attacks_black_lookup_f3,
    attacks_black_lookup_g3,
    attacks_black_lookup_h3,
    attacks_black_lookup_a2,
    attacks_black_lookup_b2,
    attacks_black_lookup_c2,
    attacks_black_lookup_d2,
    attacks_black_lookup_e2,
    attacks_black_lookup_f2,
    attacks_black_lookup_g2,
    attacks_black_lookup_h2,
    attacks_black_lookup_a1,
    attacks_black_lookup_b1,
    attacks_black_lookup_c1,
    attacks_black_lookup_d1,
    attacks_black_lookup_e1,
    attacks_black_lookup_f1,
    attacks_black_lookup_g1,
    attacks_black_lookup_h1
};

// A lookup table to convert our character piece convention to the lookup
//  convention.
// Note for future reference. We could get a small performance boost, at the
//  cost of debuggability, by using this bitmask convention rather than our
//  ascii convention for pieces. The advantage would simply be that the
//  to_mask[] conversion would not then be required. In fact that used to be
//  how we did it, but we changed to the ascii convention which has many
//  advantages. Maybe a compromise where debug builds use the ascii convention
//  and release builds use a faster binary convention will be the ultimate
//  solution. We'll need a new api to convert to ascii convention for display
//  of board positions etc. if we do this.
lte to_mask[] =
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   // 0x00-0x0f
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   // 0x10-0x1f
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   // 0x20-0x2f
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   // 0x30-0x3f
     0,0,B,0,0,0,0,0,0,0,0,K,0,0,N,0,   // 0x40-0x4f    'B'=0x42, 'K'=0x4b, 'N'=0x4e
     P,Q,R,0,0,0,0,0,0,0,0,0,0,0,0,0,   // 0x50-0x5f    'P'=0x50, 'Q'=0x51, 'R'=0x52
     0,0,B,0,0,0,0,0,0,0,0,K,0,0,N,0,   // 0x60-0x6f    'b'=0x62, 'k'=0x6b, 'n'=0x6e
     P,Q,R,0,0,0,0,0,0,0,0,0,0,0,0,0};  // 0x70-0x7f    'p'=0x70, 'q'=0x71, 'r'=0x72
#undef P
#undef B
#undef N
#undef R
#undef Q
#undef K

}


