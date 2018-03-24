/*
* 使用gcc -o avp_ini avp_ini.c -Wall -g -I./ -D_EN_DEBUG_进行编译链接
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "ini.h"

#define INI_MAX_FILE_SIZE (1024 * 16)
#define INT_VALUE_LEN (32)

#define UNKNOW_POS   (-1) /* 初始化的位置信息，表明位置未知 */

#define LEFT_BRACE  '['
#define RIGHT_BRACE ']'
#define LINE_CHANGE '\n'  /* 0x0A */
#define LINE_END    '\r'  /* 0x0D */
#define STR_END     '\0'  /* 0x00 */

static int IsNewLine(char v_c)
{
	return ((LINE_CHANGE == v_c) ||  (LINE_END == v_c)) ? 1 : 0;
}

static int IsStrEnd(char v_c)
{
	return (STR_END == v_c) ? 1 : 0;
}

static int IsLeftBrace(char v_c)
{
	return (LEFT_BRACE == v_c) ? 1 : 0;
}

static int IsRightBrace(char v_c)
{
	return (RIGHT_BRACE == v_c) ? 1 : 0;
}

/*******************************************************************************
函数名称: LoadIniFile
功能说明: 将ini文件的内容载入内存
输入参数:
    v_pszFileName: 要装载的ini文件名
输出参数:
    v_pszBuf     : 要装载到内存
    v_piFileSize : 文件的大小
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int LoadIniFile(const char *v_pszFileName,
    char *v_pszBuf, int *v_piFileSize)
{
	FILE *pstFile = NULL;
	int i = 0;

	assert(NULL != v_pszFileName);
	assert(NULL != v_pszBuf);

	*v_piFileSize = 0;

    /* 以只读方式打开文件 */
	pstFile = fopen(v_pszFileName, "r");
	if (NULL == pstFile)
    {
		return -INI_ERR_OPEN_FILE_FOR_READ;
	}

	/* 装载文件 */
	v_pszBuf[i] = fgetc(pstFile);
	while (v_pszBuf[i] != (char)EOF)
    {
		i++;
		if (i >= INI_MAX_FILE_SIZE)
        { /* 文件不能超过指定大小 */
            return -INI_ERR_FILE_TOO_LARGE;
		}

		v_pszBuf[i] = fgetc(pstFile);
	}

	v_pszBuf[i] = STR_END;
	*v_piFileSize = i;

	fclose(pstFile);

	return 0;
}

/*******************************************************************************
函数名称: SaveIniFile
功能说明: 将内存中的内容写入到ini文件中去
输入参数:
    v_pszFileName: 要装载的ini文件名
    v_pszBuf     : 要装载到内存
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
static int SaveIniFile(const char *v_pszFileName, const char *v_pszBuf)
{
	FILE *pstFile = NULL;

	assert(NULL != v_pszFileName);
	assert(NULL != v_pszBuf);

    /* 以只写方式打开文件 */
	pstFile = fopen(v_pszFileName, "w");
	if (NULL == pstFile)
    {
		return -INI_ERR_OPEN_FILE_FOR_WRITE;
	}

	/* 写入文件 */
	if (EOF == fputs(v_pszBuf, pstFile))
	{
		fclose(pstFile);
		return -INI_ERR_WRITE_FILE;
	}

	fclose(pstFile);

	return 0;
}

