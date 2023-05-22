#include <bits/stdc++.h>
#include "AST.h"
#include "LEXER.h"
#include "CODEGEN.h"
#include "UTILS.h"
using namespace std;

typedef pair<string, vector<string>> Statement;
typedef vector<string> Rule;

const string STARTSYMBOL = "<_>";
const string EOFTERMINAL = "$";

set<string> NONTERMINALS;
set<string> TERMINALS;
set<string> SYMBOLS;
vector<Statement> statements;
map<string, set<string>> FIRST;
map<string, set<string>> FOLLOW;


vector<vector<Rule>> rules;
Function curFunction;
vector<Function> glb_context;


bool is_terminal(string str) {return *str.begin()!='<' || *str.rbegin()!='>';}



//===================================================
// 语义分析
//=================================================== 

string getInfo_str(ASTnode *node, const string *pstr);
int getInfo_int(ASTnode *node, const string *pstr);
vector<int> getInfo_intList(ASTnode *node, const string *pstr);

void get_rules(const char *fname)
{
    ifstream fin(fname);
    assert(fin.is_open());

    rules.resize(statements.size());

    string tmpstr;
    vector<Rule> ruleList;
    Statement S;
    vector<string> Q;
    int flag = 0;       // 0: statement, 1: rules
    while (fin >>tmpstr) {
        if (tmpstr == "$") {
            // end of this group
            if (flag) {
                ruleList.push_back(Q);
                for (int i=0; i<statements.size(); i++) if (S == statements[i]) {
                    assert(rules[i].size()==0);        // 是否有重复定义规则
                    rules[i] = ruleList;
                    break;
                }
            }
            Q.clear();
            ruleList.clear();
            flag = 0;
        }
        else if (tmpstr == "|") {
            if (!flag) {
                // statement
                S = Statement(Q[0], vector<string>(Q.begin()+2, Q.end()));
                flag = 1;
            }
            else {
                // to next rule
                ruleList.push_back(Q);
            }
            Q.clear();
        }
        else {
            Q.push_back(tmpstr);
        }
    }
}

ASTinfo* getpInfo(ASTnode *node, const string *pstr) {
    assert(pstr[0] == "@");
    if (pstr[1] == "0") {
        return &node->info;
    }
    else {
        assert(atoi(pstr[1].c_str())-1 < node->sons.size());    // 检查你的语义规则
        return &node->sons[atoi(pstr[1].c_str())-1]->info;
    }
}

int getInfo_int(ASTnode *node, const string *pstr) {
    if (pstr[0] == "@") {
        ASTinfo *p = getpInfo(node, pstr);
        if (pstr[2] == "addr") {
            return p->addr;
        }
        else if (pstr[2] == "tmpInt") {
            return p->tmpInt;
        }
        else if (pstr[2] == "entry") {
            return p->entry;
        }
        else if (pstr[2] == "null") {
            return 0;
        }
        else {
            logError("[semantics]", "getInfo_int", node->info.pos);
        }
    }
    else {
        if (pstr[0] == "new_temp") {
            int ret = curFunction.new_temp();
            curFunction.get_var(ret)->second = node->info.type;
            return ret;
        }
        else if (pstr[0] == "get_nxtcid") {
            return curFunction.next_cid();
        }
        else if (pstr[0] == "get_var") {
            string varName;
            if (pstr[1] == "@") {
                varName = getInfo_str(node, pstr+1);
            }
            else {
                varName = pstr[1];
            }
            VarInfo *pVarInfo = curFunction.get_var(varName);
            if (!pVarInfo) logError("[semantics]", (string("unknown var ")+varName).c_str(), node->info.pos);
            return pVarInfo->first;
        }
        else {
            logError("[semantics]", "getInfo_int", node->info.pos);
        }
    }
    return -1;
}

vector<int> getInfo_intList(ASTnode *node, const string *pstr) {
    assert(pstr[0] == "@");
    ASTinfo *p = getpInfo(node, pstr);
    if (pstr[2] == "trueList") {
        return p->trueList;
    }
    else if (pstr[2] == "falseList") {
        return p->falseList;
    }
    else if (pstr[2] == "nextList") {
        return p->nextList;
    }
    else {
        logError("[semantics]", "getInfo_intList", node->info.pos);
    }
    return vector<int>();
}

