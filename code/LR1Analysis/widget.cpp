#include "widget.h"
#include "ui_widget.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileDialog>
#include <QTextCodec>
#include <QMessageBox>
#include <iostream>
#include <map>
#include <vector>
#include <stack>
#include <unordered_map>
#include <queue>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#pragma execution_character_set("utf-8")
using namespace std;

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

// 全局文法变量
string grammarStr;


unordered_map<string, set<vector<string>>> grammarMap2;

// 文法unit
struct grammarUnit
{
    int gid;
    string left;
    vector<string> right;
    grammarUnit(string l, vector<string> r)
    {
        left = l;
        right = r;
    }
};

// 文法数组
deque<grammarUnit> grammarDeque;

// 文法查找下标
map<pair<string, string>, int> grammarToInt;

// 用于存储所有非终结符的vector
vector<string> nonTerminals;
// 用于存储所有终结符的vector
vector<string> terminals;

/*************  公用函数 ****************/


/**/
bool isBigAlpha(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool isSmallAlpha(char c)
{
    return !(c >= 'A' && c <= 'Z') && c != '@';
}

// 判断是否是非终结符（基于nonTerminals向量来判断）
bool isNonTerminal(const string& symbol)
{
    return find(nonTerminals.begin(), nonTerminals.end(), symbol)!= nonTerminals.end();
}

// 判断是否是终结符（基于terminals向量来判断）
bool isTerminal(const string& symbol)
{
    return find(terminals.begin(), terminals.end(), symbol)!= terminals.end();
}

void reset();

// 开始符号
string startSymbol;

// 增广后开始符号
string trueStartSymbol;



/************* 文法初始化处理 ****************/

// 辅助函数，将vector<string>转换为string（方便输出等操作，可根据实际需求调整格式）
string vectorToString(const vector<string>& vec)
{
    string result = "";
    for (const auto& word : vec)
    {
        result += word + " ";
    }
    // 去掉最后一个空格
    if(result.length()){
        result.pop_back();
    }
    return result;
}

// 处理文法
void handleGrammar()
{
    vector<string> lines;
    istringstream iss(grammarStr);
    string line;

    // 防止中间有换行符
    while (getline(iss, line))
    {
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    for (const auto& rule : lines)
    {
        istringstream ruleStream(rule);
        string nonTerminalStr;
        ruleStream >> nonTerminalStr;  // 读取非终结符（现在可能是单词形式）

        // 判断读取到的是否是非终结符（单词或单个大写字母为非终结符）
        bool isWordNonTerminal = nonTerminalStr.size() > 1;
        bool isSingleAlphaNonTerminal = nonTerminalStr.size() == 1 && isBigAlpha(nonTerminalStr[0]);
        if (!(isWordNonTerminal || isSingleAlphaNonTerminal))
        {
            QMessageBox::critical(nullptr, "Error", "文法开头必须是非终结符（单词或大写字母）!");
            continue;
        }

        // 将非终结符添加到非终结符集合中（如果还没添加过）
        if (find(nonTerminals.begin(), nonTerminals.end(), nonTerminalStr) == nonTerminals.end())
        {
            nonTerminals.push_back(nonTerminalStr);
        }

        // 跳过箭头及空格
        ruleStream.ignore(numeric_limits<streamsize>::max(), '-');
        ruleStream.ignore(numeric_limits<streamsize>::max(), '>');

        vector<string> rightHandSideVec;
        string word;
        while (ruleStream >> word)
        {
            // 判断读取到的符号是终结符还是非终结符，并分别添加到对应的集合中
            if (word.size() == 1 && isSmallAlpha(word[0]))
            {
                if (find(terminals.begin(), terminals.end(), word) == terminals.end())
                {
                    terminals.push_back(word);
                }
            }
            else
            {
                // 非终结符情况，添加到非终结符集合（如果还没添加过）
                if (find(nonTerminals.begin(), nonTerminals.end(), word) == nonTerminals.end())
                {
                    nonTerminals.push_back(word);
                }
            }
            rightHandSideVec.push_back(word);
        }

        // 如果是第一条规则，则认为是开始符号
        if (grammarMap2.empty())
        {
            startSymbol = nonTerminalStr;
            trueStartSymbol = startSymbol;
        }

        // 将文法结构化，存入新的数据结构grammarMap2中
        grammarMap2[nonTerminalStr].insert(rightHandSideVec);

        // 为LR0做准备，构建grammarDeque，存储更详细的文法单元信息（考虑非终结符可能是单词形式）
        grammarDeque.push_back(grammarUnit(nonTerminalStr, rightHandSideVec));
    }

    // 增广处理
    if (grammarMap2[startSymbol].size() > 1)
    {
        // 构建增广后的非终结符（ "E'" 作为增广后的开始符号）
        string augmentedNonTerminal = "E'";
        grammarDeque.push_front(grammarUnit(augmentedNonTerminal, vector<string>(1, startSymbol)));
        trueStartSymbol = "E'";
    }

    // 开始编号
    int gid = 0;
    for (auto& g : grammarDeque)
    {
        g.gid = gid++;
        string leftStr = "";
        for (const auto& word : g.left)
        {
            leftStr += word + " ";
        }
        grammarToInt[make_pair(g.left, vectorToString(g.right))] = g.gid;
    }
    // 测试，输出grammarMap2的键和值
    for (const auto& it : grammarMap2) {
        // 输出键（非终结符）
        cout << "非终结符（键）: " << it.first << endl;
        // 输出值（对应非终结符的产生式右部集合）
        cout << "产生式右部集合（值）: " << endl;
        for (const auto& vec : it.second) {
            for (const auto& symbol : vec) {
                cout << symbol << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}

/************* First集合求解 ****************/


// First集合单元
struct firstUnit
{
    set<string> s;
    bool isEpsilon = false;
};

// 非终结符的First集合
map<string, firstUnit> firstSets;


// 计算First集合
bool calculateFirstSets()
{

    bool flag = false;
    for (auto& grammar : grammarMap2)
    {
        string nonTerminal = grammar.first;
        // 保存当前First集合的大小，用于检查是否有变化
        size_t originalSize = firstSets[nonTerminal].s.size();
        bool originalE = firstSets[nonTerminal].isEpsilon;
        for (auto& g : grammar.second)
        {
            int k = 0;
            // 求first集时有针对遍历文法规则循环的break，
            // 只要找到终结符或非终结符中的first集没有epsilon（此非终结符不可能成为epsilon，也就没有后面字符什么事了）就直接break
            // 说明求first集看左边
            while (k <= g.size() - 1)
            {
                set<string> first_k;
                // 在求first集合的函数中遇到若非终结符的文法规则包含空字符串并不会将直接添加到first集合当中，
                // 而是将这个first集合的isEpsilon标记为true，用来说明这个first集合虽然没有把空字符串显示出来，
                // 但是它的确含有空字符串。UI处理部分得知这个信息后会把空字符串显示出来。
                // 这样处理是为了处理简便，使得无论first集合是否含有空字符串，都能使用统一的逻辑处理first集合
                // 同时还能实现“first(K) - {@}”的规则
                if (g[k] == "@")
                {
                    k++;
                    continue;
                }
                else if (isTerminal(g[k]))
                {
                    first_k.insert(g[k]);
                }
                else
                {
                    first_k = firstSets[g[k]].s;
                }
                firstSets[nonTerminal].s.insert(first_k.begin(), first_k.end());
                // 如果是终结符或者没有空串在非终结符中，直接跳出
                // 某个非终结符（A）的文法规则中含非终结符（B）打头，如果B含有空串，则不能跳过
                // 即需要扫描B后面的字符，将该字符的first集合加入到A的first集合当中
                if (isTerminal(g[k]) || !firstSets[g[k]].isEpsilon)
                {
                    break;
                }
                k++;
            }
            if (k == g.size())
            {
                firstSets[nonTerminal].isEpsilon = true;
            }
        }
        // 看原始大小和是否变化epsilon，如果变化说明还得重新再来一次
        if (originalSize != firstSets[nonTerminal].s.size() || originalE != firstSets[nonTerminal].isEpsilon)
        {
            flag = true;
        }
    }
    return flag;
}

void getFirstSets()
{
    // 不停迭代，直到First集合不再变化
    bool flag = false;
    do
    {
        flag = calculateFirstSets();
    } while (flag);
}


/************* Follow集合求解 ****************/
// Follow集合单元
struct followUnit
{
    set<string> s;
};

// 非终结符的Follow集合
map<string, followUnit> followSets;

// 添加Follow集合
void addToFollow(string nonTerminal, const set<string>& elements)
{
    followSets[nonTerminal].s.insert(elements.begin(), elements.end());
}

// 计算Follow集合
// 假设有 S->AB A->aB B->c B->@
bool calculateFollowSets()
{
    bool flag = false;
    for (auto& grammar : grammarMap2)
    {
        string nonTerminal = grammar.first;

        for (auto& g : grammar.second)
        {
            // 在求follow集时，没有针对遍历文法规则循环的break，说明求follow集看右边
            for (int i = 0; i < g.size(); ++i)
            {
                if (isTerminal(g[i]) || g[i] == "@")
                {
                    continue;  // 跳过终结符
                }

                set<string> follow_k;
                size_t originalSize = followSets[g[i]].s.size();

                if (i == g.size() - 1)
                {
                    // Case A: A -> αB, add Follow(A) to Follow(B)
                    follow_k.insert(followSets[nonTerminal].s.begin(), followSets[nonTerminal].s.end());
                }
                else
                {
                    // Case B: A -> αBβ
                    int j = i + 1;
                    while (j < g.size())
                    {
                        if (isTerminal(g[j]))
                        {   // 终结符直接加入并跳出
                            follow_k.insert(g[j]);
                            break;
                        }
                        else
                        {   // 非终结符加入first集合
                            set<string> first_beta = firstSets[g[j]].s;
                            // A的follow集包含B的first集
                            follow_k.insert(first_beta.begin(), first_beta.end());

                            // 如果没有空串在first集合中，停止。
                            // 换言之，如果有空串在g[j]first集合中，g[j]就有成为空串的可能，此时相当于g[i]后面跟着的是g[j]后面的字符，
                            // 当然g[j]后面也可能没有字符，此时g[i]相当于最后一个字符。无论那种情况都要++j，读取g[j]后一位。
                            // 如果g[j]后面有字符就会继续循环（符合j < g.size()的循环条件），将g[j]后一个字符（即++j后得到的g[j]字符）的first集合加进g[i]的follow。
                            // 如果g[j]后面没有字符，那么++j后，j等于文法规则的长度，不符合循环条件，退出循环。然后将非终结符的follow集加入到g[i]的follow集中。
                            if (!firstSets[g[j]].isEpsilon)
                            {
                                break;
                            }

                            ++j;
                        }

                    }

                    // If β is ε or β is all nullable, add Follow(A) to Follow(B)
                    if (j == g.size())
                    {
                        // A的follow集包含S的follow集
                        follow_k.insert(followSets[nonTerminal].s.begin(), followSets[nonTerminal].s.end());
                    }
                }

                addToFollow(g[i], follow_k);
                // 检查是否变化
                if (originalSize != followSets[g[i]].s.size())
                {
                    flag = true;
                }
            }
        }


    }

    return flag;
}

void getFollowSets()
{
    // 开始符号加入$
    addToFollow(trueStartSymbol, { "$" });
    addToFollow(startSymbol, { "$" });

    // 不停迭代，直到Follow集合不再变化
    bool flag = false;
    do
    {
        flag = calculateFollowSets();
    } while (flag);
}/**/

/************* LR0 DFA表生成 ****************/

// 状态编号
int scnt = 0;
// 项目编号
int ccnt = 0;
// DFA表每一项项目的结构
struct dfaCell
{
    int cellid; // 这一项的编号，便于后续判断状态相同
    int gid; // 文法编号
    int index = 0; // .在第几位，如i=3, xxx.x，i=0, .xxxx, i=4, xxxx.
};
// 用于通过编号快速找到对应结构
vector<dfaCell> dfaCellVector;
struct nextStateUnit
{
    string c; // 通过什么字符进入这个状态
    int sid; // 下一个状态id是什么
};
// DFA表状态
struct dfaState
{
    int sid; // 状态id
    vector<int> originV;    // 未闭包前的cell
    vector<int> cellV;  // 存储这个状态的cellid
    bool isEnd = false; // 是否为规约状态
    vector<nextStateUnit> nextStateVector; // 下一个状态集合
    set<string> right_VNs; // 判断是否已经处理过这个非终结符
};
// 用于通过编号快速找到对应结构
vector<dfaState> dfaStateVector;

// 非终结符集合
set<string> VN;
// 终结符集合
set<string> VT;

// 判断是不是新结构
int isNewCell(int gid, int index)
{
    for (const dfaCell& cell : dfaCellVector)
    {
        // 检查dfaCellVector中是否存在相同的gid和index的dfaCell
        if (cell.gid == gid && cell.index == index)
        {
            return cell.cellid; // 不是新结构
        }
    }
    return -1; // 是新结构
}

// 判断是不是新状态
int isNewState(const vector<int>& cellIds)
{
    for (const dfaState& state : dfaStateVector)
    {
        // 检查状态中的originV是否相同
        if (state.originV.size() == cellIds.size() &&
            equal(state.originV.begin(), state.originV.end(), cellIds.begin()))
        {
            return state.sid; // 不是新状态
        }
    }

    return -1; // 是新状态
}

// DFS深度优先标记数组
set<int> visitedStates;

// 创建LR0的初始状态
void createFirstState()
{
    // 由于增广，一定会只有一个入口
    dfaState zero = dfaState();
    zero.sid = scnt++; // 给他一个id
    dfaStateVector.push_back(zero); // 放入数组中

    // 添加初始的LR0项，即E' -> .S
    dfaCell startCell;
    startCell.gid = 0; // 这里假设增广文法的编号为0
    startCell.index = 0;
    startCell.cellid = ccnt++;

    dfaCellVector.push_back(startCell);

    // 把初始LR0项放入初始状态
    dfaStateVector[0].cellV.push_back(startCell.cellid);
    dfaStateVector[0].originV.push_back(startCell.cellid);
}

// 生成LR0状态
void generateLR0State(int stateId)
{
    // DFS,如果走过就不走了
    if (visitedStates.count(stateId) > 0) {
        return;
    }

    // 标记走过了
    visitedStates.insert(stateId);

    qDebug() << stateId << endl;

    // 求闭包
    for (int i = 0; i < dfaStateVector[stateId].cellV.size(); ++i)
    {
        dfaCell& currentCell = dfaCellVector[dfaStateVector[stateId].cellV[i]];

        qDebug() << QString::fromStdString(grammarDeque[currentCell.gid].left) << "->" << QString::fromStdString(vectorToString(grammarDeque[currentCell.gid].right)) << endl;

        qDebug() << "closure current index:" << currentCell.index << endl;

        // 如果点号在产生式末尾或者空串，则跳过（LR0不需要结束）
        if (currentCell.index == grammarDeque[currentCell.gid].right.size() || vectorToString(grammarDeque[currentCell.gid].right) == "@")
        {
            dfaStateVector[stateId].isEnd = true;
            continue;
        }

        string nextSymbol = grammarDeque[currentCell.gid].right[currentCell.index];
        

        // 如果nextSymbol是非终结符，则将新项添加到状态中
        if (isNonTerminal(nextSymbol) && dfaStateVector[stateId].right_VNs.find(nextSymbol) == dfaStateVector[stateId].right_VNs.end())
        {
            dfaStateVector[stateId].right_VNs.insert(nextSymbol);
            for (auto& grammar : grammarMap2[nextSymbol])
            {
                // 获取通过nextSymbol转移的新LR0项
                dfaCell nextCell = dfaCell();
                nextCell.gid = grammarToInt[make_pair(nextSymbol,vectorToString(grammar))];
                nextCell.index = 0;
                int nextcellid = isNewCell(nextCell.gid, nextCell.index);
                if (nextcellid == -1)
                {
                    nextCell.cellid = ccnt++;
                    dfaCellVector.push_back(nextCell);
                    dfaStateVector[stateId].cellV.push_back(nextCell.cellid);
                }
                else dfaStateVector[stateId].cellV.push_back(nextcellid);
            }

        }
    }

    // 暂存新状态
    map<string,dfaState> tempSave;
    // 生成新状态，但还不能直接存到dfaStateVector中，我们要校验他是否和之前的状态一样
    for (int i = 0; i < dfaStateVector[stateId].cellV.size(); ++i)
    {
        dfaCell& currentCell = dfaCellVector[dfaStateVector[stateId].cellV[i]];

        qDebug() << QString::fromStdString(grammarDeque[currentCell.gid].left) << "->" << QString::fromStdString(vectorToString(grammarDeque[currentCell.gid].right)) << endl;

        qDebug() << "temp current index:" << currentCell.index << endl;

        // 如果点号在产生式末尾，则跳过（LR0不需要结束）
        if (currentCell.index == grammarDeque[currentCell.gid].right.size() || vectorToString(grammarDeque[currentCell.gid].right) == "@")
        {
            continue;
        }

        // 下一个字符
        string nextSymbol = grammarDeque[currentCell.gid].right[currentCell.index];

        // 创建下一个状态（临时的）
        dfaState& nextState = tempSave[nextSymbol];
        dfaCell nextStateCell = dfaCell();
        nextStateCell.gid = currentCell.gid;
        nextStateCell.index = currentCell.index + 1;

        // 看看里面的项目是否有重复的，如果重复拿之前的就好，不重复生成
        int nextStateCellid = isNewCell(nextStateCell.gid, nextStateCell.index);
        if (nextStateCellid == -1)
        {
            nextStateCell.cellid = ccnt++;
            dfaCellVector.push_back(nextStateCell);
        }
        else nextStateCell.cellid = nextStateCellid;
        nextState.cellV.push_back(nextStateCell.cellid);
        nextState.originV.push_back(nextStateCell.cellid);

        // 收集一下，方便后面画表
        if (isNonTerminal(nextSymbol))
        {
            VN.insert(nextSymbol);
        }
        else if (isTerminal(nextSymbol))
        {
            VT.insert(nextSymbol);
        }
    }

    // 校验状态是否有重复的
    for (auto& t : tempSave)
    {
        dfaState nextState = dfaState();
        int newStateId = isNewState(t.second.originV);
        // 不重复就新开一个状态
        if (newStateId == -1)
        {
            nextState.sid = scnt++;
            nextState.cellV = t.second.cellV;
            nextState.originV = t.second.originV;
            dfaStateVector.push_back(nextState);
        }
        // 如果重复那么nextState的sid是已经存在的状态的id
        else nextState.sid = newStateId;
        // 存入现在这个状态的nextStateVector
        // 无论是否重复，当前状态都需要指向 nextState
        // 也就是说重复则指向已有的状态，不重复就指向新建的状态
        nextStateUnit n = nextStateUnit();
        n.sid = nextState.sid;
        n.c = t.first;
        dfaStateVector[stateId].nextStateVector.push_back(n);
    }

    // 对每个下一个状态进行递归
    int nsize = dfaStateVector[stateId].nextStateVector.size();
    for (int i = 0; i < nsize; i++)
    {
        auto& nextunit = dfaStateVector[stateId].nextStateVector[i];
        generateLR0State(nextunit.sid);
    }
}

// 生成LR0入口
void getLR0()
{
    visitedStates.clear();

    // 首先生成第一个状态
    createFirstState();

    // 递归生成其他状态
    generateLR0State(0);
}

// 拼接字符串，获取状态内的文法
string getStateGrammar(const dfaState& d)
{
    string result = "";
    for (auto cell : d.cellV)
    {
        const dfaCell& dfaCell = dfaCellVector[cell];
        // 拿到文法
        int gid = dfaCell.gid;
        grammarUnit g = grammarDeque[gid];
        // 拿到位置
        int i = dfaCell.index;
        // 拼接结果
        string r = "";
        r += g.left == "^"? "E\'->" : g.left + "->";
        vector<string> gRight = g.right;
        i == gRight.size()? gRight[i - 1].insert(gRight[i - 1].size(), 1, '.') : gRight[i].insert(0, 1, '.');
        string right = vectorToString(g.right) == "@" ? "" : vectorToString(gRight);
//        right.insert(i, 1, '.');
        r += right;
        result += r + " ";
    }
    qDebug() << "result: " << QString::fromStdString(result);
    return result;
}
/**/

/************* LR1 DFA表生成 ****************/

// LR(1)项目结构
struct LR1Item
{
    int gid;  // 文法编号
    int index; //.在第几位，如i=3, xxx.x，i=0,.xxxx, i=4, xxxx.
    string lookahead; // 向前看符号（Lookahead符号）
};
// 状态编号
int scntLR1 = 0;
// 项目编号
int ccntLR1 = 0;
// DFA表每一项项目的结构
struct dfaCellLR1
{
    int cellid; // 这一项的编号，便于后续判断状态相同
    LR1Item item; // LR(1)项目信息
};
// 用于通过编号快速找到对应结构
vector<dfaCellLR1> dfaCellVectorLR1;
struct nextStateUnitLR1
{
    string c; // 通过什么字符进入这个状态
    int sid; // 下一个状态id是什么
};
// DFA表状态
struct dfaStateLR1
{
    int sid; // 状态id
    vector<int> originV;    // 未闭包前的cell
    vector<int> cellV;  // 存储这个状态的cellid
    bool isEnd = false; // 是否为规约状态
    vector<nextStateUnitLR1> nextStateVector; // 下一个状态集合
    set<string> right_VNs; // 判断是否已经处理过这个非终结符
};

// 用于通过编号快速找到对应结构
vector<dfaStateLR1> dfaStateVectorLR1;

// 非终结符集合
set<string> VNLR1;
// 终结符集合
set<string> VTLR1;

// 判断是不是新结构（LR(1)版本，多考虑Lookahead符号）
int isNewCellLR1(int gid, int index, string lookahead)
{
    qDebug() << "generateLR1State: 20";
    for (const dfaCellLR1& cell : dfaCellVectorLR1)
    {

        if (cell.item.gid == gid && cell.item.index == index && cell.item.lookahead == lookahead)
        {
            return cell.cellid; // 不是新结构
        }
    }
    return -1; // 是新结构
}
// 判断是不是新状态（LR(1)，考虑同心项和Lookahead符号）
int isNewStateLR1(const vector<int>& cellIds)
{
    for (const dfaStateLR1& state : dfaStateVectorLR1)
    {
        if (state.originV.size() == cellIds.size() &&
            equal(state.originV.begin(), state.originV.end(), cellIds.begin()))
        {// 进一步检查Lookahead符号是否都相同
            bool sameLookahead = true;
            for (size_t i = 0; i < state.originV.size(); ++i)
            {
                int cellId = state.originV[i];
                int otherCellId = cellIds[i];
                if (dfaCellVectorLR1[cellId].item.lookahead !=
                    dfaCellVectorLR1[otherCellId].item.lookahead)
                {
                    sameLookahead = false;
                    break;
                }
            }
            if (sameLookahead)
            {
                return state.sid; // 不是新状态
            }
        }
    }
    return -1; // 是新状态
}

// DFS深度优先标记数组
set<int> visitedStatesLR1;

// 求LR(1)项目的闭包（核心函数，考虑Lookahead符号）
// 求LR(1)项目的闭包（修正后，基于Follow集来确定向前看符号）
//void closureLR1(vector<int>& items)
//{

//}

// 创建LR(1)的初始状态
void createFirstStateLR1()
{
    qDebug() << "createFirstStateLR1 start";
    // 由于增广，一定会只有一个入口
    dfaStateLR1 zero = dfaStateLR1();
    zero.sid = scntLR1++; // 给他一个id
    dfaStateVectorLR1.push_back(zero); // 放入数组中

    // 获取开始符号对应的文法编号（假设增广后的开始符号相关文法在grammarDeque中第一个位置）
    int startGid = grammarDeque[0].gid;

    // 添加初始的LR(1)项，即E' ->.S,$
    dfaCellLR1 startCell;
    startCell.item.gid = startGid; // 这里假设增广文法的编号为0
    startCell.item.index = 0;
    startCell.item.lookahead = "$";
    startCell.cellid = ccntLR1++;

    dfaCellVectorLR1.push_back(startCell);

    // 把初始LR(1)项放入初始状态
    dfaStateVectorLR1[0].cellV.push_back(startCell.cellid);
    dfaStateVectorLR1[0].originV.push_back(startCell.cellid);
}


void generateLR1State(int stateId)
{
    // DFS,如果走过就不走了
    if (visitedStatesLR1.count(stateId) > 0) {
        return;
    }

    // 标记走过了
    visitedStatesLR1.insert(stateId);

    //if (dfaStateVector[stateId].isEnd)
    //{
    //    return;
    //}

    qDebug() << stateId << endl;

    // 求闭包
    for (int i = 0; i < dfaStateVectorLR1[stateId].cellV.size(); ++i)
    {
        getFirstSets();
        getFollowSets();
        dfaCellLR1& currentCell = dfaCellVectorLR1[dfaStateVectorLR1[stateId].cellV[i]];

        qDebug() << QString::fromStdString(grammarDeque[currentCell.item.gid].left) << QString::fromStdString("->") << QString::fromStdString(vectorToString(grammarDeque[currentCell.item.gid].right)) << endl;

        qDebug() << "closure current index:" << currentCell.item.index << endl;

        // 如果点号在产生式末尾或者空串，则跳过（LR1不需要结束）
        vector<string> grammarRight = grammarDeque[currentCell.item.gid].right;
        if (currentCell.item.index == grammarRight.size() || find(grammarRight.begin(), grammarRight.end(), "@") != grammarRight.end())
        {
            dfaStateVectorLR1[stateId].isEnd = true;
            continue;
        }

        qDebug() << "generateLR1State: 1";

        string nextSymbol = grammarRight[currentCell.item.index];
        qDebug() << "nextSymbol: " << QString::fromStdString(nextSymbol);
        qDebug() << "grammarDeque[currentCell.item.gid].left: " << QString::fromStdString(grammarDeque[currentCell.item.gid].left);
        string lookahead = "";
        qDebug() << "Follow set for " << QString::fromStdString(grammarDeque[currentCell.item.gid].left) << " : ";
        for (string c : followSets[grammarDeque[currentCell.item.gid].left].s) {
            qDebug() << QString::fromStdString(c);
        }
        // 如果点后的字符（串）是非终结符且是文法规则的最后一个字符（串），那么下一条项目的lookahead就是当前项目左部的follow集
        if(isNonTerminal(nextSymbol) && currentCell.item.index + 1 >= grammarRight.size()){
            for(string follow: followSets[grammarDeque[currentCell.item.gid].left].s){
                lookahead += follow;
                lookahead += "/";
                qDebug() << "follow: " << QString::fromStdString(follow);
                qDebug() << "generateLR1State: 21";
            }
            if(lookahead.length()){
                qDebug() << "generateLR1State: 22";
                lookahead.pop_back();
            }
            qDebug() << "lookahead1: " << QString::fromStdString(lookahead);
            qDebug() << "generateLR1State: 2";
        }
        else if(isTerminal(nextSymbol)){
            continue;
        }
        else{
            qDebug() << "generateLR1State: 3";
            // 查看点后面字符（串）的后一个字符（串）
            string nextNextSymbol = grammarRight[currentCell.item.index + 1];
            qDebug() << "nextNextSymbol: " << QString::fromStdString(nextNextSymbol);
            if(nextNextSymbol == "@"){
                // 合法的文法规则右部若有多个终结符或非终结符，那么右部最后一个字符不可能是空字符串，因此可以把空串的后一个字符（串）作为点后面字符（串）的后一个字符（串）
                qDebug() << "generateLR1State: 4";
                nextNextSymbol = grammarDeque[currentCell.item.gid].right[currentCell.item.index + 2];
            }
            else if(isTerminal(nextNextSymbol)){
                // 如果点后面字符（串）的后一个字符（串）是终结符，则将其作为下一条项目的lookahead
                qDebug() << "generateLR1State: 5";
                lookahead = nextNextSymbol;
                qDebug() << "lookahead2: " << QString::fromStdString(lookahead);
            }
            else if(isNonTerminal(nextNextSymbol)){
                // 如果点后面字符（串）的后一个字符（串）是非终结符，则将此非终结符的first集作为下一条项目的lookahead
                qDebug() << "generateLR1State: 6";
                for(string first: firstSets[nextNextSymbol].s){
                    lookahead += first;
                    lookahead += "/";
                }
                if(lookahead.length()){
                    qDebug() << "generateLR1State: 7";
                    lookahead.pop_back();
                }
                qDebug() << "lookahead3: " << QString::fromStdString(lookahead);
            }
        }

        // 如果nextSymbol是非终结符，则将新项添加到状态中
        if (isNonTerminal(nextSymbol) && dfaStateVectorLR1[stateId].right_VNs.find(nextSymbol) == dfaStateVectorLR1[stateId].right_VNs.end())
        {
            qDebug() << "generateLR1State: 8";
            dfaStateVectorLR1[stateId].right_VNs.insert(nextSymbol);
            for (auto& grammar : grammarMap2[nextSymbol])
            {
                // 获取通过nextSymbol转移的新LR0项
                qDebug() << "generateLR1State: 10";
                dfaCellLR1 nextCell = dfaCellLR1();
                qDebug() << "generateLR1State: 11";
                nextCell.item.gid = grammarToInt[make_pair(nextSymbol,vectorToString(grammar))];
                qDebug() << "generateLR1State: 12";
                nextCell.item.index = 0;
                qDebug() << "generateLR1State: 13";
                int nextcellid = isNewCellLR1(nextCell.item.gid, nextCell.item.index, lookahead);
                qDebug() << "generateLR1State: 14";
                if (nextcellid == -1)
                {
                    qDebug() << "generateLR1State: 15";
                    nextCell.cellid = ccntLR1++;
                    qDebug() << "generateLR1State: 16";
                    nextCell.item.lookahead = lookahead;
                    qDebug() << "generateLR1State: 17";
                    dfaCellVectorLR1.push_back(nextCell);
                    qDebug() << "generateLR1State: 18";
                    dfaStateVectorLR1[stateId].cellV.push_back(nextCell.cellid);
                    qDebug() << "generateLR1State: 19";
                }
                else dfaStateVectorLR1[stateId].cellV.push_back(nextcellid);
            }
            qDebug() << "generateLR1State: 9";
        }
    }

    // 暂存新状态（通过哪一个字符（串）进入暂存的新状态）
    map<string, dfaStateLR1> tempSave;
    // 生成新状态，但还不能直接存到dfaStateVector中，我们要校验他是否和之前的状态一样
    qDebug() << "cellV size: " << dfaStateVectorLR1[stateId].cellV.size();
    for (int i = 0; i < dfaStateVectorLR1[stateId].cellV.size(); ++i)
    {
        qDebug() << "generateLR1State: 31";
        dfaCellLR1& currentCell = dfaCellVectorLR1[dfaStateVectorLR1[stateId].cellV[i]];
        vector<string> grammarRight = grammarDeque[currentCell.item.gid].right;

        qDebug() << QString::fromStdString(grammarDeque[currentCell.item.gid].left) << "->" << QString::fromStdString(vectorToString(grammarDeque[currentCell.item.gid].right)) << endl;

        qDebug() << "temp current index:" << currentCell.item.index << endl;

        // 如果点号在产生式末尾，则跳过（LR1不需要结束）
        if (currentCell.item.index == grammarRight.size() || find(grammarRight.begin(), grammarRight.end(), "@") != grammarRight.end())
        {
            qDebug() << "generateLR1State: 23";
            continue;
        }

        qDebug() << "generateLR1State: 24";
        // 下一个字符
        string nextSymbol = grammarDeque[currentCell.item.gid].right[currentCell.item.index];

        // 创建下一个状态（临时的）
        dfaStateLR1& nextState = tempSave[nextSymbol];
        dfaCellLR1 nextStateCell = dfaCellLR1();
        nextStateCell.item.gid = currentCell.item.gid;
        nextStateCell.item.index = currentCell.item.index + 1;
        nextStateCell.item.lookahead = currentCell.item.lookahead;

        // 看看里面的项目是否有重复的，如果重复拿之前的就好，不重复生成
        int nextStateCellid = isNewCellLR1(nextStateCell.item.gid, nextStateCell.item.index, nextStateCell.item.lookahead);
        if (nextStateCellid == -1)
        {
            qDebug() << "generateLR1State: 25";
            nextStateCell.cellid = ccntLR1++;
            dfaCellVectorLR1.push_back(nextStateCell);
        }
        else nextStateCell.cellid = nextStateCellid;
        qDebug() << "generateLR1State: 26";
        nextState.cellV.push_back(nextStateCell.cellid);
        nextState.originV.push_back(nextStateCell.cellid);

        // 收集一下，方便后面画表
        if (isNonTerminal(nextSymbol))
        {
            VNLR1.insert(nextSymbol);
        }
        else if (isTerminal(nextSymbol))
        {
            VTLR1.insert(nextSymbol);
        }
    }

    // 校验状态是否有重复的
    for (auto& t : tempSave)
    {
        qDebug() << "generateLR1State: 27";
        dfaStateLR1 nextState = dfaStateLR1();
        int newStateId = isNewStateLR1(t.second.originV);
        // 不重复就新开一个状态
        if (newStateId == -1)
        {
            qDebug() << "generateLR1State: 28";
            nextState.sid = scntLR1++;
            nextState.cellV = t.second.cellV;
            nextState.originV = t.second.originV;
            dfaStateVectorLR1.push_back(nextState);
        }
        // 如果重复那么nextState的sid是已经存在的状态的id
        else nextState.sid = newStateId;
        // 存入现在这个状态的nextStateVector
        // 无论是否重复，当前状态都需要指向 nextState
        // 也就是说重复则指向已有的状态，不重复就指向新建的状态
        qDebug() << "generateLR1State: 29";
        nextStateUnitLR1 n = nextStateUnitLR1();
        n.sid = nextState.sid;
        n.c = t.first;
        dfaStateVectorLR1[stateId].nextStateVector.push_back(n);
    }

    qDebug() << "generateLR1State: 30";
    // 对每个下一个状态进行递归
    int nsize = dfaStateVectorLR1[stateId].nextStateVector.size();
    for (int i = 0; i < nsize; i++)
    {
        auto& nextunit = dfaStateVectorLR1[stateId].nextStateVector[i];
        generateLR1State(nextunit.sid);
    }
}

// 生成LR(1)入口
void getLR1()
{
    visitedStatesLR1.clear();

    // 首先生成第一个状态
    createFirstStateLR1();

    // 递归生成其他状态
    generateLR1State(0);
}

// 拼接字符串，获取LR(1)状态内的文法（包含超前查看符号）
string getLR1StateGrammar(const dfaStateLR1& d)
{
    string result = "";
    for (auto cellId : d.cellV)
    {
        qDebug() << "getLR1StateGrammar 1";
        const dfaCellLR1& dfaCell = dfaCellVectorLR1[cellId];
        // 拿到LR(1)项目信息
        qDebug() << "getLR1StateGrammar 2";
        const LR1Item& item = dfaCell.item;
        // 拿到文法
        qDebug() << "getLR1StateGrammar 3";
        int gid = item.gid;
        grammarUnit g = grammarDeque[gid];
        qDebug() << "getLR1StateGrammar 4";
        // 拿到位置
        int i = item.index;
        // 拼接结果
        qDebug() << "getLR1StateGrammar 5";
        string r = "[";
        qDebug() << "getLR1StateGrammar 6";
        r += g.left == "^"? "E\'->" : g.left + "->";
        qDebug() << "getLR1StateGrammar 7";
        vector<string> gRight = g.right;
        qDebug() << "getLR1StateGrammar 8";
        qDebug() << "item.index: " << i;
        qDebug() << "gRight[0]: " << QString::fromStdString(gRight[0]);
//        qDebug() << "gRight[1]: " << QString::fromStdString(gRight[1]);
        i == gRight.size()? gRight[i - 1].insert(gRight[i - 1].size(), 1, '.') : gRight[i].insert(0, 1, '.');
        qDebug() << "getLR1StateGrammar 9";
        string right = vectorToString(g.right) == "@" ? "" : vectorToString(gRight);
        qDebug() << "getLR1StateGrammar 10";
//        right.insert(i, 1, '.');
        r += right;
        qDebug() << "getLR1StateGrammar 11";
        // 添加超前查看符号
        r += ", " + item.lookahead;
        qDebug() << "getLR1StateGrammar 12";
        result += r + "] ";
    }
    qDebug() << "result: " << QString::fromStdString(result);
    qDebug() << "getLR1StateGrammar 13";
    return result;
}/**/

/******************** LR1分析 ***************************/
// LR(1)分析表的单元结构，包含动作（action）和去向（goto）
struct LR1TableUnit
{
    map<string, string> action;  // 动作，以终结符为键，对应动作字符串（如移进 "s<状态编号>"、规约 "r<文法编号>"、接受 "ACCEPT" 等）为值
    map<string, string> goTo;  // 去向，以非终结符为键，对应状态编号为值（对于非终结符的转移情况）
};

// 用于存储LR(1)分析表，以状态编号为索引
vector<LR1TableUnit> LR1Table;

// 根据当前状态和一个符号（终结符或非终结符），获取转移到的下一个状态编号，如果不存在则返回 -1
int getNextStateId(int stateId, string symbol)
{
    for (const nextStateUnitLR1& nextUnit : dfaStateVectorLR1[stateId].nextStateVector) {
        if (nextUnit.c == symbol) {
            return nextUnit.sid;
        }
    }
    return -1;
}

// 判断当前状态针对某个终结符是移进还是规约等动作，返回相应动作字符串
string getActionForTerminal(int stateId, string terminal)
{
    for (int itemId : dfaStateVectorLR1[stateId].cellV) {
        dfaCellLR1& itemCell = dfaCellVectorLR1[itemId];
        // 如果点未到文法规则末尾，且点后符号是遇到的终结符，那么移进
        if (itemCell.item.index < grammarDeque[itemCell.item.gid].right.size() &&
            grammarDeque[itemCell.item.gid].right[itemCell.item.index] == terminal) {
            // 是移进动作，返回移进对应的状态编号
            return "s" + to_string(getNextStateId(stateId, terminal));
        }
        // 如果点已到文法规则末尾，且遇到的终结符在项目的向前搜索符中，那么规约
        else if (itemCell.item.index == grammarDeque[itemCell.item.gid].right.size() &&
                 itemCell.item.lookahead.find(terminal)!= string::npos) {
            // 是规约动作，返回对应的文法编号
            return "r" + to_string(itemCell.item.gid);
        }
    }
    return "";  // 如果既不是移进也不是规约，返回空字符串
}
// 生成LR(1)分析表的主函数
void generateLR1AnalysisTable()
{
    set<string> VTLR1_copy;
    VTLR1_copy = VTLR1;
    VTLR1_copy.insert({"$"});
    LR1Table.resize(dfaStateVectorLR1.size());  // 根据LR(1) DFA状态数量初始化分析表大小

    // 遍历每个LR(1)状态来填充分析表
    for (size_t stateId = 0; stateId < dfaStateVectorLR1.size(); ++stateId) {
        LR1TableUnit& tableUnit = LR1Table[stateId];

        // 处理终结符对应的动作
        for (string terminal : VTLR1_copy) {
            qDebug() << "terminal: " << QString::fromStdString(terminal);
            string action = getActionForTerminal(stateId, terminal);
            if (!action.empty()) {
                tableUnit.action[terminal] = action;
            }
        }

        // 处理非终结符对应的去向（goto）
        for (string nonTerminal : VNLR1) {
            int nextStateId = getNextStateId(stateId, nonTerminal);
            if (nextStateId!= -1) {
                tableUnit.goTo[nonTerminal] = to_string(nextStateId);
            }
        }

        // 检查是否为接受状态
        // 如果当前状态的规约项目文法左部是文法扩展后的开始符号、项目的向前搜索符号是'$'（在本系统中，能出现这种情况的都是状态第一条项目），
        // 则此项目为接受状态
        dfaStateLR1 currentState = dfaStateVectorLR1[stateId];
        int fisrtCellId = currentState.cellV[0];
        if (currentState.isEnd &&
//            currentState.cellV.size() == 1 &&
            grammarDeque[dfaCellVectorLR1[fisrtCellId].item.gid].left == trueStartSymbol &&
            dfaCellVectorLR1[fisrtCellId].item.index == grammarDeque[dfaCellVectorLR1[fisrtCellId].item.gid].right.size() &&
            dfaCellVectorLR1[fisrtCellId].item.lookahead == "$") {
            tableUnit.action["$"] = "ACCEPT";
            qDebug() << "ACCEPT?";
        }
    }
//    for(int i = 0; i < dfaStateVectorLR1.size(); i++){
//        for (char terminal : VTLR1) {
//            qDebug() << "state" << i << " action: " << QString::fromStdString(LR1Table[i].action[terminal]);
//        }
//    }
}/**/

/******************** SLR1分析 ***************************/
/**/
// 检查移进-规约冲突
bool SLR1Fun1()
{
    for (const dfaStateLR1& state : dfaStateVectorLR1)
    {
        set<string> a; // 规约项目的左边集合
        set<string> rVT; // 终结符
        if (!state.isEnd) continue; // 不是规约状态不考虑
        for (int cellid : state.cellV) // 遍历规约状态内的项目
        {
            const dfaCellLR1& cell = dfaCellVectorLR1[cellid]; // 当前cell
            const grammarUnit gm = grammarDeque[cell.item.gid]; // 获取文法
            // 判断是不是规约项目
            if (cell.item.index == gm.right.size() || vectorToString(gm.right) == "@")
            {
                a.insert(gm.left);
            }
            else // 如果当前项目不是规约项目，则判断点后符号是不是终结符（涉及移进操作）
            {
                if (isTerminal(gm.right[cell.item.index]))
                {
                    rVT.insert(gm.right[cell.item.index]);
                }
            }
        }
        for (string c : a)
        {
            for (string v : rVT)
            {
                // 在面对输入符号v时，既可以按照非终结符c对应的产生式进行规约，又可以将v移进，这就产生了移进-规约冲突
                if (followSets[c].s.find(v) != followSets[c].s.end())
                {
                    return true;
                }

            }
        }
    }
    return false;
}

bool SLR1Fun2()
{
    // 检查规约-规约冲突
    for (const auto& state : dfaStateVectorLR1)
    {
        set<string> a; // 规约项目的左边集合
        if (!state.isEnd) continue; // 不是规约状态不考虑
        for (int cellid : state.cellV) // 遍历规约状态内的项目
        {
            const dfaCellLR1& cell = dfaCellVectorLR1[cellid]; // 当前cell
            const grammarUnit gm = grammarDeque[cell.item.gid]; // 获取文法
            if (cell.item.index == gm.right.size() || vectorToString(gm.right) == "@")  // 判断是不是规约项目
            {
                a.insert(gm.left);
            }
        }
        for (string c1 : a)
        {
            for (string c2 : a)
            {
                if (c1 != c2)
                {   // 判断followSets[c1]和followSets[c2]是否有交集
                    set<string> followSetC1 = followSets[c1].s;
                    set<string> followSetC2 = followSets[c2].s;
                    set<string> intersection;
                    // 利用STL算法求交集
                    set_intersection(
                        followSetC1.begin(), followSetC1.end(),
                        followSetC2.begin(), followSetC2.end(),
                        inserter(intersection, intersection.begin())
                    );
                    // 如果交集非空，说明存在规约-规约冲突
                    if (!intersection.empty())   return true;
                }
            }
        }
    }
    return false;
}


// SLR1分析
int SLR1Analyse()
{
    // 开始符号添加follow集合
    followSets["E'"].s.insert("$");

    bool flag1 = SLR1Fun1();
    bool flag2 = SLR1Fun2();
    if (flag1 && flag2)
    {
        return 3;
    }
    else if (flag1)
    {
        return 1;
    }
    else if (flag2)
    {
        return 2;
    }
    // 没有冲突，是SLR(1)文法
    return 0;
}


/*清空全局变量*/
void reset()
{
    grammarMap2.clear();
    firstSets.clear();
    nonTerminals.clear();
    terminals.clear();
    followSets.clear();
    grammarDeque.clear();
    dfaStateVector.clear();
    dfaCellVector.clear();
    dfaStateVectorLR1.clear();
    dfaCellVectorLR1.clear();
    VT.clear();
    VN.clear();
    VTLR1.clear();
    VNLR1.clear();
    LR1Table.clear();
    scnt = 0;
    ccnt = 0;
    ccntLR1 = 0;
    scntLR1 = 0;
}

/******************** UI界面 ***************************/
// 查看输入规则
void Widget::on_pushButton_7_clicked()
{
    QString message = "本系统支持以单个大写字母作为非终结符，也支持以单词作为非终结符。在本系统中终结符号存在的形式是除大写字母外，键盘上单个可键入的符号（其中，“@”代表空字符串，“$”代表结束符）。在输入文法规则的过程中，请务必保证文法规则的每个“元素”（非终结符、箭头和终结符）之间都有空格相间。";

    QMessageBox::information(this, "输入规则", message);
}

// 打开文法规则
void Widget::on_pushButton_3_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), QDir::homePath(), tr("文本文件 (*.txt);;所有文件 (*.*)"));

    if (!filePath.isEmpty())
    {
        ifstream inputFile;
        QTextCodec* code = QTextCodec::codecForName("GB2312");

        string selectedFile = code->fromUnicode(filePath.toStdString().c_str()).data();
        inputFile.open(selectedFile.c_str(), ios::in);


        //        cout<<filePath.toStdString();
        //        ifstream inputFile(filePath.toStdString());
        if (!inputFile) {
            QMessageBox::critical(this, "错误信息", "导入错误！无法打开文件，请检查路径和文件是否被占用！");
            cerr << "Error opening file." << endl;
        }
        // 读取文件内容并显示在 plainTextEdit_2
        stringstream buffer;
        buffer << inputFile.rdbuf();
        QString fileContents = QString::fromStdString(buffer.str());
        ui->plainTextEdit_2->setPlainText(fileContents);
    }
}

// 保存文法规则
void Widget::on_pushButton_4_clicked()
{
    QString saveFilePath = QFileDialog::getSaveFileName(this, tr("保存文法文件"), QDir::homePath(), tr("文本文件 (*.txt)"));
    if (!saveFilePath.isEmpty() && !ui->plainTextEdit_2->toPlainText().isEmpty()) {
        QFile outputFile(saveFilePath);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&outputFile);
            stream << ui->plainTextEdit_2->toPlainText();
            outputFile.close();
            QMessageBox::about(this, "提示", "导出成功！");
        }
    }
    else if (ui->plainTextEdit_2->toPlainText().isEmpty())
    {
        QMessageBox::warning(this, tr("提示"), tr("输入框为空，请重试！"));
    }
}

