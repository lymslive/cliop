/**
 * @file cliop.h
 * @author lymslive
 * @date 2022-04-25
 * @brief lib to deal with command line option argument.
 * */
#ifndef CLIOP_H__
#define CLIOP_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>

namespace cli
{

/** Error code definition. */
enum ErrorCode
{
    ERROR_CODE_HELP = 0xABCDE0,    //< for --help or --version
    ERROR_CODE_COMMAND_UNSUPPORTED,//< invalid command in argv[1]
    ERROR_CODE_OPTION_INCOMPLETE,  //< no argument for the last option
    ERROR_CODE_CONFIG_UNREADABLE,  //< can not read config file
    ERROR_CODE_CONFIG_INVALID,     //< config line may confuse or invalid
    ERROR_CODE_ARGUMENT_INVALID,   //< argument may confuse or invalid
    ERROR_CODE_ARGTYPE_UNMATCH,    //< argument bound type is unmatch
    ERROR_CODE_OPTION_REQUIRED,    //< required option absent
    ERROR_CODE_OPTION_UNKNOWN,     //< unexpected option encountered

    ERROR_CODE_OPTION_EMPTY,       //< option long name is empty
    ERROR_CODE_OPTION_REDEFINE,    //< option name is redefined
    ERROR_CODE_FLAG_INVALID,       //< flag letter invalid
    ERROR_CODE_FLAG_REDEFINE,      //< flag letter is redefined

    ERROR_CODE_END
};

/** Error handler function type.
 * @param [IN] code: the error code.
 * @param [IN] text: the error description.
 * */
typedef std::function<void (int code, const std::string& text)> FErrorHandler;

/** Set a new error handler function, return the old one.
 * @note The default error handle only printf to stderr, if no one provided.
 * */
FErrorHandler SetErrorHandler(FErrorHandler fn);

/** Error code in runtime. */
class CErrorRun
{
    std::set<int> m_setCatch;      //< only catch these errors
    std::string m_strContext;      //< extra context information for m_nError
    int m_nError = 0;              //< current or last error code

public:
    /** Get last error code. */
    int Code() { return m_nError; }

    /** Get error string for last, basic description appended by contex. */
    std::string String();

    /** Convertion to bool, error 0 means true. */
    operator bool() { return m_nError == 0; }

    /** Convertion to int, which is the error code. */
    operator int()  { return Code(); }

    /** Convertion to string, the description for error. */
    operator std::string() { return String(); }

    /** Catch the specified error code in later process. */
    void CatchError(int code) { m_setCatch.insert(code); }

    /** Ignore the specified error code in later process. */
    void IgnoreError(int code) { m_setCatch.erase(code); }

    /** Check wheter will catch the error specified code. */
    bool IsCatch(int code) { return m_setCatch.count(code) > 0; }

    /** Set error code with context information. */
    void SetError(int code, const std::string& context = "");
};

/** Option Attribute Constant: the option has following argument. */
const int OPTION_ARGUMENT = 1;
/** Option Attribute Constant: the option is requried that must provided. */
const int OPTION_REQUIRED = 2;
/** Option Attribute Constant: the option can provide multiple times. */
const int OPTION_REPEATED = 4;

/** Data struct for an option information. */
struct COption
{
    char m_cShortName = '\0';     //< short name: -n
    bool m_bArgument = false;     //< has argument followed: -n arg
    bool m_bRequired = false;     //< must provided option
    bool m_bRepeated = false;     //< allow multiple times: -e1 -e2
    int m_iBindIndex = 0;         //< can also appear in position argument
    std::string m_strLongName;    //< long name: --LongName
    std::string m_strDescription; //< description text in help usage
    std::string m_strDefault;     //< default argument value if not provided
    std::string m_strEnvName;     //< can also read from environment variable

