#include "LEXER.h"
#include "UTILS.h"

#include <iostream>
#include <cstring>
#include <cctype>

static const char* RESERVED_WORDS[] {
    "bool", "char", "int",
    "true", "false",
    "and", "or", "not",
    "if", "then", "else", "while", "break", "return"
};

char Lexer::getNextChar() {
    for (curCh=fin.get(); curCh=='\n' || curCh=='\r'; curCh=fin.get()) if (curCh=='\n') {
        pos.first++;
        pos.second = 0;
    }
    if (curCh == EOF) return curCh=0;
    pos.second++;
    return curCh;
}

ASTnode* Lexer::getNextToken() {

    std::string tmpstr;
    ASTnode *ret = NULL;

    while (isspace(curCh) && getNextChar());

    std::pair<int, int> pos_stamp = pos;

    if (isalpha(curCh) || curCh == '_') {
        while (isalnum(curCh) || curCh == '_') {
            tmpstr += (char)curCh;
            if (!getNextChar()) break;
        }
        for (auto k: RESERVED_WORDS) if (tmpstr == k) {
            ret = new ASTnode(tmpstr, tmpstr);
        }
        if (!ret) ret = new ASTnode("ID", tmpstr);
    }
    else if (isdigit(curCh)) {
        while (isdigit(curCh)) {
            tmpstr += (char)curCh;
            if (!getNextChar()) break;
        }
        ret = new ASTnode("NUMCONST", tmpstr);
    }
    else {
        tmpstr += (char)curCh;

        if (curCh == '\'') {
            getNextChar();
            while (curCh != '\'') {
                if (curCh == '\\') {
                    getNextChar();
                    if (curCh == 'n') curCh = '\n';
                }
                tmpstr += (char)curCh;
                if (!getNextChar()) {
                    logError("no match for \'", pos_stamp);
                }
            }
            tmpstr += '\'';
            getNextChar();
            ret = new ASTnode("CHARCONST", tmpstr);
        }
        else if (curCh == '\"') {
            getNextChar();
            while (curCh != '\"') {
                if (curCh == '\\') {
                    getNextChar();
                    if (curCh == 'n') curCh = '\n';
                }
                tmpstr += (char)curCh;
                if (!getNextChar()) {
                    logError("no match for \"", pos_stamp);
                }
            }
            tmpstr += '\"';
            getNextChar();
            ret = new ASTnode("STRINGCONST", tmpstr);
        }
        else if (curCh > 0) {
            if (getNextChar() && std::string("*/+-=!><").find(tmpstr[0]) != std::string::npos && curCh=='=') {
                tmpstr += (char)curCh;
                getNextChar();
            }
            ret = new ASTnode(tmpstr, tmpstr);
        }
        else {
            return NULL;
        }
    }

    ret->info.pos = pos_stamp;

    return ret;
}