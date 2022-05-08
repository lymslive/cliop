#include "tinytast.hpp"
#include "cliop.h"

DEF_TAST(error_setoption, "error in preset option")
{
    cli::CEnvBase env;
    env.Option('e', "", "empty long name");
    env.Option('e', "empty", "non-empty long name");
    env.Option(0, "empty", "with same long name");
    env.Option('+', "non-letter", "non-letter short name");
    env.Flag('e', "effect", "with same short name");

    DESC("by default no error occures");
    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};
    COUT(env.Feed(argc, argv), 0);

    DESC("option name is empty");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_INVALID).Flag('e', "", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_INVALID);
    }

    DESC("option name is invalid, begin with -");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_INVALID).Flag('e', "-empty", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_INVALID);
    }

    DESC("option name is invalid, has =");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_INVALID).Flag('e', "empty=on", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_INVALID);
    }

    DESC("option name is duplicated");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_REDEFINE)
            .Flag('\0', "empty", "empty long name")
            .Flag('e', "empty", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_REDEFINE);
    }

    DESC("option letter is invalid");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_FLAG_INVALID).Flag('+', "plus", "invalid letter");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_FLAG_INVALID);
    }

    DESC("option letter is redefined");
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_FLAG_REDEFINE).Flag('e', "empty", "").Option('e', "expr", "");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_FLAG_REDEFINE);
    }
}

DEF_TAST(error_catchmore, "catch more error at one time")
{
    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};

    cli::CEnvBase env1;
    env1.Catch(cli::ERROR_CODE_OPTION_INVALID)
        .Catch(cli::ERROR_CODE_OPTION_REDEFINE)
        .Catch(cli::ERROR_CODE_FLAG_INVALID)
        .Catch(cli::ERROR_CODE_FLAG_REDEFINE)
        .Option('e', "", "empty long name")
        .Option('e', "empty", "non-empty long name")
        .Option(0, "empty", "with same long name")
        .Option('+', "non-letter", "non-letter short name")
        .Flag('e', "effect", "with same short name");
    COUT(env1.Feed(argc, argv) != 0, true);

    int errors[] = {cli::ERROR_CODE_OPTION_INVALID, cli::ERROR_CODE_OPTION_REDEFINE, cli::ERROR_CODE_FLAG_INVALID, cli::ERROR_CODE_FLAG_REDEFINE};
    cli::CEnvBase env2;
    env2.Catch(errors, sizeof(errors)/sizeof(errors[0]))
        .Option('e', "empty", "non-empty long name")
        .Option(0, "empty", "with same long name")
        .Option('e', "", "empty long name")
        .Option('+', "non-letter", "non-letter short name")
        .Flag('e', "effect", "with same short name");
    COUT(env2.Feed(argc, argv) != 0, true);
}

DEF_TAST(error_catchall, "catch all error")
{
    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};

    cli::CEnvBase env;
    env.CatchAll()
        .Option('e', "empty", "non-empty long name")
        .Option(0, "empty", "with same long name")
        .Option('e', "", "empty long name")
        .Option('+', "non-letter", "non-letter short name")
        .Flag('e', "effect", "with same short name");
    COUT(env.Feed(argc, argv) != 0, true);

    DESC("ignore option name is empty");
    {
        cli::CEnvBase env;
        env.CatchAll().Ignore(cli::ERROR_CODE_OPTION_INVALID)
            .Flag('e', "", "empty long name");
        int nFeed = env.Feed(argc, argv);
        COUT(nFeed);
        COUT(nFeed != cli::ERROR_CODE_OPTION_INVALID, true);
    }

    DESC("ignore option letter is invalid");
    {
        cli::CEnvBase env;
        env.CatchAll()
            .Ignore(cli::ERROR_CODE_FLAG_INVALID).Ignore(cli::ERROR_CODE_FLAG_REDEFINE)
            .Flag('+', "plus", "invalid letter")
            .Flag('e', "empty", "").Option('e', "expr", "");
        int nFeed = env.Feed(argc, argv);
        COUT(nFeed);
        COUT(nFeed != cli::ERROR_CODE_FLAG_INVALID, true);
        COUT(nFeed != cli::ERROR_CODE_FLAG_REDEFINE, true);
    }
}

