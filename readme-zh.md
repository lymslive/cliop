# C++ 命令行程序启动参数处理库

C/C++ 程序 `main()` 函数的 `argc` 与 `argv` 接收从 shell 命令令行启动传入的参
数。大多程序支持许多选项定制程序行为，由此也产生了通用的命令行参数处理的需求。
这是另一个功能丰富的参数处理库。

## 特性

* 支持常规的长短命令行参数习惯用法
* 支持配置文件与命令行参数的映射
* 支持从环境变量预设命令行选项
* 支持位置参数绑定，省略常用选项名
* 支持子命令分发

## 编译

直接在本目录下 make 生成静态库于 `lib/`，可选编译单元测试程序与示例程序于 `bin/`；
如果安装有 `doxygen`, 还可以从注释生成文档：

```bash
make
make test
make sample
make docs
```

无额外依赖，只需 STL 与 C++11 标准。单元测试只依赖轻量单元测试框架头文件
`tinytast.hpp` （见 [couttast](https://github.com/lymslive/couttast)）。也可只
用 `make utest` 生成测试程序然后手动运行各个测试用例：

```bash
bin/utest-cliop [test case]
```

注意：单元测试须在本项目根目录运行，因为部分测试用例会读取相对路径下的测试用配
置文件。

编译目标程序时需要 `cliop.h` 头文件，并加入 `-lcliop` 链接选项。也可以将本库少
量几个 `.cpp` 文件直接拷入工程源码一起编译。

基本使用可参考 [sample/](sample) 目录下的示例，以及 [utest/](utest) 目录下的单
元测试用例源码。详细使用说明请继续往下阅读。

## 使用手册

本库为命令行参数处理提供了一个核心的类 `cli::CEnvBase` 。`Env` 的命名源于将命
令行参数视为程序启动的定制环境，同时本库也包含一个（非关键的）特性，可将常规环
境变量绑定到命令行选项参数。`cli` 则是命名空间，`cmdline` 的缩写。

本手册提到的“参数”术语，可能有几个相近的含义，期望能根据上下文分辨理解：

* 命令行参数，也即 `argv` 所接收的所有参数，包含选项与普通参数
* 紧跟在某选项后的参数，隶属于该选项，相对地，也有不接收参数的选项（Flag）
* 不属于任何选项的普通参数，也叫位置参数

### 按默认规则处理命令行参数

根据对常见命令行程序对选项参数的习惯用法，本库提炼出以下默认规则：

* 以一个短横 `-` 引导的单字母短选项名不带参数，并且可以合并选项，如 `-abc` 代
  表三个 Flag ，与 `-a -b -c` 等效。
* 以两个短横 `--` 引导的长选项名必带参数，既可以用空格分隔写在下一个参数，也可
  以用 `=` 粘连写在同一个参数。
* 如果用 `=` 粘连选项及其参数，则前导的 `--` 也可以省略。但是以 `=` 开头的参数，
  不认为是选项，可当作普通参数。
* 只含两个短横 `--` 的特殊参数本身被忽略，仅代表选项结束，后续所有参数都是普通
  参数，即使它包含 `-` 前缀或 `=` 中缀。
* 只含一个短横 `-` 的参数也是允许的，且只作为普通的非选项参数，具体意义由客户
  程序解释。不过许多程序将其当作特殊文件名，表示读或写终端屏幕。

如果待设计程序对于期待的命令行参数用法也符合上述规则，则使用本库非常简便，只要
定义一个 `CEnvBase` 对象，将 `main()` 函数的两个参数转发传给 `CEnvBase::Feed()`
方法即可：

```cpp
#include "cliop.h"
int main(int argc, const char* argv[])
{
    cli::CEnvBase env;
    env.Feed(argc, argv);
    // todo stuff ...
}
```

注意，这里 `argv` 的类型是 `const char**` ，对于 C++ 程序，建议尽量使用 `const`
修饰参数，不过用 `char**` 也是可以编译的。此外 `Feed()` 还有个重载方法能接收一
个 `const std::vector<std::string>&` 参数，该容器内只包含纯粹的参数，即排除表
示程序名 `argv[0]` 的参数。或者可认为 `argv[]` 是从 1 开始计数的数组。

#### 获取一个参数

然后，就可以利用 `env.Has()` 方法确定某个选项是否存在， `env.Get()` 方法获取选
项参数或位置参数，也可以用等效的 `[]` 操作符，返回值都是 `std::string` 字符串
类型。传入选项名得到该选项的参数，未输入该选项时返回空字符串，可以是长选项名或
短选项名（长度为 1 的字符串，不是字符类型），但不要前导 `-` 或 `--` 。传入整数
当作位置参数的索引，从 1 开始计数，兼容 `argv[]` 数组的习惯定义。例如：

```cpp
bool bHas = env.Has("flag");
std::string arg;
arg = env.Get("option"); // env["option"]
arg = env.Get("o");      // env["o"]
arg = env.Get(1);        // env[1]
arg = env.Get("--");     // env["--"]
arg = evn.Get(0);        // env[0]
```

特殊索引 `0` 或 `"--"` 表示获取所有位置参数，以空字符 `\0` 拼接成一个字符串存
于 `std::string` 变量中返回。

`Get()` 方法还有双参数重载版本，综合了上述 `Has()` 与 `Get()` 的功能，其中第二
个是传出的引用参数，而用返回值表示选项是否存在。如果选项不存在，返回 `false`
，并且不修改第二引用参数的原值，因此可预设默认值。第二引用参数不仅可以是字符串
，还可以是数值类型，如 `int` 或 `double` ，如此将自动将原始的字符串参数转为相
应的类型。而对于位置参数 `"--"` ，可以传出为数组 `vector` ，如：

```cpp
std::string strArg;
int iArg = 0;
std::vector<std::string> args;
bool bHas;
bHas = Get("flag", bHas);
bHas = Get("option", strArg);
bHas = Get("option", iArg);
bHas = Get("--", args); // args 也允许是 std::vector<int> 类型
```

#### 获取参数聚合容器

除了用 `Get()` 方法取得一个参数，本库还提供了以下方法可将所有参数置于标准容器
中返回：

* `const std::map<std::string, std::string>& Args()` 返回选项参数的关联数组。
* `const std::vector<std::string>& Argv()` 返回位置参数的数组。
* `int Argc()` 返回位置参数的个数，即 `Argv().size()` 。
* `const std::string& Arg0()` 返回程序名，可能是 `argv[0]` 但能被其他方法修改。

这些方法与 `main()` 的参数 `argc/argv` 意义有明显的对应关系。如果输入命令行参
数不包含任何选项时，`Argv()` 与 `argv[]` 数组就包含几乎相同的参数，除了表示程
序名的 `argv[0]` 被排除在外，单独存在 `Arg0()` 中，因为它一般不认为是传给程序
的业务参数，也因此 `Argc()` 比 `argc` 小 1 。

而当输入参数包含选项时，它们将存于 `Args()` 表示的 `map` 中。键为选项名，值为
参数值。如果是无参数的 Flag （如短名字母选项），则 `map` 中存在该 Flag 名字（
可能是只有一个字母的字符串），其值非空但不确定（当前是 "1" ）。

当需要迭代遍历输入的参数时，这些方法能提供便利。

### 继承类封装成员绑定选项参数

当程序支持与关注的命令行选项比较多时，如果分别对每个选项都在 `main()` 中定义为
局部变量，就很快会遭遇变量管理问题，颇为不便。为解决这个需求，`CEnvBase` 如其
命名，是允许继承的。客户程序可以扩展自己的派生类，添加一系列的成员变量，绑定命
令行选项参数。当用 `Feed()` 方法接收、解析参数后，类成员就保存了各个选项的值，
并且转换为了相应的使用类型。

在基类提供的 `CEnvBase::Bind()` 方法能实现这绑定绑定操作，须在 `Feed()` 方法之
前调用，其实质就是在 `Feed()` 之后自动调用一系列的 `Get()` 方法。既然使用了派
生类，可以就在类的构造函数定义相关绑定。例如：

```cpp
struct CMyEnv : public cli::CEnvBase
{
    bool debug = false;
    int log_level = 0;
    std::string input;
    std::string output;
    std::vector<std::string> others;

    CMyEnv()
    {
        Bind("d", debug);
        Bind("log.level", log_level);
        Bind("input", input);  // also macro BIND_OPTION(input);
        Bind("output", output);
        Bind("--", others);
    }
};

int main(int argc, const char* argv[])
{
    CMyEnv env;
    // env.Bind(...) here also ok
    env.Feed(argc, argv);
    // todo stuff with env member
    if (env.debug) ...
}
```

这个程序大约可接收如下的命令参数：

```bash
./exe -d --log.level=1 --input=input.txt --output=output.txt *.tex
```

其中，`*.tex` 将由 shell 展开为一系列匹配的文件名，将由程序的 `env.others` 接
收保存这些文件名。如果有某些文件命名不规范，包含 `-` 前缀或 `=` 符号，则可在
`*.tex` 前面额外加上 `--` 参数。

在很多情况下，如果需要绑定的选项名与成员名完全相同，则可用 `BIND_OPTION` 代替
`Bind()` 方法，这可以避免选项名拼写错误的意外。

派生类不仅可以封装选项参数，也可以封装依托于此的业务逻辑，只要重写虚函数
`Run()` 即可。若如此做，`main()` 函数将简化到只转发命令行参数：

```cpp
int CMyEnv::Run(int argc, const char* argv[])
{
    // read input, iterate others
    // write output
    return 0;
}

int main(int argc, const char* argv[])
{
    CMyEnv env;
    return env.Feed(argc, argv);
}
```

`Run()` 方法将被 `Feed()` 调用，其函数原型也类似 `main()` ，只是参数 `argc` 与
`argv` 很可能用不到了，因为已经被解析成更易用的成员变量。而保留这两个参数，主
要为体现设计意图。

### 设置选项定制选项行为

仅管不必设置选项直接按默认规则解析命令行参数也能满足大部分需求，但一般应用程序
都会（精心）规划与设置选项。因为这能带来诸多好处与灵活性，如：

* 给一个长选项名关联一个单字母短选项名，用户在命令行使用其一效果相同。
* 指定短选项带参数，或长选项不带参数。
* 指定某个（些）选项参数必须提供。
* 为非必须选项提供默认值。
* 让选项在缺省的情况下读环境变量。
* 指定某个选项允许在命令行多次出现。
* 允许将位置参数提升为某个常用选项参数。
* 为选项设计提供帮助信息，让用户有所参考。

其中对解析参数影响最大的是选项是否带参数的特性。比如默认情况 `-abc` 表示输入了
三个独立无参选项 Flag ，但如果设置 `-a` 选项带参数，那么 `bc` 就认为是 `-a` 的
参数了。

选项设置的其他特性则属于扩展功能。

#### 不同类型的选项设置方法

`CEnvBase` 类提供了如下几个方法分别设置常用类型的选项：

* `Flag()` 设置无参选项
* `Option()` 设置带参选项
* `Required()` 设置必须提供的带参选项

它们都返回 `*this` ，可以链式调用。其中 `Option()` 方法还有个重载版本较为通用
，也能实现 `Flag()` 与 `Required()` 的功能。例如：

```cpp
cli::CEnvBase env;
env.Flag('f', "flag", "simple flag without argument"); // -f --flag
env.Flag('F', "Flag", "description", bBindFlag); // bool bBindFlag
env.Option('o', "option", "option with argument");     // -o --option=
env.Option('O', "Option", "option with argument", "defalut value");
env.Requrired('r', "required", "option must be provided");
env.Option('m', "multiple", "option allowed mutiple provided", "default",
    cli::OPTION_ARGUMENT | cli::OPTION_REPEATED);
```

以上方法前三个参数意义相同，分别表示短选项名、长选项名与描述帮助文本。`Flag()`
方法可以直接添加第四参数绑定一个 bool 变量，但 `Option()` 没有提供该功能，原因
有二。其一是 `Option()` 的参数数量已经比较多了，其次选项参数在使用上可能有不同
类型，这将导致过多重载函数且与 `Bind()` 犯重。故建议分两次函数调用实现选项设置
与绑定的功能，如：

```cpp
// declaration of variable foo and bar first
env.Option('o', "option", "option with argument")
   .Option('\0', "foo", "").Bind("foo", foo)
   .Option(0, "bar", "").Bind("bar", bar);
```

为同一个 `CEnvBase` 对象设置的选项，长短选项名都不应该重复。长选项名是必须的，
但短选项名非必须，因为字母资源有限，对于非常用选项可以不设置短名。如此可将第一
参数传空字符 `\0` 或简单的 `0` 。描述与默认值参数在不想设置时也可以传空字符串
。

选项设置必须在调用 `Feed()` 方法之前才能生效。如果有设置必须提供的选项而用户未
提供，`Feed()` 将返回非 0 值。但是与未设置任何选项的默认行为一致，仍允许用户输
入任意选项，如果要检查是否有不在预设置选项范围内的选项输入，可开启严格解析模式
，如：

```cpp
int nError = env.SetOptionOnly().Feed(argc, argv);
```

这可能检测到用户意外将选项名拼写错误的情况，可由客户程序自己决定是否开启。

#### 通用的选项设置方法

以上 `Option()` 方法虽有比较通用的重载版本，但参数众多，且仍未覆盖所有选项特性
，因此另外提供了一个最通用的 `Set()` 方法。它只接收三个参数：选项名，描叙信息
与可选的绑定变量。其中关键是第一参数，它不是一个简单的字符串，而是有着约定格式
的字符串，可以包含多个由空格分隔的部分（token），各自表达了某种选项特性，包括
：

* 以 `-` 引导单字母的短选项名。
* 以 `--` 引导的长选项名。
* 以 `#` 引导一个数字表示绑定的位置参数索引。
* 以 `$` 引导的环境变量名与选项绑定。
* 长选项名之后的 `=` 表示该选项要求带参数。
* `=` 之后的 `?` 表示该选项必须提供，`+` 表示选项可以多次提供。
* 用中括号 `[]` 括起的表示该选项的默认参数。

例如，可以这样定义一组选项：

```cpp
env.Set("-f --flag", "simple flag without argument")
   .Set("-F --Flag", "simple flag without argument", bBindFlag)
   .Set("-o --option=", "option with argument")
   .Set("-O --Option= [val]", "option with argument and defult")
   .Set("-r --required=?", "option must be provided")
   .Set("-m --multiple=+ [val]", "option allowed mutiple provided")
   .Set("--foo=", "", foo).Set("--bar", "", bar)
   .Set("-u $USER --user=", "user name can be from env", user)
   .Set("-i #1 --input=", "", input)
   .Set("-p #2 --output=", "", output);

// note: foo bar user input output is bind variables
```

其中，第一参数只有长选项名是必须的，第二参数描述文字可空，最后一个绑定变量可缺
省不绑定。

#### 自定义选项的帮助信息

本库内置了无参选项 `--help` ，将打印帮助信息，其中主要包括已设置的选项列表信息
，打印内容格式就如同 `Set()` 方法的前两个参数，这也是那第一个参数的格式约定来
由。

但是默认情况下，并没有为 `--help` 选项设置相应的短选项名，因为考虑到字母资源的
稀缺性，应由客户程序决定如何分配使用。如果客户程序需要 `-h` 作为 `--help` 的短
名称，则可以显式地设定这个选项，用如下两条语句之一：

```cpp
env.Set("-h --help", "print this help message");
env.Flag('h', "help", "print this help message");
```

如果 `-h` 已被其他选项占用，可改用其他字母如 `-H`。但是不能改变长选项名，如
`--Help` ，否则将作为其他自定义的普通选项，需要由客户程序自己编码如何处理这个
选项。比如也可以调用 `CEnvBase::Help()` 方法打印帮助信息。

如果对本库默认打印的帮助信息不满意，可以在派生类中重写 `Usage(std::string& outText)`
方法生成帮助文本。

#### 环境变量及默认值设置

当在选项设置中引入了 `$` 环境变量时，使用程序时就可以通过环境变量来代替命令行
参数了。例如按上述对 `user` 选项的设置，下面这样两种使用是等效的：

```bash
./exe --user=lymslive *.txt
USER=lymslive ./exe *.txt
```

当关联的环境变量已被导出（`export`）时，不必在每次启动程序都在程序名前定义该环
境变量，达到简化命令行输入的作用。环境变量按一般习惯是全大写，但具体允许怎样的
命名由系统决定。

命令行参数、默认参数与环境变量的优先级如下：

1. 在命令行显式为选项指定的参数优先。
2. 然后是关联的环境变量的值。
3. 如果选项的环境变量未设定或值为空，最后再取选项的默认值。

当涉及环境变量或默认值设置时，`CEnvBase::Get()` 双参数版的返回值与 `Has()` 可
能有微妙差异。因为 `Has()` 只判断命令行只是输入了某选项，而 `Get()` 只要读取到
了非空默认值，就能返回 `true` 。

#### 位置参数提升设置

如果某个选项极为常用，则可以将其设置为允许从位置参数中提升，如上述对 `#1 --input`
与 `#2 --output` 选项的设置。这样能达到简化输入的作用，如下两种写法是等效的：

```bash
./exe --input 1.txt --output 2.txt
./exe 1.txt 2.txt
```

那为什么不直接取消这两个选项的设置，直接读取位置参数呢？那是因为通过选项设置，
可以在 `--help` 中打印出相关信息，既对参数命名，又防止忘记参数顺序。而在写脚本
时，又可以明确写出选项名，增加可读性。

位置参数提升有个限制，只能依次尝试提升前几个位置参数，失败后即终止提升流程。例
如：

* 若只有某个选项设置了 `#2` 绑定，而没有任何选项绑定 `#1` ，那是没效果的。
* 若绑定 `#1` 的选项已经明确输入命令行了，则不会提升第 1 个参数，也不会再提升
  后面的参数，即使有选项设置绑定了 `#2` 。

这是为了规则简单，避免理解困难、使用混乱而设。而且一般建议不要设置三个以上的位
置参数提升绑定。

位置参数提升绑定设置的索引从 1 开始，也是按 `argv[]` 数组习惯的。

### 配置文件与命令行参数的映射

#### 配置映射规则

配置文件用于解决超长行命令行参数复用与维护问题。本库对此的基本原则是，将配置文
件的每一行，等效与命令行中由空格分隔的每一个参数。在最简单的情况下，将一行命令
行参数复制到文件，并将空格批量替换为回车符后保存，就能达到与手动输入命令行等同
的效果。而在实用中，可完全兼容 `.ini` 或 `.properties` 配置文件格式。

具体地，对配置文件的每一行进行如下处理（大致顺序）：

* 剪除首尾空白。
* 忽略空行与注释行，以 `#` 或 `;` 开头的是注释行。
* 配置行一般形如 `key=val` ，类似 `--key=val` 选项参数，只是 `=` 前后允许空白。
* ini 段名形如 `[section]` ，将成为该段下每个键的前缀，形成类似 `--section.key` 
  的完整选项名。
* 特殊行 `--` 或 `[--]` 之后的每一行，将当作命令行的位置参数。
* 其余不含 `=` 的行，不管有没有 `-` 前缀，都按命令行参数规则同样地解析。

例如这样一份配置文件：

```ini
# 无段名选项
-abc
abc
efg = 1
--hi
jk

[xxx]
hi = jk

[yyy]
hi = JK

# 选项结束，位置参数开始
[--]
1
uvw
```

对应如下的命令行选项：

```bash
-abc abc --efg=1 --hi jk --xxx.hi=jk --yyy.hi=JK -- 1 uvw
```

其中 `-abc` 的解析取决于之前是否有 `-a` 选项的设置以及该选项是否带参数，同样
`--hi jk` 两行，也取决于 `--hi` 选项设置是否带参数。当手动编辑配置，建议始终坚
持用长选项名，并且在同一行用等号连接选项与参数，必要时加注释或分段。

#### 读取配置文件选项

`--config` 是另一个内置选项，能带参数，有默认值是与程序名同名再加 `.ini` 后缀
的文件名（从当前工作目录下读取）。也就是说，使用本库编译的程序，在启动时都会尝
试读取配置文件，即使没有指定 `--config` 选项，或根本不知道有这个功能。

如果配置文件不可读，只是默默忽略，跳过读取配置文件阶段。也可以显式指定
`--config=NONE` 忽略读取配置文件，即使当前目录直的有个名叫 `NONE` 的奇葩文件。
事实上随意输入一个不存在的文件名，就能跳过读配置，只是 `NONE` 意图明显。

`--config` 选项默认没有短选项名字母，但可以像 `--help` 那样自己定个字母。同时
可以修改默认值，比如设置默认值为 `NONE` 就能使在命令行未显式指定时跳过读配置：

```cpp
env.Set("-c --cofing= [NONE]", "read argument from config file");
```

当然如有需要可以绑定一个环境变量，如 `$MY_CONFIG --config=` 。

还有一种用法，如果客户程序在使用时，配置文件是最重要的参数，则可以将其绑定到位
置参数，如：

```cpp
env.Set("#1 --config=?", "read argument from config file");
```

于是，在命令行中就可以通过不同配置文件来启程程序：

```bash
./exe someone.ini
./exe another.ini
```

#### 配置文件与命令行的优先级

`CEnvBase::Feed()` 方法首先读入命令行参数 `argv[]` ，然后根据命令行指定的配置
文件名，读入配置行，当作命令行参数再次解析。配置文件如果也有 `--config` 选项，
则不会再起作用。

因此命令行显示输入的选项优化级更高。可以在配置文件中写入常用基本配置，然后在命
令行中临时覆盖配置文件的值。

但这只针对一般的不可重复选项而言。如果某个选项设置为可重复，则配置文件中读到的
同名选项值（甚至可多个），会附加到之前可能在命令行读入的选项值之后。同样地，如
果在配置文件中有（`--`之后）位置参数，也会附加到命令行直接输入的位置参数之后。

位置参数提升至选项参数，只会发生在原始输入的命令行参数中，不会作用于配置文件。
例如，按之前的 `#1 --input=` 选项设置：

```
// from bash
./exe 1.txt

// form config
input=11.txt

// in program
assert(input == "1.txt")
```

命令行输入的位置参数 `1.txt` 提升为选项参数 `--input` ，也等效于在命令行中输入
了该选项，于是忽略在配置文件读取的同名选项值 `11.txt` 。

另注：只要是不可重复选项，同在命令行中重复输入，也是先输入的选项值生效，忽略后
输入的值。

### 子命令的参数与行为分发

#### 子命令处理句柄

本库也支持子命令模式，即将第一个参数特殊处理，当作子命令分发，每个子命令完成一
类业务功能。可以使用 `CEnvBase::SubCommand()` 方法添加子命令，例如：

```cpp
env.SubCommand("foo", "description test", subcmd_function)
   .SubCommand("bar", "description test", subcmd_object);
```

其中前两个参数分别是子命令名与相关描述（`--help` 可打印信息），第三个参数是处
理该子命令的句柄，有两种句柄可选其一。

第一种是简单直接的函数句柄，其定义大约如下：

```cpp
int subcmd_function(int argc, const char* argv[], cli::CEnvBase* args)
{
    // make use of parsed argv in args to do stuff ...
    return 0;
}
```

前两个参数来自 `main()` ，但进行了偏移修正，略过原来的第一个参数。即 `argc` 数
量少 1 ，而 `argv[0]` 就是子命令名称。一般情况下可能已用不到前两个参数，只要利
用第三参数的 `CEnvBase` 对象指针获取已解析的命令行参数完成业务逻辑。但假如不同
子命令有着完全不同的选项设置，尤其是同一个选项在不同子命令中有不同的意义，那就
需要在句柄函数中重新解析 `argv[]` 命令行参数。

第二种处理子命令的句柄是另一个 `CEnvBase` 或其派生类的对象。这种句柄对象的功能
更丰富，它由各自的子命令相关的 `env` 对象解析（除第一个子命令名的）剩余命令行
参数，并且能自动响应子命令后的 `--help` 。而前一种句柄函数只由一个主命令相关的
`env` 对象解析所有参数。

#### 主命令处理函数句柄

类似子命令的处理函数句柄，也可以给主命令（或者不设计子命令的单一命令程序）注册
相同原型的函数句柄，它能在 `Feed()` 函数解析命令行参数后自动调用之。使用方式也
相似：

```cpp
env.Command("program_name", "description test", cmd_function);
```

这个功能的作用是可以某种程度上避免继承并重写 `Run()` 方法，而只需要使用基类对
象，并根据需要注册不同处理函数即可。因为现在许多开发者开始反对滥用继承机制。
例如不同子命令可不必继承许多类，只用基类创建不同对象用于处理相同子命令的工作：

```cpp
int func1(int argc, const char* argv[], cli::CEnvBase* args);
int func2(int argc, const char* argv[], cli::CEnvBase* args);
int main(int argc, const char* argv[])
{
    cli::CEnvBase env, env1, env2;
    env1.Set(...).Command("cmd1", "", func1);
    env2.Set(...).Command("cmd2", "", func2);
    env.SubCommand("cmd1", "", env1).SubCommand("cmd2", "", env2);
    return env.Feed(argc, argv);
}
```

因为主命令的函数句柄是不必须的，所以 `Command()` 方法可只有前面两个甚至一个参
数。而第一个参数作为命令名称会显示在 `--help` 帮助信息中，如果从未设置命令名称
，则会将命令行参数 `argv[0]` 当作命令名称（可能包含路径）。但只要用
`Command()` 方法注册了处理函数句柄，该 `env` 对象就不再调用其 `Run()` 方法。

#### 严格子命令模式

默认情况下，当一个 `env` 对象注册了子命令，并不意味着一定要使用子命令。如果第
一参数不能匹配任意一个子命令，它也能继续按无子命令的程序那样解析所有命令行参数
。如果想要限定第一参数必须是有效子命令名，可以额外调用 `SubCommandOnly()` 方法
开启严格子命令模式，如：

```cpp
env.SubCommand("foo", "description test", subcmd_function)
   .SubCommand("bar", "description test", subcmd_object)
   .SubCommandOnly();
```

此时，当第一参数是无效子命令名时，`Feed()` 方法将返回非 0 错误码。

#### 嵌套子命令

因为子命令的处理句柄可以是另一个 `env` 对象，所以理论上该子命令也仍然可以有其
自己的另一套子命令集，且嵌套深度也无限制。但实用上基本只见到一层子命令的程序，
比如流行的 `git` 与 `docker` 命令行工具。

#### 通过程序软链接名分发子命令

在 Linux 下，可以通过创建软链接（或硬链接）为程序关联多个文件名（命令名），于
是有些程序也可以通过程序名 `argv[0]` 来以不同方式启动，执行不同任务。例如 `vim` 
的链接名 `view` 与 `vimdiff` 。

这种用法可能已不多见，目前以第一参数 `argv[1]` 作为子命令分发的程序更为流行
，所以本库也优先按这原则处理。但是也能兼容前者，当 `argv[1]` 找不到子命令时，
也会再尝试通过 `argv[0]` 或 `program_invocation_short_name` 寻找子命令。另外，
客户程序若有特殊需求时，也大可按需重新构建 `argv[]` 传给 `CEnvBase::Feed()` 。

### 错误处理

在默认情况下，`CEnvBase::Feed()` 方法将尽可能读入并解析命令行参数，只有显式的
`--help` 或 `--version` 会提前返回一个特殊错误码 `ERROR_CODE_HELP` ，否则将进
入 `Run()` 方法或注册的处理函数，返回后者的返回值。而如果没有重写 `Run()` 方法
也没有注册处理函数，则 `Feed()` 一般会返回 `0` ，让调用者继续处理业务逻辑。

可以显式要求 `CEnvBase` 对象在解析命令行参数时捕获某些错误。比如上文提及的
`SubCommandOnly()` 与 `SetOptionOnly()` 就是两个常用方法：只允许用户输入已预设
的子命令或选项。将选项设置为必须输入也是另一种显式要求。

#### 捕获与忽略错误的一般方法

其实有更一般的 `Catch()` 方法来预声明捕获某个（些错误），如：

```cpp
// using namespace cli;
env.Catch(ERROR_CODE_COMMAND_UNKNOWN).Catch(ERROR_CODE_OPTION_UNKNOWN);

// 或等效地：
int errors[] = {ERROR_CODE_COMMAND_UNKNOWN, ERROR_CODE_OPTION_UNKNOWN};
env.Catch(errors, sizeof(errors)/sizeof(errors[0]));
```

也可用 `CatchAll()` 方法捕获所有能处理的错误，然后相应地有 `Ignore()` 方法忽略
某些错误。例如要求捕获所有错误，但是允许未定义选项，也可忽略读配置文件：

```cpp
env.CatchAll().Ignore(ERROR_CODE_OPTION_UNKNOWN).Ignore(ERROR_CODE_CONFIG_UNREADABLE);
```

但是若用 `Set()` 设置为必须输入的选项如 `--option=?` ，它所触发的错误是不能忽
略的，毕竟这两个要求是矛盾的。

#### 错误码列表

本库定义的错误码在 `cli::ErrorCode` 枚举类型中。详情请参考相关注释或文档。在单
元测试文件 [test-error.cpp](utest/test-error.cpp) 也有大量示例演示何时返回哪个
错误码。

用 `Catch()` 方法捕获这些错误码，需要在 `Feed()` 解析参数之前调用，部分规范选
项、子命令设计的错误，还应在设置选项、子命令之前调用。

#### 自定义错误报告函数

当检测到错误时，默认情况下将向标准错误 `stderr` 打印一行信息，以提示用户使用错
误。但是也允许客户程序提供自己的错误报告函数，例如：

```cpp
void my_error_report(int code, const std::string& text)
{
    // todo ...
    fprintf(stdout, "MY-E%d: %s\n", code, text.c_str());
}

auto save_handler = cli::SetErrorHandler(my_error_report);
// 这里触发错误将调用 my_error_report()
cli::SetErrorHandler(save_handler);
```

错误报告或处理函数将接收两个参数，整数型的错误码及相应的描叙字符串。

自由函数 `cli::SetErrorHandler()` 用于注册错误处理函数，并且返回原来的处理函数
，如有需要可以保存下来以备后来回滚之用。

#### 未定义行为

其他未处理的错误，或暂称其为未定义行为。未定义的意思是取决于实现，当前的实现或
许可接受，但不保证后续开发迭代时兼容逻辑。

以下是一些明显不合理的未定义行为列表（未必齐全），用户宜尽量避免：

* 注册子命令名为空，或重复
* 对无参数的 Flag 选项取值

## 参考实现

当我未开发这个库之前，我曾用过以下库来解析命令行参数：

* GNU `getopt` and `getopt_long`
* [CLI11](https://github.com/CLIUtils/CLI11) 一个只有头文件的命令行参数库