// 求解first集合按钮
void Widget::on_pushButton_5_clicked()
{

    reset();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getFirstSets();

    QTableWidget* tableWidget = ui->tableWidget_3;

    // 清空表格内容
    tableWidget->clearContents();

    // 设置表格的列数
    tableWidget->setColumnCount(2);

    // 设置表头
    QStringList headerLabels;
    headerLabels << "非终结符" << "First集合";
    tableWidget->setHorizontalHeaderLabels(headerLabels);

    // 设置行数
    tableWidget->setRowCount(firstSets.size());

    // 遍历非终结符的First集合，将其展示在表格中
    int row = 0;
    for (const auto& entry : firstSets)
    {
        string nonTerminal = entry.first;
        const set<string>& firstSet = entry.second.s;

        // 在表格中设置非终结符
        QTableWidgetItem* nonTerminalItem = new QTableWidgetItem(QString::fromStdString(nonTerminal));
        tableWidget->setItem(row, 0, nonTerminalItem);

        // 在表格中设置First集合，将set<char>转换为逗号分隔的字符串
        QString firstSetString;
        for (string symbol : firstSet)
        {
            firstSetString += QString::fromStdString(symbol) + ",";
        }
        if (entry.second.isEpsilon)
        {
            firstSetString += QString('@') + ",";
        }
        // 去掉最后一个逗号
        if (!firstSetString.isEmpty())
        {
            firstSetString.chop(1);
        }

        QTableWidgetItem* firstSetItem = new QTableWidgetItem(firstSetString);
        tableWidget->setItem(row, 1, firstSetItem);

        // 增加行数
        ++row;
    }/**/
}