    // constructor, 0-3 arguments, most important is long name
    COption() {}
    COption(const std::string& strLongName);
    COption(const std::string& strLongName, const std::string strDescription);
    COption(char cShortName, const std::string& strLongName, const std::string strDescription);
};

/** The argument struct that actually read from cmdline. */
struct CArgument
{
    /// Option and their argument saved in map, with long name as key.
    /// @note The value of flag that has no argument is unimportant.
    std::map<std::string, std::string> m_mapArgs;

    /// other position argument saved in vector
    std::vector<std::string> m_vecArgs;
};

/** The type of option argument value. */
enum EOptionType
{
    OPTION_BOOL = 0,    //< flag that has no argument just used as bool
    OPTION_STR = 1,     //< argument used as st::string
    OPTION_INT = 2,     //< argument used as int
    OPTION_DOUBLE = 3,  //< argument used as double

    OPTION_STR_LIST = 11,    //< used as std::vector<std::string>
    OPTION_INT_LIST = 12,    //< used as std::vector<int>
    OPTION_DOUBLE_LIST = 13, //< used as std::vector<double>
};

/** Convert string to other type value. */
void ConvertValue(const std::string& src, int &dest);
void ConvertValue(const std::string& src, double &dest);
void ConvertValue(const std::string& src, std::vector<std::string> &dest);
void ConvertValue(const std::string& src, std::vector<int> &dest);
void ConvertValue(const std::string& src, std::vector<double> &dest);

/** Information for option bind with variable. */
struct COptionBind
{
    EOptionType m_eValueType = OPTION_STR;
    void* m_pBindValue = nullptr;

    COptionBind(EOptionType eValueType = OPTION_STR, void* pBindValue = nullptr)
        : m_eValueType(eValueType), m_pBindValue(pBindValue) {}
};

class CEnvBase;

/** Command handle function type.
 * @param [IN] argc: argument count as main()
 * @param [IN] argv: argument vector as main()
 * @param [IN] args: parsed arguemnt object pointer.
 * @return int: error code
 * */
typedef std::function<int (int argc, const char* argv[], CEnvBase* args)> FCommandHandler;

/** Command data collection */
struct CommandInfo
{
    std::string m_strName;        //< command name
    std::string m_strDescription; //< command help description text
    FCommandHandler m_fnHandler;  //< simple handle function without object
    CEnvBase* m_pEnvBase = nullptr;   //< command handle object

    CommandInfo() {}
    CommandInfo(const std::string& strName, const std::string& strDescription)
        : m_strName(strName), m_strDescription(strDescription) {}
};

/** Manage option and argument from cmdline, as environment to start up program. */
class CEnvBase
{
    std::vector<COption> m_vecOptions; //< option setup
    std::map<std::string, COptionBind> m_mapBind; //< option bind
    CArgument m_stArgRecv;             //< actually received option and argument

    std::string m_strVersion;          //< command version
    CommandInfo m_stCommand;           //< command detail and handle
    std::vector<CommandInfo> m_vecCommand; //< sub-commands
    CommandInfo* m_pSubCommand = nullptr; //< current sub-command

    CErrorRun m_stError;     //< runtime error, only save the last one

public:

    /** Dummy function to deal with argument.
     * @note argc and argv is from main(), may useless as already parsed in this
     * object.
     * */
    virtual int Run(int argc, const char* argv[]) { return 0; }

    /** read vector of string as cmdline.
     * @param [IN] vecArgs: cmdline argument stored in vector.
     * @return int: error code 0 for success
     * @note The vector is usually split from cmdline that separated by space,
     * where each item is an option or argument, may contain space if orinally
     * embed by escape or quote.
     * @note The vector not contain argv[0], please use additional method @ref
     * Command() to set command name if needed.
     * */
    int Feed(const std::vector<std::string>& vecArgs);

    /** read raw cmdline, typically from main().
     * @param [IN] argc: argument count, including program name as arv[0]
     * @param [IN] argv: C-Style string array for all argument
     * @return int: error code 0 for success
     * @note Real arguments is in range argv[1 ... argc-1], and argv[argc] is null.
     * @note The first argv[0] is name of command or program.
     * */
    int Feed(int argc, char* argv[]);
    int Feed(int argc, const char* argv[]);

