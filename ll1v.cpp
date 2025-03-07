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
#include <iomanip>
using namespace std;

// 定义产生式
struct Production {
    string lhs;                // 左部非终结符
    vector<string> rhs;        // 右部产生式

    Production() {}
    Production(string left, vector<string> right) : lhs(left), rhs(right) {}
};

// LL1 解析器类
class LL1Parser {
private:
    vector<Production> productions;               // 产生式列表
    set<string> nonTerminals;                     // 非终结符集合
    set<string> terminals;                        // 终结符集合
    string startSymbol;                           // 开始符号

    // First 和 Follow 集合
    map<string, set<string>> First;
    map<string, set<string>> Follow;

    // 分析表：非终结符 -> (终结符 -> 产生式编号)
    map<string, map<string, int>> parseTable;

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
                    productions.emplace_back(lhs, vector<string>{ "e" });
                }
                else {
                    vector<string> symbols = split(alt, ' ');
                    productions.emplace_back(lhs, symbols);
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
            for (int i = 0; i < productions.size(); ++i) {
                Production& prod = productions[i];
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

        // 打印 FIRST 集合
        cout << "\nFIRST sets:\n";
        for (auto& pair : First) {
            cout << pair.first << ": { ";
            for (auto& sym : pair.second) {
                cout << sym << " ";
            }
            cout << "}\n";
        }
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
            for (int i = 0; i < productions.size(); ++i) {
                Production& prod = productions[i];
                string A = prod.lhs;
                vector<string> alpha = prod.rhs;
                for (int j = 0; j < (int)alpha.size(); j++) {
                    string B = alpha[j];
                    if (nonTerminals.find(B) != nonTerminals.end()) {
                        // β = alpha[j+1 ... end]
                        vector<string> beta(alpha.begin() + j + 1, alpha.end());
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

        // 打印 FOLLOW 集合
        cout << "\nFOLLOW sets:\n";
        for (auto& pair : Follow) {
            cout << pair.first << ": { ";
            for (auto& sym : pair.second) {
                cout << sym << " ";
            }
            cout << "}\n";
        }
    }

    // 构造分析表
    void buildParseTable() {
        for (int i = 0; i < (int)productions.size(); ++i) {
            Production& prod = productions[i];
            string A = prod.lhs;
            vector<string> alpha = prod.rhs;

            // 计算 First(alpha)
            set<string> firstAlpha = computeFirstOfString(alpha);

            // 对于 a ∈ First(alpha) - {e}, M[A,a] = i+1
            for (auto& a : firstAlpha) {
                if (a != "e") {
                    if (parseTable[A].find(a) != parseTable[A].end()) {
                        // 检查是否有冲突
                        cerr << "Parse table conflict at M[" << A << "," << a << "] between productions "
                            << parseTable[A][a] << " and " << (i + 1) << endl;
                        exit(1);
                    }
                    parseTable[A][a] = i + 1; // 1-based indexing
                }
            }

            // 如果 e ∈ First(alpha), 对于 b ∈ Follow(A), M[A,b] = i+1
            if (firstAlpha.find("e") != firstAlpha.end()) {
                for (auto& b : Follow[A]) {
                    if (parseTable[A].find(b) != parseTable[A].end()) {
                        // 检查是否有冲突
                        cerr << "Parse table conflict at M[" << A << "," << b << "] between productions "
                            << parseTable[A][b] << " and " << (i + 1) << endl;
                        exit(1);
                    }
                    parseTable[A][b] = i + 1;
                }
            }
        }

        // 不在 buildParseTable() 中打印解析表，而是使用单独的函数
    }

    // 打印预测分析表
    void printParseTable() {
        // 收集所有终结符（包括 $）
        vector<string> termList;
        for (const auto& t : terminals) {
            if (t != "e") // 'e' 作为特殊符号单独处理
                termList.push_back(t);
        }
        termList.push_back("$"); // 添加结束符

        // 收集所有非终结符
        vector<string> ntList(nonTerminals.begin(), nonTerminals.end());

        // 打印 Action 表
        cout << "\nLL(1) Parse Table (Action):\n";
        // 打印表头
        cout << left << setw(15) << "Non-Terminal";
        for (const auto& term : termList) {
            cout << left << setw(15) << term;
        }
        cout << "\n";

        // 打印每个非终结符的行
        for (const auto& A : ntList) {
            cout << left << setw(15) << A;
            for (const auto& term : termList) {
                if (parseTable.find(A) != parseTable.end() && parseTable[A].find(term) != parseTable[A].end()) {
                    cout << left << setw(15) << parseTable[A][term];
                }
                else {
                    cout << left << setw(15) << "";
                }
            }
            cout << "\n";
        }

        // 如果需要，可以分开 Action 和 Goto 表
        // 但在LL(1)中，Goto表通常不需要，因为解析表已经包含了所有必要的信息
    }

    // 解析输入字符串
    void parseInput() {
        // 分词：将输入字符串按字符拆分
        vector<string> inputTokens = tokenize(inputString);
        inputTokens.push_back("$"); // 末尾加入 $

        // 初始化解析栈
        vector<string> parseStack;
        parseStack.push_back("$");
        parseStack.push_back(startSymbol);

        int ip = 0; // 输入指针
        bool accept = false;

        // 输出表头
        cout << "\nParsing Actions:\n";
        cout << left << setw(30) << "Stack" << setw(30) << "Input" << "Action\n";

        while (!parseStack.empty()) {
            // 构造堆栈字符串
            string stackStr = "";
            for (auto& s : parseStack) {
                stackStr += s + " ";
            }

            // 构造剩余输入字符串
            string inputStr = "";
            for (int i = ip; i < (int)inputTokens.size(); i++) {
                inputStr += inputTokens[i] + " ";
            }

            // 获取栈顶符号
            string X = parseStack.back();
            string a = inputTokens[ip];

            // 检查是否接受
            if (X == "$" && a == "$") {
                cout << left << setw(30) << stackStr << setw(30) << inputStr << "accept\n";
                accept = true;
                break;
            }

            // 如果 X 是终结符
            if (isTerminal(X)) {
                if (X == a) {
                    // match
                    cout << left << setw(30) << stackStr << setw(30) << inputStr << "match\n";
                    parseStack.pop_back();
                    ip++;
                }
                else {
                    // error
                    cout << left << setw(30) << stackStr << setw(30) << inputStr << "error\n";
                    break;
                }
            }
            else { // X 是非终结符
                // 查找 M[X, a]
                if (parseTable.find(X) != parseTable.end() && parseTable[X].find(a) != parseTable[X].end()) {
                    int prodNum = parseTable[X][a];
                    // 检查生产式编号是否有效
                    if (prodNum <= 0 || prodNum > (int)productions.size()) {
                        cerr << "Error: Invalid production number " << prodNum << " for M[" << X << "," << a << "].\n";
                        cout << left << setw(30) << stackStr << setw(30) << inputStr << "error\n";
                        break;
                    }
                    Production& prod = productions[prodNum - 1]; // 1-based indexing

                    // 输出使用的产生式编号
                    cout << left << setw(30) << stackStr << setw(30) << inputStr << "Use production " << prodNum << ": " << prod.lhs << " -> ";
                    for (const auto& sym : prod.rhs) {
                        cout << sym << " ";
                    }
                    cout << "\n";

                    // 弹出栈顶
                    parseStack.pop_back();

                    // 将产生式右部逆序压栈（如果不是 e）
                    if (!(prod.rhs.size() == 1 && prod.rhs[0] == "e")) {
                        for (int i = prod.rhs.size() - 1; i >= 0; --i) {
                            parseStack.push_back(prod.rhs[i]);
                        }
                    }
                }
                else {
                    // error
                    cout << left << setw(30) << stackStr << setw(30) << inputStr << "error\n";
                    break;
                }
            }
        }

        if (accept) {
            cout << "\nParsing accepted.\n";
        }
        else {
            cout << "\nParsing failed.\n";
        }
    }

    // 运行解析器
    void run() {
        readGrammar();
        computeFirst();
        computeFollow();
        buildParseTable();
        printParseTable(); // 输出预测分析表
        parseInput();
    }

private:
    string inputString; // 待分析的输入字符串

    // 辅助函数：判断是否是非终结符
    bool isNonTerminal(const string& sym) {
        return nonTerminals.find(sym) != nonTerminals.end();
    }

    // 辅助函数：判断是否是终结符
    bool isTerminal(const string& sym) {
        return terminals.find(sym) != terminals.end();
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

    // 分词函数：将输入字符串转化为终结符序列
    vector<string> tokenize(const string& input) {
        vector<string> tokens;
        // 假设所有终结符都是单字符，或者特定多字符符号
        // 这里假设终结符都是单字符
        for (char c : input) {
            string token(1, c);
            tokens.push_back(token);
        }
        return tokens;
    }
};

int main() {
    LL1Parser parser;
    parser.run();
    return 0;
}
