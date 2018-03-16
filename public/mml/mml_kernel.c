/*******************************************************************************

            版权所有(C), 2012~2015, AXEN工作室
********************************************************************************
文 件 名: MML_KERNEL.C
作    者: 曾华荣 (zeng_hr@163.com)
版    本: 1.00
日    期: 2012年4月15日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2012年4月15日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#include "os_adapter.h"
#include "os_mml.h"
#include "os_log.h"

static OS_RWLOCK g_printMutex;

#define PRINT_BUF_SIZE   1024
static char g_printBuf[PRINT_BUF_SIZE];

static uint32_t g_datLen = 0;

static MML_CMD g_kCmd;

MML_MODULE g_mmlModules[MML_PID_NUM];

#define MML_ARGV_NUM  512
static uint32_t g_argc;
static char  *g_argv[MML_ARGV_NUM];

/*******************************************************************************
函数名称: CmdInit
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
void CmdInit(void)
{
    g_kCmd.status = MML_STATUS_INIT;
    g_datLen = 0;
    g_kCmd.datLen = 0;
    g_kCmd.dat[0] = 0;
}

/*******************************************************************************
函数名称: MML_Ioctl
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
int32_t MML_Ioctl(uint32_t v_cmd, void *v_arg)
{
    int32_t ret = 0;
    
    switch (v_cmd)
    {
        case MML_CMD_INIT:
        {
            CmdInit();
            break;
        }
        
        case MML_CMD_PROCESS_CMD:
        {
            ret = copy_from_user(&g_kCmd, v_arg, sizeof(g_kCmd));
            if (0 != ret)
            {
                LOG_ERROR("copy from user failed. ret(%d)\n", ret);
            }
            break;
        }
            
        case MML_CMD_GET_RESULT:
        {
            if (MML_STATUS_INIT == g_kCmd.status)
            {
                break;
            }

            ret = copy_to_user(v_arg, &g_kCmd, sizeof(g_kCmd));
            if (0 != ret)
            {
                LOG_ERROR("copy from user failed. ret(%d)\n", ret);
            }

            if (MML_STATUS_OUTPUT_CONTINUE == g_kCmd.status)
            {
                g_kCmd.status = MML_STATUS_OUTPUT_BEGIN;
            }

            
            break;
        }
            
        default:
        {
            LOG_ERROR("Invalid cmd. cmd(%d)\n", v_cmd);
            return -ENOTTY;
        }
    }
    
    return 0;
}

/*******************************************************************************
函数名称: MML_Print
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
void MML_Print(const char *format, ...)
{
    va_list ap;
    uint32_t len = 0;
    uint32_t waitTimeCnt = 0;

    if (MML_STATUS_OUTPUT_BEGIN != g_kCmd.status)
    {
        LOG_ERROR("MML status incorrect. status(%d)\n", g_kCmd.status);
        return;
    }

    OS_MUTEX_LOCK(&g_printMutex);
    
    va_start(ap, format);
    OS_VSNPRINTF(g_printBuf, PRINT_BUF_SIZE, format, ap);
    va_end(ap);

    len = strlen(g_printBuf);
    if ((g_datLen + len) >= MML_DAT_LEN)
    {
        g_kCmd.status = MML_STATUS_OUTPUT_CONTINUE;
        for (;;)
        {
            if (MML_STATUS_OUTPUT_BEGIN == g_kCmd.status)
            {
                g_datLen = 0;
                g_kCmd.datLen = 0;
                g_kCmd.dat[0] = 0;
                break;
            }

            if (MML_STATUS_OUTPUT_CONTINUE == g_kCmd.status)
            {
                waitTimeCnt += MML_WAIT_TIME;
                if (MML_WAIT_TIMEOUT < waitTimeCnt)
                {
                    OS_PRINT("Wait for result overtime.\n");
                    LOG_ERROR("Wait for status change overtime.\n");
                    CmdInit();
                    return;
                }
            }
            else /* 接收到了新命令或者当前异常退出了 */
            {
                g_datLen = 0;
                g_kCmd.datLen = 0;
                g_kCmd.dat[0] = 0;
                break;
            }
        }
    }

    strncat(g_kCmd.dat, g_printBuf, len);
    g_datLen += len;

    return;
}

