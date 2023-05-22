#include <bits/stdc++.h>
#include "LEXER.h"
#include "AST.h"

using namespace std;



int main(int argc, char **argv)
{
    assert(argc == 2);

    Lexer A(argv[1]);

    ASTnode *X;

    while (X=A.getNextToken()) {
        printf("(%3d, %3d)  %15s    %s\n", X->info.pos.first, X->info.pos.second, X->type.c_str(), X->hint.c_str());
        X->destroy();
        delete X;
    }


}