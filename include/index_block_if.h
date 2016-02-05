/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*******************************************************************************

            ��Ȩ����(C), 2011~2014, AXEN������
********************************************************************************
�� �� ��: OS_BLOCK_MAN.H
��    ��: 1.00
��    ��: 2011��6��19��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2011��6��19��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#ifndef __OS_BLOCK_MAN_H__
#define __OS_BLOCK_MAN_H__

#ifdef  __cplusplus
extern "C"
{
#endif

/* �ṹ��BLOCK_HANDLE_S��uiFlags����λ�ĺ��� */
#define FLAG_NOSPACE     0x00000002     /* �Ƿ��޿ռ��� */
#define FLAG_DIRTY       0x00000001     /* �б�Ƿ����˱仯 */

/* �����ϵͳ�����ṹ */
typedef struct tagBLOCK_HANDLE_S
{
    void *file_hnd;                       /* �ļ�������� */
    BITMAP_HANDLE *bitmap_hnd;                     /* λͼ������� */
    char name[FILE_NAME_SIZE];      /* �ļ��� */
    BLOCK_BOOT_SECTOR_S sb;           /* ������ */
    uint32_t flags;                    /* �����ϵͳ��һЩ��ʶ */
    OS_RWLOCK rwlock;                        /* ���������λͼ���� */
} BLOCK_HANDLE_S;

/* ��������򴴽����򿪡��رյȲ��� */
extern int32_t block_create(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t total_blocks, uint32_t block_size_shift, uint32_t reserved_sectors,
    uint64_t start_lba);
extern int32_t block_open(BLOCK_HANDLE_S ** hnd, const char * path,
    uint64_t start_lba);
extern int32_t block_close(BLOCK_HANDLE_S * f);
extern int32_t block_reset_bitmap(BLOCK_HANDLE_S * hnd);
extern bool_t block_need_fixup(BLOCK_HANDLE_S * hnd);
extern int32_t block_finish_fixup(BLOCK_HANDLE_S * hnd);

/* ��������ͷŽӿ� */
extern int32_t block_set_status(BLOCK_HANDLE_S * hnd, uint64_t start_vbn, uint32_t blk_cnt,
    bool_t is_used);
extern int32_t block_alloc(BLOCK_HANDLE_S * hnd, uint32_t blk_cnt,
    uint64_t * start_vbn);
#define block_free(hnd, start_vbn, blk_cnt) \
    block_set_status(hnd, start_vbn, blk_cnt, B_FALSE)

/* ԭʼ�����ݶ�д�ӿ� */
extern int32_t index_write_block(BLOCK_HANDLE_S * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t * vbn);
extern int32_t index_update_block(BLOCK_HANDLE_S * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);
extern int32_t index_read_block(BLOCK_HANDLE_S * hnd, void * buf,
    uint32_t size, uint32_t start_lba, uint64_t vbn);

/* ��fixup���ԵĿ����ݶ�д�ӿ� */
extern int32_t index_write_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t * vbn);
extern int32_t index_update_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_read_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
     uint64_t vbn, uint32_t obj_id, uint32_t alloc_size);

/* ��pingpong���ԵĿ����ݶ�д�ӿ� */
extern int32_t index_update_block_pingpong_init(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_update_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn);
extern int32_t index_read_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn, uint32_t obj_id, uint32_t alloc_size);

/* ֱ�Ӷ�дlba��ַ */
extern int32_t index_update_sectors(BLOCK_HANDLE_S * f, void * buf,
    uint32_t size, uint64_t lba);
extern int32_t index_read_sectors(BLOCK_HANDLE_S * f, void * buf,
    uint32_t size, uint64_t lba);

/* ��������ز������ڲ����� */
extern int32_t block_update_super_block(BLOCK_HANDLE_S * hnd);

extern int32_t check_and_set_fixup_flag(BLOCK_HANDLE_S * hnd);

#ifdef  __cplusplus
}
#endif

#endif            
