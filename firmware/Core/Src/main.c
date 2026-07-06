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
#define KEY_A 0x04 // Keyboard a and A
#define KEY_B 0x05 // Keyboard b and B
#define KEY_C 0x06 // Keyboard c and C
#define KEY_D 0x07 // Keyboard d and D
#define KEY_E 0x08 // Keyboard e and E
#define KEY_F 0x09 // Keyboard f and F
#define KEY_G 0x0a // Keyboard g and G
#define KEY_H 0x0b // Keyboard h and H
#define KEY_I 0x0c // Keyboard i and I
#define KEY_J 0x0d // Keyboard j and J
#define KEY_K 0x0e // Keyboard k and K
#define KEY_L 0x0f // Keyboard l and L
#define KEY_M 0x10 // Keyboard m and M
#define KEY_N 0x11 // Keyboard n and N
#define KEY_O 0x12 // Keyboard o and O
#define KEY_P 0x13 // Keyboard p and P
#define KEY_Q 0x14 // Keyboard q and Q
#define KEY_R 0x15 // Keyboard r and R
#define KEY_S 0x16 // Keyboard s and S
#define KEY_T 0x17 // Keyboard t and T
#define KEY_U 0x18 // Keyboard u and U
#define KEY_V 0x19 // Keyboard v and V
#define KEY_W 0x1a // Keyboard w and W
#define KEY_X 0x1b // Keyboard x and X
#define KEY_Y 0x1c // Keyboard y and Y
#define KEY_Z 0x1d // Keyboard z and Z

#define KEEB_ROWS 3
#define KEEB_COLS 3
#define MAX_MACRO_LEN 16
#define MACRO_VECTOR_SECTOR_STARTD_ADDRESS 0x08040000
#define USB_SEND_DELAY 15
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
  uint8_t macro[MAX_MACRO_LEN];
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
  Macro *macro_To_Send;

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
void format_macros_for_writing(Macro *macros, uint8_t *buffer)
{
  uint8_t current_byte = 0;
  for (int i = 0; i < 9; i++)
  {
    buffer[current_byte++] = macros[i].macro_len;
    buffer[current_byte++] = macros[i].modifier;
    buffer[current_byte++] = 0;
    buffer[current_byte++] = 0;
    for (int j = 0; j < MAX_MACRO_LEN; j++)
    {
      buffer[current_byte++] = macros[i].macro[j];
    }
  }
}
uint8_t GetSector(uint32_t StartSectorAddress)
{
  if (StartSectorAddress > 0x0803FFFF)
  {
    return 6;
  }
  else if ((StartSectorAddress > 0x0805FFFF))
  {
    return 7;
  }
}
uint32_t Flash_Write_Data(uint32_t StartSectorAddress)
{
  const static uint8_t NUMBER_OF_WORDS =
      (9 * (MAX_MACRO_LEN + 1) / 4) +
      1; // (1 info byte + 16 macro bytes for each macro) / divided by 4bytes
         // (word len) + 1 for safety
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SECTORError;
  int sofar = 0;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Erase the user Flash area */

  /* Get the number of sector to erase from 1st sector */

  uint32_t StartSector = GetSector(StartSectorAddress);
  uint32_t EndSectorAddress = StartSectorAddress + NUMBER_OF_WORDS * 4;
  uint32_t EndSector = GetSector(EndSectorAddress);

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = StartSector;
  EraseInitStruct.NbSectors = (EndSector - StartSector) + 1;

  /* Note: If an erase operation in Flash memory also concerns data in the data
     or instruction cache, you have to make sure that these data are rewritten
     before they are accessed during code execution. If this cannot be done
     safely, it is recommended to flush the caches by setting the DCRST and
     ICRST bits in the FLASH_CR register. */
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    return HAL_FLASH_GetError();
  }

  /* Program the user Flash area word by word
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  uint8_t Data[9 * (MAX_MACRO_LEN + 1)] = {0};
  format_macros_for_writing(*MK_Struct.Macros_Array,
                            Data); // TODO: FIX MACROS ARRAY TO BE FLAT 2D

  while (sofar < NUMBER_OF_WORDS)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartSectorAddress,
                          Data[sofar]) == HAL_OK)
    {
      StartSectorAddress +=
          4; // use StartPageAddress += 2 for half word and 8 for double word
      sofar++;
    }
    else
    {
      /* Error occurred while writing data in Flash memory*/
      return HAL_FLASH_GetError();
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return 0;
}

