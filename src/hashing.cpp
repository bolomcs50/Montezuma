#include "hashing.h"

namespace montezuma {
uint64_t zobristHash64Calculate(thc::ChessRules &cr){
    /* The hash is calculated as key=piece^castle^enpassant^turn;
    with the following offsets:
    RandomPiece     (offset:   0, length: 768)
    RandomCastle    (offset: 768, length:   4) 
    RandomEnPassant (offset: 772, length:   8)
    RandomTurn      (offset: 780, length:   1)*/
    
    int64_t hash = 0;
    
    // Key: offset_piece=64*kind_of_piece+8*row+file;
    char *p = cr.squares;
    for( int i=0; i<64; i++ )
    {
        char c = *p++;
        if( c >= 'B' && c <= 'r' ){// If the piece is a valid one
            hash ^= Random64[64*pieceNumber[c-'B']+8*(7-i/8)+i%8];
        }  
    }
    
    /* Castling:
    white can castle short     0
    white can castle long      1
    black can castle short     2
    black can castle long      3 */
    if (cr.wking_allowed())
        hash ^= Random64[castleOffset+0];
    if (cr.wqueen_allowed())
        hash ^= Random64[castleOffset+1];
    if (cr.bking_allowed())
        hash ^= Random64[castleOffset+2];
    if (cr.bqueen_allowed())
        hash ^= Random64[castleOffset+3];
    
    // En Passant
    if (cr.groomed_enpassant_target()!=thc::SQUARE_INVALID)
        hash ^= Random64[enPassantOffset+get_file(cr.groomed_enpassant_target())-97];

    // Turn
    if (cr.white)
        hash ^= Random64[turnOffset];
            
    return hash;
}

// Hash update before playng/pushing mv, and after popping
uint64_t zobristHash64Update( uint64_t hash, thc::ChessRules &cr, thc::Move move){
    // There is a "piece" at (row;file): hash^=Random64[64*kind_of_piece+8*rank+file];
            char piece  = cr.squares[move.src];
            int srcFile = thc::get_file(move.src)-97, srcRank = thc::get_rank(move.src)-49;
            char target = cr.squares[move.dst];
            int dstFile = thc::get_file(move.dst)-97, dstRank = thc::get_rank(move.dst)-49;
    switch( move.special )
    {
        default:
        {
            if(target !=' ' ) // If moving to occupied square, then this is a capture (!!! Not true the other way around: en passant)
                hash ^= Random64[64*pieceNumber[target-'B']+ 8*dstRank + dstFile];   // remove captured piece
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*srcRank + srcFile];   // remove piece from source
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*dstRank + dstFile];   // put piece in destination
            
            if (move.special == thc::SPECIAL_WPAWN_2SQUARES || move.special == thc::SPECIAL_BPAWN_2SQUARES){ // If the move opens an enpassant, encode it. I don't like this solution
                cr.PushMove(move);
                if (cr.groomed_enpassant_target()!=thc::SQUARE_INVALID)
                    hash ^= Random64[enPassantOffset+get_file(cr.groomed_enpassant_target())-97];
                cr.PopMove(move);
            } else if (piece=='K') {    // If the moved piece is a king/rook and castling is still allowed, disallow it
                hash ^= (cr.wking_allowed() ? Random64[castleOffset+0] : 0);
                hash ^= (cr.wqueen_allowed() ? Random64[castleOffset+1] : 0);
            } else if (piece=='k') {
                hash ^= (cr.bking_allowed() ? Random64[castleOffset+2] : 0);
                hash ^= (cr.bqueen_allowed() ? Random64[castleOffset+3] : 0);
            } else if (piece=='R') {
                hash ^= (srcFile==7 && srcRank==0 && cr.wking_allowed() ? Random64[castleOffset+0] : 0);
                hash ^= (srcFile==0 && srcRank==0 && cr.wqueen_allowed() ? Random64[castleOffset+1] : 0);
            } else if (piece=='r') {
                hash ^= (srcFile==7 && srcRank==7 && cr.bking_allowed() ? Random64[castleOffset+2] : 0);
                hash ^= (srcFile==0 && srcRank==7 && cr.bqueen_allowed() ? Random64[castleOffset+3] : 0);
            }
            
            break;
        }
        case thc::SPECIAL_WK_CASTLING:
        {
            hash ^= Random64[64*pieceNumber['K'-'B']+ 8*0 + 4];     // remove white king from e1
            hash ^= Random64[64*pieceNumber['K'-'B']+ 8*0 + 6];     // place white king on g1
            hash ^= Random64[64*pieceNumber['R'-'B']+ 8*0 + 7];     // remove white rook from h1
            hash ^= Random64[64*pieceNumber['R'-'B']+ 8*0 + 5];     // place white rook on f1
            hash ^= Random64[castleOffset+0];                       // Update castling flags. One side evidently was legal (we just castled), check if the other was active
            hash ^= (cr.wqueen_allowed() ? Random64[castleOffset+1] : 0);
            break;
        }
        case thc::SPECIAL_BK_CASTLING:
        {
            hash ^= Random64[64*pieceNumber['k'-'B']+ 8*7 + 4];     // remove black king from e8
            hash ^= Random64[64*pieceNumber['k'-'B']+ 8*7 + 6];     // place black king on g8
            hash ^= Random64[64*pieceNumber['r'-'B']+ 8*7 + 7];     // remove black rook from h8
            hash ^= Random64[64*pieceNumber['r'-'B']+ 8*7 + 5];     // place black rook on f8
            hash ^= Random64[castleOffset+2];                       // Update castling flags
            hash ^= (cr.bqueen_allowed() ? Random64[castleOffset+3] : 0);
            break;
        }
        case thc::SPECIAL_WQ_CASTLING:
        {
            hash ^= Random64[64*pieceNumber['K'-'B']+ 8*0 + 4];     // remove white king from e1
            hash ^= Random64[64*pieceNumber['K'-'B']+ 8*0 + 2];     // place white king on c1
            hash ^= Random64[64*pieceNumber['R'-'B']+ 8*0 + 0];     // remove white rook from a1
            hash ^= Random64[64*pieceNumber['R'-'B']+ 8*0 + 3];     // place white rook on d1
            hash ^= (cr.wking_allowed() ? Random64[castleOffset+0] : 0); // Update castling flags
            hash ^= Random64[castleOffset+1];
            break;
        }
        case thc::SPECIAL_BQ_CASTLING:
        {
            hash ^= Random64[64*pieceNumber['k'-'B']+ 8*7 + 4];     // remove black king from e8
            hash ^= Random64[64*pieceNumber['k'-'B']+ 8*7 + 2];     // place black king on c8
            hash ^= Random64[64*pieceNumber['r'-'B']+ 8*7 + 0];     // remove black rook from a8
            hash ^= Random64[64*pieceNumber['r'-'B']+ 8*7 + 3];     // place black rook on d8
            hash ^= (cr.bking_allowed() ? Random64[castleOffset+2] : 0); // Update castling flags
            hash ^= Random64[castleOffset+3];
            break;
        }
        case thc::SPECIAL_PROMOTION_QUEEN:
        {
            char piece  = cr.squares[move.src];
            int srcFile = thc::get_file(move.src)-97, srcRank = thc::get_rank(move.src)-49;
            char target = cr.squares[move.dst];
            int dstFile = thc::get_file(move.dst)-97, dstRank = thc::get_rank(move.dst)-49;
            
            if( target !=' ' )
                hash ^= Random64[64*pieceNumber[target-'B']+ 8*dstRank + dstFile];   // remove captured piece
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*srcRank + srcFile];   // remove piece from source
            hash ^= Random64[64*pieceNumber[(piece=='P'?'Q':'q')-'B']+ 8*dstRank + dstFile];   // put piece in destination
            break;
        }
        case thc::SPECIAL_PROMOTION_ROOK:
        {
            char piece  = cr.squares[move.src];
            int srcFile = thc::get_file(move.src)-97, srcRank = thc::get_rank(move.src)-49;
            char target = cr.squares[move.dst];
            int dstFile = thc::get_file(move.dst)-97, dstRank = thc::get_rank(move.dst)-49;
            
            if( target !=' ' )
                hash ^= Random64[64*pieceNumber[target-'B']+ 8*dstRank + dstFile];   // remove captured piece
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*srcRank + srcFile];   // remove piece from source
            hash ^= Random64[64*pieceNumber[(piece=='P'?'R':'r')-'B']+ 8*dstRank + dstFile];   // put piece in destination
            break;
        }
        case thc::SPECIAL_PROMOTION_BISHOP:
        {
            char piece  = cr.squares[move.src];
            int srcFile = thc::get_file(move.src)-97, srcRank = thc::get_rank(move.src)-49;
            char target = cr.squares[move.dst];
            int dstFile = thc::get_file(move.dst)-97, dstRank = thc::get_rank(move.dst)-49;
            
            if( target !=' ' )
                hash ^= Random64[64*pieceNumber[target-'B']+ 8*dstRank + dstFile];   // remove captured piece
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*srcRank + srcFile];   // remove piece from source
            hash ^= Random64[64*pieceNumber[(piece=='P'?'B':'b')-'B']+ 8*dstRank + dstFile];   // put piece in destination
            break;
        }
        case thc::SPECIAL_PROMOTION_KNIGHT:
        {
            char piece  = cr.squares[move.src];
            int srcFile = thc::get_file(move.src)-97, srcRank = thc::get_rank(move.src)-49;
            char target = cr.squares[move.dst];
            int dstFile = thc::get_file(move.dst)-97, dstRank = thc::get_rank(move.dst)-49;
            
            if( target !=' ' )
                hash ^= Random64[64*pieceNumber[target-'B']+ 8*dstRank + dstFile];   // remove captured piece
            hash ^= Random64[64*pieceNumber[piece-'B']+ 8*srcRank + srcFile];   // remove piece from source
            hash ^= Random64[64*pieceNumber[(piece=='P'?'N':'n')-'B']+ 8*dstRank + dstFile];   // put piece in destination
            break;
        }
        case thc::SPECIAL_WEN_PASSANT:
        {
            int srcFile = thc::get_file(move.src)-97;  // source en passant white rank is always the 5th
            int dstFile = thc::get_file(move.dst)-97; // dest en passant white rank is always the 6th

            hash ^= Random64[64*pieceNumber['P'-'B']+ 8*4 + srcFile];   // remove white pawn from source
            hash ^= Random64[64*pieceNumber['P'-'B']+ 8*5 + dstFile];   // put white pawn in destination
            hash ^= Random64[64*pieceNumber['p'-'B']+ 8*4 + dstFile];   // remove captured black pawn (same rank as src, same file as dst)
            break;
        }
        case thc::SPECIAL_BEN_PASSANT:
        {
            int srcFile = thc::get_file(move.src)-97;  // source en passant black rank is always the 5th
            int dstFile = thc::get_file(move.dst)-97; // dest en passant black rank is always the 6th

            hash ^= Random64[64*pieceNumber['p'-'B']+ 8*3 + srcFile];   // remove black pawn from source
            hash ^= Random64[64*pieceNumber['p'-'B']+ 8*2 + dstFile];   // put black pawn in destination
            hash ^= Random64[64*pieceNumber['P'-'B']+ 8*3 + dstFile];   // remove captured white pawn (same rank as src, same file as dst)
            break;
        }
    }
    
    if (move.capture){  // If capturing a rook, deal with castling rights
        if (target=='r'){
            hash ^= (dstFile==7 && dstRank==7 && cr.bking_allowed() ? Random64[castleOffset+2] : 0);
            hash ^= (dstFile==0 && dstRank==7 && cr.bqueen_allowed() ? Random64[castleOffset+3] : 0);
        } else if (target == 'R'){
            hash ^= (dstFile==7 && dstRank==0 && cr.wking_allowed() ? Random64[castleOffset+0] : 0);
            hash ^= (dstFile==0 && dstRank==0 && cr.wqueen_allowed() ? Random64[castleOffset+1] : 0);
        }
    }
    
    if (cr.groomed_enpassant_target()!=thc::SQUARE_INVALID) // If an enpassant is valid, remove it, wether it has been palyed or not
        hash ^= Random64[enPassantOffset+get_file(cr.groomed_enpassant_target())-97];
    // Change turn
    hash ^= Random64[turnOffset];
    return hash;
}

} // end namespace montezuma