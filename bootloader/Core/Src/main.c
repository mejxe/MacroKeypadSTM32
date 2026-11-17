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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "flash_layout.h"
#include "tusb.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
  Idle,
  FlashToUpdateBuffer,
  FlashToAppBuffer,
} FlasherMode;

typedef struct
{
  uint8_t *message_buf;
  uint32_t in_buffer;
  uint32_t flash_offset;

  bool flash_ready;
  bool should_flash;
  bool should_finish;

  FlasherMode mode;

} Flasher_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MESSAGE_BUFSIZE 4096
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;
PCD_HandleTypeDef hpcd_USB_OTG_FS;
CRC_HandleTypeDef hcrc;

/* USER CODE BEGIN PV */

bool should_copy_to_main_app;
bool should_restart_after_flash;

uint32_t app_size_in_update_buf_B;

uint32_t expected_crc_val;

static uint8_t message_buf[MESSAGE_BUFSIZE];
Flasher_t Flasher = {.message_buf = message_buf};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_RTC_Init(void);
static void CRC_Init(void);
/* USER CODE BEGIN PFP */
bool should_flash_firmware(void);
void set_should_not_flash_firmware(void);
uint32_t write_buf_to_flash(uint32_t flash_address, uint32_t *buf,
                            uint32_t bufsize);
void write_flash(Flasher_t *flasher);
void finish_flash(Flasher_t *flasher);
uint32_t flasher_get_sector(Flasher_t *f);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

bool compare_crc_val() { return hcrc.Instance->DR == expected_crc_val; }

void flash_task()
{
  if (!Flasher.flash_ready)
    return;

  if (Flasher.should_flash)
  {
    printf("Writing to flash buffer\r\n");
    write_flash(&Flasher);
    printf("Written to flash buffer\r\n");
    Flasher.should_flash = false;
    Flasher.in_buffer = 0;
  }
  else if (Flasher.should_finish)
  {
    finish_flash(&Flasher);
  }
  else if (Flasher.in_buffer >= MESSAGE_BUFSIZE)
  {
    Flasher.should_flash = true;
  }
}
void finish_flash(Flasher_t *flasher)
{
  if (Flasher.in_buffer != 0)
  {
    Flasher.should_flash = true;
    printf("Finishing flash\r\n");
    return;
  }
  Flasher.in_buffer = 0;
  Flasher.should_flash = false;
  Flasher.should_finish = false;
  Flasher.mode = Idle;
  Flasher.flash_offset = 0;
  HAL_FLASH_Lock();

  if (!compare_crc_val())
  {
    printf("CRC Value mismatch, flash aborted, try again.\n\r");
    should_copy_to_main_app = false;
    should_restart_after_flash = true;
  }

  printf("Finished flash\r\n");
}

void write_flash(Flasher_t *flasher)
{
  uint32_t cast_buffer[MESSAGE_BUFSIZE / 4 + 1];
  uint32_t words_to_write = (flasher->in_buffer + 3) / 4;
  uint32_t TARGET_FLASH_BUFFER = flasher_get_sector(flasher);

  memcpy(cast_buffer, flasher->message_buf, flasher->in_buffer);

  HAL_CRC_Accumulate(&hcrc, cast_buffer, words_to_write);

  uint32_t words_written = write_buf_to_flash(
      TARGET_FLASH_BUFFER + flasher->flash_offset, cast_buffer, words_to_write);

  if (words_written != words_to_write)
  {
    Error_Handler();
    return;
  }
  flasher->flash_offset += words_written * 4;
}

void *flasher_get_message_buf_pos(Flasher_t *f)
{
  return (f->message_buf + f->in_buffer);
}

typedef void (*func_pointer)(void);
void jump_to_app(void)
{
  uint32_t app_stack;
  uint32_t app_reset_vec;
  func_pointer app_entry;

  app_stack = *(volatile uint32_t *)APP_SECTOR_START;
  app_reset_vec = *(volatile uint32_t *)(APP_SECTOR_START + 4);
  if (app_stack < 0x20000000 || app_stack > 0x20020000)
    return;
  app_entry = (func_pointer)app_reset_vec;

  __disable_irq();

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  HAL_RCC_DeInit();
  __set_MSP(app_stack);

  app_entry();
}

// returns how much written
uint32_t write_buf_to_flash(uint32_t flash_address, uint32_t *buf,
                            uint32_t bufsize)
{
  uint32_t number_of_words = bufsize;
  uint32_t written_words = 0;

  while (written_words < number_of_words)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address,
                          buf[written_words]) == HAL_OK)
    {
      written_words++;
      flash_address += sizeof(*buf);
    }
    else
    {
      /* Error occurred while writing data in Flash memory*/
      HAL_FLASH_Lock();
      return HAL_FLASH_GetError();
    }
  }
  return written_words;
}