/*******************************************************************************
函数名称: DoCmd
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
void DoCmd(void)
{
    uint32_t pid = 0;

    if (strlen(g_argv[0]) > MML_PID_NAME_SIZE)
    {
        LOG_ERROR("module name too long. name(%s)\n", g_argv[0]);
        return;
    }

    for (pid = 0; pid < MML_PID_NUM; pid++)
    {
        if (NULL == g_mmlModules[pid].DoCmd)
        {
            continue;
        }
        
        if (0 != strncmp(g_argv[0], g_mmlModules[pid].name, MML_PID_NAME_SIZE))
        {
            continue;
        }

        g_mmlModules[pid].DoCmd(g_argv[1]);
    }

    if (MML_PID_NUM <= pid)
    {
        LOG_ERROR("module not found. name(%s)\n", g_argv[0]);
    }
    
    return;
}

/*******************************************************************************
函数名称: MML_Thread
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
void *MML_Thread(void *v_para)
{
    LOG_INFO("MML thread begin.\n");

    while (!kthread_should_stop())
    {
        if (MML_STATUS_INPUT_OK != g_kCmd.status)
        {
            OS_SLEEP_MS(MML_WAIT_TIME);
            continue;
        }

        g_kCmd.status = MML_STATUS_OUTPUT_BEGIN;
        DoCmd();
        g_kCmd.status = MML_STATUS_OUTPUT_END;
    }
    
    LOG_INFO("MML thread end.\n");

    return NULL;
}

/*******************************************************************************
函数名称: MML_Register
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
int32_t MML_Register(uint32_t v_pid, MML_MODULE *v_module)
{
    uint32_t len = 0;
    
    ASSERT(NULL != v_module);
    ASSERT(NULL != v_module->name);
    ASSERT(NULL != v_module->DoCmd);
    
    len = strlen(v_module->name);
    if (MML_PID_NAME_SIZE <= len)
    {
        LOG_ERROR("module name too long. name(%s) max(%d)\n",
            v_module->name, MML_PID_NAME_SIZE);
        return -MML_ERR_PARAMETER;
    }
    
    if (0 == len)
    {
        LOG_ERROR("module name is none.\n");
        return -MML_ERR_PARAMETER;
    }

    if (MML_PID_NUM <= v_pid)
    {
        LOG_ERROR("module pid too large. pid(%d) max(%d)\n",
            v_pid, MML_PID_NUM - 1);
        return -MML_ERR_PARAMETER;
    }

    if (0 != strlen(g_mmlModules[v_pid].name))
    {
        LOG_ERROR("module pid corrupt. new(%s) old(%s)\n",
            v_module->name, g_mmlModules[v_pid].name);
        return -MML_ERR_PID_CORRUPT;
    }

    strncpy(g_mmlModules[v_pid].name ,v_module->name, len);
    g_mmlModules[v_pid].DoCmd = v_module->DoCmd;

    return 0;
}

/*******************************************************************************
函数名称: MML_Unregister
功能说明: 无
输入参数: 无
输出参数: 无
返 回 值:
    >=0: 成功
    < 0: 错误代码
作    者: 曾华荣 (zeng_hr@163.com)
修改时间: 2012年4月15日
说    明: 无
*******************************************************************************/
int32_t MML_Unregister(uint32_t v_pid)
{
    if (MML_PID_NUM <= v_pid)
    {
        LOG_ERROR("module pid too large. pid(%d) max(%d)\n",
            v_pid, MML_PID_NUM - 1);
        return -MML_ERR_PARAMETER;
    }

    g_mmlModules[v_pid].name[0] = 0;
    
    return 0;
}

