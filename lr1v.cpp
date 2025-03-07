// C++ includes used for precompiling -*- C++ -*-

// Copyright (C) 2003-2016 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file stdc++.h
 *  This is an implementation file for a precompiled header.
 */

 // 17.4.1.2 Headers

 // C
#ifndef _GLIBCXX_NO_ASSERT
#include <cassert>
#endif
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <ciso646>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if __cplusplus >= 201103L
#include <ccomplex>
#include <cfenv>
#include <cinttypes>
#include <cstdalign>
#include <cstdbool>
#include <cstdint>
#include <ctgmath>
#include <cuchar>
#include <cwchar>
#include <cwctype>
#endif

// C++
#include <algorithm>
#include <bitset>
#include <complex>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <vector>

#if __cplusplus >= 201103L
#include <array>
#include <atomic>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <forward_list>
#include <future>
#include <initializer_list>
#include <mutex>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <system_error>
#include <thread>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#endif

#if __cplusplus >= 201402L
#include <shared_mutex>
#endif
using namespace std;

// 定义产生式
struct Production {
    int id; // 产生式编号
    string lhs; // 左部非终结符
    vector<string> rhs; // 右部符号串

    Production(int identifier, string left, vector<string> right)
        : id(identifier), lhs(left), rhs(right) {
    }
};

// 定义LR(1)项目
struct LR1Item {
    string lhs;
    vector<string> rhs;
    int dot; // 点的位置
    string lookahead;
    int prodId; // 产生式编号

    LR1Item(string left, vector<string> right, int d, string la, int pid)
        : lhs(left), rhs(right), dot(d), lookahead(la), prodId(pid) {
    }

    bool operator<(const LR1Item& other) const {
        if (lhs != other.lhs)
            return lhs < other.lhs;
        if (rhs != other.rhs)
            return rhs < other.rhs;
        if (dot != other.dot)
            return dot < other.dot;
        if (lookahead != other.lookahead)
            return lookahead < other.lookahead;
        return prodId < other.prodId;
    }

    bool operator==(const LR1Item& other) const {
        return lhs == other.lhs && rhs == other.rhs && dot == other.dot && lookahead == other.lookahead && prodId == other.prodId;
    }
};

// 语法分析器类
class LR1Parser {
private:
    vector<Production> productions;
    set<string> nonTerminals;
    set<string> terminals;
    string startSymbol;

    // First 和 Follow 集合
    map<string, set<string>> First;
    map<string, set<string>> Follow;

    // 项目集规范族
    vector<set<LR1Item>> C;

    // 分析表
    // Action 表：状态 -> (终结符 -> Action)
    // Action 的值为 "shift X", "reduce Y", "accept"
    map<int, map<string, string>> Action;

    // Goto 表：状态 -> (非终结符 -> 状态)
    map<int, map<string, int>> GotoTable;

public:
    // 读取文法规则
    void readGrammar() {
        // 输入开始符号
        cin >> startSymbol;

        // 输入非终结符集合
        string line;
        getline(cin, line); // 读取剩余的换行符
        getline(cin, line);
        vector<string> ntTokens = split(line, ' ');
        nonTerminals = set<string>(ntTokens.begin(), ntTokens.end());

        // 输入终结符集合（包括 'e' 作为 ε）
        getline(cin, line);
        vector<string> tTokens = split(line, ' ');
        terminals = set<string>(tTokens.begin(), tTokens.end());
        terminals.insert("e"); // 将 'e' 作为终结符添加

        // 输入产生式数量
        int P;
        cin >> P;
        getline(cin, line); // 读取剩余的换行符

        // 读取产生式
        for (int i = 0; i < P; ++i) {
            getline(cin, line);
            line = trim(line);
            size_t arrow = line.find("->");
            if (arrow == string::npos) {
                cerr << "Invalid production format: " << line << endl;
                exit(1);
            }
            string lhs = trim(line.substr(0, arrow));
            string rhsPart = trim(line.substr(arrow + 2));
            vector<string> alternatives = split(rhsPart, '|');
            for (auto& alt : alternatives) {
                alt = trim(alt);
                if (alt == "e") { // 将 'e' 视为 ε
                    productions.emplace_back(i + 1, lhs, vector<string>{ "e" });
                }
                else {
                    vector<string> symbols = split(alt, ' ');
                    productions.emplace_back(i + 1, lhs, symbols);
                }
            }
        }

        // 读取待分析的输入字符串
        cin >> line;
        inputString = line;
    }

