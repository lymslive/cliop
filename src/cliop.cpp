#include "cliop.h"
#include <stdlib.h>
#include <cassert>
#include <errno.h>
#include <fstream>
#include "util-string.h"

namespace cli
{

const char* OPTION_NAME_HELP = "help";
const char* OPTION_NAME_CONFIG = "config";
const char* OPTION_NAME_VERSION = "version";

static FErrorHandler s_fnErrorReporter;
FErrorHandler SetErrorHandler(FErrorHandler fn)
{
    FErrorHandler old = s_fnErrorReporter;
    s_fnErrorReporter = fn;
    return old;
}

static void ReportError(int code, const std::string& text)
{
    if (s_fnErrorReporter)
    {
        return s_fnErrorReporter(code, text);
    }
    fprintf(stderr, "E%d: %s\n", code, text.c_str());
}

// invalid argument, option name or subcommand name
inline bool IsInvalidArgument(const std::string& strArg)
{
    return (strArg.empty() || strArg[0] == '-' || strArg.find('=') != std::string::npos);
}

struct CErrorTips
{
    std::map<int, std::string> m_mapTips;

    CErrorTips()
    {
        m_mapTips[ERROR_CODE_HELP] = "ony show information as demanded";
        m_mapTips[ERROR_CODE_COMMAND_UNKNOWN] = "unsupported command";
        m_mapTips[ERROR_CODE_OPTION_INCOMPLETE] = "no argument for the last option";
        m_mapTips[ERROR_CODE_CONFIG_UNREADABLE] = "can't read config file";
        m_mapTips[ERROR_CODE_CONFIG_INVALID] = "config line may confuse or invalid";
        m_mapTips[ERROR_CODE_ARGUMENT_INVALID] = "argument may confuse or invalid";
        m_mapTips[ERROR_CODE_ARGTYPE_UNMATCH] = "argument bound type is unmatch";
        m_mapTips[ERROR_CODE_POSITION_BIND] = "position argument bound index mistake";
        m_mapTips[ERROR_CODE_OPTION_REQUIRED] = "required option absent";
        m_mapTips[ERROR_CODE_OPTION_UNKNOWN] = "unexpected option encountered";

        m_mapTips[ERROR_CODE_OPTION_INVALID] = "option name may confuse or invalid";
        m_mapTips[ERROR_CODE_OPTION_REDEFINE] = "option name is redefined";
        m_mapTips[ERROR_CODE_FLAG_INVALID] = "flag letter invalid";
        m_mapTips[ERROR_CODE_FLAG_REDEFINE] = "flag letter is redefined";

        m_mapTips[ERROR_CODE_SUBCMD_INVALID] = "sub-command name may confuse or invalid";
        m_mapTips[ERROR_CODE_SUBCMD_REDEFINE] = "sub-command name is redefined";
    }
};

std::string CErrorRun::String()
{
    static CErrorTips s_text;
    auto it = s_text.m_mapTips.find(m_nError);
    if (it == s_text.m_mapTips.end())
    {
        return "";
    }
    std::string str = it->second;
    if (!m_strContext.empty())
    {
        str.append(": ").append(m_strContext);
    }
    return str;
}

void CErrorRun::SetError(int code, const std::string& context)
{
    if (code == 0)
    {
        m_nError = 0;
        m_strContext.clear();
    }
    else if (IsCatch(code))
    {
        m_nError = code;
        m_strContext = context;
        ReportError(Code(), String());
    }
}

void ConvertValue(const std::string& src, int &dest)
{
    dest = atoi(src.c_str());
}
void ConvertValue(const std::string& src, double &dest)
{
    dest = atof(src.c_str());
}
void ConvertValue(const std::string& src, std::vector<std::string> &dest)
{
    util::SplitByNull(src, dest);
}
void ConvertValue(const std::string& src, std::vector<int> &dest)
{
    std::vector<std::string> vecSplit;
    util::SplitByNull(src, vecSplit);
    for (auto it = vecSplit.begin(); it != vecSplit.end(); ++it)
    {
        dest.push_back(atoi(it->c_str()));
    }
}
void ConvertValue(const std::string& src, std::vector<double> &dest)
{
    std::vector<std::string> vecSplit;
    util::SplitByNull(src, vecSplit);
    for (auto it = vecSplit.begin(); it != vecSplit.end(); ++it)
    {
        dest.push_back(atof(it->c_str()));
    }
}

COption::COption(const std::string& strLongName)
    : m_strLongName(strLongName) {}
COption::COption(const std::string& strLongName, const std::string strDescription)
    : m_strLongName(strLongName), m_strDescription(strDescription) {}
COption::COption(char cShortName, const std::string& strLongName, const std::string strDescription)
    : m_cShortName(cShortName), m_strLongName(strLongName), m_strDescription(strDescription) {}

#define CHECK_ERROR do { if (!m_stError) return m_stError.Code(); } while(0)

int CEnvBase::Feed(const std::vector<std::string>& vecArgs)
{
    ClearArgument();

    ParseCmdline(vecArgs); CHECK_ERROR;

    if(Has(OPTION_NAME_HELP) && m_pSubCommand == nullptr)
    {
        Help();
        return ERROR_CODE_HELP;
    }
    if(Has(OPTION_NAME_VERSION) && m_pSubCommand == nullptr)
    {
        HelpVersion();
        return ERROR_CODE_HELP;
    }

    MoveArgument(); CHECK_ERROR;

    // read config file
    std::vector<std::string> cfgArgs;
    std::string strFile = Get(OPTION_NAME_CONFIG);
    if (strFile != "NONE")
    {
        ReadConfig(strFile, cfgArgs); CHECK_ERROR;
        ParseCmdline(cfgArgs); CHECK_ERROR;
    }

    GetBind(); CHECK_ERROR;

    if (!CheckRequiredOption())
    {
        return ERROR_CODE_OPTION_REQUIRED;
    }

    if (m_stError.IsCatch(ERROR_CODE_OPTION_UNKNOWN) && !CheckUnknownOption())
    {
        return ERROR_CODE_OPTION_UNKNOWN;
    }

    return 0;
}

int CEnvBase::Feed(int argc, char* argv[])
{
    return Feed(argc, const_cast<const char**>(argv));
}

int CEnvBase::Feed(int argc, const char* argv[])
{
    if (argc <= 0 || argv == nullptr)
    {
        return -1;
    }

    CHECK_ERROR;

    if (m_stCommand.m_strName.empty())
    {
        Command(argv[0]);
    }

    int iShift = 0;
    m_pSubCommand = FindCommand(argc, argv, iShift);

    if (m_pSubCommand && m_pSubCommand->m_pEnvBase)
    {
        return m_pSubCommand->m_pEnvBase->Feed(argc-iShift, argv+iShift);
    }

    std::vector<std::string> vecArgs;
    for (int i = iShift + 1; i < argc && argv[i] != nullptr; ++i)
    {
        vecArgs.push_back(argv[i]);
    }

    int nRet = Feed(vecArgs);
    if (nRet != 0)
    {
        return nRet;
    }

    if (!m_pSubCommand && !m_vecCommand.empty() && m_stError.IsCatch(ERROR_CODE_COMMAND_UNKNOWN))
    {
        m_stError.SetError(ERROR_CODE_COMMAND_UNKNOWN, argc > 1 ? argv[1] : argv[0]);
        return ERROR_CODE_COMMAND_UNKNOWN;
    }

    if (m_pSubCommand && m_pSubCommand->m_fnHandler)
    {
        nRet = m_pSubCommand->m_fnHandler(argc-iShift, argv+iShift, this);
    }
    else if (m_stCommand.m_fnHandler)
    {
        nRet = m_stCommand.m_fnHandler(argc, argv, this);
    }
    else
    {
        nRet = Run(argc, argv);
    }

    return nRet;
}

int CEnvBase::ParseCmdline(const std::vector<std::string>& vecArgs, size_t pos)
{
    ReservedOption();
    bool bEndOption = false;
    COption* pLastOption = nullptr;
    std::string strLastOption;
    for (size_t i = pos; i < vecArgs.size(); ++i)
    {
        if (vecArgs[i].empty())
        {
            continue;
        }
        if (vecArgs[i] == "--")
        {
            bEndOption = true;
            continue;
        }
        if (bEndOption)
        {
            SaveArgument(vecArgs[i]);
            continue;
        }

        if (pLastOption != nullptr)
        {
            CheckOptionArgument(vecArgs[i]);
            SaveOption(*pLastOption, vecArgs[i]);
            pLastOption = nullptr;
            continue;
        }
        else if(!strLastOption.empty())
        {
            CheckOptionArgument(vecArgs[i]);
            SaveOption(strLastOption, vecArgs[i]);
            strLastOption.clear();
            continue;
        }
        CHECK_ERROR;

        std::string strArgTemp = vecArgs[i];
        size_t iDash= util::TrimLeft(strArgTemp, '-');
        size_t iEqual = strArgTemp.find('=');
        if (iEqual != std::string::npos)
        {
            // --LongName=argument; even no leading - or empty after =
            std::string strOpt = strArgTemp.substr(0, iEqual);
            std::string strArg = strArgTemp.substr(iEqual + 1);
            COption* pOption = FindOption(strOpt);
            if (pOption != nullptr)
            {
                SaveOption(*pOption, strArg);
            }
            else
            {
                SaveOption(strOpt, strArg);
            }
            continue;
        }

        if (iDash == 0 || iDash == vecArgs[i].size())
        {
            SaveArgument(vecArgs[i]);
        }
        else if (iDash == 1)
        {
            // -flags : "lags" may argument or other falgs
            for (size_t index = iDash; index < vecArgs[i].size(); ++index)
            {
                char cOpt = vecArgs[i][index];
                COption* pOption = FindOption(cOpt);
                if (pOption != nullptr)
                {
                    if (pOption->m_bArgument)
                    {
                        if (index + 1 == vecArgs[i].size())
                        {
                            pLastOption = pOption;
                        }
                        else
                        {
                            SaveOption(*pOption, vecArgs[i].substr(index + 1));
                        }
                        break;
                    }
                    else
                    {
                        SaveOption(*pOption);
                    }
                }
                else
                {
                    SaveOption(cOpt);
                }
            }
        }
        else
        {
            // --LongName
            COption* pOption = FindOption(strArgTemp);
            if (pOption != nullptr)
            {
                if (pOption->m_bArgument)
                {
                    pLastOption = pOption;
                }
                else
                {
                    SaveOption(*pOption);
                }
            }
            else
            {
                strLastOption = strArgTemp;
            }
        }
    }

    if (pLastOption != nullptr || !strLastOption.empty())
    {
        if (m_stError.IsCatch(ERROR_CODE_OPTION_INCOMPLETE))
        {
            m_stError.SetError(ERROR_CODE_OPTION_INCOMPLETE,
                    pLastOption != nullptr ? pLastOption->m_strLongName : strLastOption);
            return ERROR_CODE_OPTION_INCOMPLETE;
        }
    }
    return 0;
}

bool CEnvBase::Has(const std::string& strOptionName)
{
    if (strOptionName.empty())
    {
        return false;
    }

    bool bArg = m_stArgRecv.m_mapArgs.count(strOptionName) > 0;
    if (!bArg && strOptionName.size() == 1)
    {
        COption* pOption = FindOption(strOptionName[0]);
        if (pOption != nullptr)
        {
            bArg = Has(pOption->m_strLongName);
        }
    }
    return bArg;
}

std::string CEnvBase::Get(const std::string& strOptionName)
{
    if (strOptionName == "--")
    {
        std::string strJoin;
        util::Join(m_stArgRecv.m_vecArgs, strJoin, '\0');
        return strJoin;
    }

    if (strOptionName.empty())
    {
        return "";
    }

    auto it = m_stArgRecv.m_mapArgs.find(strOptionName);
    if (it != m_stArgRecv.m_mapArgs.end())
    {
        return it->second;
    }

    std::string strArg = GetDefault(strOptionName);
    if (strArg.empty() && strOptionName.size() == 1)
    {
        COption* pOption = FindOption(strOptionName[0]);
        if (pOption != nullptr)
        {
            strArg = Get(pOption->m_strLongName);
        }
    }

    return strArg;
}

std::string CEnvBase::GetDefault(const std::string& strOptionName)
{
    std::string strArg;
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (strOptionName == it->m_strLongName)
        {
            if (!it->m_strEnvName.empty())
            {
                strArg = getenv(it->m_strEnvName.c_str());
            }
            if (strArg.empty())
            {
                strArg = it->m_strDefault;
            }
            break;
        }
    }
    return strArg;
}

