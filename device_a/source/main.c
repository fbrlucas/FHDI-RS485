#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"
#include "circ_buffer.h"
#include "crc16.h"
#include "device_info.h"

// Connect to target, reset (black button), then load and press c
uint32_t id = ID;
uint8_t cb_area[CMD_MAX_SIZE];
volatile uint8_t new_frame = 0;
circ_buffer_t cb;
frame_t frame;

extern point_t points[N_POINT];

static void SystemClock_Config(void);
static void Error_Handler(void);

UART_HandleTypeDef uart_dev3;
ADC_HandleTypeDef adc_dev;
TIM_HandleTypeDef tim4;

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

void setup_adc(void)
{
    ADC_ChannelConfTypeDef channel;
    GPIO_InitTypeDef  adc_pin;

    // Ligando os clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Setup do pino
    adc_pin.Pin = GPIO_PIN_0;
    adc_pin.Mode = GPIO_MODE_ANALOG;
    adc_pin.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &adc_pin);

    // CONFIGURAÇÕES GERAIS DO ADC
    // Qual ADC ? 1, 2 ou 3?
    adc_dev.Instance = ADC1;
    // Do manual, máximo é 30MHz (APB2/4 = 84/4 = 21MHz)
    adc_dev.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4;
    // Qual resolução ?
    adc_dev.Init.Resolution = ADC_RESOLUTION_12B;
    // Conversão multi channel ou single channel  ?
    adc_dev.Init.ScanConvMode = DISABLE;
    // Fica lendo continuamente ou somente uma vez ?
    adc_dev.Init.ContinuousConvMode = DISABLE;
    // Algum tipo de leitura discontínua ? Se sim, quantas ?
    adc_dev.Init.DiscontinuousConvMode = DISABLE;
    adc_dev.Init.NbrOfDiscConversion = 0;
    // Alguma fonte externa que inicia a conversão ?
    adc_dev.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc_dev.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    // Alinha à direita ou esquerda ?
    adc_dev.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    // Quantas conversões serão feitas ? (multi channel, 1 a 16)
    adc_dev.Init.NbrOfConversion = 1;
    // Vamos usar DMA ?
    adc_dev.Init.DMAContinuousRequests = DISABLE;
    // Interrupção no fim do scan ou no fim de cada canal ?
    adc_dev.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if(HAL_ADC_Init(&adc_dev) != HAL_OK)
      Error_Handler();

    // CONFIGURAÇÕES DO/DOS CANAL/CANAIS

    // Qual canal está sendo confgurado ?
    channel.Channel = ADC_CHANNEL_8;
    // Posição ?
    channel.Rank = 1;
    // Tempo de conversão usado ?
    channel.SamplingTime = ADC_SAMPLETIME_28CYCLES;
    // Não usado
    channel.Offset = 0;

    if(HAL_ADC_ConfigChannel(&adc_dev, &channel) != HAL_OK)
      Error_Handler();
}

static void setup_leds(void)
{
    GPIO_InitTypeDef  GPIO_out;

    // liga o clock para a porta D
    __GPIOD_CLK_ENABLE();

    // setup dos pinos PD15-PD12
    GPIO_out.Pin = GPIO_PIN_15|GPIO_PIN_14;
    GPIO_out.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_out.Pull = GPIO_PULLUP;
    GPIO_out.Speed = GPIO_SPEED_FAST;

    HAL_GPIO_Init(GPIOD, &GPIO_out);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15|GPIO_PIN_14, GPIO_PIN_RESET);
}

static void setup_switch(void)
{
	GPIO_InitTypeDef  GPIO_inp;
    __GPIOA_CLK_ENABLE();  // liga o clock para a porta A

    GPIO_inp.Pin = GPIO_PIN_0;
    GPIO_inp.Mode = GPIO_MODE_INPUT;
    GPIO_inp.Pull = GPIO_NOPULL;
    GPIO_inp.Speed = GPIO_SPEED_FAST;

    HAL_GPIO_Init(GPIOA, &GPIO_inp);
}

