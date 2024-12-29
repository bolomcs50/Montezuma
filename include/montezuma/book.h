/* 
 * File:   book.h
 * Author: michele
 * 
 * Created on October 15, 2022, 9:38 AM
 * 
 * Definitions for opening book management
 */
 
#include <fstream>
#include <iostream>
#include "hashing.h"
#include "thc.h"

#ifndef BOOK_H
#define BOOK_H

namespace montezuma {
    
struct polyBookEntry {
    uint64_t key;
    unsigned short move;
    unsigned short weight;
    unsigned int learn; 
};



class Book{
public:
    Book();
    bool initialize(std::string fileName);
    // Provides a list of polyBookEntries representing the suggested moves
    bool listMoves(uint64_t hash, std::vector<polyBookEntry> &moves);
    // Return the suggested best move in the current position
    bool getMove(thc::ChessEvaluation cr, uint64_t hash, char* moveTerse);
    
private:
    
    
    std::vector<polyBookEntry> positionList_;
};

unsigned short endianSwapU16(unsigned short x);
unsigned int endianSwapU32(unsigned int x);
uint64_t endianSwapU64 (uint64_t x);
    
} // end namespace montezuma

#endif /* BOOK_H */