string getInfo_str(ASTnode *node, const string *pstr) {
    //assert(pstr[0] == "@");
    if (pstr[0] == "@") {
        ASTinfo *p = getpInfo(node, pstr);
        if (pstr[2] == "type") {
            return p->type;
        }
        else if (pstr[2] == "tmpStr") {
            return p->tmpStr;
        }
    }
    else if (pstr[0] == "get_vartype") {
        string varName;
        if (pstr[1] == "@") {
            varName = getInfo_str(node, pstr+1);
        }
        else {
            varName = pstr[1];
        }
        VarInfo *pVarInfo = curFunction.get_var(varName);
        if (!pVarInfo) logError("[semantics]", (string("unknown var ")+varName).c_str(), node->info.pos);
        return pVarInfo->second;
    }
    else {
        logError("[semantics]", "getInfo_int", node->info.pos);
    }
    return "error";
}



int codegen(ASTnode *node, int rid)
{
    if (node->sons.size()) node->info.pos = node->sons[0]->info.pos;

    if (is_terminal(node->type)) {
        // 终结符，手动写写
        const auto &t = node->type;

        // entry 记录
        node->info.entry = curFunction.next_cid();

        if (t == "ID") {
            if (curFunction.name == "") curFunction.name = node->hint;  // 2023.5.21，当前函数名，用于递归
            node->info.tmpStr = node->hint;
        }
        else if (t == "NUMCONST") {
            int res_ = curFunction.get_const(ConstInfo("int", node->hint));
            //curFunction.new_code(res_, (string("num ")+(node->hint)).c_str(), -1, -1);
            node->info.type = "int";
            node->info.addr = res_;
        }
        else if (t == "{") {
            curFunction.env_push();
        }
        else if (t == "}") {
            curFunction.env_pop();
        }
        else if ((set<string>{"int", "char"}).count(t)) {
            node->info.type = t;
        }
        else {
            node->info.tmpStr = node->hint;
        }

    }
    else {
        // 非终结符，根据规则文件自动搞

        // entry
        node->info.entry = node->sons[0]->info.entry;

        for (const auto &r: rules[rid]) {
            string op;
            const string *a1=NULL, *a2=NULL;
            ASTinfo *p1=NULL, *p2=NULL;
            ASTnode pp;

            if (r.size() > 1) {

                // a1
                a1 = &r[0];
                if (a1[0] == "@") {
                    p1 = getpInfo(node, a1);
                    op = r[3];
                    a2 = &r[4];
                }
                else {
                    op = r[1];
                    a2 = &r[2];
                }

                // a2
                if (a2[0] == "@") {
                    p2 = getpInfo(node, a2);
                }
                else {
                    p2 = &pp.info;
                }
            }
            else {
                op = r[0];
            }


            // 变量声明
            // 根据 type 和 tmpStrList，声明变量
            // 保存在 curFunction 的局部变量表中
            if (op == "alloc_vars") {
                auto varType = node->info.type;
                const auto &varNameList = node->info.tmpStrList;
                for (auto varName: varNameList) {
                    if (!curFunction.new_var(varName, varType))
                        goto hehepig_goto_abel_err;
                }
            }

            // 打包当前函数
            else if (op == "function_pack") {
                for (auto argName: node->info.tmpStrList) {
                    curFunction.args.push_back(*(curFunction.get_var(argName)));
                }
                curFunction.type = node->info.type;
                curFunction.name = node->info.tmpStr;
                for (auto &func: glb_context) if (func.name == curFunction.name) {
                    logError("[semantics]", (string("redeclaration of function")+curFunction.name).c_str(), node->info.pos);
                }
            }

            else if (op == "function_push") {
                glb_context.push_back(curFunction);
                curFunction = Function();
            }

            // 检查并更新类型（运算符）
            else if (op == "update_type") {
                if (node->sons.size() == 1) {
                    node->info.type = node->sons[0]->info.type;
                }
                else if (node->sons.size() == 2) {
                    if (node->type == "<unaryRelExp>") {
                        if (node->sons[1]->info.type != "bool") {
                            logError("[semantics]", (string("wrong type: ")+node->type+" "+node->sons[1]->info.type).c_str(), node->info.pos);
                        }
                        node->info.type = "bool";
                    }
                    else
                        node->info.type = node->sons[0]->info.type;
                }
                if (node->sons.size() == 3) {
                    if (node->sons[0]->info.type != node->sons[2]->info.type) {
                        logError("[semantics]", (string("wrong type ")+node->sons[0]->info.type+" op "+node->sons[2]->info.type).c_str(), node->info.pos);
                    }
                    if (set<string>({"<smipleExp>","<andExp>","<relExp>"}).count(node->info.type))
                        node->info.type = "bool";
                    else
                        node->info.type = node->sons[0]->info.type;
                }
            }

            // 函数调用
            else if (op == "gen_call") {
                p1 = &(node->info);

                // 检查此函数是否存在
                Function *pfun = NULL;
                for (int i=0; i<glb_context.size(); i++) if (glb_context[i].name == p1->tmpStr) {
                    pfun = &glb_context[i];
                    break;
                }
                if (curFunction.name == p1->tmpStr) {
                    pfun = &curFunction;
                }
                if (pfun == NULL) {
                    logError("[semantics]", (string("unknown function call ")+p1->tmpStr).c_str(), node->info.pos);
                }

                // 类型
                p1->type = curFunction.get_var(p1->addr)->second = pfun->type;

                // 检查参数列表是否对应，并添加代码（call, arg0, arg1, ...）
                // res, "call funName", argc, null
                // arg, aid, v
                const auto &L1 = p1->tmpIntList;
                const auto &L2 = pfun->args;
                if (L1.size() != L2.size()) {
                    logError("[semantics]", (string("wrong argcnt")+to_string(L1.size())+" (should be "+to_string(L2.size())+")").c_str(), node->info.pos);
                }

                curFunction.new_code(p1->addr, string("call ")+p1->tmpStr, L1.size(), 0);
                for (int i=0; i<L1.size(); i++) {
                    if (curFunction.get_type(L1[i]) != L2[i].second) {
                        logError("[semantics]", (string("wrong arg[")+to_string(i)+"] type "+curFunction.get_type(L1[i])+"(should be "+L2[i].second+")").c_str(), node->info.pos);
                    }
                    curFunction.new_code(0, "arg", i, L1[i]);
                }
            }

            // 回填
            else if (op == "backpatch") {
                auto list = getInfo_intList(node, a2);
                int v = getInfo_int(node, a2+3);
                for (int cid: list) {
                    curFunction.get_code(cid).v1 = v;
                    cerr <<"fill " <<v <<" in [" <<cid <<" ]" <<endl;   
                }
            }

            else if (op == "gen") {
                const string *pi = &r[2];
                int _res, _v1, _v2;
                string _op;

                // res
                if (*pi == "@") {
                    _res = getInfo_int(node, pi);
                    pi += 3;
                }
                else {
                    goto hehepig_goto_abel_err;
                }

                // op
                if (*pi == "@") {
                    _op = getInfo_str(node, pi);
                    pi += 3;
                }
                else {
                    _op = *pi;
                    pi += 1;
                }

                // v1
                if (*pi == "@") {
                    _v1 = getInfo_int(node, pi);
                    pi += 3;
                }
                else {
                    goto hehepig_goto_abel_err;
                }

                // v2
                if (*pi == "@") {
                    _v2 = getInfo_int(node, pi);
                    pi += 3;
                }
                else {
                    goto hehepig_goto_abel_err;
                }

                curFunction.new_code(_res, _op, _v1, _v2);
            }

            // 赋值
            else if (op == "=") {
                assert(p1 && p2);
                if (a1[2] == "type") {
                    p1->type = getInfo_str(node, a2);
                }
                else if (a1[2] == "tmpIntList") {
                    p1->tmpIntList = p2->tmpIntList;
                }
                else if (a1[2] == "tmpStr") {
                    p1->tmpStr = getInfo_str(node, a2);
                }
                else if (a1[2] == "tmpStrList") {
                    p1->tmpStrList = p2->tmpStrList;
                }
                else if (a1[2] == "tmpInt") {
                    p1->tmpInt = getInfo_int(node, a2);
                }
                else if (a1[2] == "addr") {
                    p1->addr = getInfo_int(node, a2);
                }
                else if (a1[2] == "nextList") {
                    p1->nextList = getInfo_intList(node, a2);
                }
                else if (a1[2] == "trueList") {
                    p1->trueList = getInfo_intList(node, a2);
                }
                else if (a1[2] == "falseList") {
                    p1->falseList = getInfo_intList(node, a2);
                }
                else {
                    goto hehepig_goto_abel_err;
                }
            }

            // 添加
            else if (op == "push") {
                assert(p1 && p2);
                if (a1[2] == "tmpIntList") {
                    if (a2[2] == "tmpIntList") {
                        for (int x: p2->tmpIntList)
                            p1->tmpIntList.push_back(x);
                    }
                    else {
                        p1->tmpIntList.push_back(getInfo_int(node, a2));
                    }
                }
                else if (a1[2] == "tmpStrList") {
                    if (a2[2] == "tmpStr") {
                        p1->tmpStrList.push_back(p2->tmpStr);
                    }
                    else if (a2[2] == "tmpStrList") {
                        for (auto &x: p2->tmpStrList)
                            p1->tmpStrList.push_back(x);
                    }
                    else {
                        goto hehepig_goto_abel_err;
                    }
                }
                else if (a1[2] == "nextList") {
                    if (a2[2].find("List") == string::npos) {
                        p1->nextList.push_back(getInfo_int(node, a2));
                    }
                    else {
                        auto tmpList = getInfo_intList(node, a2);
                        for (auto x: tmpList)
                            p1->nextList.push_back(x);
                    }
                }
                else if (a1[2] == "trueList") {
                    if (a2[2].find("List") == string::npos) {
                        p1->trueList.push_back(getInfo_int(node, a2));
                    }
                    else {
                        auto tmpList = getInfo_intList(node, a2);
                        for (auto x: tmpList)
                            p1->trueList.push_back(x);
                    }
                }
                else if (a1[2] == "falseList") {
                    if (a2[2].find("List") == string::npos) {
                        p1->falseList.push_back(getInfo_int(node, a2));
                    }
                    else {
                        auto tmpList = getInfo_intList(node, a2);
                        for (auto x: tmpList)
                            p1->falseList.push_back(x);
                    }
                }
                else {
                    goto hehepig_goto_abel_err;
                }
            }

            continue;
hehepig_goto_abel_err:
            logError("[semantics]", (string("unknown member ")+a1[2]).c_str(), node->info.pos);
        }


    }

    return 0;
}



