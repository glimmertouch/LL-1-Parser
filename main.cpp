#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <deque>

#define MaxNonTerminal 10 // 非终结符个数
#define MaxRules 5        // 单个标识符最多规则数
using namespace std;

extern FILE *yyin;
extern int yylex(void);
extern int SizeInput;

char terminal[] = {'i', '+', '*', '(', ')'};
string startCode;
typedef struct single
{
    int RulesNum = 0;            // 该非终结符有多少条语法
    deque<char> right[MaxRules]; // 具体的语法
} Grammars;

map<string, Grammars> G;                                   // <非终结符，语法集>
map<pair<string, deque<char>>, set<char>> CollectionFirst; // <<非终结符，单条语法>，first集>
map<string, set<char>> CollectionFollow;                   // <非终结符，follow集>
map<pair<string, char>, deque<char>> AnalChart;            // <<非终结符，终结符>, 该用的语法>
char Origin[100];
extern char Input[100];  // 输入串
deque<char> Process;     // 分析过程
string Finished;         // 完成串
stack<char> LeftStack;   // 余留栈
stack<string> AnalStack; // 分析栈

void init(char *grammar, char *input)
{
    ifstream ifs;
    ifs.open(grammar, ios::in);
    string buf;
    bool first = false;
    while (getline(ifs, buf))
    {
        // cout << buf << endl;
        bool left = true;
        string sign = "";
        for (unsigned int i = 0; i < buf.size(); i++)
        {
            if (left == true && buf[i] == '-' && buf[i + 1] == '>')
            {
                left = false;
                i += 2;
                G[sign].RulesNum = 1;
            }
            if (left == 1)
                sign += buf[i];
            else
            {
                if (buf[i] == '|')
                {
                    G[sign].RulesNum++;
                    continue;
                }
                int num = G[sign].RulesNum;
                G[sign].right[num].push_back(buf[i]);
            }
            if (!first)
            {
                first = true;
                startCode = sign;
            }
        }
    }
    ifs.close();
    /*
    for (auto iter = G.begin(); iter != G.end(); iter++)
    {
        Grammars st = iter->second;
        for (int i = 1; i <= st.RulesNum; i++)
        {
            cout << iter->first << " : ";
            while (!st.right[i].empty())
            {
                cout << st.right[i].front() << " ";
                st.right[i].pop_front();
            }
            cout << endl;
        }
        cout << endl;
    }
    */
    if (!(yyin = fopen(input, "r")))
        perror(input);
    fgets(Origin, 100, yyin);
    rewind(yyin);
    cout << "原符号串：" << Origin << endl;
    while (yylex() != 0)
        ;
    cout << "词法分析结果：" << Input << endl
         << endl;
    for (int i = SizeInput - 1; i >= 0; i--)
        LeftStack.push(Input[i]);
    fclose(yyin);
}

bool IsTerminator(char c)
{
    for (int i = 0; i < 5; i++)
        if (c == terminal[i])
            return true;
    return false;
}

bool First(pair<string, deque<char>> rules) // 返回该符号是否产生空字符串
{
    bool Ignore = false;
    set<char> first;                 // 存储非终结符key的first集
    deque<char> rule = rules.second; // 获取第i条语法
    char FrontBit = rule.front();
    if (IsTerminator(FrontBit)) // 如果该语法的第一个字符是终结符，则直接将该终结符加入first集
    {
        first.insert(FrontBit);
    }
    else if (FrontBit == 'e')
    {
        first.insert('e');
        Ignore = true;
    }
    else // 否则，递归调用First函数求该非终结符的first集，并将其并入first集
    {
        string substr = {FrontBit};
        bool subIgnore = false;
        for (int i = 1; i <= G[substr].RulesNum; i++)
        {
            deque<char> subrule = G[substr].right[i];
            auto subrules = make_pair(substr, subrule);
            if (First(subrules))
            {
                Ignore = true;    // 整个符号都是可忽略的，在求follow集中有用
                subIgnore = true; // 需要继续寻找下一个符号
            }
            else
            {
                set<char> subFirst = CollectionFirst[subrules];
                first.insert(subFirst.begin(), subFirst.end());
            }
        }
        while (subIgnore) // 当前非终结符含含空字符串，需要读去下一个字符
        {
            rule.pop_front();
            substr = 'e'; // 表示该First集并不参与后续的计算
            deque<char> subrule = rule;
            auto subrules = make_pair(substr, subrule);
            subIgnore = First(subrules);
            set<char> subFirst = CollectionFirst[subrules];
            first.insert(subFirst.begin(), subFirst.end());
        }
    }
    CollectionFirst[rules] = first; // 将非终结符key的first集存储到CollectionFirst中
    return Ignore;
}

