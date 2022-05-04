#include "tinytast.hpp"
#include "cliop.h"
#include "util-string.h"
#include "test-os.h"

std::ostream& operator<<(std::ostream& os, const cli::CArgument& stArgRecv)
{
    os << "\n" << stArgRecv.m_mapArgs << "\n" << stArgRecv.m_vecArgs;
    return os;
}

DEF_TAST(cliop_feed1, "test directlly feed argv")
{
    std::vector<std::string> vecArgs = 
    {"-abc", "--abc", "11", "--efg=xyz", "file1", "file2", "-g", "-i"};

    cli::CEnvBase env;
    env.Feed(vecArgs);

    const std::vector<std::string>& argv = env.Argv();
    COUT(argv.size(), 2);
    COUT(argv[0], "file1");
    COUT(argv[1], "file2");

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);

    COUT(stArgRecv.m_mapArgs.size(), 7);
    COUT(env.Has("a"), true);
    COUT(env.Has("b"), true);
    COUT(env.Has("c"), true);
    COUT(env.Has("abc"), true);
    COUT(env.Get("abc"), "11");
    int iArg = 0;
    COUT(env.Get("abc", iArg), true);
    COUT(iArg, 11);
    COUT(env.Get("efg"), "xyz");
    COUT(env.Has("g"), true);
    COUT(env.Has("i"), true);

    COUT(env.Has("G"), false);
    COUT(env.Has("ABC"), false);
}

DEF_TAST(cliop_feed2, "test feed argv having --")
{
    std::string cmdline = "-abc --abc 11 file1 - --efg=xyz -- file2 -g -i";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    COUT(vecArgs.size(), 10);
    cli::CEnvBase env;
    env.Feed(vecArgs);

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 5);
    COUT(stArgRecv.m_vecArgs[1], "-");
    COUT(stArgRecv.m_vecArgs[3], "-g");

    COUT(env.Get("abc"), "11");
    COUT(env.Get("efg"), "xyz");
    COUT(env.Has("g"), false);
    COUT(env.Has("i"), false);

    DESC("can also get by operator[]");
    COUT(env[2], "-");
    COUT(env[4], "-g");
    COUT(env["abc"], "11");
    COUT(env["efg"], "xyz");
}

DEF_TAST(cliop_option1_preset, "custome option setting")
{
    std::string cmdline = "-abc --abc 11 -g file1 - --efg=xyz -- file2 -i";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    COUT(vecArgs.size(), 10);

    bool bGood = false;
    cli::CEnvBase env;
    DESC("add flag or option settings");
    env.Flag('g', "good", "good flag", bGood).Flag('i', "inline", "inline flag");
    env.Option('a', "advance", "adv opt").Option('A', "abc", "abc option")
        .Option('e', "efg", "efg option").Option('E', "EFG", "EFG opt", "0default");

    DESC("bind option to variable");
    int iArg = 0;
    std::string strArg;
    env.Bind("abc", iArg);
    env.Bind("efg", strArg);

    env.Feed(vecArgs);

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 4);

    DESC("can use short or long name to get option argument");
    COUT(env.Has("a"), true);
    COUT(env.Has("good"), true);
    COUT(bGood, true);
    COUT(iArg, 11);
    COUT(strArg, "xyz");

    COUT(env.Has("b"), false);
    COUT(env.Has("c"), false);
    COUT(env.Has("advance"), true);
    COUT(env.Has("a"), true);
    COUT(env.Get("advance"), "bc");
    COUT(env.Has("A"), true);
    COUT(env.Get("abc"), "11");
    COUT(env.Get("A"), "11");

    DESC("can get default of option if not input from cmdline");
    COUT(env.Has("EFG"), false);
    COUT(env.Get("EFG"), "0default");
    COUT(env.Get("inline").empty(), true);
}

