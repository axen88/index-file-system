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

            ��Ȩ����(C), 2012~2015, AXEN������
********************************************************************************
�� �� ��: INDEX_BLOCK_RW.C
��    ��: 1.00
��    ��: 2012��6��16��
��������: 
�����б�: 
    1. ...: 
�޸���ʷ: 
    �汾��1.00  ����: ������ (zeng_hr@163.com)  ����: 2012��6��16��
--------------------------------------------------------------------------------
    1. ��ʼ�汾
*******************************************************************************/
#include "index_if.h"
MODULE(PID_INDEX);
#include "os_log.h"

/*******************************************************************************
��������: index_write_block
����˵��: ����һ����д�����ݣ���������������
�������:
    hnd         : �����ϵͳ�������
    buf       : Ҫд�������
    size     : Ҫд�����ݵĴ�С�����ֽ�Ϊ��λ
    start_lba: Ҫд��������ڿ��е���ʼλ��
�������:
    vbn    : д��Ŀ��ַ
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_write_block(BLOCK_HANDLE_S * hnd, void * buf, uint32_t size,
    uint32_t start_lba, uint64_t * vbn)
{
    int32_t ret = 0;
    uint64_t tmp_start_lba = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size)
        || (NULL == vbn))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d) vbn(%p)\n",
            hnd, buf, size, vbn);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* ����1���� */
    ret = block_alloc(hnd, 1, vbn);
    if (ret < 0)
    {
        LOG_ERROR("Allocate blocks failed. hnd(%p) blocks(%d) ret(%d)\n",
            hnd, 1, ret);
        return ret;
    }

    tmp_start_lba = start_lba + *vbn * hnd->sb.sectors_per_block
        + hnd->sb.start_lba;

    /* д������ */
    ret = os_disk_pwrite(hnd->file_hnd, buf, size, tmp_start_lba);
    if (ret != (int32_t) size)
    {
        LOG_ERROR("Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, tmp_start_lba, ret);
        (void) block_free(hnd, *vbn, 1);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

/*******************************************************************************
��������: index_update_block
����˵��: ����ָ�����е����ݣ���������������
�������:
    hnd      : �����ϵͳ�������
    buf    : Ҫд�������
    size    : Ҫд�����ݵĴ�С�����ֽ�Ϊ��λ
    start_lba: Ҫд��������ڿ��е���ʼλ��
    vbn     : Ҫд��Ŀ��ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_update_block(BLOCK_HANDLE_S * hnd, void * buf, uint32_t size,
    uint32_t start_lba, uint64_t vbn)
{
    int32_t ret = 0;
    uint64_t tmp_start_lba = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    tmp_start_lba = start_lba + vbn * hnd->sb.sectors_per_block
        + hnd->sb.start_lba;
    ret = os_disk_pwrite(hnd->file_hnd, buf, size, tmp_start_lba);
    if (ret != (int32_t) size)
    {
        LOG_ERROR("Write lba failed. file_hnd(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, tmp_start_lba, ret);
    }

    return ret;
}

/*******************************************************************************
��������: index_read_block
����˵��: ��ȡָ�����е����ݣ���������������
�������:
    hnd         : �����ϵͳ�������
    buf       : ���������ݴ洢��λ��
    size     : Ҫ�������ݵĴ�С�����ֽ�Ϊ��λ
    start_lba: Ҫ�����������ڿ��е���ʼλ��
    vbn     : Ҫ��ȡ�Ŀ��ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_read_block(BLOCK_HANDLE_S * hnd, void * buf, uint32_t size,
    uint32_t start_lba, uint64_t vbn)
{
    int32_t ret = 0;
    uint64_t tmp_start_lba = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    tmp_start_lba = start_lba + vbn * hnd->sb.sectors_per_block +
        hnd->sb.start_lba;
    ret = os_disk_pread(hnd->file_hnd, buf, size, tmp_start_lba);
    if (ret != (int32_t) size)
    {
        LOG_ERROR("Read lba failed. file_hnd(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, tmp_start_lba, ret);
    }

    return ret;
}

/*******************************************************************************
��������: assemble_object
����˵��: ��������װ�ɿ��Լ���Ƿ�д�����ĸ�ʽ
�������:
    obj   : Ҫ�����Ŀ�����
�������: ��
�� �� ֵ: ��
˵    ��: ��
*******************************************************************************/
void assemble_object(OBJECT_HEADER_S * obj)
{
    uint16_t *foot = NULL;  /* ���2���ֽ����ڵĵ�ַ */

    /* ���������� */
    ASSERT(NULL != obj);
    ASSERT(obj->alloc_size >= obj->real_size);

    foot = (uint16_t *) ((uint8_t *) obj + obj->alloc_size - sizeof(uint16_t));

    obj->fixup = *foot;
    *foot = obj->seq_no;

    return;
}

/*******************************************************************************
��������: fixup_object
����˵��: �������ݻ�ԭ����
�������:
    obj   : Ҫ�����Ŀ�����
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t fixup_object(OBJECT_HEADER_S * obj)
{
    uint16_t *foot = NULL;  /* ���2���ֽ����ڵĵ�ַ */

    /* ���������� */
    ASSERT(NULL != obj);
    ASSERT(obj->alloc_size >= obj->real_size);

    foot = (uint16_t *) ((uint8_t *) obj + obj->alloc_size - sizeof(uint16_t));

    if (*foot == obj->seq_no)
    {
        *foot = obj->fixup;
    }
    else
    {   /* ˵�����ݲ����� */
        LOG_ERROR("Found invalid object. obj(%p)\n", obj);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    obj->seq_no++;

    return 0;
}

/*******************************************************************************
��������: verify_object
����˵��: У������Ƿ�Ϸ�
�������:
    obj     : Ҫ�����Ŀ�����
    objid    : ����Ӧ�е�id
    alloc_size: �����ռ�ÿռ�
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t verify_object(OBJECT_HEADER_S * obj, uint32_t objid,
    uint32_t alloc_size)
{
    ASSERT(NULL != obj);
    ASSERT(0 != objid);
    ASSERT(0 != alloc_size);

    if (obj->blk_id != objid)
    {
        LOG_ERROR("Object id not match. obj(%p) objid(%x) expect(%x)",
            obj, obj->blk_id, objid);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if (obj->alloc_size != alloc_size)
    {
        LOG_ERROR("alloc_size not match. obj(%p) alloc_size(%d) expect(%d)",
            obj, obj->alloc_size, alloc_size);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    if ((obj->real_size > obj->alloc_size)
        || (obj->real_size < sizeof(OBJECT_HEADER_S)))
    {
        LOG_ERROR("real_size not match. obj(%p) real_size(%d) alloc_size(%d) sizeof(%d)",
            obj, obj->real_size, alloc_size, (uint32_t)sizeof(OBJECT_HEADER_S));
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    return 0;
}

/*******************************************************************************
��������: index_write_block_fixup
����˵��: ����һ����д�����ݣ�����������fixup����
�������:
    hnd        : �����ϵͳ�������
    obj    : Ҫд�������
�������:
    vbn   : д��Ŀ��ַ
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_write_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t * vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == obj) || (NULL == vbn))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p) vbn(%p)\n", hnd, obj, vbn);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* ���ñ���׼������ */
    assemble_object(obj);

    /* �������� */
    ret = index_write_block(hnd, obj, obj->alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Write object failed. hnd(%p) obj(%p) ret(%d)\n", hnd, obj, ret);
        /* ʧ�ܲ��ܷ���return */
    }

    /* ���ڴ��е������޸� */
    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    return ret;
}

/*******************************************************************************
��������: index_update_block_fixup
����˵��: ����ָ�����е����ݣ�����������fixup����
�������:
    hnd      : �����ϵͳ�������
    obj  : Ҫд�������
    vbn  : Ҫд��Ŀ��ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_update_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* ���ñ���׼������ */
    assemble_object(obj);

    /* �������� */
    ret = index_update_block(hnd, obj, obj->alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Update object failed. hnd(%p) obj(%p) ret(%d)\n",
            hnd, obj, ret);
        /* ʧ�ܲ��ܷ���return */
    }

    /* ���ڴ��е������޸� */
    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    return ret;
}

