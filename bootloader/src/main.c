#include "main.h"

#include "flash_layout.h"
#include "flasher.h"
#include "tusb.h"
#include <stdio.h>

#define HEADER_LEN 12
typedef enum
{
  Idle,
  Receive,
  Copy,
  Restart,
} BootloaderState;

typedef enum
{
  E_NothingHappened,
  E_Failure,
  E_ReceivingStarted,
  E_ReceivingFinished,
  E_CopyingFinished,
} Event;

typedef struct
{
  bool read_header;
  uint32_t expected_crc_val;
  uint8_t header[HEADER_LEN];
  uint32_t all_read;
  uint32_t binary_size;
} CDC_State_T;

typedef struct
{
  uint32_t app_size_in_update_buf_B;
  uint32_t all_read;
} Copy_State_T;

RTC_HandleTypeDef hrtc;
PCD_HandleTypeDef hpcd_USB_OTG_FS;
CRC_HandleTypeDef hcrc;

const uint8_t MAGIC_NUM[4] = {0x25, 0x07, 0xa0, 0x7a};
uint32_t restart_dispatched_ts;

BootloaderState State = Idle;

static const CDC_State_T CDC_STATE_INIT = {0};
static const Copy_State_T COPY_STATE_INIT = {0};

static CDC_State_T CdcState = CDC_STATE_INIT;
static Copy_State_T CopyState = COPY_STATE_INIT;

static uint8_t message_buf[MESSAGE_BUFSIZE];
Flasher_t Flasher = {.message_buf = message_buf, .hcrc = &hcrc};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_RTC_Init(void);
static void CRC_Init(void);
bool should_flash_firmware(void);
void set_should_not_flash_firmware(void);

void reset_program_state()
{
  CdcState = CDC_STATE_INIT;
  CopyState = COPY_STATE_INIT;
}
bool transition_state(BootloaderState *state, BootloaderState next_state)
{
  switch (next_state)
  {
  case Idle:
    if (*state == Restart)
      return false;
    if (*state == Receive || *state == Copy)
    {
      printf("CRC Value mismatch, flash aborted, try again.\r\n");
    }
    reset_program_state();
    break;

  case Receive:
    if (*state == Idle)
    {
      if (!setup_flasher(&Flasher, UpdateBufSector))
      {
        Error_Handler();
      }
      CdcState.read_header = true;
      printf("Starting to receive the firmware\r\n");
    }
    else
      return false;
    break;

  case Copy:
    if (*state == Receive)
    {
      if (!setup_flasher(&Flasher, AppSector))
      {
        Error_Handler();
      }
      CopyState.app_size_in_update_buf_B = CdcState.binary_size;
      printf("Received correct firmware, copying to main memory\r\n");
    }
    else
      return false;
    break;

  case Restart:
    if (*state != Copy)
      return false;
    printf("Copying finished, ready to restart\r\n");
    restart_dispatched_ts = HAL_GetTick();
    break;
  }
  *state = next_state;
  return true;
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

Event cdc_task(CDC_State_T *cdc_state)
{
  const uint8_t PACKET_SIZE = 64;
  // HEADER
  const uint8_t MAGIC_LEN = 4;
  const uint8_t SIZE_OFFSET = 4;
  const uint8_t CRC_OFFSET = 8;

  if (!tud_cdc_available())
    return E_NothingHappened;

  if (!cdc_state->read_header)
  {
    uint32_t read = tud_cdc_read(cdc_state->header, HEADER_LEN);

    if (read != HEADER_LEN)
      return E_NothingHappened;

    if (memcmp(MAGIC_NUM, cdc_state->header, MAGIC_LEN))
      return E_NothingHappened;

    for (int i = 0; i < 4; i++)
    {
      cdc_state->binary_size |= (cdc_state->header[SIZE_OFFSET + i] << i * 8);
    }
    for (int i = 0; i < 4; i++)
    {
      cdc_state->expected_crc_val |=
          (cdc_state->header[CRC_OFFSET + i] << i * 8);
    }
    printf("Read magic number, binary size: %d\r\n", cdc_state->binary_size);
    return E_ReceivingStarted;
  }
  else
  {
    uint8_t temp_buf[CFG_TUD_CDC_RX_BUFSIZE];
    uint32_t left_to_read = cdc_state->binary_size - cdc_state->all_read;

    uint8_t to_read = (left_to_read < PACKET_SIZE) && left_to_read != 0
                          ? left_to_read
                          : PACKET_SIZE;

    uint32_t read = tud_cdc_read(temp_buf, to_read);
    if (!write_flash(&Flasher, temp_buf, read))
      return E_Failure;

    printf("in flash: %d\r\n", Flasher.in_buffer);
    cdc_state->all_read += read;

    if (cdc_state->all_read >= cdc_state->binary_size)
    {
      printf("Read %lld bytes, finishing\r\n",
             cdc_state->all_read + HEADER_LEN); // skipped one single window of
                                                // headers hence the addition
      return E_ReceivingFinished;
    }
  }
  return E_NothingHappened;
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

Event copy_app_task(Copy_State_T *copy_state)
{
  uint32_t left_to_read =
      copy_state->app_size_in_update_buf_B - copy_state->all_read;
  uint32_t to_read =
      left_to_read > MESSAGE_BUFSIZE ? MESSAGE_BUFSIZE : left_to_read;

  if (!write_flash(&Flasher,
                   (uint8_t *)(FLASH_UPDATE_BUF + copy_state->all_read),
                   to_read))
    return E_Failure;

  copy_state->all_read += to_read;

  if (copy_state->all_read == copy_state->app_size_in_update_buf_B)
  {
    copy_state->all_read = 0;
    return E_CopyingFinished;
  }
  return E_NothingHappened;
}
void restart()
{
  if (delay_non_blocking_ms(2000, &restart_dispatched_ts))
  {
    set_should_not_flash_firmware();
    HAL_NVIC_SystemReset();
  }
}
void set_should_not_flash_firmware(void)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0);
}

void state_step(Event e)
{
  switch (e)
  {
  case E_ReceivingStarted:
    transition_state(&State, Receive);
    break;
  case E_ReceivingFinished:
    finish_flash(&Flasher, CdcState.expected_crc_val)
        ? transition_state(&State, Copy)
        : transition_state(&State, Idle);
    break;
  case E_CopyingFinished:
    finish_flash(&Flasher, CdcState.expected_crc_val)
        ? transition_state(&State, Restart)
        : transition_state(&State, Idle);
    break;
  case E_Failure:
    Error_Handler();
    break;
  default:
    return;
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_RTC_Init();
  CRC_Init();
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
    Event task_event = E_NothingHappened;
    tud_task();
    blink_task();

    switch (State)
    {
    case Idle:
      task_event = cdc_task(&CdcState);
      break;

    case Receive:
      task_event = cdc_task(&CdcState);
      break;

    case Copy:
      task_event = copy_app_task(&CopyState);
      break;

    case Restart:
      restart();
      break;
    }

    state_step(task_event);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

static void MX_RTC_Init(void)
{
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
}

static void CRC_Init(void)
{
  hcrc.Instance = CRC;
  HAL_CRC_Init(&hcrc);
}

static void MX_USB_OTG_FS_PCD_Init(void)
{
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
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

bool should_flash_firmware(void)
{
  return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 1;
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(100);
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
