/** 
 * @file testalign.cpp
 * @author lymslive
 * @date 2022-05-11
 * @brief format to align lines of text, read stdin and write stdout.
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

    cli::CEnvBase env;
    env.Command("textalign", "format to align lines of text, operate on stdin and stdout")
        .Version("v0.1.0")
        .Set("-w --width=", "max width of each column", width)
        .Set("-p --padding=", "padding between columns", padding)
        .Feed(argc, argv);
    if (env.HasError())
    {
        return -1;
    }

    auto& Argv = env.Argv();
    if (Argv.empty())
    {
        return -1;
    }

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