/*******************************************************************************
��������: index_read_block_fixup
����˵��: ��ȡָ�����е����ݣ�����������fixup����
�������:
    hnd         : �����ϵͳ�������
    obj     : Ҫ�����Ķ���洢��λ��
    objid    : ����id
    alloc_size: Ҫ�������ݵĴ�С�����ֽ�Ϊ��λ
    vbn     : Ҫд��Ŀ��ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_read_block_fixup(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size)
{
    int32_t ret = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    /* ���ݶ�ȡ */
    ret = index_read_block(hnd, obj, alloc_size, 0, vbn);
    if (0 > ret)
    {
        LOG_ERROR("Read data failed. hnd(%p) obj(%p) objid(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            hnd, obj, objid, alloc_size, vbn, ret);
        return ret;
    }

    ret = verify_object(obj, objid, alloc_size);
    if (0 > ret)
    {
        LOG_ERROR("Verify object failed. hnd(%p) obj(%p) objid(%x) alloc_size(%d) vbn(%lld) ret(%d)",
            hnd, obj, objid, alloc_size, vbn, ret);
        return ret;
    }

    /* ���ڴ��е������޸� */
    return fixup_object(obj);
}

/*******************************************************************************
��������: check_obj
����˵��: �����������Ϸ��ԣ�����ȡ���С
�������:
    hnd      : �����ϵͳ�������
    obj  : ���ݴ洢��λ��
�������: ��
�� �� ֵ:
    >=0: ���С
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t check_obj(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj)
{
    /* ���������� */
    if ((NULL == hnd) || (NULL == obj))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p)\n", hnd, obj);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    if ((obj->real_size > obj->alloc_size)
        || (obj->alloc_size > hnd->sb.block_size))
    {
        LOG_ERROR("size is invalid. real_size(%d) alloc_size(%d) block_size(%d)\n",
            obj->real_size, obj->alloc_size, hnd->sb.block_size);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    return 0;
}