void follow()
{

    // 将FOLLOW(S)赋值为{#}，其中S是文法的开始符号
    CollectionFollow[startCode].insert('#');
    // 重复步骤2和3，直到没有新的符号加入任何一个follow集为止
    bool flag = true;
    while (flag)
    {
        flag = false;
        for (auto it : G)
        {
            string A = it.first;
            // 步骤二：对于文法中的每一条产生式A -> alpha B beta，如果B是非终结符号，则将FIRST(beta)中的所有符号加入FOLLOW(B)
            for (int i = 1; i <= it.second.RulesNum; i++)
            {
                deque<char> rule = it.second.right[i];
                int size = rule.size();
                for (int j = 0; j < size - 1; j++) // 后面有beta
                {
                    char B = rule[j];
                    if (!IsTerminator(B))
                    {
                        deque<char> beta(rule.begin() + j + 1, rule.end());
                        pair<string, deque<char>> subrules = make_pair("e", beta);
                        bool f = First(subrules);
                        set<char> first = CollectionFirst[subrules];
                        int oriSize = CollectionFollow[{B}].size();
                        CollectionFollow[{B}].insert(first.begin(), first.end());
                        if (CollectionFollow[{B}].size() > oriSize)
                            flag = true;
                        if (f) // beta集可能实施空字符串，此时B满足为最后一个字符，因此按步骤三添加follow(A)
                        {
                            set<char> follow = CollectionFollow[A];
                            int oriSize = CollectionFollow[{B}].size();
                            CollectionFollow[{B}].insert(follow.begin(), follow.end());
                            if (CollectionFollow[{B}].size() > oriSize)
                                flag = true;
                        }
                    }
                }
                // 步骤三：对于文法中的每一条产生式A -> alpha B，如果B是非终结符号，则将FOLLOW(A)中的所有符号加入FOLLOW(B)
                char B = rule[size - 1];
                if (!IsTerminator(B) && B != 'e')
                {
                    set<char> follow = CollectionFollow[A];
                    int oriSize = CollectionFollow[{B}].size();
                    CollectionFollow[{B}].insert(follow.begin(), follow.end());
                    if (CollectionFollow[{B}].size() > oriSize)
                        flag = true;
                }
            }
        }
    }
    // 所有空字符删除
    for (auto it : CollectionFollow)
    {
        string str = it.first;
        set<char> Follow = it.second;
        if (Follow.count('e'))
            Follow.erase('e');
        cout << str << ":";
        for (char c : Follow)
            cout << c;
        cout << endl;
    }
}

void Analyse()
{
    for (auto it : G)
    {
        string A = it.first;
        for (int i = 1; i <= it.second.RulesNum; i++)
        {
            auto rule = it.second.right[i];
            auto rules = make_pair(A, rule);
            auto first = CollectionFirst[rules];
            for (auto it2 : first)
            {
                // 对first(α)中每一终结符a，置Ｍ[A, a]＝"A→α"
                if (it2 != 'e')
                    AnalChart[make_pair(A, it2)] = rule;
                else
                {
                    // 若 e ∈ first(α)，则对属于follow(A)中的每一符号b (b为终结符或＃)，置Ｍ[Ａ，b]＝“Ａ→α”
                    for (auto b : CollectionFollow[A])
                        AnalChart[make_pair(A, b)] = rule;
                }
            }
        }
    }

    for (auto it : AnalChart)
    {
        cout << "<" << it.first.first << ", " << it.first.second << ">"
             << ":";
        for (auto it2 : it.second)
            cout << it2;
        cout << endl;
    }
}

bool Caculate()
{
    AnalStack.push(startCode);
    cout << startCode;
    while (AnalStack.top() != "#" || LeftStack.top() != '#')
    {
        string B = {LeftStack.top()};
        while (AnalStack.top() == B)
        {
            AnalStack.pop();
            LeftStack.pop();
            Finished += B;
            B = {LeftStack.top()};
        }
        while (IsTerminator(Process.front()))
            Process.pop_front();
        char b = LeftStack.top();
        string X = AnalStack.top();
        auto rule = AnalChart[make_pair(X, b)];
        if (!Process.empty())
            Process.pop_front();
        if (rule.empty())
        {
            cout << endl;
            string opt = "";
            for (char it : terminal)
            {
                if (!AnalChart[make_pair(X, it)].empty())
                    opt += it;
            }
            if (!AnalChart[make_pair(X, '#')].empty())
                opt += '#';
            cout << "Expect: ";
            for (int i = 0; i < opt.size(); i++)
                cout << "'"<<opt[i] <<"'"<< " ";
            cout << ", buf found '" << b << "." << endl;

            cout << "LeftStack: ";
            while (!LeftStack.empty())
            {
                cout << LeftStack.top() << " ";
                LeftStack.pop();
            }
            cout << endl;
            cout << "AnalStack: ";
            while (!AnalStack.empty())
            {
                cout << AnalStack.top() << " ";
                AnalStack.pop();
            }
            cout << endl;

            return false;
        }
        AnalStack.pop();
        while (!rule.empty())
        {
            char c = rule.back();
            if (c != 'e')
            {
                Process.push_front(c);
                AnalStack.push({c});
            }
            rule.pop_back();
        }

        cout << " -> " << Finished;
        for (auto it : Process)
            cout << it;
    }
    return true;
}

int main(int argc, char **argv)
{
    AnalStack.push({'#'});
    LeftStack.push('#');
    init(argv[1], argv[2]);
    cout << "first集：" << endl;
    for (auto iter = G.begin(); iter != G.end(); iter++)
    {
        string str = iter->first;
        for (int i = 1; i <= G[str].RulesNum; i++)
        {
            deque<char> rule = G[str].right[i];
            auto rules = make_pair(str, rule);
            bool f = First(rules);
            cout << str << "->";
            for (auto iter2 = rule.begin(); iter2 != rule.end(); iter2++)
                cout << *iter2;
            cout << ":";
            for (auto iter2 = CollectionFirst[rules].begin(); iter2 != CollectionFirst[rules].end(); iter2++)
                cout << *iter2;
            cout << endl;
        }
    }
    cout << endl
         << "follow集：" << endl;
    follow();
    cout << endl
         << "分析表：" << endl;
    Analyse();
    cout << endl;
    if (Caculate())
        cout << endl
             << Origin << " 符合该文法" << endl;
    else
        cout << endl
             << Origin << " 不符合该文法" << endl;
    cout << endl;
    return 0;
}
