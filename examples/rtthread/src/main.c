#include "rtthread.h"
#include "rthw.h"
#include "board.h"

#ifdef RT_USING_ULOG
#define LOG_TAG "example"
#define LOG_LVL LOG_LVL_DBG
#include "ulog.h"
#endif

#define SOFTWARE_TIMER_PERIOD_MS 1000
#define QUEUE_SEND_PERIOD_MS     200
#define SEMAPHORE_PERIOD_MS      500
#define GPTIMER_PERIOD_MS        250

#define THREAD_STACK_SIZE 512
#define THREAD_TIMESLICE  5

// UART support for FINSH
void rt_hw_console_output(const char *str)
{
  UART_Send(MSG_UART, (unsigned char *)str, rt_strlen(str));
}

char rt_hw_console_getchar(void)
{
  unsigned char ch;
  if (UART_ReceiveCh(MSG_UART, &ch, 0) == 1) {
    return ch;
  } else {
    rt_thread_mdelay(10);
  }
  return -1;
}

// Semaphore example with tick hook
ALIGN(RT_ALIGN_SIZE)
static char sem_thread_stack[THREAD_STACK_SIZE];
static struct rt_thread sem_thread;
static rt_sem_t dynamic_sem;
void tick_hook(void)
{
  static int count = 0;
  if (++count >= SEMAPHORE_PERIOD_MS) {
    rt_sem_release(dynamic_sem);
    count = 0;
  }
}

void sem_example_thread(void * parameter)
{
  while(1) {
    rt_sem_take(dynamic_sem, RT_WAITING_FOREVER);
    GPIO_Toggle(LED_GPIO, GPIO_BIT2);
  }
}

// Timer example
static rt_timer_t timer = RT_NULL;
void timer_example(void *parameter)
{
  GPIO_Toggle(LED_GPIO, GPIO_BIT4);
}

// Message queue example
static struct rt_messagequeue mq;
static rt_uint8_t msg_pool[2048];

ALIGN(RT_ALIGN_SIZE)
static char mq_receive_stack[THREAD_STACK_SIZE];
static struct rt_thread mq_receive;
static void mq_receive_entry(void *parameter)
{
  char buf = 0;
  while (1) {
    rt_mq_recv(&mq, &buf, sizeof(buf), RT_WAITING_FOREVER);
    GPIO_Toggle(LED_GPIO, GPIO_BIT3);
  }
}

ALIGN(RT_ALIGN_SIZE)
static char mq_send_stack[THREAD_STACK_SIZE];
static struct rt_thread mq_send;
static void mq_send_entry(void *parameter)
{
  char buf = 'A';
  while (1) {
    rt_thread_mdelay(QUEUE_SEND_PERIOD_MS);
    rt_mq_send(&mq, &buf, 1);
  }
}

// GPTIMER example with interrupt
static rt_thread_t gptimer_thread;
static struct rt_semaphore gptimer_sem;
void GPTIMER0_isr(void)
{
  // Clear GPTIMER interrupt flag
  GPTIMER_ClearFlagUpdate(GPTIMER0);
  // Notify the thread
  rt_sem_release(&gptimer_sem);
}

void gptimer_entry(void *parameter)
{
  while (1) {
    rt_sem_take(&gptimer_sem, RT_WAITING_FOREVER);
    GPIO_Toggle(LED_GPIO, GPIO_BIT1);
  }
}

// Main thread will initialize all examples and delete itself
int main(void)
{
#ifdef RT_USING_ULOG
  ulog_init();
  LOG_D("Test ulog debug");
  LOG_I("Test ulog info");
  LOG_W("Test ulog warning");
  LOG_E("Test ulog error");
#endif
  // Semaphore example with tick hook
  rt_tick_sethook(tick_hook);
  dynamic_sem = rt_sem_create("tick_sem", 0, RT_IPC_FLAG_PRIO);
  rt_thread_init(&sem_thread, "tick_sem", sem_example_thread, NULL,
                 sem_thread_stack, sizeof(sem_thread_stack),
                 RT_THREAD_PRIORITY_MAX - 1, THREAD_TIMESLICE);
  rt_thread_startup(&sem_thread);

  // Timer example
  timer = rt_timer_create("timer", timer_example, NULL,
                          rt_tick_from_millisecond(SOFTWARE_TIMER_PERIOD_MS), RT_TIMER_FLAG_PERIODIC);
  rt_timer_start(timer);

  // Message queue example
  rt_mq_init(&mq, "mqt", msg_pool, 1, sizeof(msg_pool), RT_IPC_FLAG_PRIO);
  rt_thread_init(&mq_receive, "mq_recv", mq_receive_entry, RT_NULL,
                 &mq_receive_stack[0], sizeof(mq_receive_stack),
                 RT_THREAD_PRIORITY_MAX - 2, THREAD_TIMESLICE);
  rt_thread_startup(&mq_receive);

  rt_thread_init(&mq_send, "mq_send", mq_send_entry, RT_NULL,
                 &mq_send_stack[0], sizeof(mq_send_stack),
                 RT_THREAD_PRIORITY_MAX - 2, THREAD_TIMESLICE);
  rt_thread_startup(&mq_send);

  // GPTIMER example with interrupt
  rt_sem_init(&gptimer_sem, "gp_sem", 0, RT_IPC_FLAG_PRIO);
  gptimer_thread = rt_thread_create("gptimer", gptimer_entry, NULL, THREAD_STACK_SIZE,
                                    RT_THREAD_PRIORITY_MAX - 3, THREAD_TIMESLICE);
  rt_thread_startup(gptimer_thread);
  // Initialize GPTIMER
  PERIPHERAL_ENABLE(GPTIMER, 0);
  INT_EnableIRQ(GPTIMER0_IRQn, PLIC_MAX_PRIORITY);
  GPTIMER_EnableIntUpdate(GPTIMER0);
  GPTIMER_InitTypeDef init;
  GPTIMER_StructInit(&init);
  init.Autoreload = SYS_GetSysClkFreq() / 1000 * GPTIMER_PERIOD_MS;
  GPTIMER_Init(GPTIMER0, &init);
  GPTIMER_EnableCounter(GPTIMER0);

  return 0;
}


#ifdef RT_USING_HEAP
#define RT_HEAP_SIZE 4096
static uint32_t rt_heap[RT_HEAP_SIZE];
#endif

void rt_hw_board_init(void)
{
  board_init();
  SYS_SetMtimeDebugStop();
  rt_hw_interrupt_init();

#ifdef RT_USING_COMPONENTS_INIT
  rt_components_board_init();
#endif
#ifdef RT_USING_HEAP
  rt_system_heap_init(rt_heap, rt_heap + RT_HEAP_SIZE);
#endif
}

#ifdef RT_USING_ULOG
void ulog_example(void)
{
  int count = 0;
  while (count++ < 3)
  {
    /* output different level log by LOG_X API */
    LOG_D("LOG_D(%d): ulog example", count);
    LOG_I("LOG_I(%d): ulog example", count);
    LOG_W("LOG_W(%d): ulog example", count);
    LOG_E("LOG_E(%d): ulog example", count);
    ulog_d("test", "ulog_d(%d): ulog example", count);
    ulog_i("test", "ulog_i(%d): ulog example", count);
    ulog_w("test", "ulog_w(%d): ulog example", count);
    ulog_e("test", "ulog_e(%d): ulog example", count);
    rt_thread_delay(rt_tick_from_millisecond(rand() % 1000));
  }
}
MSH_CMD_EXPORT(ulog_example, run ulog example)
#endif
