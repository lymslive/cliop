# Sample application: Command line tool

[中文版](readme-zh.md)

This directory provides some sample programs, which each is simple single `.cpp` source file that demonstrate how to write command-line tools using the `cliop` library, but also hope to be usefull more or less.

## argview

A utility that shows how the `cliop` library parses command-line arguments.  Typing `--help` prints predefined options, but also allows the user to enter arbitrary command-line arguments, which will truthfully print the received raw `argv[]` arguments, parsed option and positional arguments, and default arguments for preset options.

Feel free to try it. Note that the sample code uses several different methods to get the parameter values, and they should print the same results.

## sum

A simple number summing program that demonstrates how to receive and bind
command-line arguments and implement business logic accordingly. In addition
to summing, there is an additional ability to print results in hexadecimal or
octal, as well as a non-practical product function. For example:

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

Above, suppose that the command-line tool is started in this directory, the command line entered with the prefix `bash$`, and the next line is the output result.

## math

Demonstrates how to create a program tool with subcommand patterns that perform sum or product operations on two parameters based on subcommands entered by the user. It mainly demonstrates several implementation interfaces for subcommands, but also demonstrates how to bind positional parameters to option parameters. It can be used as follows:

```bash
bash $ ../bin/math add 5 6
11
bash $ ../bin/math mul 5 6
30
bash $ ../bin/math mul --left=5 --right=6
30
```

## textalign

A practical applet that aligns several lines of text entered from the standard input into columns according some specified substrings, and then prints to the standard output. Because `cliop` implements the function of automatically aligning lines of text when printing help messages, it can be easily called to finish the work of this applet. This directory also provides a sample file for the tool to run as follows:

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

If copy the tool to the `$PATH` search path, it can be configured as a filter tool for `vim` or other text editors, increasing the editing efficiency of alignment needs.
