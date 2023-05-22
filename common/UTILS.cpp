#include "UTILS.h"

#include <bits/stdc++.h>
using namespace std;

void logError(const char *str) {
    std::cerr <<"[] error: " <<str <<std::endl;
    exit(0);
}

void logError(const char *str, std::pair<int, int> pos) {
    std::cerr <<"[Lexer] error:" <<pos.first <<":" <<pos.second <<" " <<str <<std::endl;
    exit(0);
}

void logError(const char *pre, const char *str, std::pair<int, int> pos) {
    cerr <<pre <<" error:" <<pos.first <<":" <<pos.second <<" " <<str <<endl;
    exit(0);
}