# LR1Analyzer

## 基本信息
华南师范大学 2022级 编译原理 实验四 LR(1)分析生成器

## 运行环境
QT Creator 5.0.2

QT 5.12.12 MinGW 64-bit

## 文档说明
code 源码文件夹

docs 文档文件夹

test 测试文件夹

## 项目缺陷
暂未发现

## 实验要求
一、实验内容：

  (1)要提供一个文法输入编辑界面，让用户输入文法规则（可保存、打开存有文法规则的文件）
  
  (2)求出文法各非终结符号的first集合与follow集合，并提供窗口以便用户可以查看这些集合结果。【可以采用表格的形式呈现】
  
  (3)需要提供窗口以便用户可以查看文法对应的LR(0)DFA图。（可以用画图的方式呈现，也可用表格方式呈现该图点与边数据）
  
  (4)判断该文法是否为SLR(1)文法。（应提供窗口呈现判断的结果，如果不是SLR（1）文法，需要在窗口中显示其原因）
  
  (5)需要提供窗口以便用户可以查看文法对应的LR(1)DFA图。（可以用画图的方式呈现，也可用表格方式呈现该图点与边数据）
  
  (6)需要提供窗口以便用户可以查看文法对应的LR(1)分析表。【LR(1)分析表采用表格的形式呈现】
  
  (7)应该书写完善的软件文档
  
  (8)应用程序应为Windows界面


三、完成时间：3周时间(第13周-第16周)

四、实验实现的编程语言：C++程序设计语言

五、测试样例

注意事项：输入的文法均为2型文法（上下文无关文法）。文法规则为了处理上的简单，输入时均默认输入的第一个非终结符就是文法的开始符号，用@表示空串。因此下面的文法均为正确的输入。

例1：

    E -> E + T 
    
    T -> a 

由于第一个非终结符为E，因此E是文法的开始符号。

例2：
```
exp -> exp addop term | term

addop -> + | -

term -> term mulop factor | factor

mulop -> * | /

factor -> ( exp ) | n
```
由于第一个非终结符为exp，因此exp是文法的开始符号。