std::string CEnvBase::Get(size_t pos)
{
    if (pos == 0)
    {
        return Get("--");
    }
    if (pos > m_stArgRecv.m_vecArgs.size())
    {
        return "";
    }
    return m_stArgRecv.m_vecArgs[pos-1];
}

bool CEnvBase::Get(const std::string& strOptionName, bool& bArg)
{
    bArg = Has(strOptionName);
    return bArg;
}

bool CEnvBase::Get(const std::string& strOptionName, std::string& strArg)
{
    std::string strGet = Get(strOptionName);
    if (!strGet.empty())
    {
        strArg = strGet;
        return true;
    }
    return false;
}

bool CEnvBase::Get(size_t pos, std::string& strArg)
{
    std::string strGet = Get(pos);
    if (!strGet.empty())
    {
        strArg = strGet;
        return true;
    }
    return false;
}

CEnvBase& CEnvBase::Version(const std::string& strVersion)
{
    m_strVersion = strVersion;
    return *this;
}

std::string CEnvBase::Version() const
{
    return m_strVersion;
}

CEnvBase& CEnvBase::Command(const std::string& strName)
{
    m_stCommand.m_strName = strName;
    return *this;
}

CEnvBase& CEnvBase::Command(const std::string& strName, const std::string& strDescription)
{
    m_stCommand.m_strName = strName;
    m_stCommand.m_strDescription = strDescription;
    return *this;
}

