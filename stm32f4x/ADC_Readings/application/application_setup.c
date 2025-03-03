//===---------------- application_setup.c ------------------------*- C -*-===//
//  Here the application is defined.
//    1. The peridic tasks are defined,
//    2. The components are initialized,
//    3. A list of components is associated to each task,
//    4. The application is run.
//
// PREFIX: none.
// PUBLISHED SIGNALS: None.
//===----------------------------------------------------------------------===//
#include "application_setup.h"
#include "FreeRTOS.h"
#include "adc1_sensors/adc1_sensors.h"
#include "blink/blink.h"
#include "debug/debug.h"
#include "digital_out/digital_out.h"
#include "interrupts_to_tasks.h"
#include "photovoltaic/pv.h"
#include "temperature_sensor/tempsens_LM335.h"
#include "usart2/usart2.h"
#include <task.h>

#define DEBUG 1

// Define parameters to be passed to tasks. See FreeRTOS guideline.
struct TaskParams {
  TickType_t PERIOD;
};

// Task 1000ms
TaskHandle_t xTaskHandle_1000ms;
const char NAME_1000MS[] = "Task_1000ms";
const size_t STACK_SIZE_1000MS = 128;
const uint8_t PRIORITY_1000MS = 2;
const struct TaskParams TASK_PARAMS_1000MS = {.PERIOD = 1000};

// Task 200ms
TaskHandle_t xTaskHandle_200ms;
const char NAME_200MS[] = "Task_200ms";
const size_t STACK_SIZE_200MS = 128;
const uint8_t PRIORITY_200MS = 2;
const struct TaskParams TASK_PARAMS_200MS = {.PERIOD = 200};

// Task function prototypes
static void components_init(void);
static void task_200ms(void * /*pVParameters*/);
static void task_1000ms(void * /*pVParameters*/);

static void components_init() {
  // List all the components used. Initializes queues, mutex, etc.
  adc1_sensors_init();
  digital_out_init();
  usart2_init();

  blink_init();
  debug_init();
  pv_init();
  tempsens_init();
}

void application_setup() {
  // Initialize all the components needed for this app.
  // Disable all interrupts during startup
  taskENTER_CRITICAL();
  components_init();
  interrupts_to_tasks_init();
  taskEXIT_CRITICAL();

  // Create tasks
  xTaskCreate(task_1000ms, NAME_1000MS, STACK_SIZE_1000MS,
              (void *)&TASK_PARAMS_1000MS, PRIORITY_1000MS,
              &xTaskHandle_1000ms);

  xTaskCreate(task_200ms, NAME_200MS, STACK_SIZE_200MS,
              (void *)&TASK_PARAMS_200MS, PRIORITY_200MS, &xTaskHandle_200ms);
}

static void task_1000ms(void *pVParameters) // This is a task.
{
  const struct TaskParams *params = (struct TaskParams *)pVParameters;
  TickType_t xLastWakeTime;
  // Get t0. After that, the task is scheduled at t[k] = t0+k*PERIOD
  xLastWakeTime = xTaskGetTickCount();
  /* volatile BaseType_t xMissedDeadline; */

  while (1) {
    // Run activities
    blink_step(PERIODIC_TASK);
    debug_step(PERIODIC_TASK);
    pv_step(PERIODIC_TASK);
    usart2_step(PERIODIC_TASK);
    tempsens_step(PERIODIC_TASK);

    // Task Schedule
    // TODO: Use xTaskDelayUntil, available from new FreeRTOS.
    /* xMissedDeadline = */
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(params->PERIOD));
  }
}

static void task_200ms(void *pVParameters) // This is a task.
{
  const struct TaskParams *params = (struct TaskParams *)pVParameters;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  /* volatile BaseType_t xMissedDeadline; */

  while (1) {
    adc1_sensors_step(PERIODIC_TASK);
    digital_out_step(PERIODIC_TASK);

    // Task Schedule
    /* xMissedDeadline = */
    /*     xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(params->PERIOD)); */
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(params->PERIOD));
  }
}