uint32_t setup_flasher(Flasher_t *f, FlasherMode mode)
{
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SECTORError;
  uint8_t sector;
  uint8_t nb_sectors;

  switch (mode)
  {
  case Idle:
    return 101;
    break;
  case FlashToAppBuffer:
    sector = FLASH_SECTOR_4;
    nb_sectors = 3;
    break;
  case FlashToUpdateBuffer:
    sector = FLASH_SECTOR_7;
    nb_sectors = 1;
    break;
  }

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = sector;
  EraseInitStruct.NbSectors = nb_sectors;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
  {
    HAL_FLASH_Lock();
    return HAL_FLASH_GetError();
  }
  f->flash_ready = true;
  f->mode = mode;

  __HAL_CRC_DR_RESET(&hcrc);
  return 0;
}

void cdc_task()
{
  static bool magic_number_read = false;
  static uint32_t all_read = 0;
  static uint32_t binary_size = 0;

  if (!tud_cdc_available() || Flasher.should_flash ||
      Flasher.mode == FlashToAppBuffer)
    return;

  uint8_t temp_buf[CFG_TUD_CDC_RX_BUFSIZE];
  uint32_t read = tud_cdc_read(temp_buf, 64);
  if (!magic_number_read)
  {
    if (temp_buf[0] == 0x25 && temp_buf[1] == 0x07 && temp_buf[2] == 0xa0 &&
        temp_buf[3] == 0x7a)
    {
      magic_number_read = true;
      for (int i = 0; i < 8; i++)
      {
        binary_size |= (temp_buf[4 + i] << i * 8);
      }
      for (int i = 0; i < 4; i++)
      {
        expected_crc_val |= (temp_buf[12 + i] << i * 8);
      }

      setup_flasher(&Flasher, FlashToUpdateBuffer);
      printf("Read magic number, binary size: %d\r\n", binary_size);
      return;
    }
    else
      return;
  }

  memcpy(flasher_get_message_buf_pos(&Flasher), temp_buf, read);
  Flasher.in_buffer += read;

  printf("in flash: %d\r\n", Flasher.in_buffer);
  all_read += read;

  if (all_read >= binary_size)
  {
    printf("Read %d bytes, finishing\r\n",
           all_read +
               64); // skipped one single window of headers hence the addition
    Flasher.should_finish = true;
    app_size_in_update_buf_B = all_read;
    magic_number_read = false;
    should_copy_to_main_app = true;
  }
}

bool delay_non_blocking_ms(uint32_t interval, uint32_t *start_ms)
{
  if (HAL_GetTick() - *start_ms < interval)
  {
    return false;
  }
  *start_ms += interval;
  return true;
}

void blink_task()
{
  static uint32_t last_ms = 0;
  if (delay_non_blocking_ms(300, &last_ms))
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  }
}

void deploy_app_task()
{
  if (!should_copy_to_main_app || Flasher.mode == FlashToUpdateBuffer ||
      Flasher.should_flash)
    return;

  if (Flasher.mode == Idle)
  {
    setup_flasher(&Flasher, FlashToAppBuffer);
  }
  static uint32_t all_read = 0;
  uint32_t to_read = (app_size_in_update_buf_B - all_read) > MESSAGE_BUFSIZE
                         ? MESSAGE_BUFSIZE
                         : app_size_in_update_buf_B - all_read;
  memcpy(Flasher.message_buf, (void *)FLASH_UPDATE_BUF + Flasher.flash_offset,
         to_read);

  Flasher.in_buffer = to_read;
  all_read += to_read;
  printf("read from updatebuf: %d\r\n", all_read);

  if (all_read >= app_size_in_update_buf_B)
  {
    Flasher.should_finish = true;
    should_copy_to_main_app = false;
    should_restart_after_flash = true;
  }
}
void restart_after_flash_task()
{
  if (should_restart_after_flash && Flasher.mode == Idle)
  {
    set_should_not_flash_firmware();
    HAL_NVIC_SystemReset();
  }
}
uint32_t flasher_get_sector(Flasher_t *f)
{
  switch (f->mode)
  {
  case FlashToAppBuffer:
    return APP_SECTOR_START;
  case FlashToUpdateBuffer:
    return FLASH_UPDATE_BUF;
  case Idle:
    return 0;
  }
}
void set_should_not_flash_firmware(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_RTC_Init();
  CRC_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  if (!should_flash_firmware())
  {
    jump_to_app();
  }
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);

  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                 .speed = TUSB_SPEED_AUTO};
  tusb_init(0, &dev_init);
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    tud_task();
    cdc_task();
    flash_task();
    blink_task();
    deploy_app_task();

    restart_after_flash_task();
  }
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
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only **/
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}

static void CRC_Init(void)
{
  hcrc.Instance = CRC;
  HAL_CRC_Init(&hcrc);
}

/**
 * @brief USB_OTG_FS Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */
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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

int _write(int file, char *ptr, int len)
{
  if (tud_cdc_connected())
  {
    tud_cdc_write(ptr, len);
    tud_cdc_write_flush();
  }
  return len;
}

bool should_flash_firmware(void)
{
  return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 1;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(100);
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
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