// 求解follow集合按钮
void Widget::on_pushButton_6_clicked()
{

    reset();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getFirstSets();
    getFollowSets();


    // 清空TableWidget
    ui->tableWidget_4->clear();

    // 设置表格的行数和列数
    int rowCount = followSets.size();
    int columnCount = 2; // 两列
    ui->tableWidget_4->setRowCount(rowCount);
    ui->tableWidget_4->setColumnCount(columnCount);

    // 设置表头
    QStringList headers;
    headers << "非终结符" << "Follow集合";
    ui->tableWidget_4->setHorizontalHeaderLabels(headers);

    // 遍历followSets，将数据填充到TableWidget中
    int row = 0;
    for (const auto& entry : followSets) {
        // 获取非终结符和对应的followUnit
        string nonTerminal = entry.first;
        const followUnit& followSet = entry.second;

        // 在第一列设置非终结符
        QTableWidgetItem* nonTerminalItem = new QTableWidgetItem(QString::fromStdString(nonTerminal));
        ui->tableWidget_4->setItem(row, 0, nonTerminalItem);

        // 在第二列设置followUnit，使用逗号拼接
        QString followSetStr = "";
        for (string c : followSet.s) {
            followSetStr += QString::fromStdString(c);
            followSetStr += ",";
        }
        followSetStr.chop(1); // 移除最后一个逗号
        QTableWidgetItem* followSetItem = new QTableWidgetItem(followSetStr);
        ui->tableWidget_4->setItem(row, 1, followSetItem);

        // 移动到下一行
        ++row;
    }/**/
}