//===================================================
// 自动机
//=================================================== 

void get_grammar()
{
    
    vector<string> Q;
    for (string tmpstr; cin>>tmpstr; ) {
        Q.push_back(tmpstr);
        if (!is_terminal(tmpstr)) NONTERMINALS.insert(tmpstr);
        else TERMINALS.insert(tmpstr);

        SYMBOLS.insert(tmpstr);
    }

    for (int i=0; i<Q.size(); ) {
        string L = Q[i++];
        vector<string> R;
        for (i++; i<Q.size() && (i==Q.size()-1 || Q[i+1]!="::="); i++) {
            if (Q[i] == "|") {
                statements.push_back(Statement(L, R));
                R.clear();
            }
            else {
                R.push_back(Q[i]);
            }
        }
        statements.push_back(Statement(L, R));
    }

}

void get_first()
{
    FIRST.clear();

    // 终结符号的 FIRST
    for (auto X: TERMINALS) {
        FIRST[X].insert(X);
    }

    // 非终结符号的 FIRST
    // FIRST[X] 中有 EMPTY，当且仅当 X =*=> EMPTY
    for (bool flag_done=false; !flag_done; ) {
        flag_done = true;
        for (auto [X, S]: statements) {
            bool flag_all_empty = true;
            for (auto Y: S) {
                
                for (auto t: FIRST[Y]) if (t != "EMPTY" && FIRST[X].count(t) == 0) {
                    flag_done = false;
                    FIRST[X].insert(t);
                }

                if (FIRST[Y].count("EMPTY") == 0) {flag_all_empty=false; break;}
            }

            // 别忘了这个
            if (flag_all_empty && !FIRST[X].count("EMPTY")) {
                flag_done = false;
                FIRST[X].insert("EMPTY");
            }
        }
    }
}


