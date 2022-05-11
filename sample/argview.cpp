/**
 * @file argview.cpp
 * @author lymslive
 * @date 2022-04-30
 * @brief Expample for the default behavior to parse commnad line argument and how
 * to retrieve them.
 * */
#include "cliop.h"
#include <vector>
#include <string>
#include <stdio.h>

void set_options(cli::CEnvBase& env);

int main(int argc, const char* argv[])
{
    printf("raw input arguments:\n");
    printf("\targc = %d\n", argc);
    for (int i = 0; i < argc; ++i)
    {
        printf("\targv[%d] = %s\n", i, argv[i]);
    }

    cli::CEnvBase env;
    env.Command("argview", "Examine what command line arguments received.");
    // set option is not neccessary, only effect parse the specified option
    set_options(env);

    int nFeed = env.Feed(argc, argv);
    if (nFeed != 0)
    {
        return nFeed;
    }

    const std::vector<std::string>& Argv = env.Argv();
    printf("position arguments:\n");
    for (int i = 0; i < Argv.size(); ++i)
    {
        // ways to get the same argument
        std::string arg = Argv[i];
        std::string arg2 = env.Get(i+1);
        std::string arg3 = env[i+1];
        printf("\t%s\t%s\t%s\n", arg.c_str(), arg2.c_str(), arg3.c_str());
    }

    const std::map<std::string, std::string>& Args = env.Args();
    printf("option arguments:\n");
    for (auto& item : Args)
    {
        std::string key = item.first;
        std::string val = item.second;
        std::string val2 = env.Get(key);
        std::string val3 = env[key];
        printf("\t%-15s:\t%s\t%s\t%s\n", key.c_str(), val.c_str(), val2.c_str(), val3.c_str());
    }

    printf("option defualt argument:\n");
    if (!env.Has("default"))
    {
        printf("\tdefault = %s\n", env.Get("default").c_str());
    }
    if (!env.Has("Default"))
    {
        printf("\tDefault = %s\n", env.Get("Default").c_str());
    }
    if (!env.Has("user"))
    {
        printf("\tuser = %s\n", env.Get("user").c_str());
    }

    if (env.Has("expr"))
    {
        printf("option allowed repeated has multiple value\n");
        printf("\texpr = ");
        std::vector<std::string> expr;
        env.Get("expr", expr);
        for (int i = 0; i < expr.size(); ++i)
        {
            printf("%s, ", expr[i].c_str());
        }
        printf("\n");
    }

    return 0;
}

void set_options(cli::CEnvBase& env)
{
    // Flag specify no argument, Option has argument
    // Set is the common magic method to set option
    env.Flag('d', "debug", "enbale debug mode")
        .Option('v', "verbose", "verbose level")
        .Option(0, "default", "option that has default val", "val")
        .Set("-D --Debug", "same as Flag -d")
        .Set("-V --Verbose=", "same as Option -v")
        .Set("--Default= [Val]", "option that has default val")
        .Set("-u $USER --user=", "user can be from environment")
        .Set("-e --expr=+", "express can be repeated provided")
        .Set("-i #1 --input=", "input file name")
        .Set("-o #2 --output=", "output file name")
        ;

    // requried option must be provided, otherwise env.Feed() return error
    // env.Required('-r', "required", "required option example");
    // env.Set("-r --required=?", "also required option example");
}