// 生成LR(1)DFA图
void Widget::on_pushButton_clicked()
{
    qDebug() << "generate LR(1)DFA button";
    reset();
    getFirstSets();
    getFollowSets();
    ui->tableWidget->clear();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getLR1();

    int numRows = dfaStateVectorLR1.size();
    int numCols = 2 + VTLR1.size() + VNLR1.size();

    ui->tableWidget->setRowCount(numRows);
    ui->tableWidget->setColumnCount(numCols);

    // Set the table headers
    QStringList headers;
    headers << "状态" << "状态内文法";
    map<string, int> c2int;
    int cnt = 0;
    for (string vt : VTLR1) {
        headers << QString::fromStdString(vt);
        c2int[vt] = cnt++;
    }
    for (string vn : VNLR1) {
        headers << QString::fromStdString(vn);
        c2int[vn] = cnt++;
    }
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // Populate the table with data
    for (int i = 0; i < numRows; ++i)
    {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(dfaStateVectorLR1[i].sid)));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(getLR1StateGrammar(dfaStateVectorLR1[i]))));
        qDebug() << "after getLR1StateGrammar";

        // Display nextStateVector
        for (int j = 0; j < dfaStateVectorLR1[i].nextStateVector.size(); ++j)
        {
            qDebug() << "setItem ing";
            ui->tableWidget->setItem(i, 2 + c2int[dfaStateVectorLR1[i].nextStateVector[j].c], new QTableWidgetItem(QString::number(dfaStateVectorLR1[i].nextStateVector[j].sid)));
        }
        qDebug() << "after setItem";
    }/**/
}

