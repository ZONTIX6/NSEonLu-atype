#include "tuning_flash.h"
#include "tuning_storage.h"
#include "runtime_tuning.h"
#include "bsp_tuning_flash.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>

static TuningStorage storage;
static TuningStoredImage pending;
static TuningFlashSaveGuard save_guard;
static void *save_context;

bool TuningFlashRegisterSaveGuard(TuningFlashSaveGuard guard,void *context)
{
    if(guard==NULL || save_guard!=NULL) return false;
    save_guard=guard;save_context=context;return true;
}
void TuningFlashInit(void)
{
    const TuningStorageIO io={.banks={BSPTuningFlashBank(0),BSPTuningFlashBank(1)},
        .bank_words=BSPTuningFlashBankWords(),.program=BSPTuningFlashProgram,.erase=BSPTuningFlashErase};
    save_guard=NULL;save_context=NULL;
    TuningStorageInit(&storage,&io);
    RuntimeTuningPersistenceInit(storage.valid ? &storage.image : NULL);
}
void TuningFlashPoll(void)
{
    if(!RuntimeTuningPendingImage(&pending) || !RuntimeTuningBeginFlash()) return;
    /* Recheck the entire vehicle immediately before write while control tasks
     * cannot resume. The guard must not block, allocate, or perform communication. */
    vTaskSuspendAll();
    uint32_t now=HAL_GetTick();
    if(save_guard==NULL || !save_guard(save_context,now)) {(void)xTaskResumeAll();RuntimeTuningEndFlash();return;}
    bool success=false;
    if(BSPTuningFlashBegin()) {
        success=TuningStorageSave(&storage,&pending);
        if(!BSPTuningFlashEnd()) success=false;
    }
    (void)xTaskResumeAll();
    RuntimeTuningPersistenceComplete(&pending,success);
    RuntimeTuningEndFlash();
}