void read_macros_from_flash(uint32_t start_addr, uint8_t macros_to_read_amnt,
                            MacroKeyboard *MK_Sruct)
// first byte = macro_len
// second byte = macro_modifier
// 3rd & 4th = padding
// then 4 * 4 bytes of macro
{
  Macro macro_array[9] = {0};
  uint32_t curr_addr = start_addr;
  for (int i = 0; i < macros_to_read_amnt; i++)
  {
    uint32_t curr_word = *(__IO uint32_t *)curr_addr;
    uint8_t macro_len =
        (curr_word >> 24) | 0xFF; // number of bytes needed for macro
    uint8_t macro_modifier =
        (curr_word >> 16) | 0xFF; // 1 byte for macro modifier
    uint8_t macro[MAX_MACRO_LEN] = {0};
    for (int j = 0; j < MAX_MACRO_LEN / 4; j++)
    {
      curr_addr += 4;
      uint32_t curr_word = *(__IO uint32_t *)curr_addr;
      macro[j] = (curr_word >> 0) | 0xFF;
      macro[j] = (curr_word >> 8) | 0xFF;
      macro[j] = (curr_word >> 16) | 0xFF;
      macro[j] = (curr_word >> 24) | 0xFF;
    }
    macro_array[i] = (Macro){
        .macro_len = macro_len, .modifier = macro_modifier, .macro = {*macro}};
    curr_addr += 4;
  }
}

void init_macro(uint8_t row, uint8_t col, uint8_t *macro, Modifier modifier)
{
  uint8_t macro_len;
  for (uint8_t i = 0; i < MAX_MACRO_LEN; i++)
  {
    if (macro[i] == 0)
    {
      macro_len = i;
    }
  }
  if (macro_len > MAX_MACRO_LEN)
  {
    return;
  }
  MK_Struct.Macros_Array[row][col] = (Macro){macro_len, modifier};
  memcpy(MK_Struct.Macros_Array[row][col].macro, macro,
         macro_len * sizeof(uint8_t));
};
// this function will in the future load macros from memory and set them up
// now it only sets the default ones
void setup_macros()
{
  uint8_t macro1[MAX_MACRO_LEN] = {KEY_P, KEY_P, KEY_P, KEY_P,
                                   KEY_I, KEY_W, KEY_O};
  uint8_t macro2[MAX_MACRO_LEN] = {KEY_U, KEY_M, KEY_Y, KEY_J};
  uint8_t macro3[MAX_MACRO_LEN] = {KEY_Q};
  uint8_t macro4[MAX_MACRO_LEN] = {KEY_C, KEY_H, KEY_U, KEY_J};
  init_macro(0, 0, macro1, NONE);
  init_macro(0, 1, macro2, NONE);
  init_macro(1, 0, macro3, LSUPER);
  init_macro(1, 1, macro4, NONE);
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
void send_report_with_delay(uint8_t *report)
{
  USBD_HID_SendReport(&hUsbDeviceFS, report, 8);
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // blink the diode while sending
  HAL_Delay(USB_SEND_DELAY);
}
void send_macro_USB(Macro *macro)
{
  uint8_t report[8] = {0};
  for (int i = 0; i < macro->macro_len; i++)
  {
    report[0] = macro->modifier;
    report[3] = macro->macro[i];
    send_report_with_delay(report);

    // flush the usb
    report[0] = 0;
    report[3] = 0;
    send_report_with_delay(report);
  }
  memset(report, 0, sizeof report);
  USBD_HID_SendReport(&hUsbDeviceFS, report, 8);
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