// 分析LR(1)文法
void Widget::on_pushButton_2_clicked()
{
    reset();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getLR1();
    generateLR1AnalysisTable();

    ui->tableWidget_2->clear();  // 先清空表格，避免之前的数据残留

    // 将终结符集合和非终结符集合合并，用于表头显示，并插入 '$' 符号（假设它是终结符且需要展示在表头）
    unordered_set<string> allSymbols;
    allSymbols.insert(VNLR1.begin(), VNLR1.end());
    allSymbols.insert(VTLR1.begin(), VTLR1.end());
    allSymbols.insert("$");

    int numRows = LR1Table.size();
    int numCols = 1 + allSymbols.size();

    ui->tableWidget_2->setRowCount(numRows);
    ui->tableWidget_2->setColumnCount(numCols);

    // 设置表头
    QStringList headers;
    headers << "状态";
    map<string, int> symbolToIndex;
    int index = 0;
    for (string symbol : allSymbols) {
        headers << QString::fromStdString(symbol);
        symbolToIndex[symbol] = index++;
    }
    ui->tableWidget_2->setHorizontalHeaderLabels(headers);

    // 填充表格数据
    for (size_t row = 0; row < LR1Table.size(); ++row) {
        // 设置状态编号这一列的数据
        ui->tableWidget_2->setItem(row, 0, new QTableWidgetItem(QString::number(row)));

        const LR1TableUnit& tableUnit = LR1Table[row];

        // 填充动作（action）列的数据
        for (const auto& actionPair : tableUnit.action) {
            string vt = actionPair.first;
            QString actionStr = QString::fromStdString(actionPair.second);
            ui->tableWidget_2->setItem(row, 1 + symbolToIndex[vt], new QTableWidgetItem(actionStr));
        }

        // 填充去向（goTo）列的数据
        for (const auto& goToPair : tableUnit.goTo) {
            string vn = goToPair.first;
            QString goToStr = QString::fromStdString(goToPair.second);
            ui->tableWidget_2->setItem(row, 1 + symbolToIndex[vn], new QTableWidgetItem(goToStr));
        }
    }/**/
}

