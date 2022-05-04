/**
 * @file sum.cpp
 * @author lymslive
 * @date 2022-04-30
 * @brief Example to bind option to struct field.
 * @details Implement a command to add a list of int number, and can aslo use
 * to convert number to hex/oct base, see -h or --help.
 * */
#include "cliop.h"
#include <vector>
#include <stdio.h>
#include <string.h>

struct CSummer : public cli::CEnvBase
{
    std::string base;
    std::string oper;
    int init = 0;
    std::vector<int> nums;

    CSummer()
    {
        base = "dec";
        oper = "+";

        // BIND_OPTION(base);
        Bind("base", base);
        Bind("operator", oper);
        Bind("int", init);
        Bind("--", nums);
    }
};

int main(int argc, const char* argv[])
{
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        printf("./sum [--base=dec|hex|oct] ...\n");
        printf("    add a list of num, print result in base\n");
        printf("./sum [--base=dec|hex|oct] --int=1 --operator=* ...\n");
        printf("    multiply a list of num, print result in base\n");
        return 0;
    }

    CSummer env;
    env.Feed(argc, argv);

    int sum = env.init;
    for (size_t i = 0; i < env.nums.size(); ++i)
    {
        if (env.oper == "+")
        {
            sum += env.nums[i];
        }
        else if (env.oper == "*")
        {
            sum *= env.nums[i];
        }
    }

    if (env.base == "dec")
    {
        printf("%d\n", sum);
    }
    else if (env.base == "oct")
    {
        printf("%o\n", sum);
    }
    else if (env.base == "hex")
    {
        printf("%#x\n", sum);
    }

    return 0;
}

