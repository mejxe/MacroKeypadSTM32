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
#include "tusb.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "encoder.h"
#include "macros.h"
#include "stdbool.h"
#include "utils.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* Defines ------------------------------------------------------------*/

#define MACRO_VECTOR_SECTOR_STARTD_ADDRESS 0x08040000

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
    .Macros_Array = {{0}},
    .Encoder_Macros = {0},
    .macro_profile = 0,
};
Encoder Encoder_Struct = {
    .previous_state = low_low,
    .direction = DIRECTION_NONE,
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
uint8_t GetSector(uint32_t StartSectorAddress)
{
  if (StartSectorAddress < 0x08004000)
    return FLASH_SECTOR_0;
  if (StartSectorAddress < 0x08008000)
    return FLASH_SECTOR_1;
  if (StartSectorAddress < 0x0800C000)
    return FLASH_SECTOR_2;
  if (StartSectorAddress < 0x08010000)
    return FLASH_SECTOR_3;
  if (StartSectorAddress < 0x08020000)
    return FLASH_SECTOR_4;
  if (StartSectorAddress < 0x08040000)
    return FLASH_SECTOR_5;
  if (StartSectorAddress < 0x08060000)
    return FLASH_SECTOR_6;
  return FLASH_SECTOR_7;
}
uint32_t Flash_Write_Data(uint32_t StartSectorAddress)
{
  const static uint8_t NUMBER_OF_WORDS =
      (9 * (MAX_MACRO_LEN + 1)) +
      1; // (1 info byte + 16 macro bytes for each macro) / divided by 4bytes
         // (word len) + 1 for safety
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SECTORError;
  int sofar = 0;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  uint32_t StartSector = GetSector(StartSectorAddress);
  uint32_t EndSectorAddress = StartSectorAddress + NUMBER_OF_WORDS;
  uint32_t EndSector = GetSector(EndSectorAddress);

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = StartSector;
  EraseInitStruct.NbSectors = (EndSector - StartSector) + 1;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    HAL_FLASH_Lock();
    return HAL_FLASH_GetError();
  }

  uint8_t Data[9 * (MAX_MACRO_LEN + 1)] = {0};
  // format_macros_for_writing(MK_Struct.Macros_Array,
  //                          Data);

  while (sofar < NUMBER_OF_WORDS)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, StartSectorAddress,
                          Data[sofar]) == HAL_OK)
    {
      StartSectorAddress += 1;
      sofar++;
    }
    else
    {
      /* Error occurred while writing data in Flash memory*/
      HAL_FLASH_Lock();
      return HAL_FLASH_GetError();
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return 0;
}

void read_macros_from_flash(uint32_t start_addr, MacroKeyboard *MK_Struct)
// first byte = macro_len
// second byte = macro_modifier
// 3rd & 4th = padding
// after that: 4 * 4 bytes of macro
{
  uint32_t curr_addr = start_addr;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      uint32_t curr_word = *(__IO uint32_t *)curr_addr;
      uint8_t macro_len =
          (curr_word >> 24) & 0xFF; // number of bytes needed for macro
      uint8_t macro_modifier =
          (curr_word >> 16) & 0xFF; // 1 byte for macro modifier
      uint8_t macro[MAX_MACRO_LEN] = {0};
      for (int k = 0; k < MAX_MACRO_LEN / 4; k++)
      {
        curr_addr += 4;
        uint32_t curr_word = *(__IO uint32_t *)curr_addr;
        macro[k * 4] = (curr_word >> 0) & 0xFF;
        macro[k * 4 + 1] = (curr_word >> 8) & 0xFF;
        macro[k * 4 + 2] = (curr_word >> 16) & 0xFF;
        macro[k * 4 + 3] = (curr_word >> 24) & 0xFF;
      }
      //      init_macro(i, j, macro, macro_modifier, macro_len, MK_Struct);
      //      TODO: update to profiles
      curr_addr += 4;
    }
  }
}

// this function will in the future load macros from memory and set them up
// now it only sets the default ones
void setup_macros_from_memory()
{
  read_macros_from_flash(MACRO_VECTOR_SECTOR_STARTD_ADDRESS, &MK_Struct);
}
// INTERRUPT HANDLER FROM GPIO PINS
static bool button_press = false;
static bool encoder_button_press = false;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == button_Pin)
  {
    button_press = true;
  }
  else if (GPIO_Pin == encoder_Button_Pin)
  {
    encoder_button_press = true;
  }
}

void buttons_scan(MacroKeyboard *MK)
{
  if (on_board_button_scan(MK))
  {
    Macro *fetched_macro = get_macro_by_row_col(MK, 0, 0);
    MK_Struct.macro_To_Send = fetched_macro;
  }
  else if (encoder_button_scan(MK))
  {
    cycle_profiles(MK);
  }

  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

// TIM2 Interrupt generated by overflowing (every 1ms)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  keyboard_scan(&MK_Struct);
  encoder_scan(&Encoder_Struct, &MK_Struct);
  buttons_scan(&MK_Struct);
}

void print_1s_task(uint32_t *tick)
{
  static uint32_t start_ms = 0;
  const uint32_t interval = 1000;
  if (delay_non_blocking_ms(interval, &start_ms))
  {
    printf("1 sec passed\r\n");
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
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  HAL_Delay(1000);

  setup_macros(&MK_Struct);

  // Flash_Write_Data(MACRO_VECTOR_SECTOR_STARTD_ADDRESS);
  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);

  uint8_t macro4[MAX_MACRO_LEN] = {KEY_D, KEY_U, KEY_P, KEY_A};

  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  HAL_Delay(2000);
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  // init_macro(1, 1, macro4, NONE, 4, &MK_Struct);

  // USB NVIC INTERRUPTS
  HAL_NVIC_SetPriority(OTG_FS_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                 .speed = TUSB_SPEED_AUTO};
  tusb_init(0, &dev_init);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t tick = 0;
  uint32_t button_start_ms = 0;
  uint8_t encoder_presses = 0;
  uint32_t encoder_start_ms = 0;
  while (1)
  {
    tud_task();
    print_1s_task(&tick);
    macro_task(&MK_Struct);
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
  htim2.Init.Prescaler = 59;
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
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
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

  /*Configure GPIO pins : encoder rotation */

  GPIO_InitStruct.Pin = encoder_A_Pin | encoder_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(encoder_GPIO_PORT, &GPIO_InitStruct);

  /*Configure GPIO pins : encoder button */
  GPIO_InitStruct.Pin = encoder_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(encoder_Button_GPIO_PORT, &GPIO_InitStruct);

  // set A10 as perma ground because of pcb design flaw
  GPIO_InitStruct.Pin = encoder_Button_GND;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(encoder_Button_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(encoder_Button_GPIO_PORT, encoder_Button_GND,
                    GPIO_PIN_RESET);
  HAL_GPIO_LockPin(encoder_Button_GPIO_PORT, encoder_Button_GND);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  /* USER CODE END MX_GPIO_Init_2 */
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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
  /* User can add his own implementation to report the HAL error return
   * state
   */
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(1000);
  }
  /* USER CODE END Error_Handler_Debug */
}
int _write(int file, char *ptr, int len)
{
  if (tud_cdc_connected())
  {
    tud_cdc_write(ptr, len);
    tud_cdc_write_flush();
  }
  return len;
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
