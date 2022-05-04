#include "tinytast.hpp"
#include "util-string.h"
#include "test-os.h"

DEF_TAST(string_split, "test split string")
{
    std::vector<std::string> vecDest;
    std::string strSrc = "1,2,33,4";
    COUT(util::Split(strSrc, vecDest, ','), 4);
    COUT(vecDest);

    vecDest.clear();
    strSrc = "1,2,,4";
    COUT(util::Split(strSrc, vecDest, ','), 4);
    COUT(vecDest);
    COUT(vecDest[2].empty(), true);

    vecDest.clear();
    strSrc = "1,2,33,4,";
    COUT(util::Split(strSrc, vecDest, ','), 4);
    COUT(vecDest);

    vecDest.clear();
    strSrc = ",1,2,33,4";
    COUT(util::Split(strSrc, vecDest, ','), 5);
    COUT(vecDest);
    COUT(vecDest[0].empty(), true);

    vecDest.clear();
    strSrc = ",,1,2,33,4";
    COUT(util::Split(strSrc, vecDest, ','), 6);
    COUT(vecDest);
    COUT(vecDest[0].empty(), true);
    COUT(vecDest[1].empty(), true);

    vecDest.clear();
    strSrc = "1,2,33,4,,";
    COUT(util::Split(strSrc, vecDest, ','), 5);
    COUT(vecDest);
    COUT(vecDest[4].empty(), true);

    vecDest.clear();
    strSrc = "1,2,,,4";
    COUT(util::Split(strSrc, vecDest, ','), 5);
    COUT(vecDest);
    COUT(vecDest[2].empty(), true);
    COUT(vecDest[3].empty(), true);
}

DEF_TAST(string_split_space, "test split string by space")
{
    std::vector<std::string> vecDest;
    std::string strSrc = "1 2  33\t4\n \t5";
    COUT(util::SplitBySpace(strSrc, vecDest), 5);
    COUT(vecDest);

    vecDest.clear();
    strSrc = " 1 2  33\t4\n \t5 ";
    COUT(util::SplitBySpace(strSrc, vecDest), 5);
    COUT(vecDest);
}

DEF_TAST(string_split_null, "test split string by null")
{
    std::vector<std::string> vecDest = {"11", "22", "33", "44"};
    std::string strSrc;
    for (int i = 0; i < vecDest.size(); ++i)
    {
        strSrc.append(1, '\0').append(vecDest[i]);
    }
    COUT(strSrc.size(), 12);

    vecDest.clear();
    COUT(util::SplitByNull(strSrc, vecDest), 5);
    COUT(vecDest);
    COUT(vecDest[0].empty(), true);
    COUT(vecDest[1], "11");
    COUT(vecDest[4], "44");
}

DEF_TAST(string_trim, "test trim space from string")
{
    std::string strSrc = "word";
    COUT(util::TrimLeft(strSrc), 0);
    COUT(util::TrimRight(strSrc), 0);
    COUT(util::Trim(strSrc), 0);
    COUT(strSrc, "word");

    strSrc = " \tword";
    COUT(util::TrimRight(strSrc), 0);
    COUT(util::TrimLeft(strSrc), 2);
    COUT(strSrc, "word");

    strSrc = " \tword";
    COUT(util::Trim(strSrc), 2);
    COUT(strSrc, "word");

    strSrc = "word \t";
    COUT(util::TrimLeft(strSrc), 0);
    COUT(util::TrimRight(strSrc), 2);
    COUT(strSrc, "word");

    strSrc = "word \t";
    COUT(util::Trim(strSrc), 2);
    COUT(strSrc, "word");

    strSrc = " \tword \t";
    COUT(util::Trim(strSrc), 4);
    COUT(strSrc, "word");

    strSrc = " \r\n\thello world! \t\r\n";
    COUT(util::Trim(strSrc), 8);
    COUT(strSrc, "hello world!");

    strSrc = "--word";
    COUT(util::TrimLeft(strSrc, '-'), 2);
    COUT(strSrc, "word");

    strSrc = "      ";
    COUT(util::TrimLeft(strSrc), 6);
    COUT(strSrc, "");
    strSrc = "      ";
    COUT(util::TrimRight(strSrc), 6);
    COUT(strSrc, "");
    strSrc = "      ";
    COUT(util::Trim(strSrc), 6);
    COUT(strSrc, "");
}
