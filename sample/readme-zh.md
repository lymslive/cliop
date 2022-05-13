# 示例应用程序：命令行工具

本目录提供了一些示例程序，都是些简单的单 `.cpp` 源文件程序，主要用以演示如何使用 `cliop` 库编写命令行工具，不过也希望有或多或少的实用价值。

## argview

展示 `cliop` 库如何解析命令行参数的实用小工具。输入 `--help` 会打印预定义的选项，但也允许用户输入任意命令行参数，它会如实打印接收到的原始 `argv[]` 参数，解析后的选项参数及位置参数，以及预设选项的默认参数。

请随意尝试。注意该示例代码使用几种不同方法获取参数值，它们的打印结果应该是一样的。

## sum

一个简单的数字求和程序，主要演示如何接收与绑定命令行参数，并据此实现业务逻辑。除了求和外，还有个附加的功能将结果以十六进制或八进制打印，以及一个并不实用的求积功能。例如：

```bash
bash$ ../bin/sum 1 2 3 4
10
bash$ ../bin/sum --base=hex 1 2 3 4
0xa
bash$ ../bin/sum --base=hex 15
0xf
bash$ ../bin/sum --int=1 operator=* 1 2 3 4
24
```

以上，假设在本目录启动命令行工具，含 `bash$` 前缀的行为输入的命令行，其下的行是输出结果。

## math

演示如何创建含子命令模式的程序工具，根据用户输入的子命令来对两个参数执行求和或求积运算。主要演示了子命令的几种实现接口，同时也演示了如何将位置参数绑定为选项参数。可以如下使用：

```bash
bash $ ../bin/math add 5 6
11
bash $ ../bin/math mul 5 6
30
bash $ ../bin/math mul --left=5 --right=6
30
```

## textalign

一个实用小程序，可以将标准输入的几行文本，根据特征子串分列对齐，然后打印至标准输出。因为 `cliop` 在打印帮助信息时实现过将文本行自动对齐的功能，所以可以简单调用实现该程序。本目录也为该工具提供一个示例文件，可如下试运行：

```bash
bash $ cat sample.txt 
int i = 3; // comment
double xx = 3.14; // comment
bash $ ../bin/textalign // < sample.txt 
int i = 3;        // comment
double xx = 3.14; // comment
bash $ cat sample.txt | ../bin/textalign = //
int i     = 3;    // comment
double xx = 3.14; // comment
```

将该工具复制到 `$PATH` 搜索路径，可以配置为 `vim` 或其他文本编辑器的过滤器工具，增加对齐需求的编辑效率。