DEF_TAST(cliop_option2_required, "custome and check required option")
{
    std::string cmdline = "-abc --abc 11 -g --efg=xyz -- file1 file2";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    COUT(vecArgs.size(), 8);

    cli::CEnvBase env;
    env.Option('a', "advance", "adv opt", "", cli::OPTION_ARGUMENT | cli::OPTION_REQUIRED)
        .Option('e', "efg", "efg option", "", cli::OPTION_ARGUMENT | cli::OPTION_REQUIRED)
        .Option('g', "good", "good flag", "", cli::OPTION_ARGUMENT | cli::OPTION_REQUIRED)
        .Option('E', "EFG", "EFG opt", "0default")
        .Required('i', "inline", "inline text")
        .Required('A', "abc", "abc option");

    int nFeed = env.Feed(vecArgs);
    COUT(nFeed, 2);
    COUT(nFeed == 0, false);

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 2);

    COUT(env.Has("inline"), false);
    COUT(env.Has("good"), true);
}

DEF_TAST(cliop_option3_unexpected, "check unexpected option")
{
    std::string cmdline = "-abc --abc 11 -g --efg=xyz -- file1 file2";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    COUT(vecArgs.size(), 8);

    cli::CEnvBase env;
    env.Required('a', "advance", "adv opt");

    int nFeed = env.Feed(vecArgs);
    COUT(nFeed == 0, true);

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 2);

    COUT(env.Has("inline"), false);
    COUT(env.Has("good"), false);

    DESC("set strict parser");
    env.ClearArgument();
    nFeed = env.StrictParser().Feed(vecArgs);
    COUT(nFeed, 3);
    COUT(nFeed == 0, false);

    DESC("report error, but still save received argument");
    COUT(env.Has("g"), true);
    COUT(env.Has("good"), false);
    COUT(env.Has("efg"), true);
    COUT(env.Has("abc"), true);
}

DEF_TAST(cliop_option4_repeat, "test repeat option")
{
    std::string cmdline = "-abc -e1 -e2 --expr=3 --expr 4 -e 5 --advance BC";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    COUT(vecArgs.size(), 10);

    cli::CEnvBase env;
    env.Option('a', "advance", "adv opt")
        .Option('e', "expr", "multiple expression", "", cli::OPTION_ARGUMENT | cli::OPTION_REPEATED);

    int nFeed = env.Feed(vecArgs);
    COUT(nFeed == 0, true);

    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 0);
    COUT(env.Get("a"), "bc");
    COUT(env.Get("advance"), "bc");

    std::vector<int> vecExpr;
    COUT(env.Get("expr", vecExpr), true);
    COUT(vecExpr);
    COUT(vecExpr.size(), 5);
}