void setup_tim4(void)
{
  TIM_OC_InitTypeDef pwm_chan;

  GPIO_InitTypeDef GPIO_InitStruct;

  // habilita o tim4
  __TIM4_CLK_ENABLE();
  // habilita o clock para as portas
  __GPIOD_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;

  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  tim4.Instance = TIM4;
  tim4.Init.Prescaler = 27;
  tim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  tim4.Init.Period = 59999;
  tim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&tim4);

  // configura cada canal
  pwm_chan.OCMode = TIM_OCMODE_PWM1;
  pwm_chan.Pulse = 14999;
  pwm_chan.OCPolarity = TIM_OCPOLARITY_HIGH;
  pwm_chan.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&tim4, &pwm_chan, TIM_CHANNEL_1);

  // habilita cada canal de pwm
  HAL_TIM_PWM_Start(&tim4, TIM_CHANNEL_1);
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

    // crc (escape tamb�m pode existir)
    uart3_send_byte(frame->cmd.crc >> 8, 1);
    uart3_send_byte(frame->cmd.crc & 0xFF, 1);

    // fim do frame, sem escape
    uart3_send_byte(FRAME_FLAG, 0);

    // desabilita a transmissão    
    HAL_Delay(2);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);    
}

static uint8_t check_frame(circ_buffer_t *cb, frame_t *frame)
{
    uint16_t crc;
    uint8_t n, x = 0;
    
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
    
    // payload (size bytes)
    for(n = 0 ; n < frame->cmd.size ; n++)
    {
        if(circ_buffer_get(cb, &(frame->cmd.payload[n])) != CIRC_BUFFER_OK)
            return 0;
    }

    //Verificar se existe o ponto solicitado
    if(((frame->cmd.reg) >= 0x10) && ((frame->cmd.reg) <= 0x6F))
    {
		for (int i = 0; i <= N_POINT; i++)
		{
			if ((frame->cmd.reg == ((points[i].address) + CMD_POINT_DESC_BASE)) || (frame->cmd.reg == ((points[i].address) + CMD_POINT_READ_BASE)) || (frame->cmd.reg == ((points[i].address) + CMD_POINT_WRITE_BASE)))
			{
				x = 1;
			}
		}
		if(x != 1)
		{
			x = 0;
			return 0;
		}
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

static uint8_t check_points()
{
	//Check the existence of the same addresses points
	uint8_t i, j;

    for (i = 0; i <= N_POINT; i++)
    {
        for (j = i + 1; j < N_POINT; j++)
        {
            if (points[j].address == points[i].address)
            {
            	return 0;
            }
        }
    }
    //Check if Data Types are suported and access rights
    for (i = 0; i <= N_POINT; i++)
    {
    	if ((points[i].type > 10) || (points[i].rights > 3))
    	{
    		return 0;
    	}
    }
    return 1;
}

static void decode_and_answer_version(frame_t *frame)
{
    uint8_t c;
    // troca dst/src
    c = frame->cmd.dst;
    buf_io_put8_tb(frame->cmd.src, &frame->cmd.dst);
    buf_io_put8_tb(c, &frame->cmd.src);
    buf_io_put8_tb(CMD_VERSION, &frame->cmd.payload[0]);
    buf_io_put8_tb(1, &frame->cmd.size);
	//buf_io_put16_tb(crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size), &frame->cmd.crc);
	frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    // envia
    send_frame(frame);
}

static void decode_and_answer_identification(frame_t *frame)
{
    uint8_t s;
    uint8_t *pbuf = frame->buffer;

    // troca dst/src
    s = frame->cmd.dst;
    buf_io_put8_tb(frame->cmd.src, pbuf);
    pbuf += 1;
    buf_io_put8_tb(s, pbuf);
    pbuf += 1;
    buf_io_put8_tb(frame->cmd.reg, pbuf);
    pbuf += 1;
    buf_io_put8_tb(22, pbuf);
    pbuf += 1;
    strncpy(pbuf,MODEL "        ",8);
    pbuf += 8;
    strncpy(pbuf,MANUF "        ",8);
    pbuf += 8;
    buf_io_put32_tb(id, pbuf);
    pbuf += 4;
    buf_io_put8_tb(REV, pbuf);
    pbuf += 1;
    buf_io_put8_tb(N_POINT, pbuf);
    frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);

    send_frame(frame);
}

static void decode_and_answer_description(frame_t *frame, uint8_t p_address)
{
    uint8_t s;
    uint8_t *pbuf = frame->buffer;

    // troca dst/src
    s = frame->cmd.dst;
    buf_io_put8_tb(frame->cmd.src, pbuf);
    pbuf += 1;
    buf_io_put8_tb(s, pbuf);
    pbuf += 1;
    buf_io_put8_tb(frame->cmd.reg, pbuf);
    pbuf += 1;
    buf_io_put8_tb(11, pbuf);
    pbuf += 1;
    for(int i = 0; i < N_POINT; i++)
    {
    	if(points[i].address == p_address)
    	{
    		s = i;
    	}
    }
    strncpy(pbuf, points[s].name, 8);
    pbuf += 8;
    buf_io_put8_tb(points[s].type, pbuf);
    pbuf += 1;
    buf_io_put8_tb(points[s].unit, pbuf);
    pbuf += 1;
    buf_io_put8_tb(points[s].rights, pbuf);
    frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);

    send_frame(frame);
}