CEnvBase& CEnvBase::Command(const std::string& strName, const std::string& strDescription, FCommandHandler fnHandler)
{
    m_stCommand.m_strName = strName;
    m_stCommand.m_strDescription = strDescription;
    m_stCommand.m_fnHandler = fnHandler;
    return *this;
}

CEnvBase& CEnvBase::SubCommand(const std::string& strName, const std::string& strDescription, FCommandHandler fnHandler)
{
    CommandInfo stCommand(strName, strDescription);
    stCommand.m_fnHandler = fnHandler;
    return AddCommand(stCommand);
}

CEnvBase& CEnvBase::SubCommand(const std::string& strName, const std::string& strDescription, CEnvBase& stEnvBase)
{
    CommandInfo stCommand(strName, strDescription);
    stCommand.m_pEnvBase = &stEnvBase;
    stCommand.m_pEnvBase->Command(strName, strDescription);
    return AddCommand(stCommand);
}

CEnvBase& CEnvBase::AddCommand(const CommandInfo& stCommand)
{
    if (m_stError.IsCatch(ERROR_CODE_SUBCMD_INVALID) && IsInvalidArgument(stCommand.m_strName))
    {
        m_stError.SetError(ERROR_CODE_SUBCMD_INVALID, stCommand.m_strName);
        return *this;
    }

    if (m_stError.IsCatch(ERROR_CODE_SUBCMD_REDEFINE) && FindCommand(stCommand.m_strName) != nullptr)
    {
        m_stError.SetError(ERROR_CODE_SUBCMD_REDEFINE, stCommand.m_strName);
        return *this;
    }

    m_vecCommand.push_back(stCommand);
    return *this;
}

