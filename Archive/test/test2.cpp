/*
 * final_test_01.c
 *
 * Created: 2021-12-10 오후 4:49:39
 * Author : ESE
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL
#define FOSC 16000000UL
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define BAUD_9600 103
#define interrupt_is_disabled
#define LED_COUNT 16
#include "mc_ws2812.h"
#define MAX_TXBUF_SIZE 128
#define MAX_BUFF_SIZE 128

#define LED_ALL_ON_OFF_M 0x01
#define LED_ALTERNATING_M 0x02
#define LED_SHIFT_M 0x03

#define NEO_ALL_ON_OFF_M 0x10
#define NEO_SHIFT_M 0x20
#define NEO_BRIGHTNESS_M 0x40
#define NEO_Selective_ON_M 0x80

#define ADC_ON_M 0x01
#define ADC_OFF_M 0x02
#define ADC_LED_M 0x03
#define ADC_NEO_M 0x04


#define STX 0x02
#define ETX 0x03

void tx_char(unsigned char txChar){
	while(!(UCSR1A & (1<<UDRE1)));//
	UDR1 = txChar;
}

void tx_str(unsigned char *txStr, int _len){
	for(int i = 0; i < _len; i++){
		tx_char(txStr[i]);
	}
}

void my_printf(char *fmt, ...){
	va_list arg_ptr;
	uint8_t len;
	char sText[128];
	
	/* text buffer */
	for (uint8_t i = 0; i < 128; i++){
		sText[i] = 0;
	}
	va_start(arg_ptr, fmt);
	vsprintf(sText, fmt, arg_ptr);
	va_end(arg_ptr);
	
	len = strlen(sText);
	for (uint8_t i = 0; i < len; i++){
		tx_char(sText[i]);
	}
}