// 判断是否为SLR(1)文法
void Widget::on_pushButton_8_clicked()
{

    reset();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getFirstSets();
    getFollowSets();
    getLR1();
    int result = SLR1Analyse();
    switch (result)
    {
    case 1:
        ui->plainTextEdit->setPlainText("不符合SLR(1)文法，出现归约-移进冲突");
        break;
    case 2:
        ui->plainTextEdit->setPlainText("不符合SLR(1)文法，出现归约-归约冲突");
        break;
    case 3:
        ui->plainTextEdit->setPlainText("不符合SLR(1)文法，出现归约-移进冲突和归约-归约冲突");
        break;
    case 0:
        ui->plainTextEdit->setPlainText("符合SLR(1)文法");
    }
}

// 生成LR(0)DFA图
void Widget::on_pushButton_9_clicked()
{
    qDebug() << "generate LR(0)DFA button";
    reset();
    ui->tableWidget_5->clear();
    QString grammar_q = ui->plainTextEdit_2->toPlainText();
    grammarStr = grammar_q.toStdString();
    handleGrammar();
    getFirstSets();
    getFollowSets();
    getLR0();

    int numRows = dfaStateVector.size();
    int numCols = 2 + VT.size() + VN.size();

    ui->tableWidget_5->setRowCount(numRows);
    ui->tableWidget_5->setColumnCount(numCols);

    // Set the table headers
    QStringList headers;
    headers << "状态" << "状态内文法";
    map<string, int> c2int;
    int cnt = 0;
    for (string vt : VT) {
        headers << QString::fromStdString(vt);
        c2int[vt] = cnt++;
    }
    for (string vn : VN) {
        headers << QString::fromStdString(vn);
        c2int[vn] = cnt++;
    }
    ui->tableWidget_5->setHorizontalHeaderLabels(headers);

    // Populate the table with data
    for (int i = 0; i < numRows; ++i)
    {
        ui->tableWidget_5->setItem(i, 0, new QTableWidgetItem(QString::number(dfaStateVector[i].sid)));
        ui->tableWidget_5->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(getStateGrammar(dfaStateVector[i]))));
        qDebug() << "after getStateGrammar";

        // Display nextStateVector
        for (int j = 0; j < dfaStateVector[i].nextStateVector.size(); ++j)
        {
            qDebug() << "setItem ing";
            ui->tableWidget_5->setItem(i, 2 + c2int[dfaStateVector[i].nextStateVector[j].c], new QTableWidgetItem(QString::number(dfaStateVector[i].nextStateVector[j].sid)));
        }
        qDebug() << "after setItem";
    }/**/
}