void get_follow()
{
    FOLLOW.clear();

    FOLLOW[STARTSYMBOL].insert("$");

    for (bool flag_done=false; !flag_done; ) {
        flag_done = true;
        for (auto [A, S]: statements) {

            int n = S.size();

            for (int i=0, nxt; i<n-1; i = nxt) {
                auto B = S[i];
                // 2023.5.20 加了nxt, 用于 EMPTY 的判断，处理类似 <stmtList>      ::=     <stmtList> EMPTY <stmt>
                nxt = i+1;
                while (nxt < n && S[nxt] == "EMPTY") nxt++;
                if (nxt >= n) break;
                for (auto x: FIRST[S[nxt]]) if (x != "EMPTY" && FOLLOW[B].count(x) == 0) {
                    flag_done = false;
                    FOLLOW[B].insert(x);
                }
            }

            for (int i=0; i<n; i++) if (i==n-1 || FIRST[S[i+1]].count("EMPTY")) {
                auto B = S[i];
                for (auto x: FOLLOW[A]) if (x != "EMPTY" && FOLLOW[B].count(x) == 0) {
                    flag_done = false;
                    FOLLOW[B].insert(x);
                }
            }
        }
    }
}


struct LR0_automata {
    typedef pair<int, int> Item;
    typedef set<Item> SetOfItems;