    /** Check where the option is read in. */
    bool Has(const std::string& strOptionName);

    /** Get the option argument as raw string, may GetDefault if not provided.
     * For special "--", get all position arguments joined by '\0'.
     * Usually get by long name, but can also by short name, eg. "g" not 'g'
     * */
    std::string Get(const std::string& strOptionName);

    /** Get the default argument or environment value from option setup. */
    std::string GetDefault(const std::string& strOptionName);

    /** Get postion argument by index, based from 1.
     * @note Get(0) return all position arguments as Get("--")
     * @note When pos exceed range, return empty.
     * */
    std::string Get(size_t pos);

    /** operator[] performed as Get(). */
    std::string operator[](size_t pos) { return Get(pos); }
    std::string operator[](const std::string& name) { return Get(name); }

    /** Get the option argument as expected type value.
     * @param [IN] strOptionName: the long name of option.
     * @param [IN] pos: get postion argument
     * @param [OUT] bArg: set true if option has read in or has default.
     * @param [OUT] strArg: set to the raw string argument.
     * @param [OUT] outArg: convet argument to other type.
     * @return bool: true if option read or has default value, same as bArg.
     * */
    bool Get(const std::string& strOptionName, bool& bArg);
    bool Get(const std::string& strOptionName, std::string& strArg);
    bool Get(size_t pos, std::string& strArg);
    template <typename keyT, typename valueT>
    bool Get(keyT key, valueT& outArg)
    {
        std::string strArg = Get(key);
        if (!strArg.empty())
        {
            ConvertValue(strArg, outArg);
            return true;
        }
        return false;
    }

    /** Get the all option arguments in map of string. */
    const std::map<std::string, std::string>& Args() { return m_stArgRecv.m_mapArgs; }

    /** Get the all position arguments, as vector of string. */
    const std::vector<std::string>& Argv() { return m_stArgRecv.m_vecArgs; }

    /** Get the count of position arguments. */
    int Argc() { return Argv().size(); }

    /** Get the command name, similar as argv[0] but may not same. */
    const std::string& Arg0() { return m_stCommand.m_strName; }

    /** Get the actually received argument struct, with a map and vector.*/
    const CArgument& GetArgument() { return m_stArgRecv; }

    /** Set and get version string. */
    CEnvBase& Version(const std::string& strVersion);
    std::string Version() const;

    /** Set main command.
     * @param [IN] strName: command name
     * @param [IN] strDescription: some reading text
     * @param [IN] fnHandler: command handler function
     * @return this
     * @note If command function is set, will auto called after feed argv.
     * */
    CEnvBase& Command(const std::string& strName);
    CEnvBase& Command(const std::string& strName, const std::string& strDescription);
    CEnvBase& Command(const std::string& strName, const std::string& strDescription, FCommandHandler fnHandler);

    /** Set sub command.
     * @param [IN] strName: command name
     * @param [IN] strDescription: some reading text
     * @param [IN] fnHandler: command handler function
     * @param [IN] pEnvBase: command handler object
     * @return this
     * @note Can only have one of `pEnvBase` or `fnHandler` to handle the sub-
     * command, `fnHandler` will receive parent CEnvBase that parsed argument, while 
     * `pEnvBase` will parse arguemnt by itself.
     * */
    CEnvBase& SubCommand(const std::string& strName, const std::string& strDescription, FCommandHandler fnHandler);
    CEnvBase& SubCommand(const std::string& strName, const std::string& strDescription, CEnvBase& stEnvBase);

    /** Set strict sub-command mode, check argv[1] must be valid command. */
    CEnvBase& SubCommandOnly();

    /** Set use strict parser, only allow option already set. */
    CEnvBase& SetOptionOnly();

    /** Add a pre-build option, return self. */
    CEnvBase& AddOption(const COption& stOption);

