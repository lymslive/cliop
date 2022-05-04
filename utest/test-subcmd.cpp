#include "tinytast.hpp"
#include "cliop.h"

namespace math
{

int add(int argc, const char* argv[], cli::CEnvBase* args)
{
    COUT(argv[0], std::string("add"));
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int sum = a+b;
    std::string as = args->Get(1);
    std::string bs = args->Get(2);
    int sum2 = atoi(as.c_str()) + atoi(bs.c_str());
    COUT(sum, sum2);
    return sum;
}

int mul(int argc, const char* argv[], cli::CEnvBase* args)
{
    COUT(argv[0], std::string("mul"));
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int prod = a*b;
    std::string as = args->Get(1);
    std::string bs = args->Get(2);
    int prod2 = atoi(as.c_str()) * atoi(bs.c_str());
    COUT(prod, prod2);
    return prod;
}

struct CAdd : public cli::CEnvBase
{
    int left = 0;
    int right = 0;

    CAdd()
    {
        BIND_OPTION(left);
        BIND_OPTION(right);
    }

    virtual int Run(int argc, const char* argv[]) override
    {
        COUT(argv[0], std::string("add"));
        return left + right;
    }
};

struct CMul : public cli::CEnvBase
{
    int left = 0;
    int right = 0;

    CMul()
    {
        BIND_OPTION(left);
        BIND_OPTION(right);
    }

    virtual int Run(int argc, const char* argv[]) override
    {
        COUT(argv[0], std::string("mul"));
        return left * right;
    }
};

struct CBase : public cli::CEnvBase
{
    CBase()
    {
        Command("math", "basic operation"); // for help only
        SubCommand("add", "as operator+", m_addEnv);
        SubCommand("mul", "as operator*", m_mulEnv);
    }

private:
    CAdd m_addEnv;
    CMul m_mulEnv;
};

} // math

DEF_TAST(subcmd_func, "test sub commmand with function handler")
{
    cli::CEnvBase env;
    env.Command("math", "basic operation")
        .SubCommand("add", "as operator+", math::add)
        .SubCommand("mul", "as operator*", math::mul);

    DESC("./math --version");
    {
        int argc = 2;
        const char* argv[] = {"./math", "--version", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_HELP);
    }

    DESC("./math --help");
    {
        env.ClearArgument();
        int argc = 2;
        const char* argv[] = {"./math", "--help", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_HELP);
    }

    DESC("./math add 2 3");
    {
        env.ClearArgument();
        int argc = 4;
        const char* argv[] = {"./math", "add", "2", "3", nullptr};
        COUT(env.Feed(argc, argv), 5);
    }

    DESC("./math mul 2 3");
    {
        env.ClearArgument();
        int argc = 4;
        const char* argv[] = {"./math", "mul", "2", "3", nullptr};
        COUT(env.Feed(argc, argv), 6);
    }
}

DEF_TAST(subcmd_object, "test sub commmand with Env object handler")
{

    math::CBase env;
    DESC("./math --help");
    {
        env.ClearArgument();
        int argc = 2;
        const char* argv[] = {"./math", "--help", nullptr};
        env.Feed(argc, argv);
    }

    DESC("./math add --left=2 --right=3");
    {
        env.ClearArgument();
        int argc = 4;
        const char* argv[] = {"./math", "add", "--left=2", "--right=3", nullptr};
        COUT(env.Feed(argc, argv), 5);
    }

    DESC("./math mul --left=2 --right=3");
    {
        env.ClearArgument();
        int argc = 4;
        const char* argv[] = {"./math", "mul", "--left=2", "--right=3", nullptr};
        COUT(env.Feed(argc, argv), 6);
    }
}

DEF_TAST(subcmd_invalid, "test invalid sub commmand")
{
    math::CBase env;
    env.SubCommandOnly();

    DESC("./math --help");
    {
        env.ClearArgument();
        int argc = 2;
        const char* argv[] = {"./math", "--help", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_HELP);
    }

    DESC("./math div");
    {
        env.ClearArgument();
        int argc = 4;
        const char* argv[] = {"./math", "div", "9", "3", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_COMMAND_INVALID);
    }
}