static void decode_and_answer_read(frame_t *frame, uint8_t p_address)
{
	uint8_t s, t,  x = 0;
	uint8_t *pbuf = frame->buffer;

    for(int i = 0; i < N_POINT; i++)
    {
    	if(points[i].address == p_address) //Find the position in array correspondent of the point address
    	{
    		s = i;
    	}
    }

    if((points[s].rights == 0)||(points[s].rights == 2)){
    	// troca dst/src
    	t = frame->cmd.dst;
    	buf_io_put8_tb(frame->cmd.src, pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(t, pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(frame->cmd.reg, pbuf);
    	pbuf += 3; //Pula 3 bytes diretamente para o value

    	if(points[s].type == 0x00)
    	{
    		buf_io_put8_tl(points[s].value._ui8, pbuf); //Access the value in uint8 mode (type 0)
    		x = 1; 	//Size of byte variable
    	}
    	else if(points[s].type == 0x01)
    	{
    		buf_io_put8_tl(points[s].value._i8, pbuf); //Access the value in uint8 mode (type 1)
    		x = 1; 	//Size of byte variable
    	}
    	else if(points[s].type == 0x02)
    	{
    		buf_io_put16_tl(points[s].value._us16, pbuf); //Access the value in uint8 mode (type 2)
    		x = 2; 	//Size of short variable
    	}
    	else if(points[s].type == 0x03)
    	{
    		buf_io_put16_tl(points[s].value._s16, pbuf); //Access the value in uint8 mode (type 3)
    		x = 2; 	//Size of short variable
    	}
    	else if(points[s].type == 0x04)
    	{
    		buf_io_put32_tl(points[s].value._ul32, pbuf); //Access the value in uint8 mode (type 4)
    		x = 4; 	//Size of long variable
    	}
    	else if(points[s].type == 0x05)
    	{
    		buf_io_put32_tl(points[s].value._l32, pbuf); //Access the value in uint8 mode (type 5)
    		x = 4; 	//Size of long variable
    	}
    	else if(points[s].type == 0x06)
    	{
    		buf_io_put64_tl(points[s].value._ull64, pbuf); //Access the value in uint8 mode (type 6)
    		x = 8; 	//Size of long long variable
    	}
    	else if(points[s].type == 0x07)
    	{
    		buf_io_put64_tl(points[s].value._ll64, pbuf); //Access the value in uint8 mode (type 7)
    		x = 8; 	//Size of long long variable
    	}
    	else if(points[s].type == 0x08)
    	{
    		uint8_t *pf;
    		pf = (uint8_t *) &points[s].value._f;
    		pf += 3;
    		*pbuf = *pf;
    		pbuf += 1;
    		pf -= 1;

    		*pbuf = *pf;
    		pbuf += 1;
    		pf -= 1;

    		*pbuf = *pf;
    		pbuf += 1;
    		pf -= 1;

    		*pbuf = *pf;
    		pf -= 1;
    		pbuf -= 3;

    		x = 4; 	//Size of float variable

    	}
    	else if(points[s].type == 0x09)
    	{
    		buf_io_putd_tl(buf_io_getd_fb(points[s].value._d), pbuf); //Access the value in uint8 mode (type 9)
    		x = 8; 	//Size of double variable
    	}

    	pbuf -= 2; //Retorna o ponteiro para a posi��o do size no buffer
    	buf_io_put8_tb((1+x), pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(points[s].type, pbuf);

    	frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    	send_frame(frame);
    }
}


static void decode_and_answer_write(frame_t *frame, uint8_t p_address)
{
	uint8_t s, t;
	uint8_t *pbuf = frame->buffer;

    for(int i = 0; i < N_POINT; i++)
    {
    	if(points[i].address == p_address) //Find the position in array correspondent of the point address
    	{
    		s = i;
    		break;
    	}
    }

    if((points[s].rights == 1)||(points[s].rights == 2)){
    	// troca dst/src
    	t = frame->cmd.dst;
    	buf_io_put8_tb(frame->cmd.src, pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(t, pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(frame->cmd.reg, pbuf);
    	pbuf += 1;
    	buf_io_put8_tb(0, pbuf);
    	pbuf += 1;

    	frame->cmd.crc = crc16_calc(frame->buffer, CMD_HDR_SIZE+frame->cmd.size);
    	send_frame(frame);

    	if(*pbuf == 0x00)
    	{
    		pbuf += 1;
    		buf_io_put8_tb(buf_io_get8_fb(pbuf), &points[s].value._ui8);
    	}
    	else if(*pbuf == 0x01)
    	{
    		pbuf += 1;
    		buf_io_put8_tb(buf_io_get8_fb(pbuf), &points[s].value._i8);
    	}
    	else if(*pbuf == 0x02)
    	{
    		pbuf += 1;
    		buf_io_put16_tb(buf_io_get16_fb(pbuf), &points[s].value._us16);
    	}
    	else if(*pbuf == 0x03)
    	{
    		pbuf += 1;
    		buf_io_put16_tb(buf_io_get16_fb(pbuf), &points[s].value._s16);
    	}
    	else if(*pbuf == 0x04)
    	{
    		pbuf += 1;
    		buf_io_put32_tb(buf_io_get32_fb(pbuf), &points[s].value._ul32);
    	}
    	else if(*pbuf == 0x05)
    	{
    		pbuf += 1;
    		buf_io_put32_tb(buf_io_get32_fb(pbuf), &points[s].value._l32);
    	}
    	else if(*pbuf == 0x06)
    	{
    		pbuf += 1;

    		uint8_t *pf;
    		pf = (uint8_t *) &points[s].value._ll64;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;


    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    	}
    	else if(*pbuf == 0x07)
    	{
    		pbuf += 1;

    		uint8_t *pf;
    		pf = (uint8_t *) &points[s].value._ull64;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;


    		*pf = *pbuf;
    		pbuf += 1;
    		pf += 1;

    		*pf = *pbuf;

    		//buf_io_put64_tb(buf_io_get64_fb(pbuf), &points[s].value._ll64);
    	}
    	else if(*pbuf == 0x08)
    	{
    		pbuf += 1;
    		uint8_t *pf;
    		pf = (uint8_t *) &points[s].value._f;
    		pf += 3;
    		*pf = *pbuf;
    		pbuf += 1;
    		pf -= 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf -= 1;

    		*pf = *pbuf;
    		pbuf += 1;
    		pf -= 1;

    		*pf = *pbuf;
    	}
    	else if(*pbuf == 0x09)
    	{
    		pbuf += 1;
    		buf_io_putd_tb(buf_io_getd_fb(pbuf), &points[s].value._d);
    	}
    }
}


static void answer_frame(frame_t *frame)
{
	uint8_t i;

	if(frame->cmd.reg == CMD_ITF_VER){
		decode_and_answer_version(frame);
	}
	else if(frame->cmd.reg == CMD_IDENT){
		decode_and_answer_identification(frame);
	}
	else
	{
		for(i = 1; i < 32; i++)
		{
			if(frame->cmd.reg == CMD_POINT_DESC_BASE+i)
			{
				decode_and_answer_description(frame, i);
				break;
			}
			else if(frame->cmd.reg == CMD_POINT_READ_BASE+i)
			{
				decode_and_answer_read(frame, i);
				break;
			}
			else if(frame->cmd.reg == CMD_POINT_WRITE_BASE+i)
			{
				decode_and_answer_write(frame, i);
				break;
			}
		}
	}
}

void change_pwm()
{
	uint16_t v;
	v = (uint16_t)(points[3].value._f * 100);
	if(v <= 100){
		v = v*599;
		TIM4->CCR1 = v;
	}
}

void read_ad()
{
	HAL_ADC_Start(&adc_dev);
	// espera até 2000ms para que a conversão finalize ou aborta
	if(HAL_ADC_PollForConversion(&adc_dev, 2000) == HAL_OK)
	{
		points[2].value._f = (float)((HAL_ADC_GetValue(&adc_dev)* (0.122070)-14.0));
	}
}

void change_led()
{
	if(points[0].value._ui8 == 1)
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	else if(points[0].value._ui8 == 0)
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
}

void read_switch(){
	points[1].value._ui8 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

     // inicializa o buffer circular e o controle de recepção
    circ_buffer_init(&cb,cb_area,CMD_MAX_SIZE);
    new_frame = 0;
    
    setup_switch();
    setup_leds();
    setup_uart();
    uart3_enable_int();
    setup_adc();
    setup_tim4();
    
    //Enable RX
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET); 

    if(check_points()){
        while(1)
        {
        	//leitura e escrita sensores
        	read_switch();
        	change_led();
            read_ad();
            change_pwm();

            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,PWR_SLEEPENTRY_WFI);

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