DEF_TAST(error_config_file, "test error for config file")
{
    DESC("CatchAll may report no config by default");
    cli::CEnvBase env;
    env.CatchAll();
    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};
    COUT(env.Feed(argc, argv), cli::ERROR_CODE_CONFIG_UNREADABLE);

    DESC("ignore non-existed config file");
    {
        cli::CEnvBase env;
        env.CatchAll().Ignore(cli::ERROR_CODE_CONFIG_UNREADABLE);
        int argc = 2;
        const char* argv[] = {"./exe", "file", nullptr};
        COUT(env.Feed(argc, argv), 0);
    }

    DESC("set --config default NONE");
    {
        cli::CEnvBase env;
        env.CatchAll().Set("--config= [NONE]", "not load config");
        int argc = 2;
        const char* argv[] = {"./exe", "file", nullptr};
        COUT(env.Feed(argc, argv), 0);
    }
}

DEF_TAST(error_config_line, "test error for config line")
{
    cli::CEnvBase env;
    int argc = 4;
    const char* argv[] = {"./exe", "--config", "utest/test-config.ini", "file", nullptr};
    COUT(env.Feed(argc, argv), 0);

    env.Catch(cli::ERROR_CODE_CONFIG_INVALID);
    COUT(env.Feed(argc, argv), cli::ERROR_CODE_CONFIG_INVALID);
}

DEF_TAST(error_argument1, "test argument absent in last option")
{
    int argc = 2;
    const char* argv[] = {"./exe", "--file", nullptr};

    DESC("--option has no argument followed");
    cli::CEnvBase env;
    env.Catch(cli::ERROR_CODE_OPTION_INCOMPLETE);
    COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_INCOMPLETE);

    DESC("--flag not need argument followed, no error");
    env.Flag('f', "file", "");
    env.ClearError();
    COUT(env.Feed(argc, argv), 0);
}

DEF_TAST(error_argument2, "test argument may confuse value")
{
    cli::CEnvBase env;
    env.Catch(cli::ERROR_CODE_ARGUMENT_INVALID);
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--file", "--option", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGUMENT_INVALID);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--file", "-flag", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGUMENT_INVALID);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--file", "opt=arg", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGUMENT_INVALID);
    }
}

DEF_TAST(error_argument3, "test error of option in argument input")
{
    cli::CEnvBase env;
    env.CatchAll().Ignore(cli::ERROR_CODE_CONFIG_UNREADABLE)
        .Set("-r --required=?", "");
    {
        int argc = 3;
        const char* argv[] = {"./exe", "file", "output", nullptr};
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_REQUIRED);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "-rfile", "output", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "-rfile", "-flag", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_UNKNOWN);
    }
}

DEF_TAST(error_subcmd_name, "test error on subcommand name")
{
    cli::CEnvBase env1, env2;
    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_SUBCMD_INVALID)
            .SubCommand("cmd1", "", env1).SubCommand("cmd2", "", env2);
        COUT(env.Feed(argc, argv), 0);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_SUBCMD_INVALID)
            .SubCommand("", "description", env1).SubCommand("cmd2", "", env2);
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_SUBCMD_INVALID);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_SUBCMD_INVALID)
            .SubCommand("-cmd1", "description", env1).SubCommand("cmd2", "", env2);
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_SUBCMD_INVALID);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_SUBCMD_INVALID)
            .SubCommand("cmd=1", "description", env1).SubCommand("cmd2", "", env2);
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_SUBCMD_INVALID);
    }
}

