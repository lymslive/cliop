#include "util-string.h"
#include <ctype.h>

namespace util
{

int Split(const std::string& strSrc, std::vector<std::string>& vecDest, char cSeparator)
{
    size_t iBegin = 0;
    size_t iPos = std::string::npos;
    int nCount = 0;
    while ((iPos = strSrc.find(cSeparator, iBegin)) != std::string::npos)
    {
        vecDest.push_back(strSrc.substr(iBegin, iPos - iBegin));
        nCount++;
        iBegin = iPos + 1;
    }
    if (iBegin != strSrc.size())
    {
        vecDest.push_back(strSrc.substr(iBegin));
        nCount++;
    }
	return nCount;
}

int SplitBySpace(const std::string& strSrc, std::vector<std::string>& vecDest)
{
    size_t iSize = strSrc.size();
    size_t iBegin = 0;
    while (iBegin < strSrc.size() && isspace(strSrc[iBegin]))
    {
        iBegin++;
    }
    if (iBegin == iSize)
    {
        return 0;
    }

    int nCount = 0;
    for (size_t iEnd = iBegin + 1; iEnd < iSize; ++iEnd)
    {
        if (!isspace(strSrc[iEnd]))
        {
            continue;
        }
        vecDest.push_back(strSrc.substr(iBegin, iEnd - iBegin));
        nCount++;

        iBegin = iEnd;
        while (iBegin < strSrc.size() && isspace(strSrc[iBegin]))
        {
            iBegin++;
        }
        iEnd = iBegin;
    }

    if (iBegin < strSrc.size())
    {
        vecDest.push_back(strSrc.substr(iBegin));
        nCount++;
    }

	return nCount;
}

int SplitByNull(const std::string& strSrc, std::vector<std::string>& vecDest)
{
	return Split(strSrc, vecDest, '\0');
}

int Join(const std::vector<std::string>& vecSrc, std::string& strDest, char cSeparator)
{
    for (size_t i = 0; i < vecSrc.size(); ++i)
    {
        if (!strDest.empty())
        {
            strDest.append(1, '\0');
        }
        strDest.append(vecSrc[i]);
    }
    return vecSrc.size();
}

int TrimLeft(std::string& strSrc)
{
    size_t iPos = 0;
    while (iPos < strSrc.size() && isspace(strSrc[iPos]))
    {
        iPos++;
    }
    if (iPos == strSrc.size())
    {
        strSrc.clear();
    }
    else if (iPos != 0)
    {
        strSrc = strSrc.substr(iPos);
    }
	return iPos;
}

int TrimLeft(std::string& strSrc, char c)
{
    size_t iPos = 0;
    while (iPos < strSrc.size() && c == strSrc[iPos])
    {
        iPos++;
    }
    if (iPos == strSrc.size())
    {
        strSrc.clear();
    }
    else if (iPos != 0)
    {
        strSrc = strSrc.substr(iPos);
    }
    return iPos;
}

int TrimRight(std::string& strSrc)
{
    size_t iSize = strSrc.size();
    size_t iPos = iSize - 1;
    while (iPos != std::string::npos && isspace(strSrc[iPos]))
    {
        iPos--;
    }
    if (iPos == std::string::npos)
    {
        strSrc.clear();
    }
    else if (iPos != iSize - 1)
    {
        strSrc = strSrc.substr(0, iPos + 1);
    }
    return iSize - strSrc.size();
}

int Trim(std::string& strSrc)
{
    size_t iSize = strSrc.size();
    size_t iBegin = 0;
    while (iBegin < strSrc.size() && isspace(strSrc[iBegin]))
    {
        iBegin++;
    }
    if (iBegin == iSize)
    {
        strSrc.clear();
        return iBegin;
    }

    size_t iEnd = iSize - 1;
    while (iEnd > iBegin && isspace(strSrc[iEnd]))
    {
        iEnd--;
    }
    if (iBegin > 0 || iEnd < iSize - 1)
    {
        strSrc = strSrc.substr(iBegin, iEnd - iBegin + 1);
    }
    return iSize - strSrc.size();
}

} /* util */ 