    vector<SetOfItems> closures;
    map<pair<int, string>, int> gotoMap;

    map<pair<int, string>, pair<int, int>> table;
    enum table_item {
        T_ERR = 0,
        T_S,
        T_R,
        T_ACC
    };
    // 0: err
    // 1: s 移入        <1, cid>
    // 2: r 规约        <2, sid>
    // 3: accept


    void printItem(const Item &x, ostream &fout=cout) {
        const auto &[sid, pos] = x;
        const auto &[L, R] = statements[sid];
        fout <<setw(15) <<L <<"  ->  ";
        for (int i=0; i<pos; i++) fout <<R[i] <<" ";
        fout <<"|";
        for (int i=pos; i<R.size(); i++) fout <<" " <<R[i];
    }



    SetOfItems CLOSURE(SetOfItems I) {
        for (bool flag_done=false; !flag_done; ) {
            flag_done = true;

            auto oldI = I;
            for (auto [sid, pos]: oldI) if (pos < statements[sid].second.size()) {
                for (int i=0; i<statements.size(); i++) if (statements[i].first == statements[sid].second[pos]) {
                    if (I.count(Item(i, 0)) == 0) {
                        flag_done = false;
                        I.insert(Item(i, 0));
                    }
                }

                // EMPTY
                if (statements[sid].second[pos]=="EMPTY" && I.count(Item(sid, pos+1))==0) {
                    flag_done = false;
                    I.insert(Item(sid, pos+1));
                }

            }
        }
        return I;
    }


    SetOfItems GOTO(int cid, string X) {
        if (gotoMap.count({cid, X})) return closures[gotoMap[{cid, X}]];

        SetOfItems ret;
        for (auto [sid, pos]: closures[cid]) {
            const auto &[L, R] = statements[sid];
            while (pos < R.size() && R[pos] == "EMPTY") pos++;
            if (pos < R.size() && R[pos] == X) {
                ret.insert(Item(sid, pos+1));
            }
        }
        return CLOSURE(ret);
    }

    bool check_table(int i, string x, int t, int v) {
        if (table.count({i, x}) && (table[{i, x}].first != t || table[{i, x}].second != v)) {
            cerr <<"!!! " <<i <<" " <<x <<"  ";
            auto tv = table[{i, x}];
            if (tv.first == T_S) cerr <<"s " << tv.second;
            else if (tv.first == T_R) {cerr <<"r "; printItem({tv.second, 0}, cerr);}
            else if (tv.first == T_ACC) cerr <<"acc";
            cerr <<"  ( " <<t <<", " <<v <<")" <<endl;
            return false;
        }
        return true;
    }


    void generate_DFA() {
        closures.clear();
        closures.push_back(CLOSURE(SetOfItems{Item(0, 0)}));

        for (bool flag_done=false; !flag_done; ) {
            flag_done = true;

            for (int i=0; i<closures.size(); i++) {
                for (auto &X: SYMBOLS) if (X != "EMPTY") {
                    auto tmpI = GOTO(i, X);
                    if (tmpI.empty()) continue;
                    for (int j=0; j<closures.size(); j++) if (closures[j] == tmpI)
                        gotoMap[{i, X}] = j;
                    if (gotoMap.count({i, X}) == 0) {
                        gotoMap[{i, X}] = closures.size();
                        closures.push_back(tmpI);
                        flag_done = false;
                    }
                }
            }
        }
    }

    void generate_table() {
        table.clear();

        for (int i=0; i<closures.size(); i++) {
            for (auto [sid, pos]: closures[i]) {

                const auto &[L, R] = statements[sid];

                if (pos < R.size()) {   // 移入
                    if (is_terminal(R[pos]) && R[pos]!="EMPTY" && gotoMap.count({i, R[pos]})) {
                        check_table(i, R[pos], T_S, gotoMap[{i, R[pos]}]);
                        table[{i, R[pos]}] = {T_S, gotoMap[{i, R[pos]}]};
                    }
                }
                else {      // 规约
                    if (L != STARTSYMBOL) {
                        for (auto x: FOLLOW[L]) if (x!="EMPTY") {
                            if (!check_table(i, x, T_R, sid))
                                continue;   // 优先移入，处理 else
                            table[{i, x}] = {T_R, sid};
                        }
                    }
                    else {
                        table[{i, EOFTERMINAL}] = {T_ACC, 0};
                    }
                }
            }
        }
    }