/*******************************************************************************
��������: index_update_block_pingpong_init
����˵��: д�����ݣ�����������pingpong����
�������:
    hnd      : �����ϵͳ�������
    obj  : Ҫд�������
    vbn : д��Ŀ��ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_update_block_pingpong_init(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;

    ret = check_obj(hnd, obj);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. hnd(%p) buf(%p) ret(%d)", hnd, obj, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = hnd->sb.block_size;

    buf = OS_MALLOC(block_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    /* ���ñ���׼������ */
    assemble_object(obj);

    memset(buf, 0, block_size);
    memcpy(buf + (obj->alloc_size * (obj->seq_no % (block_size / obj->alloc_size))), obj, obj->real_size);       /*lint !e414 */

    /* �������� */
    ret = index_update_block(hnd, buf, block_size, 0, vbn);

    OS_FREE(buf);
    buf = NULL;

    /* ���ڴ��е������޸� */
    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    if (ret != (int32_t)block_size)
    {
        LOG_ERROR("Update block data failed. hnd(%p) obj(%p) size(%d) vbn(%lld) ret(%d)\n",
            hnd, obj, block_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

/*******************************************************************************
��������: index_update_block_pingpong
����˵��: ����ָ�����е����ݣ�����������pingpong����
�������:
    hnd      : �����ϵͳ�������
    obj  : Ҫд�������
    vbn  : Ҫд��Ŀ��ַ
�������: ��
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_update_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn)
{
    int32_t ret = 0;
    int32_t ret2 = 0;
    uint32_t block_size = 0;
    uint32_t update_lba = 0;

    ret = check_obj(hnd, obj);
    if (ret < 0)
    {
        LOG_ERROR("Get blocksize failed. hnd(%p) buf(%p) ret(%d)\n", hnd, obj, ret);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    block_size = hnd->sb.block_size;

    /* ���ñ���׼������ */
    assemble_object(obj);

    /* �������� */
    update_lba = (obj->alloc_size * (obj->seq_no % (block_size / obj->alloc_size))) >> BYTES_PER_SECTOR_SHIFT;    /*lint !e414 */

    ret = index_update_block(hnd, obj, obj->alloc_size, update_lba, vbn);

    /* ���ڴ��е������޸� */
    ret2 = fixup_object(obj);
    if (0 > ret2)
    {
        LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret2);
        return ret2;
    }

    if (ret != (int32_t) obj->alloc_size)
    {
        LOG_ERROR("Update block data failed. hnd(%p) obj(%p) size(%d) vbn(%lld) ret(%d)\n",
            hnd, obj, obj->alloc_size, vbn, ret);
        return -FILE_BLOCK_ERR_WRITE;
    }

    return 0;
}