    /** Add a simple flag to option-setting.
     * @param [IN] cShortName: short name as -s
     * @param [IN] strLongName: long name as --LongName
     * @param [IN] strDescription: any meaningful text
     * @param [IN] bBindValue: auto bind variable
     * @return *this, self object.
     * @note A flag is special simple option that has no argument followed,
     * and so it can only bind with bool variable, to mark if it is provided.
     * */
    CEnvBase& Flag(char cShortName, const std::string& strLongName, const std::string& strDescription);
    CEnvBase& Flag(char cShortName, const std::string& strLongName, const std::string& strDescription, bool& bBindValue);

    /** Add an option setup.
     * @param [IN] cShortName: short name as -s, can be 0 for unspecified
     * @param [IN] strLongName: long name as --LongName, must not empty
     * @param [IN] strDescription: any meaningful text
     * @param [IN] strDefault: default value if not read in the option
     * @param [IN] uBitorAttr: extra attribute, may bit or of  
     *   OPTION_ARGUMENT | OPTION_REQUIRED | OPTION_REPEATED
     * @return *this, self object.
     * @note Option() differ from Flag() that it is usually has argument, but
     * can also add flag by explicitly pass uBitorAttr=0, and ignore arbitrary
     * `strDefault`.
     * @note Option argument may bind to different type variable, call extra
     * @ref Bind() if wanted, or mannualy call @ref Get() when needed.
     * */
    CEnvBase& Option(char cShortName, const std::string& strLongName, const std::string& strDescription);
    CEnvBase& Option(char cShortName, const std::string& strLongName, const std::string& strDescription, const std::string& strDefault, int uBitorAttr = OPTION_ARGUMENT);

    /** Add a required option.
     * @note Parameters are the same as Option(), and no default. */
    CEnvBase& Required(char cShortName, const std::string& strLongName, const std::string& strDescription);

    /** Common setup an option.
     * @param [IN] strName: simplly option name or complex with many attribute.
     * @param [IN] strDescription: any human reading text
     * @param [IN] refVal: auto bind option argument to the variable
     * @return *this, self object.
     * @note `strName` can combine with words for short name, long name and
     * other option attribute, much as the syntax for help information:
     *   -n #1 $ENV_NAME --LongName=?+ [default-value]
     *   Where `-` mark short name, `--` mark long name, `=` mark has argument,
     *   `?` mark required and `+` mark allowed repeated option,
     *   `#` mark bind index of position argument, `$` mark environment variable,
     *   then `[]` mark default value. Order is unimportant.
     *   The most common used may be `--LongName=` to setup an option.
     * @note supported bind value type @ref Bind() overrides.
     * */
    CEnvBase& Set(const std::string& strName, const std::string& strDescription);

    template <typename valueT>
    CEnvBase& Set(const std::string& strName, const std::string& strDescription, valueT& refVal)
    {
        Set(strName, strDescription);
        COption& stOption = m_vecOptions.back();
        return Bind(stOption.m_strLongName, refVal);
    }

    /** Bind an option with variable.
     * @param [IN] strOptionName: the long name of option.
     * @param [IN] refVal: bind option argument received to the variable
     * @return *this self object.
     * @note The lifetime of bind variable should long enouth compare to this.
     * @note Should call before parse cmdline.
     * */
    CEnvBase& Bind(const std::string& strOptionName, bool& refVal);
    CEnvBase& Bind(const std::string& strOptionName, std::string& refVal);
    CEnvBase& Bind(const std::string& strOptionName, int& refVal);
    CEnvBase& Bind(const std::string& strOptionName, double& refVal);
    CEnvBase& Bind(const std::string& strOptionName, std::vector<std::string>& refVal);
    CEnvBase& Bind(const std::string& strOptionName, std::vector<int>& refVal);
    CEnvBase& Bind(const std::string& strOptionName, std::vector<double>& refVal);

    /** Clear received argument, may be called before another Feed(); */
    void ClearArgument();

    void ClearError();

    /** Check if has any error. */
    bool HasError() { return !m_stError; }