    void init() {

        // automata
        generate_DFA();

        // table
        generate_table();

        
    }

    void show() {
        cout <<"================== closures ===================" <<endl;
        for (int i=0; i<closures.size(); i++) {
            cout <<"[ " <<i <<" ]" <<endl;
            for (auto x: closures[i]) {
                printItem(x);
                cout <<endl;
            }
        }
        cout <<endl;

        cout <<"================== table ======================" <<endl;
        int lsti = -1;
        for (auto [p, v]: table) {
            auto [i, x] =p;
            if (i>0 && i != lsti) {
                for (auto X: NONTERMINALS) if (gotoMap.count({lsti, X})) {
                        cout <<"[ " <<lsti <<" ]  " <<setw(15) <<X <<"      ";
                        cout <<"goto " <<gotoMap[{lsti, X}] <<endl;
                }
                cout <<endl;
            }
            cout <<"[ " <<i <<" ]  " <<setw(15) <<x <<"      ";
            if (v.first == T_S) cout <<"s " << v.second <<endl;
            else if (v.first == T_R) {cout <<"r "; printItem({v.second, 0}); cout <<endl;}
            else if (v.first == T_ACC) cout <<"acc" <<endl;
            lsti = i;
        }
        cout <<endl;
    }


    void logError(const char *str, std::pair<int, int> pos, int sid, string X) {
        cerr <<"[Parser] err (" <<pos.first <<", " <<pos.second <<") <" <<sid <<" " <<X <<">  " <<str <<endl;
        exit(0);
    }

    ASTnode* analyze(Lexer &lexer) {
        
        vector<int> stack_state;
        vector<ASTnode*> stack_node;

        stack_state.push_back(0);

        ASTnode *curNode = lexer.getNextToken();

        while (1) {
            if (curNode == NULL) curNode = new ASTnode("$", "$");

            int top_sid = stack_state.back();
            string X = curNode->type;

            if (table.count({top_sid, X})) {
                const auto &[t, v] = table[{top_sid, X}];
                if (t == T_ACC) {       // 完成
                    break;
                }
                else if (t == T_S) {    // 移入
                    stack_state.push_back(v);
                    codegen(curNode, -1);   // 生成代码
                    stack_node.push_back(curNode);

                    curNode = lexer.getNextToken(); // 读取下一个输入
                }
                else if (t == T_R) {    // 规约

                    auto [L, R] = statements[v];
                    // 滤掉 R 中的 EMPTY
                    //for (auto p=find(R.begin(), R.end(), "EMPTY"); p!=R.end(); p=find(R.begin(), R.end(), "EMPTY"))
                    //    R.erase(p);

                    // 先规约
                    vector<ASTnode*> tmpvec;
                    for (int i=R.size()-1; i>=0; i--) {
                        if (R[i] == "EMPTY") {
                            ASTnode *emptyNode = new ASTnode("EMPTY", "EMPTY");
                            codegen(emptyNode, -1);     // 为了 if、while 的回填
                            tmpvec.push_back(emptyNode);
                        }
                        else {
                            assert(stack_node.size());
                            tmpvec.push_back(stack_node.back());
                            stack_node.pop_back();
                            stack_state.pop_back();
                        }
                    }
                    reverse(tmpvec.begin(), tmpvec.end());
                    //assert(stack_node.size() >= R.size());
                    //tmpvec.assign(stack_node.end()-R.size(), stack_node.end());
                    //stack_node.erase(stack_node.end()-R.size(), stack_node.end());
                    auto *newNode = new ASTnode(L, L, tmpvec);
                    codegen(newNode, v);   // 生成代码
                    stack_node.push_back(newNode);

                    // 再 GOTO
                    //assert(stack_state.size() > R.size());
                    //stack_state.erase(stack_state.end()-R.size(), stack_state.end());
                    assert(gotoMap.count({stack_state.back(), L}) > 0);
                    stack_state.push_back(gotoMap[{stack_state.back(), L}]);
                }
                else {                  // 未知
                    logError("???1", curNode->info.pos, top_sid, X);
                }
            }
            else {
                logError("???2", curNode->info.pos, top_sid, X);
            }

            for (auto x: stack_state) cout <<x <<" ";
            cout <<'\t';
            for (auto x: stack_node) cout <<x->type <<" ";
            cout <<endl;
            cout <<endl;

        }

        assert(stack_node.size() == 1);
        return stack_node.back();
    }


};




//===================================================
// 杂函数
//===================================================