    // 计算 First 集合
    void computeFirst() {
        // 初始化终结符的 First 集合
        for (auto& t : terminals) {
            if (t != "e") // 'e' 作为特殊符号单独处理
                First[t].insert(t);
        }
        // 'e' 的 FIRST 集合
        First["e"].insert("e");

        // 初始化非终结符的 First 集合为空
        for (auto& nt : nonTerminals) {
            First[nt] = set<string>();
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& prod : productions) {
                string A = prod.lhs;
                vector<string> alpha = prod.rhs;

                // 计算 First(alpha)
                set<string> firstAlpha = computeFirstOfString(alpha);

                // 将 First(alpha) - {e} 加入 First(A)
                for (auto& sym : firstAlpha) {
                    if (sym != "e" && First[A].find(sym) == First[A].end()) {
                        First[A].insert(sym);
                        changed = true;
                    }
                }

                // 如果 e ∈ First(alpha)，将 e 加入 First(A)
                if (firstAlpha.find("e") != firstAlpha.end()) {
                    if (First[A].find("e") == First[A].end()) {
                        First[A].insert("e");
                        changed = true;
                    }
                }
            }
        }

        // Debug: printFirstFollow();
    }

    // 计算 Follow 集合
    void computeFollow() {
        // 初始化 Follow 集合为空
        for (auto& nt : nonTerminals) {
            Follow[nt] = set<string>();
        }
        // 开始符号的 Follow 集加入 $
        Follow[startSymbol].insert("$");

        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& prod : productions) {
                string A = prod.lhs;
                vector<string> alpha = prod.rhs;
                for (int i = 0; i < (int)alpha.size(); i++) {
                    string B = alpha[i];
                    if (nonTerminals.find(B) != nonTerminals.end()) {
                        // β = alpha[i+1 ... end]
                        vector<string> beta(alpha.begin() + i + 1, alpha.end());
                        if (beta.empty()) {
                            // 如果 β 为空，则将 Follow(A) 加入 Follow(B)
                            for (auto& f : Follow[A]) {
                                if (Follow[B].find(f) == Follow[B].end()) {
                                    Follow[B].insert(f);
                                    changed = true;
                                }
                            }
                        }
                        else {
                            // 计算 First(beta)
                            set<string> firstBeta = computeFirstOfString(beta);

                            // 将 First(beta) - {e} 加入 Follow(B)
                            for (auto& sym : firstBeta) {
                                if (sym != "e" && Follow[B].find(sym) == Follow[B].end()) {
                                    Follow[B].insert(sym);
                                    changed = true;
                                }
                            }

                            // 如果 e ∈ First(beta)，则将 Follow(A) 加入 Follow(B)
                            if (firstBeta.find("e") != firstBeta.end()) {
                                for (auto& f : Follow[A]) {
                                    if (Follow[B].find(f) == Follow[B].end()) {
                                        Follow[B].insert(f);
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Debug: printFirstFollow();
    }

    // 闭包操作
    set<LR1Item> closure(const set<LR1Item>& I) {
        set<LR1Item> closureSet = I;
        bool changed = true;

        while (changed) {
            changed = false;
            set<LR1Item> newItems;
            for (auto& item : closureSet) {
                if (item.dot < (int)item.rhs.size()) {
                    string B = item.rhs[item.dot];
                    if (nonTerminals.find(B) != nonTerminals.end()) {
                        // β = item.rhs[item.dot +1 ... end]
                        vector<string> beta(item.rhs.begin() + item.dot + 1, item.rhs.end());
                        // a = item.lookahead
                        string a = item.lookahead;
                        // FIRST(beta a)
                        vector<string> symbols = beta;
                        symbols.push_back(a);
                        set<string> firstBetaA = computeFirstOfString(symbols);

                        for (auto& prod : productions) {
                            if (prod.lhs == B) {
                                for (auto& la : firstBetaA) {
                                    if (la == "e") continue; // 在 LR(1) 项目中，通常不添加 lookahead 为 ε 的项目
                                    LR1Item newItem(prod.lhs, prod.rhs, 0, la, prod.id);
                                    if (closureSet.find(newItem) == closureSet.end() && newItems.find(newItem) == newItems.end()) {
                                        newItems.insert(newItem);
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            closureSet.insert(newItems.begin(), newItems.end());
        }

        return closureSet;
    }

    // 迁移操作
    set<LR1Item> goto_func(const set<LR1Item>& I, const string& X) {
        set<LR1Item> J;
        for (auto& item : I) {
            if (item.dot < (int)item.rhs.size() && item.rhs[item.dot] == X) {
                LR1Item movedItem = item;
                movedItem.dot += 1;
                J.insert(movedItem);
            }
        }
        return closure(J);
    }

    // 构建项目集规范族
    void buildCanonicalCollection() {
        // 初始项集 C0 = closure({ S' -> . S, $ })
        set<LR1Item> C0;
        // 生产式 1: S' -> E
        C0.emplace(LR1Item(productions[0].lhs, productions[0].rhs, 0, "$", productions[0].id));
        set<LR1Item> closureC0 = closure(C0);
        C.push_back(closureC0);

        // 使用 BFS 构建项目集
        queue<int> q;
        q.push(0);

        while (!q.empty()) {
            int i = q.front();
            q.pop();
            set<string> symbols;
            for (auto& item : C[i]) {
                if (item.dot < (int)item.rhs.size()) {
                    symbols.insert(item.rhs[item.dot]);
                }
            }

            for (auto& X : symbols) {
                set<LR1Item> gotoI = goto_func(C[i], X);
                if (gotoI.empty()) continue;

                // Check if gotoI already exists in C
                int j = -1;
                for (int k = 0; k < (int)C.size(); ++k) {
                    if (C[k] == gotoI) {
                        j = k;
                        break;
                    }
                }

                if (j == -1) {
                    C.push_back(gotoI);
                    j = C.size() - 1;
                    q.push(j);
                }

                // 填充 Action 和 Goto 表
                if (terminals.find(X) != terminals.end()) {
                    Action[i][X] = "shift " + to_string(j);
                }
                else if (nonTerminals.find(X) != nonTerminals.end()) {
                    GotoTable[i][X] = j;
                }
            }
        }
    }

    // 构建分析表
    void buildParseTable() {
        for (int i = 0; i < (int)C.size(); ++i) {
            for (auto& item : C[i]) {
                if (item.dot < (int)item.rhs.size()) {
                    string a = item.rhs[item.dot];
                    if (terminals.find(a) != terminals.end()) {
                        // 查找 goto(Ci, a)
                        set<LR1Item> gotoSet = goto_func(C[i], a);
                        if (!gotoSet.empty()) {
                            // 查找状态 j
                            int j = -1;
                            for (int k = 0; k < (int)C.size(); ++k) {
                                if (C[k] == gotoSet) {
                                    j = k;
                                    break;
                                }
                            }
                            if (j != -1) {
                                Action[i][a] = "shift " + to_string(j);
                            }
                        }
                    }
                }
                else {
                    if (item.lhs != productions[0].lhs) {
                        // A -> α ., a
                        // Action[i, a] = reduce prod.id
                        Action[i][item.lookahead] = "reduce " + to_string(item.prodId);
                    }
                    else {
                        // S' -> S ., $
                        if (item.lookahead == "$") {
                            Action[i][item.lookahead] = "accept";
                        }
                    }
                }
            }
        }
    }

    // 解析输入字符串并输出分析过程
    void parseInput() {
        // 分词：将输入字符串按字符拆分
        vector<string> inputTokens = tokenize(inputString);
        inputTokens.push_back("$"); // 末尾加入 $

        // 初始化解析栈
        vector<int> parseStack;
        parseStack.push_back(0);

        int ip = 0; // 输入指针
        bool accept = false;

        // 存储输出动作
        vector<string> actions;

        while (true) {
            int state = parseStack.back();
            string a = inputTokens[ip];

            // 查找 Action[state][a]
            if (Action.find(state) != Action.end() && Action[state].find(a) != Action[state].end()) {
                string action = Action[state][a];
                if (action.substr(0, 5) == "shift") {
                    actions.push_back("shift");
                    // 获取状态 j
                    int j = stoi(action.substr(6));
                    parseStack.push_back(j);
                    ip++;
                }
                else if (action.substr(0, 6) == "reduce") {
                    // 获取生产式编号
                    int prodId = stoi(action.substr(7));
                    actions.push_back(to_string(prodId-1));
                    // 注意：prodId 是从1开始的
                    if (prodId <= 0 || prodId > (int)productions.size()) {
                        cerr << "Error: Invalid production ID " << prodId << ".\n";
                        break;
                    }
                    Production prod = productions[prodId - 1]; // 产生式编号从1开始

                    // 弹出 rhs.size() 个状态
                    for (size_t k = 0; k < prod.rhs.size(); ++k) {
                        if (!parseStack.empty())
                            parseStack.pop_back();
                        else {
                            cerr << "Error: Stack underflow during reduction.\n";
                            break;
                        }
                    }
                    // 获取当前状态
                    if (parseStack.empty()) {
                        cerr << "Error: Stack is empty after reduction.\n";
                        break;
                    }
                    int currentState = parseStack.back();
                    // Goto[currentState][A] = j
                    if (GotoTable.find(currentState) != GotoTable.end() && GotoTable[currentState].find(prod.lhs) != GotoTable[currentState].end()) {
                        int j = GotoTable[currentState][prod.lhs];
                        parseStack.push_back(j);
                    }
                    else {
                        cerr << "Error: Goto table entry not found for state " << currentState << " and non-terminal " << prod.lhs << ".\n";
                        break;
                    }
                }
                else if (action == "accept") {
                    actions.push_back("accept");
                    accept = true;
                    break;
                }
            }
            else {
                // 查找 Action[state][a] 不存在，解析错误
                actions.push_back("error");
                break;
            }
        }

        // 输出动作
        cout << "Parsing Actions:\n";
        for (auto& act : actions) {
            cout << act << "\n";
        }

        if (accept) {
            cout << "Parsing accepted.\n";
        }
        else {
            cout << "Parsing failed.\n";
        }
    }

    // 辅助函数：判断是否是非终结符
    bool isNonTerminal(const string& sym) {
        return nonTerminals.find(sym) != nonTerminals.end();
    }

    // 辅助函数：去除字符串首尾空白
    string trim(const string& s) {
        string result = s;
        // 去除左侧空白
        result.erase(result.begin(), find_if(result.begin(), result.end(), [](int ch) {
            return !isspace(ch);
            }));
        // 去除右侧空白
        result.erase(find_if(result.rbegin(), result.rend(), [](int ch) {
            return !isspace(ch);
            }).base(), result.end());
        return result;
    }

    // 辅助函数：按分隔符分割字符串
    vector<string> split(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        stringstream ss(s);
        while (getline(ss, token, delimiter)) {
            if (!token.empty())
                tokens.push_back(token);
        }
        return tokens;
    }

    // 计算一串符号的 First 集合
    set<string> computeFirstOfString(const vector<string>& symbols) {
        set<string> result;
        bool epsilonFound = true;
        for (auto& sym : symbols) {
            for (auto& f : First[sym]) {
                if (f != "e") {
                    result.insert(f);
                }
            }
            if (First[sym].find("e") == First[sym].end()) {
                epsilonFound = false;
                break;
            }
        }
        if (epsilonFound) {
            result.insert("e");
        }
        return result;
    }

    // 将符号串转为字符串（用于映射）
    string stringify(const vector<string>& symbols) {
        string s;
        for (auto& sym : symbols) {
            s += sym + " ";
        }
        return s;
    }

    // 分词函数：将输入字符串转化为终结符序列
    vector<string> tokenize(const string& input) {
        vector<string> tokens;
        // 假设所有终结符都是单字符
        for (char c : input) {
            string token(1, c);
            tokens.push_back(token);
        }
        return tokens;
    }

    // 打印项目集规范族（调试用）
    void printCanonicalCollection() {
        cout << "\nCanonical Collection of LR(1) Items:\n";
        for (int i = 0; i < (int)C.size(); ++i) {
            cout << "C" << i << ":\n";
            for (auto& item : C[i]) {
                cout << "  " << item.lhs << " -> ";
                for (int j = 0; j < (int)item.rhs.size(); ++j) {
                    if (j == item.dot)
                        cout << ". ";
                    cout << item.rhs[j] << " ";
                }
                if (item.dot == (int)item.rhs.size())
                    cout << ". ";
                cout << ", " << item.lookahead << "\n";
            }
            cout << "\n";
        }
    }

    // 打印 First 和 Follow 集合（调试用）
    void printFirstFollow() {
        cout << "\nFIRST sets:\n";
        for (auto& pair : First) {
            cout << pair.first << ": { ";
            for (auto& sym : pair.second) {
                cout << sym << " ";
            }
            cout << "}\n";
        }

        cout << "\nFOLLOW sets:\n";
        for (auto& pair : Follow) {
            cout << pair.first << ": { ";
            for (auto& sym : pair.second) {
                cout << sym << " ";
            }
            cout << "}\n";
        }
    }

    // 打印分析表（改进版）
    void printParseTable() {
        // 打印 Action 表
        cout << "\nAction Table:\n";

        // 收集所有终结符（包括 $）
        vector<string> termList;
        for (const auto& t : terminals) {
            if (t != "e") // 'e' 作为特殊符号单独处理
                termList.push_back(t);
        }
        termList.push_back("$"); // 添加结束符

        // 打印表头
        cout << "State\t";
        for (auto& term : termList) {
            cout << term << "\t";
        }
        cout << "\n";

        // 打印每个状态的 Action 表项
        for (int i = 0; i < (int)C.size(); ++i) {
            cout << i << "\t";
            for (auto& term : termList) {
                if (Action.find(i) != Action.end() && Action[i].find(term) != Action[i].end()) {
                    cout << Action[i][term] << "\t";
                }
                else {
                    cout << "\t";
                }
            }
            cout << "\n";
        }

        // 打印 Goto 表
        cout << "\nGoto Table:\n";

        // 收集所有非终结符
        vector<string> ntList(nonTerminals.begin(), nonTerminals.end());

        // 打印表头
        cout << "State\t";
        for (auto& nt : ntList) {
            cout << nt << "\t";
        }
        cout << "\n";

        // 打印每个状态的 Goto 表项
        for (int i = 0; i < (int)C.size(); ++i) {
            cout << i << "\t";
            for (auto& nt : ntList) {
                if (GotoTable.find(i) != GotoTable.end() && GotoTable[i].find(nt) != GotoTable[i].end()) {
                    cout << GotoTable[i][nt] << "\t";
                }
                else {
                    cout << "\t";
                }
            }
            cout << "\n";
        }
    }

    // 运行解析器
    void run() {
        readGrammar();
        computeFirst();
        computeFollow();
        buildCanonicalCollection();
        buildParseTable();
        printParseTable(); // 输出解析表
        parseInput();
    }

private:
    string inputString; // 待分析的输入字符串
};

int main() {
    LR1Parser parser;
    parser.run();
    return 0;
}
