#include "board.h"

#include "os.h"
#include "lib_mem.h"

#define APP_TASK_STK_SIZE  512u
#define APP_TASK_STK_LIMIT 32u

static OS_TCB  AppTaskStartTCB;
static CPU_STK AppTaskStartStk[APP_TASK_STK_SIZE];

static OS_TCB  AppTaskTimerTCB;
static CPU_STK AppTaskTimerStk[APP_TASK_STK_SIZE];

static OS_TCB  AppTaskSemTCB;
static CPU_STK AppTaskSemStk[APP_TASK_STK_SIZE];

static OS_TCB  AppTaskQSendTCB;
static CPU_STK AppTaskQSendStk[APP_TASK_STK_SIZE];
static OS_TCB  AppTaskQReceiveTCB;
static CPU_STK AppTaskQReceiveStk[APP_TASK_STK_SIZE];

static OS_TCB  AppTaskGPTimerTCB;
static CPU_STK AppTaskGPTimerStk[APP_TASK_STK_SIZE];

#define SOFTWARE_TIMER_PERIOD_MS 999
#define QUEUE_SEND_PERIOD_MS     200
#define SEMAPHORE_PERIOD_MS      500
#define GPTIMER_PERIOD_MS        250

OS_SEM TickSem;
OS_Q   Queue;
OS_SEM GPTimerSem;

void AppTaskTimer(void *pTaskArg)
{
  OS_ERR err;
  while (1) {
    OSTimeDlyHMSM(0, 0, 0, SOFTWARE_TIMER_PERIOD_MS, OS_OPT_TIME_HMSM_STRICT, &err);
    GPIO_Toggle(LED_GPIO, GPIO_BIT1);
  }
}

void App_OS_TimeTickHook(void)
{
  OS_ERR err;
  static int count = 0;
  if (++count >= SEMAPHORE_PERIOD_MS) {
    OSSemPost(&TickSem, OS_OPT_POST_1, &err);
    count = 0;
  }
}

void AppTaskSem(void *pTaskArg)
{
  OS_ERR err;
  while (1) {
    OSSemPend(&TickSem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
    GPIO_Toggle(LED_GPIO, GPIO_BIT2);
  }
}

void AppTaskQSend(void *pTaskArg)
{
  OS_ERR err;
  while (1) {
    OSTimeDlyHMSM(0, 0, 0, QUEUE_SEND_PERIOD_MS, OS_OPT_TIME_HMSM_STRICT, &err);
    OSQPost(&Queue, NULL, 1, OS_OPT_POST_ALL, &err);
  }
}

void AppTaskQReceive(void *pTaskArg)
{
  OS_ERR err;
  OS_MSG_SIZE msg_size;
  while (1) {
    OSQPend(&Queue, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
    GPIO_Toggle(LED_GPIO, GPIO_BIT3);
  }
}

void GPTIMER0_isr(void)
{
  printf("isr\n");
  OS_ERR err;
  // Clear GPTIMER interrupt flag
  GPTIMER_ClearFlagUpdate(GPTIMER0);
  // Notify the task
  OSSemPost(&GPTimerSem, OS_OPT_POST_1, &err);
}

void AppTaskGPTimer(void *pTaskArg)
{
  OS_ERR err;
  while (1) {
    OSSemPend(&GPTimerSem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);
    printf("task\n");
    GPIO_Toggle(LED_GPIO, GPIO_BIT4);
  }
}

void AppTaskStart(void *pTaskArg)
{
  OS_ERR err;
  OSStatTaskCPUUsageInit(&err);

  // Simple timer using OSTimeDlyHMSM
  OSTaskCreate(&AppTaskTimerTCB, "Timer", AppTaskTimer, 0, 2,
               AppTaskTimerStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);

  // Semaphore from App_OS_TimeTickHook to task
  OS_AppTimeTickHookPtr = App_OS_TimeTickHook;
  OSSemCreate(&TickSem, "Tick Semaphore", 0, &err);
  OSTaskCreate(&AppTaskSemTCB, "Semaphore", AppTaskSem, 0, 3,
               AppTaskSemStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);

  // Message queue from one task to another
  OSQCreate(&Queue, "Message queue", 4, &err);
  OSTaskCreate(&AppTaskQSendTCB, "Queue Send", AppTaskQSend, 0, 4,
               AppTaskQSendStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);
  OSTaskCreate(&AppTaskQReceiveTCB, "Queue Receive", AppTaskQReceive, 0, 5,
               AppTaskQReceiveStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);

  // GPTIMER with interrupt
  PERIPHERAL_ENABLE(GPTIMER, 0);
  INT_EnableIRQ(GPTIMER0_IRQn, PLIC_MAX_PRIORITY);
  GPTIMER_EnableIntUpdate(GPTIMER0);
  GPTIMER_InitTypeDef init;
  GPTIMER_StructInit(&init);
  init.Autoreload = SYS_GetSysClkFreq() / 1000 * GPTIMER_PERIOD_MS;
  GPTIMER_Init(GPTIMER0, &init);
  GPTIMER_EnableCounter(GPTIMER0);

  OSSemCreate(&GPTimerSem, "GPTimer Semaphore", 0, &err);
  OSTaskCreate(&AppTaskGPTimerTCB, "GPTimer task", AppTaskGPTimer, 0, 6,
               AppTaskGPTimerStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);

  // Delete the start up task
  OSTaskDel(NULL, &err);
}

int main(void)
{
  board_init();
  CPU_Init();
  Mem_Init();

  OS_ERR err;
  OSInit(&err);
  OSTaskCreate(&AppTaskStartTCB, "Start", AppTaskStart, 0, 1,
               AppTaskStartStk, APP_TASK_STK_LIMIT, APP_TASK_STK_SIZE,
               0, 0, 0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);
  OSStart(&err);
}