static int MML_Init(void)
{
    memset(&g_kCmd, 0, sizeof(g_kCmd));
    memset(&g_mmlModules, 0, sizeof(g_mmlModules));
    OS_MUTEX_INIT(&g_printMutex);
    
	return 0;
}

static void MML_Exit(void)
{
    return;
}

#include <linux/fs.h>
#include <linux/cdev.h>

static int devMajor = 0;
static unsigned char devBuffer[1024*1024];
struct cdev cdev;

static ssize_t Dev_Read(struct file *filp, char *buf, size_t count, 
    loff_t *pos)
{
    ssize_t readCnt = count;

    LOG_INFO("Dev read. count(%d) pos(%ld)\n", count, *pos);

    if (*pos >= sizeof(devBuffer))
    {
        LOG_ERROR("Read buffer overlap\n");
        *pos = sizeof(devBuffer);
        return 0;
    }

    if ((count + *pos) > sizeof(devBuffer))
    {
        LOG_ERROR("count + pos > buffer size\n");
        readCnt = sizeof(devBuffer) - *pos;
    }

    copy_to_user(buf, &devBuffer[*pos], readCnt);
    *pos += readCnt;

    return readCnt;
}

static ssize_t Dev_Write(struct file *filp, const char *buf, size_t count,
    loff_t *pos)
{
    ssize_t writeCnt = count;

    LOG_INFO("Dev write. count(%d) pos(%ld)\n", count, *pos);

    if (*pos >= sizeof(devBuffer))
    {
        LOG_ERROR("Write buffer overlap\n");
        *pos = sizeof(devBuffer);
        return 0;
    }

    if ((count + *pos) > sizeof(devBuffer))
    {
        LOG_ERROR("count + pos > buffer size\n");
        writeCnt = sizeof(devBuffer) - *pos;
    }

    copy_from_user(&devBuffer[*pos], buf, writeCnt);
    *pos += writeCnt;

    return writeCnt;
}

static int Dev_Ioctl(struct inode *inode, struct file *file,
    unsigned int cmd, unsigned long arg)
{
    if (_IOC_TYPE(cmd) != MML_CMD_MAGIC)
    {
        LOG_ERROR("ioctl magic not match. cmd(%d) type(%d)\n", cmd, _IOC_TYPE(cmd));
        return -ENOTTY;
    }
    
    return MML_Ioctl(cmd, (void *)arg);
}

static struct file_operations devFops
= {
    owner:THIS_MODULE,
    write:Dev_Write,
    read:Dev_Read,
    ioctl:Dev_Ioctl,
};

static int __init Dev_Init(void)
{
    int32_t ret = 0;
    
    LOG_INFO("Module mml init start...\n");
    
    /* Driver register */
    devMajor = register_chrdev(0, MML_DEV_NAME, &devFops);
    if(devMajor < 0)
    {
        LOG_ERROR("Register device failed! name(%s)\n", MML_DEV_NAME);
        return devMajor;
    }

    LOG_INFO("Register device success! name(%s) major(%d)\n",
        MML_DEV_NAME, devMajor);

    /*初始化cdev结构，并传递file_operations结构指针*/
    cdev_init(&cdev, &devFops);
    //cdev.owner = THIS_MODULE;
    /*指定所属模块*/
    //cdev.ops = &devFops;
    /* 注册字符设备 */
    ret = cdev_add(&cdev, MKDEV(devMajor, 0), 1);
    if (ret < 0)
    {
        LOG_ERROR("Add device failed! name: %s, major: %d\n",
            MML_DEV_NAME, devMajor);
    }

    MML_Init();

    return 0;
}

static void __exit Dev_Exit(void)
{
    LOG_INFO("Module mml exit...\n");

    /* Driver unregister */
    if(devMajor >= 0)
    {
        cdev_del(&cdev);   /*注销设备*/
        unregister_chrdev(devMajor, MML_DEV_NAME);
    }

    MML_Exit();

    return;
}

module_init(Dev_Init);
module_exit(Dev_Exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("axen");


