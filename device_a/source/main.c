#include <stdint.h>
#include <string.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"
#include "circ_buffer.h"
#include "crc16.h"

// Debug tip:
// Connect to target, reset (black button), then load and press c

uint8_t cb_area[CMD_MAX_SIZE];
volatile uint8_t new_frame = 0;
circ_buffer_t cb;
frame_t frame;

uint32_t id = ID;
uint8_t crc1[2];
//char model[8] = {'F','H','D','I',' ',' ',' ',' '};

static void SystemClock_Config(void);
static void Error_Handler(void);

UART_HandleTypeDef uart_dev3;

static void setup_uart(void)
{

    uart_dev3.Instance          = USART3;
    // taxa desejada
    uart_dev3.Init.BaudRate     = 115200;
    // palavra de 8 bits
    uart_dev3.Init.WordLength   = UART_WORDLENGTH_8B;
    // 1 stop bit
    uart_dev3.Init.StopBits     = UART_STOPBITS_1;
    // sem paridade
    uart_dev3.Init.Parity       = UART_PARITY_NONE;
    // sem controle de fluxo
    uart_dev3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    // habilita TX e RX
    uart_dev3.Init.Mode         = UART_MODE_TX_RX;
    // 16 amostras por bit
    uart_dev3.Init.OverSampling = UART_OVERSAMPLING_16;

    if(HAL_UART_Init(&uart_dev3) != HAL_OK)
        Error_Handler();        

}

static void uart3_enable_int(void)
{
    // habilita interrupções para outros erros 
    //   (Frame error, noise error, overrun error)
    USART3->CR3 |= USART_CR3_EIE; 
    // habilita interrupções para erros de paridade
    // habilita interrupção para chegada de byte
    USART3->CR1 |= USART_CR1_PEIE | USART_CR1_RXNEIE;
    // habilitação global da linha de interrupção e prioridade
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

static uint8_t add_byte(uint8_t b)
{
	static uint8_t buffer_size = 0;
	static uint8_t buffer_esc = 0;

    if ((buffer_size == 0) && (b == FRAME_FLAG))
        return 0;

    if ((!buffer_esc) && (b == FRAME_ESC))
	{
            buffer_esc = 1;
            return 0;
	}

    if ((!buffer_esc) && (b == FRAME_FLAG))
        return 1;

    if (buffer_size >= CMD_MAX_SIZE)
	{
            buffer_size = 0;
            buffer_esc = 0;
            return 0;
	}

    if(circ_buffer_put(&cb,b) == CIRC_BUFFER_OK)
        buffer_size += 1;

    if (buffer_esc == 1)
        buffer_esc = 0;

    return 0;
}


void USART3_IRQHandler(void)
{
    uint8_t c;
    uint8_t v = 0;
    uint32_t sr;

    // lê o status register e trata erros até que desapareçam
    // (o manual manda ler o SR e em seguida DR)
    sr = USART3->SR;
    while(sr & (USART_FLAG_ORE | USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE))
    {
        sr = USART3->SR;
        c = USART3->DR;
    }

    if(sr & USART_FLAG_RXNE)
    {
        // finalmente, dado válido sem erro
        c = USART3->DR;
        v = 1;
    }

    if(v)
    {
        if(new_frame == 0)
        {
            if(add_byte(c))
                new_frame = 1;
        }
    }   
}

static void uart3_send_byte(uint8_t c, uint8_t with_esc)
{
    // esperar estar livre para transmitir
    while( !(USART3->SR & USART_FLAG_TXE)) {}
    
    // verifica o stuffing antes
    if(with_esc)
    {
        // envia o ESC, caso seja um caso de escape
        if(c == FRAME_FLAG || c == FRAME_ESC)
        {
            USART3->DR = FRAME_ESC;
            while( !(USART3->SR & USART_FLAG_TXE)) {}
        }
    }
    
    // transmite
    USART3->DR = c;
}

static void send_frame(frame_t *frame)
{
    uint16_t n;

    // habilita a transmissão    
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(2);
    
    // início do frame, sem escape
    uart3_send_byte(FRAME_FLAG, 0);

    // payload, com possíveis escape
    for(n = 0 ; n < (CMD_HDR_SIZE+frame->cmd.size) ; n++)
        uart3_send_byte(frame->buffer[n], 1);

#if 0
    // crc (escape tamb�m pode existir)
    uart3_send_byte(frame->cmd.crc >> 8, 1);
    uart3_send_byte(frame->cmd.crc & 0xFF, 1);
#endif
#if 1
    //-------------------------------------------------------------------------------------
    uart3_send_byte(crc1[1], 1);
    uart3_send_byte(crc1[0] & 0xFF, 1);
    //-------------------------------------------------------------------------------------
#endif
    
    // fim do frame, sem escape
    uart3_send_byte(FRAME_FLAG, 0);

    // desabilita a transmissão    
    HAL_Delay(2);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);    
}