/*******************************************************************************
函数名称: ParseFile
功能说明: 无
输入参数:
    v_pszBuf     : 装载有ini文件内容的内存
    v_pszSection : 要查找的section的名字
    v_pszKey     : 要查找的key的名字
输出参数:
    v_piSecStart  : section的起始位置
    v_piSecEnd    : section的结束位置
    v_piKeyStart  : key的起始位置
    v_piKeyEnd    : key的结束位置
    v_piValueStart: value的起始位置
    v_piValueEnd  : value的结束位置
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
#if 0

static int ParseFile(
    const char *v_pszBuf, const char *v_pszSection, const char *v_pszKey,
    int *v_piSecStart, int *v_piSecEnd, int *v_piKeyStart, int *v_piKeyEnd,
    int *v_piValueStart, int *v_piValueEnd)
{
	const char *p = v_pszBuf;
	int i = 0;
	int iSecLen;
	int iKeyLen;

    iSecLen = strlen(v_pszSection);
    iKeyLen  = strlen(v_pszKey);

	assert(NULL != v_pszBuf);
	assert((NULL != v_pszSection) && (0 != iSecLen));
	assert((NULL != v_pszKey) && (0 != iKeyLen));

    *v_piSecStart = UNKNOW_POS;
	*v_piSecEnd = UNKNOW_POS;
    *v_piKeyStart = UNKNOW_POS;
    *v_piKeyEnd = UNKNOW_POS;
    *v_piValueStart = UNKNOW_POS;
    *v_piValueEnd = UNKNOW_POS;

	while (!IsStrEnd(p[i]))
    {
		if (((0 == i) ||  IsNewLine(p[i-1])) && IsLeftBrace(p[i]))
		{ /* 找到了'[' */
			int iSecStart = i + 1;

			/* 寻找']' */
			do
            {
				i++;
			} while (!IsRightBrace(p[i]) && !IsStrEnd(p[i]));

			if ((0 == strncmp(p + iSecStart, v_pszSection, i - iSecStart))
                && (iSecLen == i - iSecStart))
            {
				int iNewLineStart = 0;

				i++;

				/* 跳过']'后的空白 */
				while (isspace(p[i]))
				{
					i++;
				}

				/* 找到了section */
				*v_piSecStart = iSecStart;
				*v_piSecEnd = i;

				while (!(IsNewLine(p[i-1]) && IsLeftBrace(p[i]))
				    && !IsStrEnd(p[i]))
			    {
					int j = 0;

					/* 到了一个新行 */
					iNewLineStart = i;

					while (!IsNewLine(p[i]) && !IsStrEnd(p[i]))
                    {
						i++;
					}

					/* now i  is equal to end of the line */
					j = iNewLineStart;

					if (';' != p[j]) /* skip over comment */
					{
						while ((j < i) && ('=' != p[j]))
                        {
							j++;
							if ('=' == p[j])
                            {
								if ((strncmp(v_pszKey,p+iNewLineStart,j-iNewLineStart)==0)
                                    && (iKeyLen==j-iNewLineStart))
								{
									/*find the key ok */
									*v_piKeyStart = iNewLineStart;
									*v_piKeyEnd = j-1;

									*v_piValueStart = j+1;
									*v_piValueEnd = i;

									return 0;
								}
							}
						}
					}

					i++;
				}
			}
		}
		else
		{
			i++;
		}
	}

	return -INI_ERR_PARSE_FILE;
}

#else

static int ParseFile(
    const char *v_pszBuf, const char *v_pszSection, const char *v_pszKey,
    int *v_piSecStart, int *v_piSecEnd, int *v_piKeyStart, int *v_piKeyEnd,
    int *v_piValueStart, int *v_piValueEnd)
{
	const char *p = v_pszBuf;
	int i = 0;
	int iSecLen;
	int iKeyLen;

    iSecLen = strlen(v_pszSection);
    iKeyLen  = strlen(v_pszKey);

	assert(NULL != v_pszBuf);
	assert((NULL != v_pszSection) && (0 != iSecLen));
	assert((NULL != v_pszKey) && (0 != iKeyLen));

    *v_piSecStart = UNKNOW_POS;
	*v_piSecEnd = UNKNOW_POS;
    *v_piKeyStart = UNKNOW_POS;
    *v_piKeyEnd = UNKNOW_POS;
    *v_piValueStart = UNKNOW_POS;
    *v_piValueEnd = UNKNOW_POS;

	while (!IsStrEnd(p[i]))
    {
		if (((0 == i) ||  IsNewLine(p[i-1])) && IsLeftBrace(p[i]))
		{ /* 找到了'[' */
			int iSecStart = i + 1;

			/* 寻找']' */
			do
            {
				i++;
			} while (!IsRightBrace(p[i]) && !IsStrEnd(p[i]));

			if ((0 == strncmp(p + iSecStart, v_pszSection, i - iSecStart))
                && (iSecLen == i - iSecStart))
            {
				int iNewLineStart = 0;

				i++;

				/* 跳过']'后的空白 */
				while (isspace(p[i]))
				{
					i++;
				}

				/* 找到了section */
				*v_piSecStart = iSecStart;
				*v_piSecEnd = i;

				while (!(IsNewLine(p[i-1]) && IsLeftBrace(p[i]))
				    && !IsStrEnd(p[i]))
			    {
					int j = 0;

					/* 到了一个新行 */
					iNewLineStart = i;

					while (!IsNewLine(p[i]) && !IsStrEnd(p[i]))
                    {
						i++;
					}

					/* now i  is equal to end of the line */
					j = iNewLineStart;

					if (';' != p[j]) /* skip over comment */
					{
						while ((j < i) && ('=' != p[j]))
                        {
							j++;
							if ('=' == p[j])
                            {
								if ((strncmp(v_pszKey,p+iNewLineStart,j-iNewLineStart)==0)
                                    && (iKeyLen==j-iNewLineStart))
								{
									/*find the key ok */
									*v_piKeyStart = iNewLineStart;
									*v_piKeyEnd = j-1;

									*v_piValueStart = j+1;
									*v_piValueEnd = i;

									return 0;
								}
							}
						}
					}

					i++;
				}
			}
		}
		else
		{
			i++;
		}
	}

	return -INI_ERR_PARSE_FILE;
}