unsigned int GetADCData(unsigned char aIn){
	volatile unsigned int result;
	ADMUX = aIn; // Channel Selection (0~7), 8~15는 0x100000을 더해야 함.
	ADMUX |= (1 << REFS0); // AVCC 옵션 (외부공급 5V)
	ADCSRA = (1 << ADEN) | (1<<ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	// ADC Enable, ADC prescaler : 128
	ADCSRA |= (1 << ADSC); // ADC Start Conversion
	while(!(ADCSRA&(1 << ADIF))); // ADIF is set when an ADC completes
	_delay_ms(300); // 안전장치 ADC에는 시간이 좀 걸림. 위 와일문에서 기다려주지만 그래도 안전장치 하나 더.
	result = ADCL + (ADCH << 8); // left adjust
	ADCSRA = 0x00; // 초기화
	return result;
}

void led_on_by_ADC(rgbw_color color, int _ADC_level){
	// 몇 번째 led를 켤려고 하면 바로 그 인덱스에
	// 쓰는게 아니라 전체 led를 하나씩 쉬프트 하면서
	// 해당 led까지 가야함.
	
	int on_led_level = _ADC_level / 64; // 1024를 64로 나누면 16등분이 됨. 따라서 받은 레벨을 64 나누고 그 몫만 가지고 그 몫번째 led 키면 됨.
	rgbw_color cArr[LED_COUNT];
	for(int i = 0; i<LED_COUNT; i++){
		if(i <= on_led_level) cArr[i] = color;
		else cArr[i] = (rgbw_color) {0, 0, 0, 0};
	}
	ws2812_setleds_rgbw_PB(6, cArr,LED_COUNT);
}

void LED_ALL_ON_OFF(uint8_t _cnt)
{
	for(int i=0; i<_cnt; i++)
	{
		PORTC = 0x00;
		_delay_ms(200);
		PORTC = 0xFF;
		_delay_ms(200);
	}
	return;
}
void LED_SHIFT(uint8_t _cnt)
{
	for(int k=0; k<_cnt; k++)
	{
		for(int i = 0; i<8 ;i++)
		{
			PORTC = ~(0x01 << i);
			_delay_ms(100);
		}
		for(int i=0; i<8; i++)
		{
			PORTC = ~(0x80 >> i);
			_delay_ms(100);
		}
	}
}

void LED_ALTERNATING(uint8_t _cnt)
{
	for(int i = 0; i < _cnt; i++)
	{
		PORTC = 0xAA;
		_delay_ms(200);
		PORTC = (unsigned char)~(0xAA);
		_delay_ms(200);
	}
	return;
}


void neo_led_on(rgbw_color color, int index){
	// 몇 번째 led를 켤려고 하면 바로 그 인덱스에
	// 쓰는게 아니라 전체 led를 하나씩 쉬프트 하면서
	// 해당 led까지 가야함.
	rgbw_color cArr[LED_COUNT];
	for(int i = 0; i<LED_COUNT; i++){
		if(i == index) cArr[i] = color;
		else if (i == 15-index) cArr[i] = color;
		else cArr[i] = (rgbw_color) {0, 0, 0, 0};
	}
	ws2812_setleds_rgbw_PB(6, cArr,LED_COUNT);
}

void set_led_ring_brightness(int _level){
	rgbw_color cArr[LED_COUNT];
	
	for(int j = 0; j < LED_COUNT; j++){
		cArr[j] = (rgbw_color) {0, 0, 0, _level};
	}
	ws2812_setleds_rgbw_PB(6, cArr, LED_COUNT);

}

void neo_all_off(){
	rgbw_color tempColors[LED_COUNT];
	for(unsigned char i = 0; i < LED_COUNT; i++){
		tempColors[i] = (rgbw_color) {0, 0, 0, 0};
	}
	ws2812_setleds_rgbw_PB(6, tempColors, LED_COUNT);
}

void neo_all_on(){
	rgbw_color tempColors[LED_COUNT];
	for(unsigned char i = 0; i < LED_COUNT; i++){
		tempColors[i] = (rgbw_color) {0, 0, 0, 128/64};
	}
	ws2812_setleds_rgbw_PB(6, tempColors, LED_COUNT);
}

void neo_all_on_off(uint8_t _cnt)
{
	for(int i = 0; i < _cnt; i++)
	{
		neo_all_on();
		_delay_ms(200);
		neo_all_off();
		_delay_ms(200);
	}
}

void neo_shift(rgbw_color color){
	rgbw_color cArr[LED_COUNT];
	
	for(int j = 0; j < LED_COUNT; j++){
		for(int i = 0; i<LED_COUNT; i++){
			if(i == j) cArr[i] = color;
			else cArr[i] = (rgbw_color) {0, 0, 0, 0};
		}
		ws2812_setleds_rgbw_PB(6, cArr,LED_COUNT);
		_delay_ms(100);
	}
	
}

void neo_ring_brightness(){
	rgbw_color cArr[LED_COUNT];
	int dir = 0;
	for(int i = 0; dir? i>=0 : i < 25; dir? i-- : i++){
		for(int j = 0; j < LED_COUNT; j++){
			cArr[j] = (rgbw_color) {i*10, 0, 0, 0};
		}
		ws2812_setleds_rgbw_PB(6, cArr, LED_COUNT);
		_delay_ms(50);
		if(i==24){
			dir=1;
		}
	}
}


int decode = 0; // 0 : normal, 1 : command reception, 2 : data reception, 3 : termination state
uint8_t _command = 0;
uint8_t _data = 0;
uint8_t len = 0;
uint8_t _func = 0;
uint8_t _param = 0;



uint8_t _hw = 0;
int sint_flag = 0;

// 데이터 받을 때 마다 한 바이트씩 동작하는 인터럽트.
unsigned char com_size = 0;
unsigned char _buff[MAX_BUFF_SIZE];
uint8_t len_cnt = 0;


int adc_flag = 0;
void disp_led(unsigned int adc)
{
	PORTC = 0xFF;
	
	while(adc_flag == 1)
	{
		_delay_ms(10);
		for(int i=0; i<=(adc/128); i++){
			PORTC &= (unsigned char)~(0x01<<i);
		}
	}
	
	PORTC = 0xFF;
}

int adc_led_control = 0;
void hex_command(uint8_t func_, uint8_t param_, unsigned char _com_size){
	//PORTC = (unsigned char)~(0x40);
	_delay_ms(500);
	rgbw_color redColor;
	rgbw_color greenColor;
	rgbw_color blueColor;
	// Color order : G, R, B, W
	redColor = (rgbw_color) {0, 128, 0, 0};
	greenColor = (rgbw_color) {128, 0, 0, 0};
	blueColor = (rgbw_color) {0, 0, 128, 0};
	
	//if(func_ == 0x01)
	//{
		//LED_ALL_ON_OFF(param_);
	//}
	
	uint8_t tmp[MAX_BUFF_SIZE];
	uint8_t ret = '\0';
	memset(tmp, '\0', MAX_BUFF_SIZE);
	memcpy((uint8_t*)tmp, (uint8_t*)_buff,_com_size);
	
	if(strstr((char*)tmp, "LED") != NULL){
		neo_all_off();
		adc_flag = 0;
		adc_led_control =0;
		com_size = strlen(tmp);
		tx_str(_buff, com_size);
		
		if(func_ == 0x01)
		{
			LED_ALL_ON_OFF(param_);
			PORTC = 0xFF;
		}
		else if(func_ == 0x02)
		{
			LED_ALTERNATING(param_);
			PORTC = 0xFF;
		}
		else if(func_ == 0x03)
		{
			LED_SHIFT(param_);
			PORTC = 0xFF;
		}
		
	}
	else if(strstr((char*)tmp, "NEOPIXEL") != NULL){
		PORTC = 0xFF;
		adc_flag = 0;
		adc_led_control =0;
		com_size = strlen(tmp);
		tx_str(_buff,com_size);
		//PORTC = 0x00;
		//_delay_ms(1000);
		//PORTC = 0xF0;
		//neo_all_on();
		if(func_ == 0x10)
		{
			neo_all_on_off(_param);
		}
		else if(func_ == 0x20)
		{
			for(int i = 0; i < _param; i++)
			{
				neo_shift(redColor);
			}
			neo_all_off();
		}
		else if(func_ == 0x40)
		{
			for(int i = 0; i < param_; i++)
			{
				neo_ring_brightness();
			}
			neo_all_off();
		}
		else if(func_==0x80)
		{
			neo_led_on(redColor, _param);
			//neo_all_off();
		}
		
		
	}
	else if(strstr((char*)tmp, "ADC") != NULL){
		com_size = strlen(tmp);
		tx_str(_buff,com_size);
		if(func_ == 0x01)
		{
			adc_flag = 1;
		}
		else if(func_ == 0x02)
		{
			adc_flag = 0;
			adc_led_control =0;
		}
		else if(func_ == 0x03 && adc_flag == 1)
		{
			adc_led_control = 1;
		}
		else if(func_ == 0x04 && adc_flag == 1)
		{
			adc_led_control =0;
		}
		
	}
}

ISR (USART1_RX_vect){
	unsigned char _usart_rcv = 0;
	_usart_rcv = UDR1; // 받은 1바이트 대입.
	if(decode == 0 && _usart_rcv == STX){
		//PORTC = (unsigned char)~0x01;
		decode = 1;
	}
	else if(decode == 1){ // len 받기
		len = _usart_rcv;
		decode = 2;
		//PORTC = (unsigned char)~0x02;
	}
	else if(decode == 2){ // len에 맞춰서 H/W받기
		_buff[com_size++] = _usart_rcv;
		len_cnt++;
		if(len_cnt == len - 2)
		{
			decode = 3;
		}

		//PORTC = (unsigned char)~0x04;
		// L 1
		// E 2
		// D 3
		
	}
	else if(decode == 3){
		_func = _usart_rcv;
		decode = 4;
		//PORTC = (unsigned char)~0x08;
	}
	else if(decode == 4){
		_param = _usart_rcv;
		decode = 5;
		//PORTC = (unsigned char)~0x10;
	}
	else if(decode == 5){
		if(_usart_rcv == ETX){
			sint_flag = 1;
			//PORTC = (unsigned char)~0x20;
		}
		else{
			my_printf("RX error\n");
		}
		
		decode = 0;
		
	}
	sei();
}

int start_pro = 0;

int main(void)
{
	cli();
	DDRC = 0xFF;
	PORTC = 0xFF;
	DDRB |= 0x01 << 6;
	rgbw_color redColor;
	redColor = (rgbw_color) {0, 32, 0, 0};
	UBRR1H = (unsigned char) (BAUD_9600 >>8) ;
	UBRR1L = (unsigned char) BAUD_9600;
	UCSR1B = (1<< TXEN1)| (1<< RXEN1); // tx, rx 1번 채널로 활성화.
	UCSR1B |= (1<<RXCIE1); // rx할 때마다 int 하는 것. or로 넣야함. 그냥 넣으면 바로 윗줄 tx, rv 설정한거 무산됨.
	UCSR1C = (1<< UCSZ11) | (1<< UCSZ10) | (1<<USBS1);
	
	// 연결 완료
	sei();
	
	//PORTC = 0x55; // 0101 0101
	
	/* Replace with your application code */
	while (1)
	{
		//while (!(UCSR1A & 1 << UDRE1)); // 데이터가 다 보내졌는가
		if(start_pro ==0)
		{
			UDR1 = "O";
		}

		//unsigned int adc_value = GetADCData(0);
		if(sint_flag){
			start_pro = 1;
			hex_command(_func, _param, com_size);
			
			// 모든 전역 변수 초기화.
			sint_flag = 0;
			_command = 0;
			_data = 0;
			len = 0;
			_func = 0;
			_param = 0;
			len_cnt = 0;
			_hw = 0;
			com_size = 0;
			
			
			//if(adc_led_control == 1)
			//{
				//while(adc_led_control == 1 && adc_flag == 1)
				//{
					//unsigned int adc_value = GetADCData(0);
					//disp_led(adc_value);
				//}
				//adc_led_control = 0;
				//PORTC = 0xFF;
			//}
			
			
		}
		
	}
}
