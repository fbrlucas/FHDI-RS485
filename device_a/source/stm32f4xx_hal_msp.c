#include <stdint.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{  
    GPIO_InitTypeDef  uart_gpio;

    uart_gpio.Mode      = GPIO_MODE_AF_PP;
    uart_gpio.Pull      = GPIO_NOPULL;
    uart_gpio.Speed     = GPIO_SPEED_FAST;

    if(huart->Instance == USART3)
    {
        // UART3: PD8 (TX) e PD9 (RX)   
        // CTRL PD10
        
        __HAL_RCC_GPIOD_CLK_ENABLE(); 
        __HAL_RCC_USART3_CLK_ENABLE(); 

        uart_gpio.Pin       = GPIO_PIN_8;
        uart_gpio.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &uart_gpio);
          
        uart_gpio.Pin = GPIO_PIN_9;
        HAL_GPIO_Init(GPIOD, &uart_gpio);
        
        uart_gpio.Pin       = GPIO_PIN_10;
        uart_gpio.Mode      = GPIO_MODE_OUTPUT_PP;
        uart_gpio.Pull      = GPIO_PULLDOWN;
        HAL_GPIO_Init(GPIOD, &uart_gpio);            
    }    
}


void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    // reset o dispositivo e remove a inicialização dos pinos
    if(huart->Instance == USART3)
    {
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_9);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_10);

        __HAL_RCC_GPIOD_CLK_DISABLE();
        __HAL_RCC_USART3_CLK_DISABLE();
    }    
}