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

int CEnvBase::Feed(const std::vector<std::string>& vecArgs)
{
    ParseCmdline(vecArgs);

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

    MoveArgument();

    // read config file
    std::vector<std::string> cfgArgs;
    std::string strFile = Get(OPTION_NAME_CONFIG);
    if (strFile != "NONE")
    {
        ReadConfig(strFile, cfgArgs);
        ParseCmdline(cfgArgs);
    }

    GetBind();

    int nRet = CheckRequiredOption();
    if (nRet != 0)
    {
        return nRet;
    }

    if (m_bStrictParser)
    {
        nRet = CheckExtraOption();
    }

    return nRet;
}

int CEnvBase::Feed(int argc, char* argv[])
{
    return Feed(argc, const_cast<const char**>(argv));
}

int CEnvBase::Feed(int argc, const char* argv[])
{
    if (m_stCommand.m_strName.empty())
    {
        Command(argv[0]);
    }

    m_pSubCommand = nullptr;
    int iShift = 0;
    if (!m_vecCommand.empty())
    {
        if (1 < argc && argv[1] != nullptr)
        {
            m_pSubCommand = FindCommand(argv[1]);
            if (m_pSubCommand != nullptr)
            {
                iShift = 1;
            }
        }
        if (m_pSubCommand == nullptr)
        {
            m_pSubCommand = FindCommand(argv[0]);
        }
        if (m_pSubCommand == nullptr)
        {
            m_pSubCommand = FindCommand(program_invocation_short_name);
        }
    }

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

    if (m_pSubCommand == nullptr && m_bSubCommandOnly)
    {
        return ERROR_CODE_COMMAND_INVALID;
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
            SaveOption(*pLastOption, vecArgs[i]);
            pLastOption = nullptr;
            continue;
        }
        if(!strLastOption.empty())
        {
            SaveOption(strLastOption, vecArgs[i]);
            strLastOption.clear();
            continue;
        }

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
        return ERROR_CODE_OPTION_INCOMPLETE;
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

CEnvBase& CEnvBase::StrictParser(bool tf /*= true*/)
{
    m_bStrictParser = tf;
    return *this;
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
    m_vecCommand.push_back(stCommand);
    return *this;
}

CEnvBase& CEnvBase::SubCommand(const std::string& strName, const std::string& strDescription, CEnvBase& stEnvBase)
{
    CommandInfo stCommand(strName, strDescription);
    stCommand.m_pEnvBase = &stEnvBase;
    stCommand.m_pEnvBase->Command(strName, strDescription);
    m_vecCommand.push_back(stCommand);
    return *this;
}

CEnvBase& CEnvBase::SubCommandOnly(bool tf/* = true*/)
{
    m_bSubCommandOnly = tf && m_vecCommand.size() > 0;
    return *this;
}

CEnvBase& CEnvBase::AddOption(const COption& stOption)
{
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
        if (it->second.m_eValueType == OPTION_BOOL)
        {
            Get(it->first, *(static_cast<bool*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_STR)
        {
            Get(it->first, *(static_cast<std::string*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_INT)
        {
            Get(it->first, *(static_cast<int*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_DOUBLE)
        {
            Get(it->first, *(static_cast<double*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_STR_LIST)
        {
            Get(it->first, *(static_cast<std::vector<std::string>*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_INT_LIST)
        {
            Get(it->first, *(static_cast<std::vector<int>*>(it->second.m_pBindValue)));
        }
        else if (it->second.m_eValueType == OPTION_DOUBLE_LIST)
        {
            Get(it->first, *(static_cast<std::vector<double>*>(it->second.m_pBindValue)));
        }
    }
}

void CEnvBase::ClearArgument()
{
    m_stArgRecv.m_mapArgs.clear();
    m_stArgRecv.m_vecArgs.clear();
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

int CEnvBase::CheckRequiredOption()
{
    int nCount = 0;
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_bRequired && m_stArgRecv.m_mapArgs.count(it->m_strLongName) == 0)
        {
            nCount++;
        }
    }
    return nCount;
}

int CEnvBase::CheckExtraOption()
{
    int nCount = 0;
    for (auto it = m_stArgRecv.m_mapArgs.begin(); it != m_stArgRecv.m_mapArgs.end(); ++it)
    {
        if (nullptr == FindOption(it->first))
        {
            nCount++;
        }
    }
    return nCount;
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

    std::vector<COption*> vecOptions(nSize, nullptr);
    for (auto it = m_vecOptions.begin(); it != m_vecOptions.end(); ++it)
    {
        if (it->m_iBindIndex > 0 && it->m_iBindIndex <= nSize)
        {
            vecOptions[it->m_iBindIndex - 1] = &(*it);
        }
    }

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
    // outText.append(m_stCommand.m_strDescription) .append("\t").append(m_strVersion).append("\n");

    outText.append("Usage: ").append(m_stCommand.m_strName);
    if (!m_vecCommand.empty())
    {
        outText.append(" command");
    }
    outText.append(" [options] [arguments] ...\n");

    if (!m_vecCommand.empty())
    {
        outText.append("Command:\n");
        for (size_t i = 0; i < m_vecCommand.size(); ++i)
        {
            outText.append("  ")
                .append(m_vecCommand[i].m_strName)
                .append("\t")
                .append(m_vecCommand[i].m_strDescription)
                .append("\n");
        }
    }

    outText.append("Option:\n");
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
        outText.append(strName).append("\n\t").append(it->m_strDescription);
        outText.append(1, '\n');
    }
}

} /* cli */ 