#endif

/*******************************************************************************
函数名称: IniReadString
功能说明: 从ini文件中读指定字段的值
输入参数:
    v_pszFileName    : 要读取的ini文件名
    v_pszSection     : 要读取的section的名字
    v_pszKey         : 要读取的key的名字
    iValueSize       : 读取到的value的最大长度
    v_pszDefaultValue: 如果找不到对应的字段，那么就将此默认值赋值给v_pszValue
输出参数:
    v_pszValue   : 读取到的value值
返 回 值:
    > 0: 成功代码
    ==0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int IniReadString(
    const char *v_pszFileName, const char *v_pszSection, const char *v_pszKey,
    char *v_pszValue, int v_iValueSize, const char *v_pszDefaultValue)
{
	char *pszBuf = NULL;
	int iFileSize = 0;
	int iSecStart = 0;
    int iSecEnd = 0;
    int iKeyStart= 0;
    int iKeyEnd = 0;
    int iValueStart = 0;
    int iValueEnd = 0;
    int iCopyCnt = 0;

	/* 检查参数 */
    if ((NULL == v_pszFileName) || (0 == strlen(v_pszFileName)))
    {
        return -INI_ERR_NO_FILE_NAME;
    }

    if ((NULL == v_pszSection) || (0 == strlen(v_pszSection)))
    {
        return -INI_ERR_NO_SECTION;
    }

    if ((NULL == v_pszKey) || (0 == strlen(v_pszKey)))
    {
        return -INI_ERR_NO_KEY;
    }

    if ((NULL == v_pszValue) || (0 >= v_iValueSize))
    {
        return -INI_ERR_NO_VALUE;
    }

    /* 分配内存 */
    pszBuf = malloc(INI_MAX_FILE_SIZE);
    if (NULL == pszBuf)
    {
        return -INI_ERR_MALLOC;
    }

    /* 载入文件 */
	if (0 != LoadIniFile(v_pszFileName, pszBuf, &iFileSize))
	{ /* 载入失败 */
		if (NULL != v_pszDefaultValue)
		{
			strncpy(v_pszValue, v_pszDefaultValue, v_iValueSize);
            free(pszBuf);
            return INI_ERR_LOAD_FILE;
		}

        free(pszBuf);
		return -INI_ERR_LOAD_FILE;
	}

	if (0 != ParseFile(pszBuf, v_pszSection, v_pszKey, &iSecStart, &iSecEnd,
        &iKeyStart, &iKeyEnd, &iValueStart, &iValueEnd))
	{ /* 指定字段未找到 */
		if (NULL != v_pszDefaultValue)
		{
			strncpy(v_pszValue, v_pszDefaultValue, v_iValueSize);
            free(pszBuf);
            return INI_ERR_PARSE_FILE;
		}

        free(pszBuf);
		return -INI_ERR_PARSE_FILE;
	}

	iCopyCnt = iValueEnd - iValueStart;

    /* 扣除字符串结束符，所以要-1 */
	if ((v_iValueSize - 1) < iCopyCnt)
	{
		iCopyCnt = (v_iValueSize - 1);
	}

	memcpy(v_pszValue, pszBuf + iValueStart, iCopyCnt);
	v_pszValue[iCopyCnt] = STR_END;

    free(pszBuf);
	return 0;
}