DEF_TAST(cliop_set1, "test common set option")
{
    cli::CEnvBase env;

    bool debug = false;
    std::string input;
    std::string output;
    std::string user;
    std::vector<std::string> topics;
    int level = -1;

    env.Set("--log.debug", "enable debug log", debug)
        .Set("--log.name=", "log name")
        .Set("-l --log.level=?", "log level", level)
        .Set("-z --log.size= [1024]", "log size")
        .Set("--log.topic=+", "log topics", topics)
        .Set("-i #1 --input=", "input file, or first argument", input)
        .Set("-o #2 --output=", "output file, or second argument", output)
        .Set("-u $USER --user=", "user name, can read environment", user)
        .Set("-h --help", "print this help information");

    std::string cmdline = "-h";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    int nFeed = env.Feed(vecArgs);
    COUT(nFeed, cli::ERROR_CODE_HELP);

    const cli::CArgument& stArgRecv = env.GetArgument();

    env.ClearArgument();
    vecArgs.clear();
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 1);

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "--log.debug -l2 --log.topic=foo --log.topic=bar -u self";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(env.Has("log.debug"), true);
    COUT(debug, true);
    COUT(env.Get("log.level"), "2");
    COUT(level, 2);
    COUT(env.Get("z"), "1024");
    COUT(user, "self");
    COUT(topics.size(), 2);
    COUT(topics[0], "foo");
    COUT(topics[1], "bar");

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "-l2 --input 1.txt --output 2.txt 3.txt";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(input, "1.txt");
    COUT(output, "2.txt");
    COUT(user);
    COUT(stArgRecv.m_vecArgs.size(), 1);

    DESC("read environment and position argument");
    setenv("USER", "lymslive", 1);

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "-l2 11.txt 22.txt 33.txt";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(env.Has("input"), true);
    COUT(env.Has("output"), true);
    COUT(input, "11.txt");
    COUT(output, "22.txt");
    COUT(user, "lymslive");
    COUT(stArgRecv.m_vecArgs.size(), 1);

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "-l2 -o222.txt 111.txt 333.txt";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(env.Has("input"), true);
    COUT(env.Has("output"), true);
    COUT(input, "111.txt");
    COUT(output, "222.txt");
    COUT(stArgRecv.m_vecArgs.size(), 1);

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "-l2 -i111.txt 222.txt 333.txt";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(env.Has("input"), true);
    COUT(env.Has("output"), false);
    COUT(input, "111.txt");
    COUT(output, "222.txt");
    COUT(stArgRecv.m_vecArgs.size(), 2);
}

// refer to: utest/test-config.ini
DEF_TAST(cliop_config1, "test read argument from config file")
{
    cli::CEnvBase env;
    env.Set("--log.debug=", "enable debug log")
        .Set("--log.name=", "log name")
        .Set("-l --log.level=?", "log level")
        .Set("-z --log.size= [1024]", "log size")
        .Set("--log.topic=+", "log topics")
        .Set("-i #1 --input=", "input file, or first argument")
        .Set("-o #2 --output=", "output file, or second argument")
        .Set("-u $USER --user=", "user name, can read environment")
        .Set("-h --help", "print this help information");

    const cli::CArgument& stArgRecv = env.GetArgument();

    std::string cmdline;
    std::vector<std::string> vecArgs;
    int nFeed = env.Feed(vecArgs);
    COUT(nFeed); // required option absent
    COUT(env.Has("config"), false);
    COUT(env.Get("config"));

    env.ClearArgument();
    vecArgs.clear();
    cmdline = "--config utest/test-config.ini";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(stArgRecv.m_mapArgs.size());

    COUT(env.Has("config"), true);
    COUT(env.Get("config"), "utest/test-config.ini");

    DESC("read option/argument from config file");
    COUT(env.Get("user"), "lymslive");
    COUT(env.Get("pass"), "123456");
    COUT(env.Get("port"), "8080");

    COUT(env.Has("a"), true);
    COUT(env.Has("b"), true);
    COUT(env.Has("c"), true);
    COUT(env.Get("this"), "1");
    COUT(env.Get("they"), "-1");
    COUT(env.Get("that"), "2");

    COUT(env.Get("log.debug"), "1");
    COUT(env.Get("log.name"), "utest");
    COUT(env.Get("log.level"), "2");
    COUT(env.Get("log.size"), "1024");
    std::vector<std::string> topics;
    COUT(env.Get("log.topic", topics), true);
    COUT_ASSERT(topics.size(), 2);
    COUT(topics[0], "foo");
    COUT(topics[1], "bar");

    COUT(env.Get("group.date"), "now");
    COUT(env.Get("group.author"), "you");

    // COUT(env.Get("input"), "input.txt");
    // COUT(env.Get("output"), "output.txt");
    DESC("argument from config file cannot move to option");
    COUT(env.Has("input"), false);
    COUT(env.Has("output"), false);

    COUT(stArgRecv.m_vecArgs.size(), 3);
    COUT(stArgRecv.m_vecArgs[0], "input.txt");
    COUT(stArgRecv.m_vecArgs[1], "output.txt");
    COUT(stArgRecv.m_vecArgs[2], "any-more.txt");

    DESC("cmdline option take precedence over from config");
    env.ClearArgument();
    vecArgs.clear();
    cmdline = "--config utest/test-config.ini --group.date=NOW --group.author=YOU";
    COUT(cmdline);
    util::SplitBySpace(cmdline, vecArgs);
    nFeed = env.Feed(vecArgs);
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(stArgRecv.m_mapArgs.size());
    COUT(env.Get("group.date"), "NOW");
    COUT(env.Get("group.author"), "YOU");
    COUT(env.Get("user"), "lymslive");
}

