/* 
 * File:   book.h
 * Author: michele
 * 
 * Created on October 15, 2022, 9:38 AM
 * 
 * Definitions for opening book management
 */

#include "hashing.h"

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
    
private:
    // Initializes a book, copying from fileName into memory
    int initialize(std::string fileName);
    // Provides a list of polyBookEntries representing the suggested moves
    int probe(uint64_t hash, std::vector<polyBookEntry> moves);
    // Clears the allocated memory
    void clear();
    
    std::vector<polyBookEntry> positionList;
};


    
} // end namespace montezuma

#endif /* BOOK_H */

