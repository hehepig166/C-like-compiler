#include "CODEGEN.h"

using namespace std;



void Function::env_push() {
    envs.push_back(NameMap());
}

void Function::env_pop() {
    envs.pop_back();
}

int Function::new_var(string varName, VarType varType) {

    auto &top = envs.back();

    // 重名出错，返回 NULL
    if (top.count(varName)) return -1;

    // 新建一个局部变量
    cnt++;
    auto newVar = VarInfo(cnt, varType);
    top[varName] = newVar;

    // 局部变量要记录下来，最后一起声明
    variables.push_back(newVar);

    return cnt;
}

int Function::new_temp() {
    cnt++;
    temps.push_back(VarInfo(cnt, "unknown"));
    return cnt;
}


VarInfo* Function::get_var(string varName) {
    for (auto env = envs.rbegin(); env != envs.rend(); env++) {
        auto p = env->find(varName);
        if (p != env->end()) {
            for (int i=0; i<=variables.size(); i++) if (variables[i] == p->second)
                    return &variables[i];
            return NULL;
        }
    }
    return NULL;
}

VarInfo* Function::get_var(int index) {
    for (int i=0; i<variables.size(); i++) if (variables[i].first == index) return &variables[i];
    for (int i=0; i<temps.size(); i++) if (temps[i].first == index) return &temps[i];
    return NULL;
}

VarType Function::get_type(int index) {
    if (index > 0) {
        return get_var(index)->second;
    }
    else if (index < 0) {
        return consts[-index].first;
    }
    else {
        return "unknown";
    }
}

int Function::get_const(const ConstInfo &x) {
    auto p = constMap.find(x);
    if (p != constMap.end()) return p->second;

    consts.push_back(x);
    return constMap[x] = -(++cnt_const);
}


int Function::new_code(int res, string op, int v1, int v2) {
    codes.emplace_back(res, op, v1, v2);
    return codes.size()-1;
}

Code& Function::get_code(int index) {
    return codes[index];
}