/*******************************************************************************
��������: get_last_correct_dat
����˵��: �ӿ��л�ȡһ�ݺ��ʵ���ȷ������
�������:
    buf       : ���ݴ洢��λ��
    objid     : ����id
    alloc_size : ����ռ�ÿռ��С�����ֽ�Ϊ��λ
    cnt       : ��ǰbuf�д洢�Ķ�����Ŀ
�������: ��
�� �� ֵ:
    !=NULL: �ɹ�
    ==NULL: ����
˵    ��: ��
*******************************************************************************/
OBJECT_HEADER_S *get_last_correct_dat(uint8_t * buf, uint32_t objid,
    uint32_t alloc_size, uint32_t cnt)
{
    uint32_t i = 0;
    uint32_t prev_i = 0;
    uint16_t prev_seq_no = 0;
    int32_t ret = 0;
    OBJECT_HEADER_S *obj = NULL;

    ASSERT(NULL != buf);
    ASSERT(0 != alloc_size);

    for (i = 0; i < cnt; i++)
    {
        obj = (OBJECT_HEADER_S *)(buf + alloc_size * i);

        if (0 == obj->blk_id)
        {       /* �ҵ���δд�������� */
            break;
        }

        if ((0 != i) && ((prev_seq_no + 1) != obj->seq_no))
        {       /* ��ǰ�ͺ�������ݲ������µ� */
            break;
        }

        prev_seq_no = obj->seq_no;
        prev_i = i;
    }

    /* ���һ���Բ��޸����� */
    for (i = 0; i < cnt; i++)
    {
        obj = (OBJECT_HEADER_S *)(buf + alloc_size * prev_i);

        if (0 == obj->blk_id)
        {       /* �ҵ���δд�������� */
            break;
        }

        ret = verify_object(obj, objid, alloc_size);
        if (0 > ret)
        {       /* У��ʧ�� */
            LOG_ERROR("Verify object failed. buf(%p) obj(%p) objid(%x) alloc_size(%d) ret(%d)\n",
                buf, obj, objid, alloc_size, ret);
        }
        else
        {       /* ���ڴ��е������޸� */
            ret = fixup_object(obj);
            if (0 <= ret)
            {
                return obj;
            }

            LOG_ERROR("Fixup object failed. obj(%p) ret(%d)\n", obj, ret);
        }

        if (0 == prev_i)
        {
            prev_i = cnt;
        }

        prev_i--;
    }

    LOG_ERROR("No valid object. buf(%p) obj(%p) objid(%x) alloc_size(%d)",
        buf, obj, objid, alloc_size);

    return NULL;
}

