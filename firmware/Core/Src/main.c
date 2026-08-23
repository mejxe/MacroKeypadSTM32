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
#include "stdbool.h"
#include "usb_descriptors.h"
#include <stdio.h>

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
#define MAX_KEYCODE = 0x28
typedef enum
{
  NONE = 0,
  LCTRL = 0x01,
  LSHIFT = 0x02,
  LALT = 0x04,
  LSUPER = 0x08,
  RCTRL = 0x10,
  RSHIFT = 0x20,
  RALT = 0x40,
  RSUPER = 0x80,
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
    .Macros_Array = {{0}},
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
static const char keycode_names[] = {
    [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd', [KEY_E] = 'e',
    [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h', [KEY_I] = 'i', [KEY_J] = 'j',
    [KEY_K] = 'k', [KEY_L] = 'l', [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o',
    [KEY_P] = 'p', [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x', [KEY_Y] = 'y',
    [KEY_Z] = 'z', [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_0] = '0',
};
static const uint8_t char_to_keycode[128] = {
    ['a'] = KEY_A, ['b'] = KEY_B, ['c'] = KEY_C, ['d'] = KEY_D, ['e'] = KEY_E,
    ['f'] = KEY_F, ['g'] = KEY_G, ['h'] = KEY_H, ['i'] = KEY_I, ['j'] = KEY_J,
    ['k'] = KEY_K, ['l'] = KEY_L, ['m'] = KEY_M, ['n'] = KEY_N, ['o'] = KEY_O,
    ['p'] = KEY_P, ['q'] = KEY_Q, ['r'] = KEY_R, ['s'] = KEY_S, ['t'] = KEY_T,
    ['u'] = KEY_U, ['v'] = KEY_V, ['w'] = KEY_W, ['x'] = KEY_X, ['y'] = KEY_Y,
    ['z'] = KEY_Z, ['0'] = KEY_0, ['1'] = KEY_1, ['2'] = KEY_2, ['3'] = KEY_3,
};

uint8_t char_to_keycode_lookup(char c)
{
  uint8_t idx = (uint8_t)c;
  return (idx < 128) ? char_to_keycode[idx] : 0; // 0 = "no key" sentinel
}
const char keycode_to_char_lookup(uint8_t keycode)
{
  return keycode_names[keycode];
}
/* USER CODE BEGIN 0 */
void format_macros_for_writing(Macro macros[3][3], uint8_t *buffer)
{
  uint8_t current_byte = 0;
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      buffer[current_byte++] = macros[i][j].macro_len;
      buffer[current_byte++] = macros[i][j].modifier;
      buffer[current_byte++] = 0;
      buffer[current_byte++] = 0;
      for (int k = 0; k < MAX_MACRO_LEN; k++)
      {
        buffer[current_byte++] = macros[i][j].macro[k];
      }
    }
  }
}
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
  format_macros_for_writing(MK_Struct.Macros_Array,
                            Data); //

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

void init_macro(uint8_t row, uint8_t col, uint8_t *macro, Modifier modifier,
                uint8_t macro_len, MacroKeyboard *MK_Struct)
{
  if (macro_len > MAX_MACRO_LEN)
  {
    return;
  }
  MK_Struct->Macros_Array[row][col] = (Macro){macro_len, modifier};
  memcpy(MK_Struct->Macros_Array[row][col].macro, macro,
         macro_len * sizeof(uint8_t));
};

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
      init_macro(i, j, macro, macro_modifier, macro_len, MK_Struct);
      curr_addr += 4;
    }
  }
}

// this function will in the future load macros from memory and set them up
// now it only sets the default ones
void setup_macros()
{
  uint8_t macro1[MAX_MACRO_LEN] = {KEY_P, KEY_P, KEY_P, KEY_P,
                                   KEY_I, KEY_W, KEY_O};
  uint8_t macro2[MAX_MACRO_LEN] = {KEY_U, KEY_M, KEY_Y, KEY_J};
  uint8_t macro3[MAX_MACRO_LEN] = {KEY_Q};
  uint8_t macro4[MAX_MACRO_LEN] = {KEY_C, KEY_H, KEY_U, KEY_J};

  init_macro(0, 0, macro1, LSHIFT, 7, &MK_Struct);
  init_macro(0, 1, macro2, NONE, 4, &MK_Struct);
  init_macro(1, 0, macro3, LSUPER, 1, &MK_Struct);
  init_macro(1, 1, macro4, NONE, 4, &MK_Struct);
}
void setup_macros_from_memory()
{
  read_macros_from_flash(MACRO_VECTOR_SECTOR_STARTD_ADDRESS, &MK_Struct);
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
void usb_keyboard_send_report(uint8_t *keycode, uint8_t modifier)
{
  uint8_t const report_id = 0;
  tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, keycode);
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // blink the diode while sending
  printf("sending key: %c and modifier: %x!\r\n",
         keycode_to_char_lookup(keycode[0]), modifier);
}
// INTERRUPT HANDLER FROM GPIO PINS
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == button_Pin)
  {
    for (volatile int i = 0; i < 500000; i++)
      ;

    if (HAL_GPIO_ReadPin(button_GPIO_Port, button_Pin) == GPIO_PIN_RESET)
    {

      Macro *fetched_macro = get_macro_by_row_col(0, 0);
      MK_Struct.macro_To_Send = fetched_macro;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }

    __HAL_GPIO_EXTI_CLEAR_IT(button_Pin);
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

bool delay_non_blocking_ms(uint32_t interval)
{
  static uint32_t start_ms = 0;
  if (HAL_GetTick() - start_ms < interval)
  {
    return false;
  }
  start_ms += interval;
  return true;
}
void print_1s_task(uint32_t *tick)
{
  if (delay_non_blocking_ms(1000))
  {
    printf("1 sec passed\r\n");
  }
}

void macro_task(MacroKeyboard *MK)
{
  if (!tud_hid_n_ready(ITF_NUM_KEYBOARD) || MK->macro_To_Send == NULL)
  {
    // printf("returning because of: %d %d\r\n", tud_hid_n_ready(ITF_NUM_HID),
    //           MK->macro_To_Send);
    return;
  }
  uint8_t static macro_curr_index;
  bool static need_flush;

  uint8_t keycode[6] = {0};
  uint8_t modifier;

  if (need_flush) // flush with zeroes after every raport
  {
    keycode[0] = 0;
    modifier = 0;
    usb_keyboard_send_report(keycode, modifier);
    need_flush = false;
    macro_curr_index++;
    if (macro_curr_index >= MK->macro_To_Send->macro_len)
    {
      MK->macro_To_Send = NULL;
      macro_curr_index = 0;
    }
  }
  else
  {
    keycode[0] = MK->macro_To_Send->macro[macro_curr_index];
    modifier = MK->macro_To_Send->modifier;
    usb_keyboard_send_report(keycode, modifier);
    need_flush = true;
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

  setup_macros();

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
  init_macro(1, 1, macro4, NONE, 4, &MK_Struct);

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

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
  // TODO not Implemented
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
  // This example doesn't use multiple report and report ID
  (void)itf;
  (void)report_id;
  (void)report_type;

  // echo back anything we received from host
  tud_hid_report(0, buffer, bufsize);
}

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