static uint8_t check_frame(circ_buffer_t *cb, frame_t *frame)
{
    uint16_t crc;
    uint8_t n;
    
    // destino
    if(circ_buffer_get(cb, &(frame->cmd.dst)) != CIRC_BUFFER_OK)
        return 0;

    // verifica o destino
    if(frame->cmd.dst != CMD_DEV_ADDR)
        return 0;
    
    // origem
    if(circ_buffer_get(cb, &(frame->cmd.src)) != CIRC_BUFFER_OK)
        return 0;
    
    // registro
    if(circ_buffer_get(cb, &(frame->cmd.reg)) != CIRC_BUFFER_OK)
        return 0;
            
    // tamanho
    if(circ_buffer_get(cb, &(frame->cmd.size)) != CIRC_BUFFER_OK)
        return 0;
    
    // verificar consistência do tamanho aqui !
    // TODO
    
    // payload (size bytes)
    for(n = 0 ; n < frame->cmd.size ; n++)
    {
        if(circ_buffer_get(cb, &(frame->cmd.payload[n])) != CIRC_BUFFER_OK)
            return 0;
    }
 
    // ler o CRC (big endian, 16 bits)
    if(circ_buffer_get(cb, &n) != CIRC_BUFFER_OK)
        return 0;
    
    frame->cmd.crc = (n << 8);
    
    if(circ_buffer_get(cb, &n) != CIRC_BUFFER_OK)
        return 0;
    
    frame->cmd.crc |= n;
    
    // verificando o CRC
    crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    
    if(crc != frame->cmd.crc)
        return 0;
        
    // frame válido
    return 1;
}

