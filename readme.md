# C++ Library for Command-Line Parameters Processing at Program Startup

[中文版](readme-zh.md)

In C/C++ Program, the `main()` function receives arguments passed from shell
command lines, named `argc` and `argv`.  Most programs support many options to
customize program behavior, which also rise the need for common command-line
parameter handling. This is another feature-rich parameter processing library.

## Features

* Support general long and short command-line parameter idioms.
* Support mapping of configuration files with command-line parameters.
* Support preset command-line options from environment variables.
* Support positional parameter binding, omit the most often used option names.
* Support subcommand dispatch.

## Compile

Just directly make in current directory to generate static libraries in
`lib/`, optionally compile unit testers and sample programs in `bin/`, and if
doxygen is installed, documentation can also be generated from comments :

```bash
make
make test
make sample
make docs
```

No additional dependencies, just STL and  C++11 standard. While unit tests
rely only on the lightweight unit test framework header file
[tinytast.hpp](https://github.com/lymslive/couttast/blob/main/include/tinytast.hpp)
from [couttast](https://github.com/lymslive/couttast). You can also build the
test program with `make utest` first and then manually run individual test
cases:

```bash
bin/utest-cliop [test case]
```

Note: Unit tests must be run in this project root directory because some test
cases read the test configuration file using relative path. 

The `cliop.h` header file is required to compile the target program and with
the `-lcliop` link option added. You can also copy the few `.cpp` files from
this library directly into the project source code and compile them together. 

For basic use, please refer to the examples in the [sample/](sample) directory
and the unit test case source code in the [utest/](utest) directory. For
detailed instructions, please read on.

## User Manual 

The library provides a core class `cli::CEnvBase` to handle command-line
arguments. `Env` is named from that command-line arguments are considered as
custom environment at program startup, and the library also includes a
(non-critical) feature that binds general environment variables to
command-line option parameters. `Cli` is the namespace, short for `cmdline`.

When deal with actual arguemnts, the term "argument" referred to in this
manual may have several similar meanings and can be expected to be understood
in context: 

* Command-line arguments, i.e., all arguments received by `argv`, contain options and ordinary arguments.
* Argument Immediately after an option, belong to that option, and conversely, there are also options that do not receive argument (Flag).
* Ordinary arguments that do not belong to any option, also called positional arguments.

Sometimes the term "parameter" is exchangeable used with "argument".

###  Handling Command-Line Arguments By Default Rules

Based on the customary use of option parameters in common command-line
programs, this library distills the following default rules:

* The short flag named with one leading dash `-` has no argument, and options
  such as `-abc` represent three flags, equivalent to `-a -b -c`.
* The long option named with two leading dashes `--` must have argument, either
  with a space-delimited argument written in the next argument or with `=`
  glued to the same argument.
* If the option and it argument are glued with `=`, the leading `--` can also
  be omitted.
* A special argument with only two dashed `--` is itself ignored, representing
  only the end of the option, and all subsequent arguments are normal arguments,
  even if it contains the `-` prefix or the `=` midfix.
* Argument with only one dash `-` is also allowed and are only used as ordinary
  non-option parameters, the specific meaning is explained by the client program.
  However, many programs treat it as a special file name, which means reading from 
  or writing to terminal screens.

If the intended command-line argument usage of the program to be designed also
conforms to the above rules, then it is very easy to use this library, as long
as you define a `CEnvBase` object, forward the two parameters of the main()
function to `CEnvBase::Feed()` method:

```cpp
#include "cliop.h"
int main(int argc, const char* argv[])
{
    cli::CEnvBase env;
    env.Feed(argc, argv);
    // todo stuff ...
}
```

Note that the type of `argv` here is `const char**`, and for C++ programs, it
is recommended to use `const` to decorate the parameters as much as possible,
but it is also possible to compile with `char**`. `Feed()` also has an
overloaded method that receives a `const std::vector<std::string>&` parameter,
which contains only pure parameters, i.e. excludes parameters representing the
program name `argv[0]`. Alternatively, you can think of `argv` as an array
that counts from 1.

#### Retrieve One Argument

After then you can take advantage of `env.Has()` method to determine Whether
an option exists, and the `env.Get()` method gets an option argument or
positional argument, or it can use the equivalent operator `[]`, where the
return value are all of type `std::string`. Passing in an option name gets the
argument for that option, and returns an empty string when the option is not
provided, where either a long option name or a short option name (a string of
length 1, not a character type) is ok, but without the leading `-` or `--` .
Passing in integers as indexes of positional arguments, counting from 1,
compatible with the customary definition of `argv` arrays. For example:

```cpp
bool bHas = env.Has("flag");
std::string arg;
arg = env.Get("option"); // env["option"]
arg = env.Get("o");      // env["o"]
arg = env.Get(1);        // env[1]
arg = env.Get("--");     // env["--"]
arg = evn.Get(0);        // env[0]
```

Passing special index `0` or option name `--` means get all posional
arguments, joined with `\0` in a single `std::string`.

The `Get()` method also has a two-parameter overloaded version that combines
the functions of `Has()` and `Get()` above, of which the second is the
outgoing reference parameter, and the return value indicates whether the
option exists. Returns `false` if the option does not exist , and the original
value of the second reference parameter is not modified, so default values can
be preset. The second reference parameter can not only be a string , but can
also be of numeric type, such as `int` or `double`, which will be
automatically converted from the original string argument.  And for the
positional parameter `--`, it can be passed out as an array `vector`, such as:

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

#### Retrieve all Arguments in Container

In addition to using the `Get()` method to retrieve individual arguemnt, the library also provides the following methods to place all arguemnts in a standard container
and return them:

* `const std::map<std::string, std::string> & Args()` returns an associative array of option arguments.
* `const std::vector<std::string> & Argv()` returns an array of positional arguments.
* `int Argc()` returns the number of positional arguments, i.e. `Argv().size()`.
* `const std::string & Arg0()` returns the program name, which may be `argv[0]` but can be modified by other methods.

These methods have a clear correspondence with the meaning of the parameter
`argc/argv` for 'main()'. If enter the command-line arguments Without any
options, the 'Argv()' and 'argv[]' arrays contain almost identical arguments,
except that the one representing the program name `argv[0]` is excluded and
exists a separate method of `Arg0()`, because it is not generally considered
to be business arguemnt passed on to a program.  And therefore `Argc()` is 1
smaller than `argc`.

When the input command-line do contain options, they will be stored in the
`map` represented by `Args()`. The key is the option name and the value is the
argument value. If it is a flag with no arguemnt (e.g. the short name letter
option), the flag name ( may be a string with only one letter) whose value is
not empty but indeterminate (currently "1").

These methods can be convenient when need to iterate over input parameters.

### Bind Option Argument with Member in Derived Class

When the program supports many command-line options, if define separate local
variable in `main()` for each option, it will soon encounter variable
management problems, which is quite inconvenient. To address this need,
`CEnvBase`, as its naming is allowed to be inherited. Client programs can
extend their own derived classes, adding a series of member variables, binding
to the command Command line option parameter. When the arguments are received
and parsed using the `Feed()` method, the class members save the values of
each option, and converted to the corresponding usage type.

The `CEnvBase::Bind()` method in the base class implements this binding
operation, before the `Feed()` method.  The essence of it is to automatically
call a series of `Get()` methods after `Feed()`. Since derivation is used, it
can define bindings in the constructor of a class. For example:

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

This program can receive approximately the following command parameters:

```bash
./exe -d --log.level=1 --input=input.txt --output=output.txt *.tex
```

where `*.tex` will be expanded by the shell into a series of matching
filenames, which will also be saved in `env.others` at program running time.
If there are some files that are not well-named, containing the `-` prefix or
the `=` symbol, you should preced an additional `--` argument before `*.tex`.

In many cases, if you need to bind an option name that is identical to the
member name, you can use `BIND_OPTION` instead of `Bind()` method, which
avoids the accident of misspelling the option name.

Derived classes can encapsulate not only option parameters, but also the
business logic that rests on them, simply by overriding the virtual function
`Run()` is enough. If this is done, the `main()` function will be simplified
to forwarding only the command line arguments:

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

The `Run()` method will be called by `Feed()` and its function prototype is
also similar to `main()`, except that the argument `argc` and `argv` is likely
to be unusable because it has been parsed into a more user-friendly member
variable. While keeping these two parameters, mainly to reflect the intent of
the design.

### Set Options to Customize their Behavior

Most of the requirements can be met without having to set options to parse
command-line arguments directly by default rules, but generally a program will
(carefully) plan and set options. Because it brings many benefits and
flexibility, such as:

* Give a long option name associated with a single-letter short option name, and the user may use one of them on the command line with the same effect.
* Specify short options with parameters, or long options without parameters.
* Specifying one (some) option parameters must be provided.
* Provides default values for optional options.
* Lets the option read the environment variable by default.
* Specifies that an option is allowed to appear multiple times on the command line.
* Allows positional parameters to be promoted to an often-used option parameter.
* Provides helpful information for option design for user reference.

One of the most influential on parsing parameters is whether the option has a parameter or not. For example, in the default case `-abc` indicates input
three independent parameterless options(flag) , but if the `-a` option is set with parameter, then `bc` is considered as the argument of `-a`.

Other characteristics of the option settings are extended functionality.

#### Methods for Different Types of Option Setting

The `CEnvBase` class provides several ways to set options for commonly used types:

* `Flag()` set an option without paramenter.
* `Option()` set an option with paramenter.
* `Required()` set an option that must provide paramenter.

They all return `*this` and can be chained. There is also an overloaded
version of the `Option()` method that is more common , which can also
implement the functions of `Flag()` and `Required()`. For example:

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

The first three parameters of the above method have the same meaning,
representing the short option name, the long option name, and the description
help text, respectively. The `Flag()` method can directly add a fourth
parameter to bind a bool variable, but `Option()` doesn't provide that
functionality as tow reasons.  The first is that the number of parameters for
`Option()` is already relatively large, and the second, option parameters may
be different in use type, which will result in too many overloaded functions
and overlapped with `Bind()`. Therefore, it is recommended to implement option
settings in two function calls if need to bind, such as:

```cpp
// declaration of variable foo and bar first
env.Option('o', "option", "option with argument")
   .Option('\0', "foo", "").Bind("foo", foo)
   .Option(0, "bar", "").Bind("bar", bar);
```

Options set for the same 'CEnvBase' object should not be duplicated in both
long and short option names. Long option name are required, however, the short
option name is not required, because the letter resources are limited, so the
short name can left to be unset for the non-common options. This can be done
by passing the first parameter with null character `\0` or simply '0'.
Descriptions and default value parameters can also pass empty strings when you
do not want to set them.

Option settings must be called before the `Feed()` method to take effect. If
there are settings that must be provided option while the user not provide ,
`Feed()` will return a non-0 value. However, consistent with the default
behavior of not setting any options, the user is still allowed to enter any
option, if you want to check if there is an option input that is not within
the range of preset options, you can turn on strict parsing mode. For example:

```cpp
int nError = env.SetOptionOnly().Feed(argc, argv);
```

This may detect that the user accidentally misspelled the option name, and it is up to the client program to decide whether to turn it on or not.

#### Method for Common Option Setting

Although the above `Option()` method has a more general overloaded version, it
has too many parameters and still does not cover all option features , so the
most general 'Set()' method is provided. It only receives three parameters:
option name, descriptive information and a opional binding variable. The key
is the first argument, which is not a simple string, but a string with a
conventional format , which can contain multiple spaces-separated parts
(tokens), each expressing certain option characteristics, including:

* Include a short option name of a single letter with `-`.
* Long option name with `--` prefix.
* Leads a number with `#` to indicate the binding positional parameter index.
* Environment variable names with prefixed `$`, which is bound to the option.
* A `=` after the long option name indicates that the option requires parameter.
* `?` after `=` indicates that the option must be provided, and `+` indicates
  that the option can be provided multiple times.
* Enclosed in brackets `[]` indicates the default parameter for this option.

For example, you can define a set of options like this:

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

Where only the long option name is required in the first parameter, the second
parameter description text can be empty, and the last bound variable can be
missing for not binding.

#### Help Information for Customized Options

The library has a built-in parameterless option, or `--help` flag that prints help information, which mainly includes the list of options that have been set
, the format of the printed content is like the first two parameters of the `Set()` method, which is also how the format convention of the first parameter brings up.

However, by default, the corresponding short option name is not set for the
`--help` option, because taking into account of scarcity of the letter
resource, it should be up to the customer program to decide how to allocate
and use. If the client program requires `-h` as a short name of `--help` , you
can explicitly set this option, using one of the following two statements:

```cpp
env.Set("-h --help", "print this help message");
env.Flag('h', "help", "print this help message");
```

If `-h` is already occupied by another option, you can use a different letter
such as `-H` instead. However, you cannot change the long option name, such as
`--Help` , otherwise it will be used as another customized option, and it will
be up to the client program to code how to handle this option. For example,
you can also call the `CEnvBase::Help()` method to print help information.

If you are not satisfied with the help information printed by default in this
library, you can override `Usage(std::string& outText)` method in the derived
class to generate help text.

#### Environment Variables and Default Value Settings

When the `$` environment variable is introduced in the option settings, the
environment variable can be used in place of the command line when start up
the program.  For example, by setting the `user` option above, the following
two command-lines are equivalent:

```bash
./exe --user=lymslive *.txt
USER=lymslive ./exe *.txt
```

When the associated environment variable has been exported, it is not necessary to define the environment variable before the program name each time the program is started,
to simplify command-line input. Environment variables are generally capitalized, but specifically how they are allowed naming is determined by the system.

Command-line arguments, default arguments, and environment variables take precedence as follows:

1. Explicitly specify argument for option on the command line will take precedence.
2. Then is the value of the associated environment variable.
3. If the environment variable for the option is not set or the value is empty, the default value of the option is taken last.

When it comes to setting environment variables or default values, the return
value of `CEnvBase::Get()` of dual-parameter version may have subtle
differences compared with `Has()`.  Because `Has()` only tells that the
command line do input an option, while `Get()` may still return `true` when it
reads non-empty default value.

#### Positional Parameter Elevation Settings

If an option is extremely common, you can set it to allow promotion from
positional parameters, as in the preceding option setting of `#1 --input` and
`#2 --output`. This simplifies the input, and the following two ways of
writing are equivalent:

```bash
./exe --input 1.txt --output 2.txt
./exe 1.txt 2.txt
```

So why not simply unset these two options and read the positional parameters directly? That's because through the option settings,
you can print the relevant information in '--help', and by naming the parameters can also prevent forgetting the parameter order. While writing a script
, you can still explicitly write out the option name to increase readability.

Positional parameter promotion has a restriction, only the first few
positional parameters can be tried in turn, and the promotion process is
terminated after failure. example as:

* If only one option has the `#2` binding set and none of the options has `#1`
  bound, it will not work.
* If the option to bind `#1` has been explicitly entered into the command line,
  the first parameter will not be promoted as well as the following parameters,
  even if there are options set bound to '#2'.

This is designed to make the rules simple, avoid difficult to understand, and
use confusion. And it is generally recommended not to set more than three
parameter promotion binding.

For positional parameters promote, the index of the binding starts at 1 and
are also customary for the `argv[]` array.

### Mapping of Configure to Command-line Argument

#### Configure Mapping Rules

Configuration file is used to troubleshoot super-long line command-line
arguments reuse and maintenance issues. The basic principle of this library on
this is that the text of each line in the configuration file is equivalent to
each argument on the command line separated by a space. In the simplest case,
if copy a long command line to a file and saved after replacing spaces with
carriage returns in bulk, it will be equivalent to manually entering the
original command line.  In practice, it is fully compatible with the
'.ini' or '.properties' configuration file formats.

Specifically, each row of the configuration file is processed as follows (in rough order):

* Trim out the white space.
* Ignore empty lines and comment lines begin with `#` or `;`.
* Configuration lines are generally in form as `key=val`, similar to the
  `--key=val` option parameter, except that blanks are allowed before and
  after `=`.
* Ini section name such as `[section]` will be prefixed with each key under
  the section, forming a full option name like '--section.key'.
* Every line after the special line `--` or `[--]` will be treated as a positional
  argument of the command line.
* The remaining lines without '=', with or without the '-' prefix, are parsed
  according to the same command-line argument rules.

For example, a configuration file like this:

```ini
# no section keys
-abc
abc
efg = 1
--hi
jk

[xxx]
hi = jk

[yyy]
hi = JK

# end of options
[--]
1
uvw
```

Corresponds to the command-line as following: 

```bash
-abc abc --efg=1 --hi jk --xxx.hi=jk --yyy.hi=JK -- 1 uvw
```

Where the parsing of `-abc` depends on whether there was a previous setting for the `-a` option and whether the option has parameters, and meanwhile, 
the `--hi jk` two-line also depends on whether the `--hi` option setting has parameter. When manually edit the configuration, it is recommended to always use
the long option name, and connect the option with it's argument with equal signs on the same line, add comment or section if necessary.

#### Options to Read Configuration

`--config` is another built-in option that takes parameter and has the default
value of being the same name as the program name plus the '.ini' suffix
(reading from the current working directory). That is to say, programs
compiled with this library will try to read the configuration file at startup,
even if you don't specify the `--config` option, or don't know there is this
feature at all.

If the configuration file is unreadable, just ignore and skip it silently. It
can also be specified explicitly `--config=NONE` to ignore reading the
configuration file, even if there is a strange file named `NONE` in the
current directory.  In fact, by entering a file name that does not exist, you
can alo skip the configuration reading, but the `NONE` intention is obvious.

The `--config` option does not have a short option name letter by default, but
you can specify a letter yourself like '--help'.  You can also modify the
default value, such as setting the default value to `NONE` to skip the reading
of configuration when not explicitly specified on the command line:

```cpp
env.Set("-c --cofing= [NONE]", "read argument from config file");
```

Of course, if necessary, you can bind an environment variable, such as `$MY_CONFIG --config=`.

There is also a usage pattern where if the configuration file is the most important argument for the client program, it can be bound into positional parameter, such as:

```cpp
env.Set("#1 --config=?", "read argument from config file");
```

Thus, the program can be started from different configuration files on the
command line:

```bash
./exe someone.ini
./exe another.ini
```

#### Priority of Configuration and Command Line Argument

The `CEnvBase::Feed()` method first reads in the command-line arguments
`argv[]` and then configures according to the command-line specified file
name, whoes lines are parsed again as a command-line arguments. If there is
also a `--config` option in the configuration file, it will no longer work.

Therefore the command line explicitly input options are at the higher priority
level. You can write a copy of common basic configurations in a file and then
temporarily overwrite some options in the command line when startup.

But this is only for the normal non-repeatable option.  If an option is set to
repeatable, when read argument again for the same option name, it is appended
to the option values that might have been read in on the command line before.
If there is any (after `--`) positional arguments in the configuration file,
they are also appended to the positional arguemnts directly input by the
command line.

Positional arguments promotation to option arguments are only occurs in the original input command-line arguments and do not act on the configuration file.
For example, according to the previous `#1 --input=` option setting:

```
// from bash
./exe 1.txt

// form config
input=11.txt

// in program
assert(input == "1.txt")
```

The positional argument `1.txt` entered on the command line is promoted to the
option parameter `--input`, which is also equivalent to being entered on the
command line, so this option is ignored in the configuration file where
otherwise read the option value of `11.txt`.

Note: As long as the option is non-repeatable, if they are entered repeatedly on the command line, then the first value of this option takes effect and ignores later values.

### Subcommand Arguments and Behavior Dispatch

#### Subcommand Handler

The library also supports the subcommand mode, where the first parameter is
specially handled and dispatched as a subcommand, with each subcommand
completing one category of business functions. You can use the
`CEnvBase::SubCommand()` method to add subcommands, for example:

```cpp
env.SubCommand("foo", "description test", subcmd_function)
   .SubCommand("bar", "description test", subcmd_object);
```

The first two parameters are the subcommand name and related description
(`--help` printable information), and the third parameter is the handle of the
subcommand, that there are two kinds of handlers to choose.

The first is a simple and straightforward function handler, which is defined
roughly as follows:

```cpp
int subcmd_function(int argc, const char* argv[], cli::CEnvBase* args)
{
    // make use of parsed argv in args to do stuff ...
    return 0;
}
```

The first two parameters are from `main()`, but make an offset correction to
skip the original first parameter. That is, the `argc` number amount is 1
less, and `argv[0]` is the subcommand name. In general, the first two
parameters may not be useful, as long as it can complete the business logic
with the `CEnvBase` object pointer in the third argument which has parsed
command-line arguments. But if any subcommands have completely different
option settings, especially with the same option has different meanings in
different subcommands, then the 'argv[]' command-line argument may need to be
re-parsed in the handle function.

The second handler for subcommand is an object of another `CEnvBase` or its
derived class. The functionality of this handler object is more abundant. It
is parsed by the respective subcommand-related `env` object (except for the
first subcommand name) of the remaining command lines arguments, and can
automatically respond to the `--help` after the subcommand. While in the
former handler function, there is only one related `env` object for  main
command to resolve all command line arguments.

#### Main Command Handler

Function handler to subcommands can also be registered to the main command (or
a single command program that does not design subcommands) with  the same
prototype, which can be automatically invoked after the `Feed()` function
parses the command-line arguments. The method is also similar:

```cpp
env.Command("program_name", "description test", cmd_function);
```

This feature can avoid inheriting and overriding the `Run()` method to some extent, and only need to use the base class object, and register different handler functions as demanded. Because now many developers are starting to oppose the abuse of inheritance mechanisms.
For example, different subcommands do not have to inherit many classes, and only use the base class to create different objects to handle the work of the subcommands:

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

Because the function handler of the main command is not essential, the
`Command()` method can have only the first two or even one argument. The first
parameter as the command name is displayed in the `--help` help message, but
if the command name is never set, the command line argument `argv[0]` is used
instead (possibly including the path). As long as use the `Command()` method
to register a handler function, the `env` object will no longer call its
`Run()` method.

#### Strict Subcommand Mode

By default, when an `env` object registers subcommands, it doesn't mean that
subcommands must be used. If the first argument cannot match any one
subcommand, then it can continue to parse all command-line arguments as a
childless program does 。 If you want to qualify that the first argument must
be a valid subcommand name, you can additionally call the `SubCommandOnly()`
method to enable strict subcommand mode, such as:

```cpp
env.SubCommand("foo", "description test", subcmd_function)
   .SubCommand("bar", "description test", subcmd_object)
   .SubCommandOnly();
```

At this point, when the first argument is an invalid subcommand name, the `Feed()` method returns a non-0 error code.

#### Nested Subcommands

Because the handler of a subcommand can be another `env` object, the
subcommand can theoretically still have another set of subcommands of its own,
and there is no limit to the depth of nesting. But in practice, most program
only use a layer of subcommands, such as the popular `git` and `docker`
command-line tools.

#### Subcommand Dispatched by Soft Link

Under Linux, you can associate multiple file names (command names) with a
program by creating a soft link (or hard link).  Yes, some programs can also
be launched in different ways and perform different tasks with the program
name `argv[0]`. e.g. `vim` has soft symbol link name of `view` and `vimdiff`.

This usage may be rare, and programs that dispatched by the first parameter
`argv[1]` as a subcommand are more popular , so this library also takes
precedence over this principle. However, it is also compatible with the
former, when cannot find the subcommand from `argv[1]`, it whill also try
again via `argv[0]` or `program_invocation_short_name`. Other than that, if
the client program has special needs, it is also possible to reconstruct
`argv[]` as needed and pass it on to `CEnvBase::Feed()`.

## Undefined Behavior

The library does not use exceptions, and many methods return `*this` for the convenience of chained calls without returning an error code.
Therefore, some cases where the usage is obviously unreasonable and the user should avoid it are not specially treated, so their behavior is not defined. Undefined means it depends on the implementation, the current implementation may be acceptable, but the logic is not guaranteed to be compatible with subsequent development.

Here is a list of some undefined behaviors (not necessarily exhaustive):

* Set the long option named empty
* Set long option name duplicated
* Set short option name duplicated
* Register subcommand named empty, or duplicated
* Get value of flag which option with no argument

## Code Reference

Before I developed this library, I had used the following library to parse
command-line arguments: 

* GNU `getopt` and `getopt_long`
* [CLI11](https://github.com/CLIUtils/CLI11)  an head only command line option lib.
