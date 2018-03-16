#ifndef INI_FILE_H_
#define INI_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tagINI_ERROR_CODE_E
{
    INI_ERR_START = 0x4000,
    INI_ERR_OPEN_FILE_FOR_READ,
    INI_ERR_OPEN_FILE_FOR_WRITE,
    INI_ERR_WRITE_FILE,
    INI_ERR_LOAD_FILE,
    INI_ERR_PARSE_FILE,
    INI_ERR_MALLOC,
    INI_ERR_FILE_TOO_LARGE,
    INI_ERR_NO_FILE_NAME,
    INI_ERR_NO_SECTION,
    INI_ERR_NO_KEY,
    INI_ERR_NO_VALUE,
} INI_ERROR_CODE_E; /* End of INI_ERROR_CODE_E */


extern int IniReadString(const char *v_pszFileName, const char *v_pszSection,
    const char *v_pszKey, char *value, int size, const char *default_value);
extern int IniReadInt(const char *v_pszFileName, const char *v_pszSection,
    const char *v_pszKey, int default_value);
extern int IniWriteString(const char *v_pszFileName, const char *v_pszSection,
    const char *v_pszKey, const char *value);

#ifdef __cplusplus
};
#endif

#endif //end of INI_FILE_H_