/*******************************************************************************
函数名称: IniWriteString
功能说明: 将值写入ini文件的指定字段
输入参数:
    v_pszFileName    : 要写入的ini文件名
    v_pszSection     : 要写入的section的名字
    v_pszKey         : 要写入的key的名字
    v_pszValue       : 要写入的value值
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int IniWriteString(
    const char *v_pszFileName, const char *v_pszSection, const char *v_pszKey,
    const char *v_pszValue)
{
	char *pszWrBuf = NULL;
	char *pszBuf = NULL;
	int iFileSize = 0;
	int iRet = 0;
	int iSecStart = UNKNOW_POS;
    int iSecEnd = 0;
    int iKeyStart= 0;
    int iKeyEnd = 0;
    int iValueStart = 0;
    int iValueEnd = 0;
    int iValueSize = (int)strlen(v_pszValue);

	/* 检查参数 */
    if ((NULL == v_pszFileName) || (0 == strlen(v_pszFileName)))
    {
        return -INI_ERR_NO_FILE_NAME;
    }

    if ((NULL == v_pszSection) || (0 == strlen(v_pszSection)))
    {
        return -INI_ERR_NO_SECTION;
    }

    if ((NULL == v_pszKey) || (0 == strlen(v_pszKey)))
    {
        return -INI_ERR_NO_KEY;
    }

    if (NULL == v_pszValue)
    {
        return -INI_ERR_NO_VALUE;
    }

    /* 分配内存 */
    pszBuf = malloc(INI_MAX_FILE_SIZE);
    if (NULL == pszBuf)
    {
        return -INI_ERR_MALLOC;
    }

    /* 分配内存 */
    pszWrBuf = malloc(INI_MAX_FILE_SIZE);
    if (NULL == pszWrBuf)
    {
        free(pszBuf);
        return -INI_ERR_MALLOC;
    }

    memset(pszWrBuf, 0, INI_MAX_FILE_SIZE);

    /* 载入文件 */
	if (0 == LoadIniFile(v_pszFileName, pszBuf, &iFileSize))
	{ /* 载入成功 */
		ParseFile(pszBuf, v_pszSection, v_pszKey, &iSecStart, &iSecEnd,
            &iKeyStart, &iKeyEnd, &iValueStart, &iValueEnd);
	}

	if (UNKNOW_POS == iSecStart)
	{ /* section位置未初始化 */
		if (0 == iFileSize)
		{ /* 无文件 */
			sprintf(pszWrBuf + iFileSize, "[%s]\n%s=%s\n",
                v_pszSection, v_pszKey, v_pszValue);
		}
		else
		{ /* 无指定的section */
			memcpy(pszWrBuf,pszBuf,iFileSize);
			sprintf(pszWrBuf + iFileSize, "\n[%s]\n%s=%s\n",
                v_pszSection, v_pszKey, v_pszValue);
		}
	}
	else if (UNKNOW_POS == iKeyStart)
	{ /* 有指定的section，但是没有指定的key */
		memcpy(pszWrBuf, pszBuf, iSecEnd);
		sprintf(pszWrBuf + iSecEnd, "%s=%s\n", v_pszKey, v_pszValue);
		memcpy(pszWrBuf + iSecEnd + strlen(v_pszKey) + iValueSize + 2,
            pszBuf + iSecEnd, iFileSize - iSecEnd);
	}
	else
	{ /* 有指定的section，也有指定的key */
		memcpy(pszWrBuf, pszBuf, iValueStart);
		memcpy(pszWrBuf + iValueStart, v_pszValue, iValueSize);
		memcpy(pszWrBuf + iValueStart + iValueSize, pszBuf + iValueEnd,
            iFileSize - iValueEnd);
	}

	iRet = SaveIniFile(v_pszFileName, pszWrBuf);

    free(pszWrBuf);
    free(pszBuf);

    return iRet;
}

/*******************************************************************************
函数名称: IniReadInt
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
说    明: 无
*******************************************************************************/
int IniReadInt(
    const char *v_pszFileName, const char *v_pszSection, const char *v_pszKey,
    int v_pszDefaultValue)
{
	char szValue[INT_VALUE_LEN] = {0};

	if (0 != IniReadString(v_pszFileName, v_pszSection, v_pszKey,
        szValue, INT_VALUE_LEN, NULL))
	{
		return v_pszDefaultValue;
	}

	return atoi(szValue);
}