DEF_TAST(cliop_config2, "test use config as first argument")
{
    cli::CEnvBase env;
    env.Set("#1 --config=", "config file");

    const cli::CArgument& stArgRecv = env.GetArgument();

    std::string cmdline = "utest/test-config.ini --group.author=YOU first.txt";
    std::vector<std::string> vecArgs;
    COUT(cmdline);

    util::SplitBySpace(cmdline, vecArgs);
    int nFeed = env.Feed(vecArgs);

    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(stArgRecv.m_mapArgs.size());
    COUT(env.Has("config"), true);
    COUT(env.Get("config"), "utest/test-config.ini");
    COUT(env.Get("group.date"), "now");
    COUT(env.Get("group.author"), "YOU");
    COUT_ASSERT(stArgRecv.m_vecArgs.size(), 4);
    COUT(stArgRecv.m_vecArgs[0], "first.txt");
    COUT(stArgRecv.m_vecArgs[1], "input.txt");
    COUT(stArgRecv.m_vecArgs[2], "output.txt");
    COUT(stArgRecv.m_vecArgs[3], "any-more.txt");
}

struct CMyEnv : cli::CEnvBase
{
    bool debug;
    int count;
    double price;
    std::string name;

    std::vector<std::string> args;
    std::vector<int> Orders;

    CMyEnv()
    {
        BIND_OPTION(debug);
        BIND_OPTION(count);
        BIND_OPTION(price);
        BIND_OPTION(name);

        Bind("--", args);
        Set("--order=+", "oreder list", Orders);
    }

    int Run(int argc, const char* argv[]) override
    {
        DESC("call virtual Run in child env class");
        COUT(debug, true);
        COUT(count, 2);
        COUT(price, 3.14, 0.01);
        COUT(name, "water");
        COUT(Orders.size(), 2);
        COUT(args.size(), 3);
        return 11;
    }
};

DEF_TAST(cliop_bindst1, "test bind struct field")
{
    CMyEnv env;
    std::string cmdline = "--debug=1 --count 2 price=3.14 --name=water --order=1 --order=2 a1 a2 a3";
    COUT(cmdline);
    std::vector<std::string> vecArgs;
    util::SplitBySpace(cmdline, vecArgs);
    int nFeed = env.Feed(vecArgs);
    const cli::CArgument& stArgRecv = env.GetArgument();
    COUT(nFeed, 0);
    COUT(stArgRecv);
    COUT(stArgRecv.m_vecArgs.size(), 3);

    COUT(env.debug, true);
    COUT(env.count, 2);
    COUT(env.price, 3.14, 0.01);
    COUT(env.name, "water");
    COUT(env.Orders.size(), 2);
    COUT(env.Orders[0], 1);
    COUT(env.Orders[1], 2);
    COUT(env.args.size(), 3);
    COUT(env.args);
    COUT(env.args[2], "a3");

    DESC("feed with argc argv as main()");
    {
        CMyEnv env;
        int argc = vecArgs.size() + 1;
        std::vector<const char*> argv(argc+1, nullptr);
        argv[0] = "./exe";
        for (size_t i = 0; i < vecArgs.size(); ++i)
        {
            argv[i+1] = vecArgs[i].c_str();
        }

        nFeed = env.Feed(argc, &argv[0]);
        COUT(nFeed, 11);
    }
}
