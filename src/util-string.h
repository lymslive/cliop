/**
 * @file util-string.h
 * @author lymslive
 * @date 2022-04-25
 * @brief utilities for string function
 * */
#ifndef UTIL_STRING_H__
#define UTIL_STRING_H__

#include <string>
#include <vector>

namespace util
{
/** Split string by a specific char.
 * @param [IN] strSrc: the source string to be split.
 * @param [OUT] vecDesc: save the splitted string to a vector.
 * @param [IN] cSeparator: the separator char.
 * @return int: the count of item added to `vecDest`.
 * @note Ignore the last separator char if it is in the very end of string,
 * but a leading or continuous separator will give out empty splitted string.
 * */
int Split(const std::string& strSrc, std::vector<std::string>& vecDest, char cSeparator);

/** Split string by any space char.
 * @param [IN] strSrc: the source string to be split.
 * @param [OUT] vecDesc: save the splitted string to a vector.
 * @return int: the count of item added to `vecDest`.
 * @note Skip the leading space, continuous space considered as one separator.
 * */
int SplitBySpace(const std::string& strSrc, std::vector<std::string>& vecDest);

/** Split string by the NULL char '\0'.
 * @param [IN] strSrc: the source string to be split.
 * @param [OUT] vecDesc: save the splitted string to a vector.
 * @return int: the count of item added to `vecDest`.
 * @note std::string can contain '\0' in the middle position.
 * */
int SplitByNull(const std::string& strSrc, std::vector<std::string>& vecDest);

/** Join vector of string with a separator character.
 * @param [IN] vecSrc: the source vector
 * @param [OUT] strDest: the joined dest string
 * @param [IN] cSeparator: the separator character
 * @return int: the size of `vecSrc`
 * */
int Join(const std::vector<std::string>& vecSrc, std::string& strDest, char cSeparator);

/** Trim the leading white space of a string, in-palce.
 * @param [IN-OUT] strSrc: the string to be trimed.
 * @return the count of trimed chars actually.
 * */
int TrimLeft(std::string& strSrc);

/** Trim the specific leading character of a string, in-palce.
 * @param [IN-OUT] strSrc: the string to be trimed.
 * @param [IN] c: trim which char
 * @return the count of trimed chars actually.
 * */
int TrimLeft(std::string& strSrc, char c);

/** Trim the ending white space of a string, in-palce.
 * @param [IN-OUT] strSrc: the string to be trimed.
 * @return the count of trimed chars actually.
 * */
int TrimRight(std::string& strSrc);

/** Trim the white space of a string in both ends, and in-palce.
 * @param [IN-OUT] strSrc: the string to be trimed.
 * @return the count of trimed chars actually.
 * */
int Trim(std::string& strSrc);

/** Format text, align with each column width as much as possible. */
class CTextAlign
{
    typedef std::vector<std::string> row_t;

    std::vector<row_t> m_vLine; // table string
    int m_iWidthMax = 36;       // max width of each column
    int m_iColSep = 4;          // add counts of space between cloumn

public:
    CTextAlign(int widthMax = 40, int colSep = 4)
        : m_iWidthMax(widthMax), m_iColSep(colSep) {}

    /** Add a line, vector of column string of this row.
     * @param [IN] line, will be moved into this object.
     * */
    CTextAlign& AddLine(std::vector<std::string>& line);

    /** Get the formated text, with "\n" at end of each row. */
    std::string GetText();

private:
};

} /* util */ 

#endif /* end of include guard: UTIL_STRING_H__ */