    /** Catch one error code.
     * @param [IN] code: one error code, @ref enum ErrorCode
     * @return *this
     * */
    CEnvBase& Catch(int code);

    /** Catch an array of error code.
     * @param [IN] code: non-null pointer to an array
     * @param [IN] size: the size of array
     * @return *this
     * */
    CEnvBase& Catch(int* code, int size);

    /** Catch all error code. */
    CEnvBase& CatchAll();

    /** Ingnore one error code. */
    CEnvBase& Ignore(int code);

    /** Ingore an array of error code. */
    CEnvBase& Ignore(int* code, int size);

private:
    /** Parse cmdline .
     * @param [IN] vecArgs: cmdline argument stored in vector.
     * @param [IN] pos: parse argument from pos, default parese all.
     * @return int: error code 0 for success.
     * @note When without any option setting,
     * any short name consider as flag with no argument, and long name
     * has argument. After read cmdline, flag can also Get() by short name,
     * since donnot known it's corresponding long name.
     * @note Setup options and sub-command if any before call this when possible.
     * @note May parse from pos 1 for sub-command.
     * @note @ref Feed() whill call this.
     * */
    int ParseCmdline(const std::vector<std::string>& vecArgs, size_t pos = 0);

    /** Resolve all bind variable.
     * @note must called after Feed() or Prase method.
     * */
    void GetBind();

    /** Find option setting by name. */
    COption* FindOption(char cShortName);
    COption* FindOption(const std::string& strLongName);

    /** Find sub command by name. */
    CommandInfo* FindCommand(const std::string& strName);

    /** Find sub command from argv[1] or argv[0].
     * If argv[1] available sub command, `iShift` is set to 1.
     * */
    CommandInfo* FindCommand(int argc, const char* argv[], int& iShift);

    /** Save received position argument. */
    void SaveArgument(const std::string& strArg);

    /** Save received option argument or flag without argument.
     * Non-repeated option can only be save once, the first once take effect.
     * */
    void SaveOption(char cShortName);
    void SaveOption(const std::string& strLongName, const std::string& strArg);
    void SaveOption(const COption& stOption);
    void SaveOption(const COption& stOption, const std::string& strArg);

    /** Check if the argument for option in valid. */
    bool CheckOptionArgument(const std::string& strArg);

    /** Check if all required options are provided. */
    bool CheckRequiredOption();

    /** Check if all option have been setup. */
    bool CheckUnknownOption();

    /** Move some postion argument to option argument as setup.
     * Only move the first several continuous argument(s) to option if they
     * are not provided explicitly. Only move real cmdline argument, but not
     * thar read from config file, as you should edit config file clearly.
     * */
    void MoveArgument();

    /** Read option/argument lines from config file.
     * @param [IN] strFile: config file path
     * @param [OUT] cfgArgs: save each line as a cmdline option or argument
     * @details
     * Normal cmdline arguments are separated by space, while
     * arguments in config file are separate by line. The config file is
     * compatible with ini file, where each `key=val` is just treated as
     * `--key=val` in cmdline. The section `[group]` is prefix of each key
     * under this group, as `--group.key=val`, but section is not necessary.
     * Config file can has commet lines begin with `#` or `;`, and space are
     * allowed around `=` or begin or end of line. The lines without `=` are
     * considered as normal argument except section line.
     * Special `--` or `[--]` is used to marked the following lines are all
     * normal argument except comment line.
     * @note If cannot read config file, just silent ignore.
     * @note Also skip read any config if specify --config=NONE
     * */
    void ReadConfig(const std::string& strFile, std::vector<std::string>& cfgArgs);

    /** reserved option: --help --config --version. */
    void ReservedOption();

    /** Print usage or help text. */
    void Help();
    void HelpVersion();

public:
    /** Generate usage help text. */
    virtual void Usage(std::string& outText);
};

} /* cli */ 

/** Bind option to field of derived class of CEnvBase. */
#define BIND_OPTION(field) Bind(#field, field)

#endif /* end of include guard: CLIOP_H__ */
