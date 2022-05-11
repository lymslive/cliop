/**
 * @file math.cpp
 * @author lymslive
 * @date 2022-04-30
 * @brief Expample to implement sub-command
 * @details Simple math command, different operation is sub-command:
 * ./math add 2 3
 * ./math mul 2 3
 * to retrieve them.
 * */
#include "cliop.h"

namespace math
{

// way 1: sub-command handle function
// argc argv maybe useless, as already parsed in CEnvBase
int add(int argc, const char* argv[], cli::CEnvBase* args)
{
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int sum = a+b;
    std::string as = args->Get(1);
    std::string bs = args->Get(2);
    int sum2 = atoi(as.c_str()) + atoi(bs.c_str());
    // assert(sum == sum2);
    printf("%d\n", sum);
    return 0;
}

// way 2: sub-command handle class derived from CEnvBase
// override virtual method Run()
struct CMul : public cli::CEnvBase
{
    int left = 0;
    int right = 0;

    CMul()
    {
        // just bind won't generate help
        // BIND_OPTION(left);
        // BIND_OPTION(right);

        // can use option arugment or just position argument
        Set("-l #1 --left=?", "left operand", left);
        Set("-r #2 --right=?", "right operand", right);
    }

    virtual int Run(int argc, const char* argv[]) override
    {
        printf("%d\n", left * right);
        return 0;
    }
};

} // math

int main(int argc, char* argv[])
{
    math::CMul mulEnv;
    cli::CEnvBase env;
    env.Command("math", "basic math opertion")
        .SubCommand("add", "as operator+", math::add)
        .SubCommand("mul", "as operator*", mulEnv);
    return env.Feed(argc, argv);
}

namespace math
{

// another way: main command encapsulation
struct CBase : public cli::CEnvBase
{
    CBase()
    {
        Command("math", "basic math operation"); // for help only
        SubCommand("add", "as operator+", add);
        SubCommand("mul", "as operator*", m_mulEnv);
    }

private:
    CMul m_mulEnv;
};

// main just transfer feed to derived CEnvBase
int main(int argc, char* argv[])
{
    math::CBase env;
    return env.Feed(argc, argv);
}

} // math
