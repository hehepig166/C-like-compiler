#ifndef __HEHEPIG_LEXER_H__
#define __HEHEPIG_LEXER_H__

#include <fstream>
#include <string>
#include <cassert>
#include "AST.h"

class Lexer {
public:
    std::ifstream fin;
    std::pair<int, int> pos;
    int curCh;
    int flag;


    Lexer(const char *fname): fin(fname), pos(1, 0), flag(0) {
        assert(fin.is_open());
        getNextChar();
    }

    // 返回 0 代表结束
    char getNextChar();
    
    ASTnode* getNextToken();

};



#endif