CEnvBase& CEnvBase::SubCommandOnly()
{
    Catch(ERROR_CODE_COMMAND_UNKNOWN);
    return *this;
}

CEnvBase& CEnvBase::SetOptionOnly()
{
    Catch(ERROR_CODE_OPTION_UNKNOWN);
    return *this;
}

CEnvBase& CEnvBase::AddOption(const COption& stOption)
{
    if (m_stError.IsCatch(ERROR_CODE_OPTION_INVALID) && IsInvalidArgument(stOption.m_strLongName))
    {
        m_stError.SetError(ERROR_CODE_OPTION_INVALID, stOption.m_strLongName);
        return *this;
    }

    if (m_stError.IsCatch(ERROR_CODE_OPTION_REDEFINE) && FindOption(stOption.m_strLongName) != nullptr)
    {
        m_stError.SetError(ERROR_CODE_OPTION_REDEFINE, stOption.m_strLongName);
        return *this;
    }

    if (m_stError.IsCatch(ERROR_CODE_FLAG_INVALID))
    {
        char c = stOption.m_cShortName;
        if (c != '\0' && !(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z'))
        {
            m_stError.SetError(ERROR_CODE_FLAG_INVALID, std::string("char%").append(std::to_string((int)c)));
            return *this;
        }
    }

    if (m_stError.IsCatch(ERROR_CODE_FLAG_REDEFINE))
    {
        char c = stOption.m_cShortName;
        if (c != '\0' && FindOption(c) != nullptr)
        {
            m_stError.SetError(ERROR_CODE_FLAG_REDEFINE, std::string(1, c));
            return *this;
        }
    }

    m_vecOptions.push_back(stOption);
    return *this;
}

CEnvBase& CEnvBase::Flag(char cShortName, const std::string& strLongName, const std::string& strDescription)
{
    COption stOption(cShortName, strLongName, strDescription);
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Flag(char cShortName, const std::string& strLongName, const std::string& strDescription, bool& bBindValue)
{
    COption stOption(cShortName, strLongName, strDescription);
    Bind(strLongName, bBindValue);
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Option(char cShortName, const std::string& strLongName, const std::string& strDescription)
{
    COption stOption(cShortName, strLongName, strDescription);
    stOption.m_bArgument = true;
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Option(char cShortName, const std::string& strLongName, const std::string& strDescription, const std::string& strDefault, int uBitorAttr /*= OPTION_ARGUMENT*/)
{
    COption stOption(cShortName, strLongName, strDescription);
    stOption.m_strDefault = strDefault;
    stOption.m_bArgument = OPTION_ARGUMENT & uBitorAttr;
    stOption.m_bRequired = OPTION_REQUIRED & uBitorAttr;
    stOption.m_bRepeated = OPTION_REPEATED & uBitorAttr;
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Required(char cShortName, const std::string& strLongName, const std::string& strDescription)
{
    COption stOption(cShortName, strLongName, strDescription);
    stOption.m_bArgument = true;
    stOption.m_bRequired = true;
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Set(const std::string& strName, const std::string& strDescription)
{
    COption stOption;
    stOption.m_strDescription = strDescription;

    std::vector<std::string> words;
    util::SplitBySpace(strName, words);

    char cShortName = 0;
    int iBindIndex = 0;
    std::string strLongName;
    std::string strEnvName;
    std::string strDefault;
    for (auto it = words.begin(); it != words.end(); ++it)
    {
        if (it->size() < 2)
        {
            continue;
        }
        if (it->size() == 2 && (*it)[0] == '-' && (*it)[1] != '-')
        {
            cShortName = (*it)[1];
        }
        else if ((*it)[0] == '#')
        {
            iBindIndex = atoi(it->c_str() + 1);
        }
        else if ((*it)[0] == '$')
        {
            strEnvName = it->substr(1);
        }
        else if (it->front() == '[' && it->back() == ']')
        {
            strDefault = it->substr(1, it->size() - 2);
        }
        else if (it->size() > 2 && (*it)[0] == '-' && (*it)[1] == '-')
        {
            size_t iEqual = it->find('=');
            if (iEqual != std::string::npos)
            {
                stOption.m_bArgument = true;
                strLongName = it->substr(2, iEqual - 2);
                std::string strAttr = it->substr(iEqual + 1);
                if (!strAttr.empty())
                {
                    if (strAttr.find('?') != std::string::npos)
                    {
                        stOption.m_bRequired = true;
                    }
                    if (strAttr.find('+') != std::string::npos)
                    {
                        stOption.m_bRepeated = true;
                    }
                }
            }
            else
            {
                stOption.m_bArgument = false;
                strLongName = it->substr(2);
            }
        }
    }

    stOption.m_cShortName = cShortName;
    stOption.m_strLongName = strLongName;
    stOption.m_strEnvName = strEnvName;
    stOption.m_strDefault = strDefault;
    stOption.m_iBindIndex = iBindIndex;
    return AddOption(stOption);
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, bool& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_BOOL, &refVal);
    return *this;
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, std::string& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_STR, &refVal);
    return *this;
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, int& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_INT, &refVal);
    return *this;
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, double& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_DOUBLE, &refVal);
    return *this;
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, std::vector<std::string>& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_STR_LIST, &refVal);
    return *this;
}

CEnvBase& CEnvBase::Bind(const std::string& strOptionName, std::vector<int>& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_INT_LIST, &refVal);
    return *this;
}
CEnvBase& CEnvBase::Bind(const std::string& strOptionName, std::vector<double>& refVal)
{
    m_mapBind[strOptionName] = COptionBind(OPTION_DOUBLE_LIST, &refVal);
    return *this;
}

void CEnvBase::GetBind()
{
    for (auto it = m_mapBind.begin(); it != m_mapBind.end(); ++it)
    {
        if (it->second.m_pBindValue == nullptr)
        {
            continue;
        }

        EOptionType eType = it->second.m_eValueType;
        if (eType == OPTION_BOOL)
        {
            Get(it->first, *(static_cast<bool*>(it->second.m_pBindValue)));
            continue;
        }

        std::string strArg = Get(it->first);
        if (strArg.empty())
        {
            continue;
        }

        if (m_stError.IsCatch(ERROR_CODE_ARGTYPE_UNMATCH))
        {
            // only roughly detect int or double
            if (eType == OPTION_INT || eType == OPTION_INT_LIST)
            {
                if (strArg.find_first_not_of("0123456789\0-", 0, 12) != std::string::npos)
                {
                    m_stError.SetError(ERROR_CODE_ARGTYPE_UNMATCH, strArg);
                    return;
                }
            }
            else if (eType == OPTION_DOUBLE || eType == OPTION_DOUBLE_LIST)
            {
                if (strArg.find_first_not_of("0123456789\0-.", 0, 13) != std::string::npos)
                {
                    m_stError.SetError(ERROR_CODE_ARGTYPE_UNMATCH, strArg);
                    return;
                }
            }
        }

        if (eType == OPTION_STR)
        {
            *(static_cast<std::string*>(it->second.m_pBindValue)) = strArg;
        }
        else if (eType == OPTION_INT)
        {
            ConvertValue(strArg, *(static_cast<int*>(it->second.m_pBindValue)));
        }
        else if (eType == OPTION_DOUBLE)
        {
            ConvertValue(strArg, *(static_cast<double*>(it->second.m_pBindValue)));
        }
        else if (eType == OPTION_STR_LIST)
        {
            ConvertValue(strArg, *(static_cast<std::vector<std::string>*>(it->second.m_pBindValue)));
        }
        else if (eType == OPTION_INT_LIST)
        {
            ConvertValue(strArg, *(static_cast<std::vector<int>*>(it->second.m_pBindValue)));
        }
        else if (eType == OPTION_DOUBLE_LIST)
        {
            ConvertValue(strArg, *(static_cast<std::vector<double>*>(it->second.m_pBindValue)));
        }
    }
}

void CEnvBase::ClearArgument()
{
    m_stArgRecv.m_mapArgs.clear();
    m_stArgRecv.m_vecArgs.clear();
}

void CEnvBase::ClearError()
{
    m_stError.SetError(0);
}

CEnvBase& CEnvBase::Catch(int code)
{
    m_stError.CatchError(code);
    return *this;
}

CEnvBase& CEnvBase::Catch(int* code, int size)
{
    for (int i = 0; i < size; ++i)
    {
        m_stError.CatchError(code[i]);
    }
    return *this;
}

CEnvBase& CEnvBase::CatchAll()
{
    for (int i = ERROR_CODE_HELP; i < ERROR_CODE_END; ++i)
    {
        m_stError.CatchError(i);
    }
    return *this;
}

CEnvBase& CEnvBase::Ignore(int code)
{
    m_stError.IgnoreError(code);
    return *this;
}

CEnvBase& CEnvBase::Ignore(int* code, int size)
{
    for (int i = 0; i < size; ++i)
    {
        m_stError.IgnoreError(code[i]);
    }
    return *this;
}

COption* CEnvBase::FindOption(char cShortName)
{
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_cShortName == cShortName)
        {
            return &(*it);
        }
    }
    return nullptr;
}