/*******************************************************************************
��������: index_read_block_pingpong
����˵��: ��ȡָ�����е����ݣ�����������pingpong����
�������:
    hnd           : �����ϵͳ�������
    obj       : ���������ݴ洢��λ��
    vbn       : �������ڵĿ��ַ
    objid      : ����id
    alloc_size  : Ҫ�������ݵĴ�С�����ֽ�Ϊ��λ
�������: ��
�� �� ֵ:
    >=0: �ɹ�
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_read_block_pingpong(BLOCK_HANDLE_S * hnd, OBJECT_HEADER_S * obj,
    uint64_t vbn, uint32_t objid, uint32_t alloc_size)
{
    int32_t ret = 0;
    uint32_t block_size = 0;
    uint8_t *buf = NULL;
    OBJECT_HEADER_S * tmp_obj = NULL;

    /* ���������� */
    if ((NULL == hnd) || (NULL == obj) || (0 == alloc_size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) obj(%p) alloc_size(%d)\n", hnd, obj, alloc_size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    block_size = hnd->sb.block_size;

    if (block_size < alloc_size)
    {
        LOG_ERROR("size is invalid. block_size(%d) alloc_size(%d)\n", block_size, alloc_size);
        return -FILE_BLOCK_ERR_INVALID_PARA;
    }

    buf = OS_MALLOC(block_size);
    if (NULL == buf)
    {
        LOG_ERROR("Allocate memory failed. size(%d)\n", block_size);
        return -FILE_BLOCK_ERR_ALLOCATE_MEMORY;
    }

    /* ���ݶ�ȡ */
    ret = index_read_block(hnd, buf, block_size, 0, vbn);
    if (0 > ret)
    {
        OS_FREE(buf);
        return ret;
    }

    tmp_obj = get_last_correct_dat(buf, objid, alloc_size, block_size / alloc_size);
    if (NULL == tmp_obj)
    {
        LOG_ERROR("Get invalid object. hnd(%p) obj(%p) vbn(%lld)\n", hnd, tmp_obj, vbn);
        OS_FREE(buf);
        return -FILE_BLOCK_ERR_INVALID_OBJECT;
    }

    ASSERT(alloc_size >= tmp_obj->real_size);
    memcpy(obj, tmp_obj, tmp_obj->real_size);
    OS_FREE(buf);

    return 0;
}

/*******************************************************************************
��������: index_update_sectors
����˵��: д�����ݵ�ָ��������
�������:
    hnd         : �����ϵͳ�������
    buf       : Ҫд�������
    size     : Ҫд�����ݵĴ�С�����ֽ�Ϊ��λ
    start_lba: Ҫд������ݵ���ʼ������ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ���������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_update_sectors(BLOCK_HANDLE_S * hnd, void * buf, uint32_t size,
    uint64_t start_lba)
{
    int32_t ret = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    start_lba += hnd->sb.start_lba;
    ret = os_disk_pwrite(hnd->file_hnd, buf, size, start_lba);
    if (ret != (int32_t)size)
    {
        LOG_ERROR("Write lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, start_lba, ret);
    }

    return ret;
}

/*******************************************************************************
��������: index_read_sectors
����˵��: ��ָ��������������
�������:
    hnd         : �����ϵͳ�������
    buf       : Ҫ����������
    size     : Ҫ�������ݵĴ�С�����ֽ�Ϊ��λ
    start_lba: Ҫ���������ݵ���ʼ������ַ
�������: ��
�� �� ֵ:
    >=0: �ɹ���������
    < 0: �������
˵    ��: ��
*******************************************************************************/
int32_t index_read_sectors(BLOCK_HANDLE_S * hnd, void * buf, uint32_t size,
    uint64_t start_lba)
{
    int32_t ret = 0;

    /* ���������� */
    if ((NULL == hnd) || (NULL == buf) || (0 >= (int32_t) size))
    {
        LOG_ERROR("Invalid parameter. hnd(%p) buf(%p) size(%d)\n", hnd, buf, size);
        return -FILE_BLOCK_ERR_PARAMETER;
    }

    start_lba += hnd->sb.start_lba;
    ret = os_disk_pread(hnd->file_hnd, buf, size, start_lba);
    if (ret != (int32_t)size)
    {
        LOG_ERROR("Read lba failed. f(%p) buf(%p) size(%d) lba(%lld) ret(%d)\n",
            hnd->file_hnd, buf, size, start_lba, ret);
    }

    return ret;
}

EXPORT_SYMBOL(index_update_block);
EXPORT_SYMBOL(index_write_block);
EXPORT_SYMBOL(index_read_block);

EXPORT_SYMBOL(index_write_block_fixup);
EXPORT_SYMBOL(index_update_block_fixup);
EXPORT_SYMBOL(index_read_block_fixup);

EXPORT_SYMBOL(index_update_block_pingpong_init);
EXPORT_SYMBOL(index_update_block_pingpong);
EXPORT_SYMBOL(index_read_block_pingpong);

EXPORT_SYMBOL(index_update_sectors);
EXPORT_SYMBOL(index_read_sectors);

