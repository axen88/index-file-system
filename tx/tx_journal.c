

#include "os_adapter.h"
#include "globals.h"
#include "utils.h"
#include "log.h"
#include "tx_cache.h"
#include "tx_journal.h"

MODULE(MID_JOURNAL);


///TODO: �������е������ݷ�װ����־������
int tx_write_journal(tx_t *tx)
{
    tx->mgr->log_sn++;
    return SUCCESS;
}