COption* CEnvBase::FindOption(const std::string& strLongName)
{
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_strLongName == strLongName)
        {
            return &(*it);
        }
    }
    return nullptr;
}

CommandInfo* CEnvBase::FindCommand(const std::string& strName)
{
    for (size_t i = 0; i < m_vecCommand.size(); ++i)
    {
        if (m_vecCommand[i].m_strName == strName)
        {
            return &(m_vecCommand[i]);
        }
    }
    return nullptr;
}

CommandInfo* CEnvBase::FindCommand(int argc, const char* argv[], int& iShift)
{
    if (m_vecCommand.empty())
    {
        return nullptr;
    }

    CommandInfo* pSubCommand = nullptr;
    if (1 < argc && argv[1] != nullptr)
    {
        pSubCommand = FindCommand(argv[1]);
        if (pSubCommand != nullptr)
        {
            iShift = 1;
        }
    }
    if (pSubCommand == nullptr)
    {
        pSubCommand = FindCommand(argv[0]);
    }
    if (pSubCommand == nullptr)
    {
        pSubCommand = FindCommand(program_invocation_short_name);
    }

    return pSubCommand;
}

void CEnvBase::SaveArgument(const std::string& strArg)
{
    m_stArgRecv.m_vecArgs.push_back(strArg);
}

