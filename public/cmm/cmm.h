/*******************************************************************************

            版权所有(C), 2011~2014, 曾华荣 (zeng_hr@163.com)
********************************************************************************
文 件 名: OSP_MEM.H
作    者: 曾华荣 (zeng_hr@163.com)
版    本: 1.00
日    期: 2011年4月24日
功能描述: 
函数列表: 
    1. ...: 
修改历史: 
    版本：1.00  作者: 曾华荣 (zeng_hr@163.com)  日期: 2011年4月24日
--------------------------------------------------------------------------------
    1. 初始版本
*******************************************************************************/
#ifndef __OSP_MEM_H__
#define __OSP_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

extern void MemInit(void);
extern unsigned long long MemGetTotal(void);
extern unsigned long long MemGetMax(void);
extern unsigned long long MemExit(void);
extern void *MemAlloc(unsigned int v_size);
extern void MemFree(void *v_pMem, unsigned int v_size);
extern void *MemAlignedAlloc(unsigned int v_size, unsigned int v_alignment);
extern void MemAlignedFree(void *v_pMem, unsigned int v_size);


#ifdef __cplusplus
}
#endif /* End of __cplusplus */

#endif /* End of __OSP_MEM_H__ */

