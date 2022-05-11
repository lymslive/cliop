/** 
 * @file testalign.cpp
 * @author lymslive
 * @date 2022-05-11
 * @brief Format to align lines of text, read stdin and write stdout.
 * */
#include "cliop.h"
#include "util-string.h"
#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>

int main(int argc, const char* argv[])
{
    int width = 36;
    int padding = 1;

    // setup command
    cli::CEnvBase env;
    env.Command("textalign", "Format to align lines of text, operate on stdin and stdout. "
            "At least one argument to specify the beginning string of the second and following columns.")
        .Version("v0.1.0") // Command info anv Version is just optional
        .Set("-w --width=", "max width of each column", width)
        .Set("-p --padding=", "padding between columns", padding);

    // paser command line arguments
    int nFeed = env.Feed(argc, argv);
    if (nFeed == cli::ERROR_CODE_HELP || env.HasError())
    {
        return -1;
    }

    auto& Argv = env.Argv();
    if (Argv.empty())
    {
        fprintf(stderr, "Please input as least one argument!\n");
        env.Help();
        return -1;
    }

    // simple business stuff
    util::CTextAlign align(width, padding);
    std::string strLine;
    std::vector<std::string> rowLine;

    while (std::getline(std::cin, strLine))
    {
        size_t pos = 0;
        for (const auto& item : Argv)
        {
            size_t end = strLine.find(item, pos);
            if (end == std::string::npos)
            {
                break;
            }
            std::string column = strLine.substr(pos, end - pos);
            util::TrimRight(column);
            rowLine.push_back(std::move(column));
            pos = end;
        }
        if (pos < strLine.size())
        {
            rowLine.push_back(strLine.substr(pos));
        }
        align.AddLine(rowLine);
    }

    std::cout << align.GetText();

    return 0;
}