void CEnvBase::SaveOption(char cShortName)
{
    std::string strShortName(1, cShortName);
    m_stArgRecv.m_mapArgs[strShortName] = "1";
}

void CEnvBase::SaveOption(const std::string& strLongName, const std::string& strArg)
{
    if (m_stArgRecv.m_mapArgs.count(strLongName) == 0)
    {
        m_stArgRecv.m_mapArgs[strLongName] = strArg;
    }
}

void CEnvBase::SaveOption(const COption& stOption)
{
    assert(!stOption.m_bArgument);
    m_stArgRecv.m_mapArgs[stOption.m_strLongName] = "1";
}

void CEnvBase::SaveOption(const COption& stOption, const std::string& strArg)
{
    assert(stOption.m_bArgument);
    if (!stOption.m_bRepeated)
    {
        if (m_stArgRecv.m_mapArgs.count(stOption.m_strLongName) == 0)
        {
            m_stArgRecv.m_mapArgs[stOption.m_strLongName] = strArg;
        }
    }
    else
    {
        std::string& strOldArg = m_stArgRecv.m_mapArgs[stOption.m_strLongName];
        if (!strOldArg.empty())
        {
            strOldArg.append(1, '\0');
        }
        strOldArg.append(strArg);
    }
}

bool CEnvBase::CheckOptionArgument(const std::string& strArg)
{
    if (m_stError.IsCatch(ERROR_CODE_ARGUMENT_INVALID) && IsInvalidArgument(strArg))
    {
        m_stError.SetError(ERROR_CODE_ARGUMENT_INVALID, strArg);
        return false;
    }
    return true;
}