void dfsshow(ASTnode *rt, string gap)
{
    cout <<gap <<rt->hint;
    if (rt->sons.size()) {
        cout <<"[" <<endl;
        for (auto s: rt->sons) {
            dfsshow(s, gap+"  ");
            cout <<endl;
        }
        cout <<gap <<"]";
    }
}

void show()
{
    cout <<"================= statements ==================" <<endl;
    for (auto [L, R]: statements) {
        cout <<"    " <<setw(15) <<L <<"   ::=  ";
        for (auto x: R) if (x != "EMPTY") cout <<" " <<x;
        cout <<endl;
    }
    cout <<endl;

    cout <<"=================== FIRST =====================" <<endl;
    for (auto X: NONTERMINALS) {
        cout <<"    " <<setw(15) <<X <<"   ->   ";
        for (auto t: FIRST[X]) cout <<" " <<t;
        cout <<endl;
    }
    cout <<endl;

    cout <<"=================== FOLLOW ====================" <<endl;
    for (auto X: NONTERMINALS) {
        cout <<"    " <<setw(15) <<X <<"   ->   ";
        for (auto x: FOLLOW[X]) cout <<" " <<x;
        cout <<endl;
    }
    cout <<endl;

    cout <<"=================== rules =====================" <<endl;
    for (int i=0; i<rules.size(); i++) if (rules[i].size()) {
        auto s = statements[i];
        auto rl = rules[i];
        cout <<s.first <<" ::= ";
        for (auto x: s.second) cout <<x <<" ";
        cout <<endl <<"{" <<endl;
        for (auto r: rl) {
            cout <<"    ";
            for (auto x: r) cout <<x <<" ";
            cout <<endl;
        }
        cout <<"}" <<endl <<endl;
    }
    cout <<endl;
}




//===================================================
// 输出目标代码
//===================================================

int getvid(int x, map<int, int> &m) {
    //return x;
    if (m.count(x)) return m[x];
    int ret = m.size()+1;
    m[x] = ret;
    return ret;
}

string to_llvm_type(string str, int flag_ref=0)
{
    if (str == "int") {
        if (flag_ref) return "i32*";
        else return "i32";
    }
    else {
        logError((string("unacceptable type [")+str+"]").c_str());
        return "err";
    }
}

string to_llvm_var(int x, Function &func, map<int, int> &m, bool show_type=0) {
    string ret;
    if (x > 0) {
        int vid = getvid(x, m);
        if (show_type) ret += to_llvm_type(func.get_var(x)->second, func.is_var(x))+string(" ");
        ret += string("%")+to_string(vid);
        return ret;
    }
    else if (x < 0) {
        x = -x;
        if (show_type) ret += to_llvm_type(func.consts[x].first)+" ";
        ret += func.consts[x].second;
        return ret;
    }
    else {
        return "???";
    }
}