void complet_string(char *str1, char x[8]){
	uint8_t n;
	char spaces[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
	strcpy(str1, x);
    n = (8- strlen(str1));
    strncat(str1, spaces, n);
}

static void decode_and_answer_version(frame_t *frame)
{
    uint8_t c;
    
    // troca dst/src
    c = frame->cmd.dst;
#if 0
    frame->cmd.dst = frame->cmd.src;
    frame->cmd.src = c;
    // informa a vers�o no payload
    frame->cmd.payload[0] = CMD_VERSION;
    // ajusta o tamanho
    frame->cmd.size = 1;
    // calcula o crc
    frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
#endif
#if 1
    buf_io_put8_tb(frame->cmd.src, &frame->cmd.dst);
    buf_io_put8_tb(c, &frame->cmd.src);
    buf_io_put8_tb(CMD_VERSION, &frame->cmd.payload[0]);
    buf_io_put8_tb(1, &frame->cmd.size);
	//buf_io_put16_tb(crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size), &frame->cmd.crc);
	frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    buf_io_put8_tb((frame->cmd.crc), &crc1[0]);
    buf_io_put8_tb((frame->cmd.crc >> 8), &crc1[1]);
#endif
    // envia
    send_frame(frame);
}

static void decode_and_answer_identification(frame_t *frame)
{
    uint8_t c;
    char str[8];

    // troca dst/src
    c = frame->cmd.dst;
    buf_io_put8_tb(frame->cmd.src, &frame->cmd.dst);
    buf_io_put8_tb(c, &frame->cmd.src);
    // ajusta o tamanho
    buf_io_put8_tb(22, &frame->cmd.size);
    // informa o modelo
    complet_string(&str, MODEL);
    for(int i=0; i<8; i++)
    	buf_io_put8_tb((uint8_t)str[i], &frame->cmd.payload[i]);
    // informa a manufatura
    complet_string(&str, MANUF);
    for(int i=8; i<16; i++)
    	buf_io_put8_tb((uint8_t)str[i-8], &frame->cmd.payload[i]);
    // informa o ID
    buf_io_put8_tb((uint8_t)(id >> 24), &frame->cmd.payload[16]);
    buf_io_put8_tb((uint8_t)(id >> 16), &frame->cmd.payload[17]);
    buf_io_put8_tb((uint8_t)(id >> 8), &frame->cmd.payload[18]);
    buf_io_put8_tb((uint8_t)(id), &frame->cmd.payload[19]);

    // informa a revisao
    buf_io_put8_tb(REV, &frame->cmd.payload[20]);
    // informa a revisao
    buf_io_put8_tb(POINT, &frame->cmd.payload[21]);

    // calcula o CRC
    frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    buf_io_put8_tb((frame->cmd.crc), &crc1[0]);
    buf_io_put8_tb((frame->cmd.crc >> 8), &crc1[1]);
    
    // envia
    send_frame(frame);
}

static void answer_frame(frame_t *frame)
{
    switch(frame->cmd.reg)
    { 
        case CMD_ITF_VER:
            decode_and_answer_version(frame);
            break;          
        case CMD_IDENT:
        	decode_and_answer_identification(frame);
            break;            
        case CMD_POINT_DESC_BASE:
        case CMD_POINT_DESC_BASE+1:
        case CMD_POINT_DESC_BASE+2:
        //case CMD_POINT_DESC_BASE+n:
        // uint8_t p = frame->cmd.reg - CMD_POINT_DESC_BASE;        
            break;  
        case CMD_POINT_READ_BASE:
        case CMD_POINT_READ_BASE+1:
        case CMD_POINT_READ_BASE+2:
        //case CMD_POINT_READ_BASE+n:
        // uint8_t p = frame->cmd.reg - CMD_POINT_READ_BASE;        
            break;  
        case CMD_POINT_WRITE_BASE:
        case CMD_POINT_WRITE_BASE+1:
        case CMD_POINT_WRITE_BASE+2:
        //case CMD_POINT_WRITE_BASE+n:
        // uint8_t p = frame->cmd.reg - CMD_POINT_WRITE_BASE;        
            break; 
        default:
            break;
    }
}

static void setup_leds(void)
{
    GPIO_InitTypeDef  GPIO_out;
    
    // liga o clock para a porta D
    __GPIOD_CLK_ENABLE();  

    // setup dos pinos PD15-PD12
    GPIO_out.Pin = GPIO_PIN_15|GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_12;
    GPIO_out.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_out.Pull = GPIO_PULLUP;
    GPIO_out.Speed = GPIO_SPEED_FAST;

    HAL_GPIO_Init(GPIOD, &GPIO_out);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15|GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_12, GPIO_PIN_RESET);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

     // inicializa o buffer circular e o controle de recepção
    circ_buffer_init(&cb,cb_area,CMD_MAX_SIZE);
    new_frame = 0;
    
    setup_leds();
    setup_uart();
    uart3_enable_int();
    
    //Enable RX
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET); 

    while(1)
    {
        if(new_frame)
        {
            // indica a comunicacao
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);    

            // se o frame for consistente, chama a rotina de tratamento de resposta
            if(check_frame(&cb, &frame))
                answer_frame(&frame);
    
            // esvazia o buffer
            circ_buffer_flush(&cb);

            // libera a recepção novamente
            new_frame = 0;
            
            // indica a comunicacao
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);    
        }
    }
    
    return 0;
}


void HAL_SYSTICK_Callback(void)
{

}


static void Error_Handler(void)
{
    while(1)
    {
    }
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
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