bool CEnvBase::CheckRequiredOption()
{
    int nCount = 0;
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_bRequired && m_stArgRecv.m_mapArgs.count(it->m_strLongName) == 0)
        {
            m_stError.SetError(ERROR_CODE_OPTION_REQUIRED, it->m_strLongName);
            return false;
        }
    }
    return true;
}

bool CEnvBase::CheckUnknownOption()
{
    for (auto it = m_stArgRecv.m_mapArgs.begin(); it != m_stArgRecv.m_mapArgs.end(); ++it)
    {
        if (nullptr == FindOption(it->first))
        {
            m_stError.SetError(ERROR_CODE_OPTION_UNKNOWN, it->first);
            return false;
        }
    }
    return true;
}

void CEnvBase::MoveArgument()
{
    if (m_stArgRecv.m_vecArgs.empty())
    {
        return;
    }

    size_t nSize = m_stArgRecv.m_vecArgs.size();
    if (nSize > m_vecOptions.size())
    {
        nSize = m_vecOptions.size();
    }

    // collect COption* whose m_iBindIndex is set
    int iMaxIndex = 0;
    std::vector<COption*> vecOptions(nSize, nullptr);
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_iBindIndex == 0)
        {
            continue;
        }
        if (it->m_iBindIndex > 0 && it->m_iBindIndex <= nSize)
        {
            if (vecOptions[it->m_iBindIndex - 1] != nullptr && m_stError.IsCatch(ERROR_CODE_POSITION_BIND))
            {
                std::string strText = "redefined of ";
                strText.append(it->m_strLongName).append("#").append(std::to_string(it->m_iBindIndex));
                m_stError.SetError(ERROR_CODE_POSITION_BIND, strText);
                return;
            }

            vecOptions[it->m_iBindIndex - 1] = &(*it);
            if (iMaxIndex < it->m_iBindIndex)
            {
                iMaxIndex = it->m_iBindIndex;
            }
        }
        else if(m_stError.IsCatch(ERROR_CODE_POSITION_BIND))
        {
            std::string strText = "beyond range of ";
            strText.append(it->m_strLongName).append("#").append(std::to_string(it->m_iBindIndex));
            m_stError.SetError(ERROR_CODE_POSITION_BIND, strText);
            return;
        }
    }

    if(m_stError.IsCatch(ERROR_CODE_POSITION_BIND))
    {
        for (int i = iMaxIndex - 1; i >= 0; --i)
        {
            if (vecOptions[i-1] == nullptr)
            {
                std::string strText = "no preposition bound index #";
                strText += std::to_string(i);
                m_stError.SetError(ERROR_CODE_POSITION_BIND, strText);
                return;
            }
        }
    }

    // move from #1 #2 ... in order
    size_t nMoved = 0;
    for (auto it = vecOptions.begin(); it != vecOptions.end(); ++it)
    {
        if (*it == nullptr)
        {
            break;
        }
        if (!Has((*it)->m_strLongName))
        {
            SaveOption(**it, m_stArgRecv.m_vecArgs[nMoved]);
            nMoved++;
        }
        else
        {
            break;
        }
    }

    if (nMoved > 0)
    {
        auto first = m_stArgRecv.m_vecArgs.begin();
        m_stArgRecv.m_vecArgs.erase(first, first + nMoved);
    }
}

