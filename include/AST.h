#ifndef __HEHEPIG_AST_H__
#define __HEHEPIG_AST_H__

#include <iostream>
#include <string>
#include <vector>

#include "CODEGEN.h"


struct ASTinfo {
    std::pair<int, int> pos;

    VarType     type;
    int         addr;
    int         entry;

    std::vector<int>    trueList;
    std::vector<int>    falseList;
    std::vector<int>    nextList;

    int                         tmpInt;
    std::vector<int>            tmpIntList;
    std::string                 tmpStr;
    std::vector<std::string>    tmpStrList;
};


struct ASTnode {

    std::string type;
    std::string hint;
    
    ASTinfo info;

    ASTnode *father;
    std::vector<ASTnode*> sons;

    ASTnode(): father(NULL) {}
    ASTnode(std::string t, std::string h): type(t), hint(h), father(NULL) {}
    ASTnode(std::string t, std::string h, const std::vector<ASTnode*> &s): type(t), hint(h), sons(s), father(NULL) {
        for (auto p: sons) p->father = this;
        if (sons.size()) info.pos = sons[0]->info.pos;
    }

    void destroy() {
        for (auto &p: sons) {
            p->destroy();
            p = NULL;
        }
    }

    friend void append_son(ASTnode *f, ASTnode *s) {
        f->sons.push_back(s);
        s->father = f;
    }
};





#endif