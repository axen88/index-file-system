/*******************************************************************************

            ��Ȩ����(C), 2011~2014, ������ (zeng_hr@163.com)
********************************************************************************
�� �� ��: OSP_MEM.H
��    ��: ������ (zeng_hr@163.com)
��    ��: 1.00
��    ��: 2011��4��24��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��4��24��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
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