void CEnvBase::ReadConfig(const std::string& strFile, std::vector<std::string>& cfgArgs)
{
    std::ifstream fin(strFile);
	if (!fin)
	{
        if (m_stError.IsCatch(ERROR_CODE_CONFIG_UNREADABLE))
        {
            m_stError.SetError(ERROR_CODE_CONFIG_UNREADABLE, strFile);
        }
		return;
	}

    bool bEndOption = false;
    std::string strGroup;
    std::string strLine;
    while (std::getline(fin, strLine))
    {
        util::Trim(strLine);
        if (strLine.empty() || strLine[0] == '#' || strLine[0] == ';')
        {
            continue;
        }

        if (strLine == "--" || strLine == "[--]")
        {
            bEndOption = true;
            cfgArgs.push_back("--");
            continue;
        }
        if (bEndOption)
        {
            cfgArgs.push_back(strLine);
            continue;
        }

        if (strLine.size() > 2 && strLine.front() == '[' && strLine.back() == ']')
        {
            strGroup = strLine.substr(1, strLine.size()-2);
            continue;
        }

        size_t iEqual = strLine.find('=');
        if (iEqual != std::string::npos)
        {
            std::string strKey = strLine.substr(0, iEqual);
            std::string strVal = strLine.substr(iEqual + 1);
            util::Trim(strKey);
            util::Trim(strVal);
            util::TrimLeft(strKey, '-');
            if (!strGroup.empty())
            {
                std::string strTemp = strGroup;
                strTemp.append(1, '.').append(strKey);
                strKey.swap(strTemp);
            }
            std::string strTemp = strKey;
            strTemp.append(1, '=').append(strVal);
            strLine.swap(strTemp);
        }
        else if (m_stError.IsCatch(ERROR_CODE_CONFIG_INVALID))
        {
            m_stError.SetError(ERROR_CODE_CONFIG_INVALID, strLine);
            return;
        }

        cfgArgs.push_back(strLine);
    }
}

void CEnvBase::ReservedOption()
{
    if (nullptr == FindOption(OPTION_NAME_CONFIG))
    {
        std::string strConfig = program_invocation_short_name;
        strConfig += ".ini";
        Option('\0', OPTION_NAME_CONFIG, "read arguments from config file", strConfig);
    }
    if (nullptr == FindOption(OPTION_NAME_VERSION))
    {
        Flag('\0', OPTION_NAME_VERSION, "print version");
    }
    if (nullptr == FindOption(OPTION_NAME_HELP))
    {
        Flag('\0', OPTION_NAME_HELP, "print help message");
    }
}

void CEnvBase::Help()
{
    std::string text;
    Usage(text);
    fprintf(stdout, "%s", text.c_str());
}

void CEnvBase::HelpVersion()
{
    fprintf(stdout, "%s\n", m_strVersion.c_str());
}

void CEnvBase::Usage(std::string& outText)
{
    outText.append("Usage: ").append(m_stCommand.m_strName);
    if (!m_vecCommand.empty())
    {
        outText.append(" command");
    }
    outText.append(" [options] [arguments] ...\n");

    if (!m_strVersion.empty())
    {
        outText.append("\t").append(m_strVersion);
    }
    if (!m_stCommand.m_strDescription.empty())
    {
        outText.append("\t").append(m_stCommand.m_strDescription);
    }
    outText.append("\n");

    std::vector<std::string> line;
    if (!m_vecCommand.empty())
    {
        outText.append("Command:\n");
        util::CTextAlign align;
        for (auto it = m_vecCommand.begin(); it != m_vecCommand.end(); ++it)
        {
            line.push_back("  " + it->m_strName);
            line.push_back(it->m_strDescription);
            align.AddLine(line);
        }
        outText.append(align.GetText());
    }

    outText.append("Option:\n");
    util::CTextAlign align;
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        std::string strName("  ");
        if (it->m_cShortName != '\0')
        {
            strName.append(1, '-').append(1, it->m_cShortName).append(1, ' ');
        }
        if (it->m_iBindIndex != 0)
        {
            strName.append(1, '#').append(std::to_string(it->m_iBindIndex)).append(1, ' ');
        }
        if (it->m_strEnvName.size() > 0)
        {
            strName.append(1, '$').append(it->m_strEnvName).append(1, ' ');
        }
        strName.append(2, '-').append(it->m_strLongName);
        if (it->m_bArgument)
        {
            strName.append(1, '=');
            if (it->m_bRequired)
            {
                strName.append(1, '?');
            }
            if (it->m_bRepeated)
            {
                strName.append(1, '+');
            }
        }
        if (it->m_strDefault.size() > 0 && !it->m_bRequired)
        {
            strName.append(" [").append(it->m_strDefault).append("]");
        }

        line.push_back(strName);
        line.push_back(it->m_strDescription);
        align.AddLine(line);
    }
    outText.append(align.GetText());
}

} /* cli */ 
