# 拓展阅读

这部分内容是与语法分析有关的拓展阅读，供学有余力的同学扩展视野，不作为实验或课程要求。

## 不同的生成器

Bison 并非唯一的解析器生成器，甚至不是最好用的。我们推荐同学们进一步了解其他生成器，以备不时之需。

可以从以下几个角度来研究：

1. 支持怎样的文法？
2. 目标语言是什么？
3. 是如何实现的？
4. 支持怎样的 lexer？
5. 效率如何？

等等。其实 Wikipedia 上就有一个[对比页面](https://en.wikipedia.org/wiki/Comparison_of_parser_generators)。

## 手写解析器

尽管解析器生成器非常好用：只要把文法倒进去，它就可以自动生成大量代码。但是有以下几个弊端：

1. 生成的是解析树而不是抽象语法树。这需要之后较多的人工工作来进行转换。
2. 报错和错误恢复可能比较复杂。
3. 如果生成器缺乏必需功能或者 bug，会造成很大的困扰。

在真实世界中，人们常常为了避免上述弊端而手写解析器。实践中，为了便于报错等，常常选择**自顶向下** (top-down) 解析器，或者是**递归下降** (recursive descent)，或者是 LL。

过去人们常常认为 top-down 解析无法处理左递归。实际上，存在一种名为 Pratt parser 的技术可以解决这个问题。它是递归下降的一个简单变体，很容易理解，但又相当强大，非常适合处理表达式（递归、运算符有结合性）。这里给两个参考文章，供有兴趣的同学阅读。

1. [Simple Top-Down Parsing in Python](http://effbot.org/zone/simple-top-down-parsing.htm) (Python)
2. [Simple but Powerful Pratt Parsing](https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html) (Rust)
3. [Pratt Parsing Index](https://www.oilshell.org/blog/2017/03/31.html) (一篇调查文章)

总之，top-down 解析器是实践中最常用的，毕竟非常好写。为此，我们推荐同学们做以下练习：

1. 在自己喜欢的语言中，用递归下降的方法编写 JSON 解析器。JSON 是目前最常用的互联网数据交换格式，它的文法可在其官网 [json.org](http://json.org) 上查阅。
2. 在自己喜欢的语言中，用 Pratt 解析的方法编写一个四则运算计算器，尝试提供用户友好的错误报告。

## 解析器组合子

解析器组合子 (parser combinator) 是一种高阶函数，它可以把多个解析器组合成单个解析器。这是什么意思呢？又有什么应用价值呢？

首先需要定义这里所说的“解析器”是什么。在这里，解析器接受一段字符串，并返回解析的输出 **和** 剩下的字符串。所以，这里说的解析器实际上是一个函数。

举一个例子，假设存在词法分析器（它实际上也是一种解析器，但接受的文法是正则文法） `number` 和 `identifier`。

```
number("123abc") ==> (Some(123), "abc")  注意这里返回的是整数 123 而不是字符串
number("abc123") ==> (None, "abc123")    识别失败，因此 number 对应的输出是 None
identifier("abc123") ==> (Some("abc123"), "")  这里返回的是字符串 "abc123"
```

（`Some(x)` 表示解析成功，输出为 `x`；`None` 表示该解析器解析失败。）

假设这个语言是计算器的语言，支持用 `2x` 表示 `2*x`。所以 `factor` 可以是数字后接标识符。假如有一种方法，把 `number` 和 `identifier` 组合起来，岂不是很好？我们引入如下组合子：

1. `seq(p,q)`: 表示将输入按顺序经过 p 和 q，并输出两者的结果；如果其中某一步失败，则整个 `seq(p,q)` 也失败。
2. `or(p,q)`: 表示首先尝试 p，如果成功则返回结果，否则接着尝试 q，否则失败。

那么就可以定义
```
factor = or( 
  seq(number,identifier).map { Expr.Mul(Expr.Const(#1), Expr.Val(#2)) },
  number.map(Expr.Const)
)
```

（上面的 `.map(...)` 用于将字符串或数字等数值转换成抽象语法树节点。可以与文法文件中的 action code 类比。）

根据上面说的，我们可以推测它的行为是：

```
factor("123") = (Some(Expr.Const(123)), "")
factor("2x") = (Some(Expr.Mul(Expr.Const(2), Expr.Val("x"))), "")
```

不难看出函数组合成大函数的过程，就是我们把小解析器组合成大解析器的过程，并且可以很自然地把自己想要的逻辑嵌入进去。更有趣的是，编译器是完全知道每个函数的类型的。

由此可见，解析器组合子是一种编程技巧而不是一种解析技术（解析技术是隐含在组合子的实现里的），使用这种技巧可以让代码模块化程度更高，并且在类型系统较强的语言中可以在编译时就捕获错误。此外，尽管代码是完全手写的，但代码却可以和使用解析器生成器一样干净整洁。感兴趣的同学请务必在自己喜欢的高级语言中尝试一番，或者亲自动手写一套组合子。

## 解析器表达式文法

[Parser Expression Grammar](https://zh.wikipedia.org/wiki/解析表达文法) 总结了一大类递归下降分析器可以解析的语言文法，它的核心就是回溯，特点是不能有二义性。PEG 可以比较直观地转换成相应的分析器代码，因此常常用于各种各样的 Parser Generator、Parser Combinator（其实上一节中的组合子组合下来就可以实现 PEG）。PEG 是目前实践中最常用的一类文法，可能没有之一。而且 PEG 是相当容易理解的。缺陷是编写者需要注意各构造的顺序，同时和 LL 文法一样无法支持左递归。

PEG 最近在工业界的一个新闻是 Python 将自己的解析器改成 PEG 的了。然后基于这个新的解析器，支持了一个模式匹配的构造。

- [PEP 617 -- New PEG parser for CPython](https://www.python.org/dev/peps/pep-0617/#background-on-peg-parsers)
- [PEP 622 -- Structural Pattern Matching](https://www.python.org/dev/peps/pep-0622/)

这里给出一个例子：

```python
match something:
    case 0 | 1 | 2:
        print("Small number")
    case [] | [_]:
        print("A short sequence")
    case str() | bytes():
        print("Something string-like")
    case _:
        print("Something else")
```

为了提供向前兼容性，Python 将 `match` 和 `case` 设为了“软关键字”（soft keyword）。这个设计最早是 Java 10 引入的 `var` 关键字，用以支持自动类型推断。下面以 Java 为例介绍一下软关键字。

- [JEP 286: Local-Variable Type Inference](https://openjdk.java.net/jeps/286)

软关键字，与硬关键字相对，其含义与上下文有关（这并不意味着上下文相关文法）。引入软关键字的目的主要是避免原先合法的代码在新版中出错。

一个硬关键字在词法分析阶段会被直接分析成关键字。例如， `if` 被分析成一个叫做 `IF` 的 token，这和 `[` 被分析成 `LBRACKET` 没有什么区别。而一个像 `var` 这样的软关键字，则会首先分析成某种 `IDENT(var)` 和 `VAR` 的复合状态，这里不妨就叫做 `VAR`，然后在语法分析阶段再进行分析。大致思想如下：

```
IDENT           ::= ... | VAR                                 // 变量名仍然可以叫 var
var-declaration ::= ... | VAR IDENT EQ initializer SEMICOLON  // var 可以开启一个变量定义
```

为什么可以这样做？可以尝试分析一下下面的例子：

```java
var();  // 旧版可以编译，新版语义不变
var = 1;  // 旧版可以编译，新版语义不变
var msg = "hello"; // 旧版报错
var var = 1;  // 旧版报错
```

这样的语法在带回溯的 PEG 中是很容易写出来的。

## 更多的解析技术

课本上介绍的解析技术非常实用，但并不是解析的全部。例如：

1. 可以处理二义文法和左递归的 [Earley parser](https://en.wikipedia.org/wiki/Earley_parser)。
2. 线性时间的 [Packrat parser](https://en.wikipedia.org/wiki/Parsing_expression_grammar)。
3. 使用动态规划思想设计的 O(n³|G|) 时间的 [CYK 算法](https://en.wikipedia.org/wiki/CYK_algorithm)。
4. 哪怕是在解析已经被视为 solved problem 的 2020 年，还有诸如 [Pika parser](https://arxiv.org/abs/2005.06444) 之类的算法在不断被提出。

当然，这些算法知道名字就行了，实践中大概率是用不到的。

## 有没有一劳永逸的办法？

聪明的同学可能会提出这样的问题：我们随便写文法，然后让机器自动检查这个文法是否是二义的，并转换成一种非常高效的文法表示，最后自动生成代码，这是可以办到的吗？

很遗憾，答案是否定的，该问题是一个[不可判定问题](https://en.wikipedia.org/wiki/Undecidable_problem)。要理解背后的原理，需要进一步学习相关理论（可计算性理论）才可以（而且证明也有点繁杂）。在这里，我们（用非常不严谨的语言）列出与上下文无关文法相关的一些结论。注：“不可判定” 的意思是 “不可能构建这样一个算法，使得其对一个 Decision Problem 总是回答正确的是或否”

1. 上下文无关文法的二义性是不可判定的。
2. 检查文法是否接受任何字符串是不可判定的。
3. 检查两个文法是否接受相同的语言是不可判定的。
4. 无法判定两个 CFG 接受的语言交集是否是空的。

另外，尽管有上面 2 这样的结论，但是 “检查文法是否什么字符串都不接受” 却是可判定的，将 CFG 转换成 Chomsky normal form 就可以轻松办到。

思考：对正则语言来说，上面这些问题的结论是怎样的？

下面介绍一个著名的问题 [Post correspondence problem](https://en.wikipedia.org/wiki/Post_correspondence_problem)，来说明有时候人类的直觉是很不靠谱的。

给定相同长度的两个字符串列表 a[1], a[2], a[3], ..., a[n] 和 b[1], b[2], b[3], ..., b[n]，回答：是否存在一列下标 i[1], i[2], ..., i[k]，使得 a[i[1]] a[i[2]] ... a[i[k]] = b[i[1]] b[i[2]] ... b[i[k]]？

考虑如下的例子

| a₁    | a₂    | a₃    |
| ----- | ----- | ----- |
| a     | ab    | bba   |

| b₁    | b₂    | b₃    |
| ----- | ----- | ----- |
| baa   | aa    | bb    |

对这组输入来说，这个问题是有解的，因为 a₃a₂a₃a₁ = b₃b₂b₃b₁。

尽管一时半会可能想不到高效的做法，但是直觉告诉我们，似乎可以去暴力枚举，然后一一比较……

**然而**，这个问题是不可能机械求解的！不可能写出一个程序来判定这个问题。事实上，不可判定问题无处不在，[莱斯定理](https://en.wikipedia.org/wiki/Rice%27s_theorem)告诉我们，任何non-trivial程序的属性都是不可判定的。


## 在线解析

学到这里，虽说大家已经可以写 parser 了，但是这在工程实践上却还不够。比如说，IDE 为了提供准确的实时报错、自动补全、代码缩进，都需要在用户编辑代码时立即提供语法树。仅仅利用 lab2 这种简单的离线解析器是完全不能满足使用的。在编辑代码时，大部分时间代码都是语法甚至词法不正确的，必须考虑到各种错误情形，并保证不会搞乱代码。此外，在提供自动缩进时，后方的错误不应该影响到前方代码的缩进。还有一个问题是，离线解析需要从头构建语法树，代价较高。受到这种“在线解析”需求的启发，涌现了不少很有实用价值的工作，比如：

1. [tree-sitter](https://github.com/tree-sitter/tree-sitter): incremental parser 框架，总是在内存中维护完整的语法树。
2. [Auto-indentation with incomplete information](https://arxiv.org/ftp/arxiv/papers/2006/2006.03103.pdf): 基于 Operator precedence parser 的用于代码缩进的框架，支持局部前向解析。尽管并不维护完整的语法树，但由于每次解析量很少，所以速度足够快。

