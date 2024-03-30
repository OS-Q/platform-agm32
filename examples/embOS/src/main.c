#include "RTOS.h"
#include "board.h"

static OS_STACKPTR int StackHP[128], StackLP[128]; // Task stacks
static OS_TASK         TCBHP, TCBLP; // Task control blocks

static void HPTask(void) {
  while (1) {
    GPIO_Toggle(LED_GPIO, GPIO_BIT1);
    OS_TASK_Delay(200);
  }
}

static void LPTask(void) {
  while (1) {
    GPIO_Toggle(LED_GPIO, GPIO_BIT2);
    OS_TASK_Delay(500);
  }
}

int main(void) {
  board_init();
  OS_Init();   // Initialize embOS
  OS_InitHW(); // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  printf("Starting embOS\n");
  OS_Start();  // Start embOS
  return 0;
}
