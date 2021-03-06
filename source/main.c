//ZX Spectrum video signal to VGA converter
//by ILIASAM

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "32f429_sdram.h"
#include "32f429_lcd.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "rgb_capture.h"
#include "stm32f4xx_dac.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t uwTimingDelay;
RCC_ClocksTypeDef RCC_Clocks;
char test_string[64];

extern volatile uint8_t capture_buf0[RGB_CAPTURE_MAX_SAMPLES];
extern volatile uint8_t capture_buf1[RGB_CAPTURE_MAX_SAMPLES];
extern volatile uint8_t new_line_is_captured;
extern volatile uint16_t captured_line_number;

/* Private function prototypes -----------------------------------------------*/
static void Delay(__IO uint32_t nTime);
void init_gpio(void);
uint16_t grayscale_to_rgb565(uint8_t value);

/* Private functions ---------------------------------------------------------*/

int main(void)
{
  uint16_t i;
  //Clock is configured at SetSysClock()
  //SYSCLK is 168MHz

  /* SysTick end of count event each 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
  SystemCoreClockUpdate();
  
  init_gpio();
  SDRAM_Init();
  Delay(5);
  
  LCD_Init();
  LCD_LayerInit();
  LTDC_Cmd(ENABLE);
  LCD_SetLayer(LCD_BACKGROUND_LAYER);
  LCD_Clear(LCD_COLOR_BLACK);
  LCD_SetFont(&Font16x24);
  
  //red bars - test
  LCD_SetTextColor(LCD_COLOR_RED);
  for (i=0; i< 640; i+= 30)
  {
    LCD_DrawFullRect(i, 0, 15, 480);
  }
  
  LCD_SetTextColor(LCD_COLOR_BLACK);
  LCD_DisplayStringLine(LINE(2), (uint8_t*)"ILIASAM");
  LCD_DisplayStringLine(LINE(3), (uint8_t*)"ZX SPECTRUIM -> VGA");
  
  Delay(1000);
 
  init_capture_timer();
  init_time_measurement_timer();
  init_capture_dma();
  init_video_data_pins();
  
  GPIO_SetBits(GPIOI, GPIO_Pin_7);//LED2
  
  LCD_Clear(LCD_COLOR_WHITE);
  LCD_DrawRect(0, 0, RGB_CAPTURE_MAX_LINES+1, RGB_CAPTURE_IMAGE_WIDTH+1);//video zone
  
  uint16_t* display_buf = (uint16_t *)LCD_FRAME_BUFFER;
  int16_t local_line_number = 0;//used to protect from changing "captured_line_number" in interrupt
  uint8_t* buf_ptr;//pointer to the buffer with video data 
  
  while (1) 
  {
    if (new_line_is_captured == 1)
    {
      new_line_is_captured = 0;
      local_line_number = captured_line_number;
      
      if (local_line_number % 1)
        buf_ptr = (uint8_t*)&capture_buf1[0];
      else
        buf_ptr = (uint8_t*)&capture_buf0[0];
      
      uint16_t pixel_color = 0;
      uint16_t local_pix_number = 0;
      
      uint32_t buffer_offset = local_line_number * LCD_PIXEL_WIDTH;
      
      for (i=0; i<RGB_CAPTURE_IMAGE_WIDTH; i++)//i - y
      {
        //Convert captured signal to a pixel color
        pixel_color = 0;
        local_pix_number = i + RGB_CAPTURE_LEFT_OFFSET;
        
        if (buf_ptr[local_pix_number] & RGB_CAPTURE_DATA_B_PIN)
          pixel_color|= LCD_COLOR_BLUE;
        
        if (buf_ptr[local_pix_number] & RGB_CAPTURE_DATA_G_PIN)
          pixel_color|= LCD_COLOR_GREEN;
        
        if (buf_ptr[local_pix_number] & RGB_CAPTURE_DATA_R_PIN)
          pixel_color|= LCD_COLOR_RED;
          
        display_buf[buffer_offset + i] = pixel_color;//Change framebuffer
      }
    }
  }
}



uint16_t grayscale_to_rgb565(uint8_t value)
{
  uint16_t red = (uint8_t)(value >> 3);
  uint16_t green = (uint8_t)(value >> 2);
  uint16_t blue = (uint8_t)(value >> 3);
  
  uint16_t result = (red << 11) | (green << 5) | (blue << 0);
  return result;
}

void init_gpio(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
  
  GPIO_StructInit(&GPIO_InitStructure);
  //LEDS
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOI, &GPIO_InitStructure);
}

void Delay(__IO uint32_t nTime)
{ 
  uwTimingDelay = nTime;

  while(uwTimingDelay != 0);
}

void delay_ms(__IO uint32_t nTime)
{
  Delay(nTime);
}


void TimingDelay_Decrement(void)
{
  if (uwTimingDelay != 0x00)
  { 
    uwTimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