DEF_TAST(error_subcmd, "test error on subcommand")
{
    cli::CEnvBase env, env1, env2;
    env.SubCommand("cmd1", "", env1).SubCommand("cmd2", "", env2)
        .Catch(cli::ERROR_CODE_COMMAND_UNKNOWN);

    DESC("on subcommand report error");
    {
        int argc = 3;
        const char* argv[] = {"./exe", "cmd", "file", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_COMMAND_UNKNOWN);
    }

    DESC("has subcommand in argv[1]");
    {
        int argc = 3;
        const char* argv[] = {"./exe", "cmd1", "file", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "cmd2", "file", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }

    DESC("has subcommand in argv[0]");
    {
        int argc = 3;
        const char* argv[] = {"cmd1", "--", "file", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
    {
        int argc = 3;
        const char* argv[] = {"cmd2", "--", "file", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
}

DEF_TAST(error_bind_position, "test postion argument bind index")
{
    int argc = 4;
    const char* argv[] = {"./exe", "file", "file2", "file3", nullptr};
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_POSITION_BIND)
            .Set("#10 --bind=", "description");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_POSITION_BIND);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_POSITION_BIND)
            .Set("#1 --input=", "description")
            .Set("#1 --output=", "description");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_POSITION_BIND);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_POSITION_BIND)
            .Set("#1 --input=", "description")
            .Set("#2 --output=", "description");
        COUT(env.Feed(argc, argv), 0);
    }
}

DEF_TAST(error_bind, "test unmatch bound type of argument")
{
    int iArg = 0;
    double fArg = 0;
    std::vector<int> viArg;
    std::vector<double> vfArg;

    cli::CEnvBase env;
    env.Catch(cli::ERROR_CODE_ARGTYPE_UNMATCH)
        .Set("--int=", "scalar int", iArg)
        .Set("--float=", "scalar double", fArg)
        .Set("--Int=+", "vector int", viArg)
        .Set("--Float=+", "vector double", vfArg);

    int argc = 7;
    const char* argv[] = {"./exe", "--int=1", "--float=1.0", "--Int=-1", "--Int=-2", "--Float=-1.0", "--Float=-2.0", nullptr};
    COUT(env.Feed(argc, argv), 0);
    COUT(iArg, 1);
    COUT(fArg, 1.0, 0.1);
    COUT(viArg.size(), 2);
    COUT(vfArg.size(), 2);
    COUT(viArg[1], -2);
    COUT(vfArg[1], -2.0, 0.1);

    {
        int argc = 2;
        const char* argv[] = {"./exe", "--int=1f", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 2;
        const char* argv[] = {"./exe", "--int=+1", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 2;
        const char* argv[] = {"./exe", "--int=0x01abcdef", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--Int=1", "--Int=1.0", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 2;
        const char* argv[] = {"./exe", "--float=x1", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 2;
        const char* argv[] = {"./exe", "--float=3.14e2", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--Float=1", "--Float=1.0", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--Float=1.0.1", "--Float=1.0", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), 0);
    }
    {
        int argc = 3;
        const char* argv[] = {"./exe", "--Float=1x", "--Float=1.0", nullptr};
        env.ClearError();
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_ARGTYPE_UNMATCH);
    }
}

static int s_myError = 0;
void my_error_report(int code, const std::string& text)
{
    s_myError = code;
    fprintf(stderr, "MY-E%d: %s\n", code, text.c_str());
}

DEF_TAST(error_report, "test custom error report handle")
{
    auto save = cli::SetErrorHandler(my_error_report);

    int argc = 2;
    const char* argv[] = {"./exe", "file", nullptr};
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_INVALID).Flag('e', "", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_INVALID);
        COUT(s_myError, cli::ERROR_CODE_OPTION_INVALID);
    }
    {
        cli::CEnvBase env;
        env.Catch(cli::ERROR_CODE_OPTION_REDEFINE)
            .Flag('\0', "empty", "empty long name")
            .Flag('e', "empty", "empty long name");
        COUT(env.Feed(argc, argv), cli::ERROR_CODE_OPTION_REDEFINE);
        COUT(s_myError, cli::ERROR_CODE_OPTION_REDEFINE);
    }

    cli::SetErrorHandler(save);
}
