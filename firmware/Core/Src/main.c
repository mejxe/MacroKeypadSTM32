/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
extern USBD_HandleTypeDef hUsbDeviceFS;
#include "stdbool.h"
#include "usbd_hid.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* Defines ------------------------------------------------------------*/
#define KEY_1 0x1e // Keyboard 1 and !
#define KEY_2 0x1f // Keyboard 2 and @
#define KEY_3 0x20 // Keyboard 3 and #
#define KEY_0 0x27 // Keyboard 0 and )

#define KEEB_ROWS 3
#define KEEB_COLS 3
#define MAXIMUM_MACRO_LEN 16
typedef enum
{
  NONE = 0,
  LCTRL = 0x01,
  LSHIFT = 0x02,
  LALT = 0x04,
  LSUPER = 0x08,
  RCTRL = 0x1,
  RSHIFT = 0x2,
  RALT = 0x4,
  RSUPER = 0x8,
} ModifierEnum;
typedef uint8_t Modifier;

typedef struct
{
  uint8_t macro_len;
  Modifier modifier;
  uint8_t macro[MAXIMUM_MACRO_LEN];
} Macro;

typedef struct
{
  uint8_t down_counter;
  bool logical_state;
} KeySwitch;

// God struct
typedef struct
{
  KeySwitch Keyboard_State_Array[KEEB_ROWS][KEEB_COLS];
  Macro Macros_Array[KEEB_ROWS][KEEB_COLS];
  const uint16_t columns[KEEB_COLS];
  const uint16_t rows[KEEB_ROWS];
  volatile Macro *macro_To_Send;

} MacroKeyboard;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
MacroKeyboard MK_Struct = {
    .columns = {col0_Pin, col1_Pin, col2_Pin},
    .rows = {row0_Pin, row1_Pin, row2_Pin},
    .macro_To_Send = NULL,
    .Macros_Array = {0},
};

/* USER CODE END PV */

/* Private function prototypes
   -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// this function will in the future load macros from memory and set them up
// now it only sets the default ones
void setup_macros()
{
  MK_Struct.Macros_Array[0][0] = (Macro){
      .macro_len = 1,
      .macro = {KEY_0},
  };
  MK_Struct.Macros_Array[0][1] = (Macro){
      .macro_len = 1,
      .macro = {KEY_1},
  };
  MK_Struct.Macros_Array[1][0] = (Macro){
      .macro_len = 1,
      .macro = {KEY_2},
  };
  MK_Struct.Macros_Array[1][1] = (Macro){
      .macro_len = 1,
      .macro = {KEY_3},
  };
}
void increment_switch_counter(KeySwitch *key) { key->down_counter += 1; }
void reset_switch_counter(KeySwitch *key) { key->down_counter = 0; }
bool should_register_keypress(KeySwitch *key)
{
  return (key->down_counter >= 8);
}
Macro *get_macro_by_row_col(uint8_t row_index, uint8_t col_index)
{
  return &MK_Struct.Macros_Array[row_index][col_index];
}

void delegate_macro(uint8_t row, uint8_t column)
{
  Macro *fetched_macro = get_macro_by_row_col(row, column);
  if (fetched_macro == NULL)
  {
    Error_Handler();
  }
  MK_Struct.macro_To_Send = fetched_macro;
}
void send_macro_USB(Macro *macro)
{
  uint8_t report[8] = {0};
  report[0] = macro->modifier;
  for (int i = 0; i < macro->macro_len; i++)
  {
    report[i + 2] = macro->macro[i];
    if (i == 5 || i + 1 == macro->macro_len)
    {
      USBD_HID_SendReport(&hUsbDeviceFS, report, 8);

      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // blink the diode while sending

      HAL_Delay(50);

      // flush the usb
      memset(report, 0, sizeof report);
      USBD_HID_SendReport(&hUsbDeviceFS, report, 8);

      report[0] = macro->modifier;

      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
  }
}
// INTERRUPT HANDLER FROM GPIO PINS
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
  case GPIO_PIN_0:
    for (volatile int i = 0; i < 10000; i++)
      ;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      //    button_pressed = 1;
      for (volatile int i = 0; i < 10000; i++)
        ;
    }
    break;
  }
}
// TIM2 Interrupt generated by overflowing (every 1ms)
// Scan the keyboard matrix and report presses
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  for (int i = 0; i < 3; i++)
  {
    HAL_GPIO_WritePin(GPIOB, MK_Struct.columns[i],
                      GPIO_PIN_RESET); // set to logical high
    for (int j = 0; j < 3; j++)
    {
      GPIO_PinState row_in = HAL_GPIO_ReadPin(GPIOB, MK_Struct.rows[j]);
      if (row_in == GPIO_PIN_RESET)
      {
        increment_switch_counter(&MK_Struct.Keyboard_State_Array[j][i]);
      }
      else if (row_in == GPIO_PIN_SET &&
               should_register_keypress(&MK_Struct.Keyboard_State_Array[j][i]))
      {
        delegate_macro(j, i);
        reset_switch_counter(&MK_Struct.Keyboard_State_Array[j][i]);
      }
      else
      {
        reset_switch_counter(&MK_Struct.Keyboard_State_Array[j][i]);
      }
    }
    HAL_GPIO_WritePin(GPIOB, MK_Struct.columns[i],
                      GPIO_PIN_SET); // set to logical low
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU
   * Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the
   * Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */
  setup_macros();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (MK_Struct.macro_To_Send != NULL)
    {
      send_macro_USB(MK_Struct.macro_To_Send);
      MK_Struct.macro_To_Send = NULL;
    }
    HAL_Delay(10);
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */
  htim2.Init.Prescaler = 479;
  htim2.Init.Period = 999;

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, col0_Pin | col1_Pin | col2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : button_Pin */
  GPIO_InitStruct.Pin = button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : col0_Pin col1_Pin col2_Pin */
  GPIO_InitStruct.Pin = col0_Pin | col1_Pin | col2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : row0_Pin row1_Pin row2_Pin */
  GPIO_InitStruct.Pin = row0_Pin | row1_Pin | row2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(1000);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
