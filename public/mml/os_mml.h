/*******************************************************************************

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: MML.H
��    ��: 1.00
��    ��: 2012��4��14��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2012��4��14��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#ifndef __MML_H__
#define __MML_H__

#define PID_MML   1

#define MML_DEV_NAME "mml_dev"


typedef enum tagMML_ERROR
{
    MML_ERR_START = 1,
    MML_ERR_SEND_CMD,
    MML_ERR_PARAMETER,
    MML_ERR_PID_CORRUPT,
} MML_ERROR;

// ����
#define MML_CMD_MAGIC 0xd0
#define MML_CMD_INIT                _IO(MML_CMD_MAGIC, 0)
#define MML_CMD_PROCESS_CMD         _IO(MML_CMD_MAGIC, 1)
#define MML_CMD_GET_RESULT          _IO(MML_CMD_MAGIC, 2)

// ״̬
#define MML_STATUS_INIT             0
#define MML_STATUS_INPUT_OK         1
#define MML_STATUS_OUTPUT_BEGIN     2
#define MML_STATUS_OUTPUT_CONTINUE  3
#define MML_STATUS_OUTPUT_END       4

#define MML_WAIT_TIME         50
#define MML_WAIT_TIMEOUT      (MML_WAIT_TIME * 120)

#define MML_DAT_LEN    (4096 - 12)

typedef struct tagMML_CMD
{
    //OS_U32 cmd;
    uint32_t status;
    uint32_t datLen;
    uint8_t  dat[MML_DAT_LEN];
} MML_CMD;

#define MML_PID_NUM         256
#define MML_PID_NAME_SIZE   8

typedef struct tagMML_MODULE
{
    //OS_U32  pid;
    char   name[MML_PID_NAME_SIZE]; /* Ψһ���ָ�module */
    void (*DoCmd)(char *);
} MML_MODULE;

extern MML_MODULE g_mmlModules[MML_PID_NUM];

extern int32_t MML_Register(uint32_t v_pid, MML_MODULE *v_module);
extern int32_t MML_Unregister(uint32_t v_pid);
extern int32_t MML_Ioctl(uint32_t v_cmd, void *v_arg);
extern void MML_Print(const char *format, ...);


#endif