void printCode_function(ofstream &fout, Function &func)
{
    

    // 计算指令跳转关系
    map<int, string> label_map;
    for (Code &c: func.codes) {
        int v1(c.v1), v2(c.v2), res(c.res), tmp;
        string op(c.op);
        if (op == "goto" || op == "br") {
            if (label_map.count(v1)) continue;
            label_map[v1] = "";
        }
    }
    int tmp_lid=0;
    for (auto &[cid, lab]: label_map) {
        lab = string("l")+to_string(tmp_lid);
        tmp_lid++;
    }


    // 输出
    auto funcName = func.name;
    fout <<"define " <<to_llvm_type(func.type) <<" @" <<funcName <<"(";
    for (int i=0; i<func.args.size(); i++) {
        fout <<to_llvm_type(func.args[i].second) <<" %arg" <<i;
        if (i != func.args.size()-1) fout <<", ";
    }
    fout <<") {" <<endl;

    map<int, int> vidMap;

    int argid = 0;
    for (auto [id, t]: func.variables) fout <<"  %" <<getvid(id, vidMap) <<" = alloca " <<to_llvm_type(t) <<", align 4" <<endl;
    for (int i=0; i<func.args.size(); i++) fout <<"  store " <<to_llvm_type(func.args[i].second) <<" %arg" <<i <<", " <<to_llvm_var(i+1, func, vidMap, true) <<", align 4" <<endl;
    

    for (int cid=0; cid<func.codes.size(); cid++) {

        if (label_map.count(cid)) {
            assert(cid > 0);
            if (set<string>({"goto", "br"}).count(func.codes[cid-1].op) == 0) {
                // fall 的直接跳转在 llvm ir 中得显示写出
                fout <<"  br label %" <<label_map[cid] <<endl;
            }
            fout <<endl <<label_map[cid] <<":" <<endl;
        }

        int v1(func.codes[cid].v1), v2(func.codes[cid].v2), res(func.codes[cid].res), tmp;
        string op(func.codes[cid].op);

        if (op == "store") {
            fout <<"  store " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap, true) <<", align 4" <<endl;
        }
        else if (op == "load") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = load ";
            fout <<to_llvm_type(func.get_var(res)->second) <<", ";
            fout <<to_llvm_var(v1, func, vidMap, true) <<", align 4" <<endl;
        }
        else if (op == "br") {
            if (cid+1<func.codes.size() && func.codes[cid+1].op == "goto") {
                fout <<"  br i1 " <<to_llvm_var(res, func, vidMap) <<", label %" <<label_map[v1] <<", label %" <<label_map[func.codes[cid+1].v1] <<endl;
                cid++;
            }
            else {
                fout <<"  br i1" <<to_llvm_var(res, func, vidMap) <<", label " <<label_map[v1] <<endl;
            }
        }
        else if (op == "goto") {
            fout <<"  br " <<"label %" <<label_map[v1] <<endl;
        }
        else if (op == "ret") {
            fout <<"  ret " <<to_llvm_var(v1, func, vidMap, true) <<endl;
        }
        else if (op.substr(0, 4) == "call") {
            string funName;
            string retType;
            int retAddr;
            int argc;
            vector<int> args;

            // funName
            funName = op.substr(5);

            // retType
            for (const auto &ff: glb_context) if (ff.name == funName) retType = ff.type;
            
            // retAddr
            retAddr = res;

            // argc
            argc = v1;
            args.resize(argc);

            // args
            for (int i=1; i<=argc; i++) {
                cid++;
                args[func.codes[cid].v1] = func.codes[cid].v2;
            }

            // printCode
            fout <<"  " <<to_llvm_var(retAddr, func, vidMap) <<" = call " <<to_llvm_type(retType) <<" @" <<funName <<"(";
            for (int i=0; i<args.size(); i++) {
                fout <<to_llvm_var(args[i], func, vidMap, true);
                if (i != args.size()-1) fout <<", ";
            }
            fout <<")" <<endl;
        }
        else if (op == "+") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = add nsw " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap) <<endl;
        }
        else if (op == "-") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = sub nsw " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap) <<endl;
        }
        else if (op == "*") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = mul nsw " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap) <<endl;
        }
        else if (op == "<") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = icmp slt " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap) <<endl;
        }
        else if (op == ">") {
            fout <<"  " <<to_llvm_var(res, func, vidMap) <<" = icmp sgt " <<to_llvm_var(v1, func, vidMap, true) <<", " <<to_llvm_var(v2, func, vidMap) <<endl;
        }

    }


    
    fout <<"  ret i32 0" <<endl;

    fout <<"}" <<endl;
}

void printCode(const char *fname)
{
    ofstream fout(fname);
    assert(fout.is_open());

    for (Function &func: glb_context) {
        printCode_function(fout, func);
        fout <<endl;
    }

}




int main(int argc, char **argv)
{
    assert(argc >= 4);

    get_grammar();

    get_first();

    get_follow();

    get_rules(argv[2]);

    LR0_automata LR0;
    LR0.init();

    show();
    LR0.show();



    cout <<"=================== analyze ===================" <<endl;
    Lexer lexer(argv[1]);
    ASTnode *root = LR0.analyze(lexer);

    cout <<"=================== tree ======================" <<endl;
    dfsshow(root, "");
    cout <<endl;
    cout <<endl;

    cout <<"=================== code ======================" <<endl;
    for (auto &func: glb_context) {
        int cid = 0;
        auto funcName = func.name;
        cout <<"define " <<func.type <<" @" <<funcName <<"(";
        for (auto [id, t]: func.args) cout <<t <<", ";
        cout <<")" <<endl;
        int argid = 0;
        for (auto [id, t]: func.args) cout <<"[    ]  " <<"alloc %" <<id <<" arg" <<++argid <<endl;
        for (int i=argid; i<func.variables.size(); i++) cout <<"[    ]  " <<"alloc %" <<func.variables[i].first <<endl;
        for (auto c: func.codes) cout <<"[ " <<setw(3) <<cid++ <<"]  " <<c <<endl;
        cout <<endl;
    }
    cout <<endl;


    printCode(argv[3]);


    root->destroy();
    delete root;

}