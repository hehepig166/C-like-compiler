#ifndef __HEHEPIG_CODEGEN_H__
#define __HEHEPIG_CODEGEN_H__

#include <vector>
#include <map>
#include <string>
#include <iostream>

typedef std::string                     VarType;
typedef std::pair<int, VarType>         VarInfo;
typedef std::map<std::string, VarInfo>  NameMap;
typedef std::pair<VarType, std::string> ConstInfo;



struct Code {
    int res;
    std::string op;
    int v1;
    int v2;

    Code() = default;
    Code(int res_, std::string op_, int v1_, int v2_): res(res_), op(op_), v1(v1_), v2(v2_) {}

    friend std::ostream& operator<<(std::ostream &fout, const Code &c) {
        if (c.op == "") {
            return fout;
        }
        if (c.op == "br") {
            fout <<c.op <<" %" <<c.res <<" " <<c.v1;
            return fout;
        }
        if (c.op == "goto") {
            fout <<c.op <<" " <<c.v1;
            return fout;
        }
        if (c.op == "store") {
            fout <<c.op <<" %" <<c.v1 <<" %" <<c.v2;
            return fout;
        }
        if (c.op.find("call") != std::string::npos) {
            fout <<c.res <<" = " <<c.op <<" " <<c.v1;
            return fout;
        }
        if (c.op == "arg") {
            fout <<"  arg" <<c.v1 <<" %" <<c.v2;
            return fout;
        }
        if (c.op == "ret") {
            fout <<c.op;
            if (c.v1) fout <<" %" <<c.v1;
            return fout;
        }
        fout <<"%" <<c.res <<" = " <<c.op;
        if (c.v1) {
            fout <<" %" <<c.v1;
            if (c.v2) fout <<" %" <<c.v2;
        }
        return fout;
    }
};

class Function {
public:
    std::string             name;
    VarType                 type;
    int cnt;
    int cnt_const;
    std::vector<VarInfo>    variables;
    std::vector<VarInfo>    temps;
    std::vector<NameMap>    envs;

    std::vector<Code>       codes;

    std::vector<VarInfo>    args;

    std::map<ConstInfo, int>        constMap;
    std::vector<ConstInfo>          consts;

public:

    Function(): cnt(0), cnt_const(0), envs(1), codes(1), consts(1) {}

    //====================================================
    // 符号处理相关 （变量、临时变量的 id > 0）
    //====================================================
    void env_push();
    void env_pop();

    int new_var(std::string varName, VarType varType);
    int new_temp();

    VarInfo* get_var(std::string varName);
    VarInfo* get_var(int index);
    VarType  get_type(int index);   // >0 var, <0 const
    bool is_var(int index) {for (auto &[id, t]: variables) if (id==index) return true; return false;}

    //====================================================
    // 常量处理相关 （常量的 id < 0）
    //====================================================
    int get_const(const ConstInfo &x);

    //====================================================
    // 代码处理相关
    //====================================================
    int next_cid() {return codes.size();}
    int new_code(int res, std::string op, int v1, int v2);

    Code& get_code(int index);

};





#endif