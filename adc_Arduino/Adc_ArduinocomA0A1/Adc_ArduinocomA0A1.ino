#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD - 1

#define LED_INICIO PB0
#define LED_FIM PB1
#define SAIDA_A0 PD7
#define SAIDA_A1 PB2

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void uart_print_bin(uint16_t val) {
    for (int i = 9; i >= 0; i--) {
        uart_transmit((val & (1 << i)) ? '1' : '0');
    }
}

void uart_print_uint(uint16_t val) {
    char buffer[6];
    itoa(val, buffer, 10);
    uart_print(buffer);
}

void adc_init(void) {
    ADMUX = (1 << REFS0); // AVcc como referência
    ADCSRA = (1 << ADEN) | (1 << ADATE) |
             (1 << ADPS2) | (1 << ADPS1); // Prescaler 64
    ADCSRB = 0;
}

void io_init(void) {
    DDRB |= (1 << LED_INICIO) | (1 << LED_FIM) | (1 << SAIDA_A1);
    DDRD |= (1 << SAIDA_A0);
}

uint16_t adc_read(uint8_t canal) {
    ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);  // Seleciona canal

    ADCSRA |= (1 << ADSC); // Inicia conversão
    PORTB |= (1 << LED_INICIO); // LED início

    while (!(ADCSRA & (1 << ADIF))); // Espera conversão

    PORTB |= (1 << LED_FIM); // LED fim

    uint8_t low = ADCL;
    uint8_t high = ADCH;
    uint16_t resultado = (high << 8) | low;

    ADCSRA |= (1 << ADIF); // Limpa flag

    _delay_ms(50);
    PORTB &= ~((1 << LED_INICIO) | (1 << LED_FIM));

    return resultado;
}

int main(void) {
    io_init();
    uart_init(MYUBRR);
    adc_init();

    uint16_t leituraA0 = 0;
    uint16_t leituraA1 = 0;

    while (1) {
        leituraA0 = adc_read(0); // A0
        leituraA1 = adc_read(1); // A1

        // Controle da saída A0 (PD7)
        if (leituraA0 > 512) {
            PORTD |= (1 << SAIDA_A0);
        } else {
            PORTD &= ~(1 << SAIDA_A0);
        }

        // Controle da saída A1 (PB2)
        if (leituraA1 > 512) {
            PORTB |= (1 << SAIDA_A1);
        } else {
            PORTB &= ~(1 << SAIDA_A1);
        }

        // Envia dados pela UART
        uart_print("A0 - BIN: ");
        uart_print_bin(leituraA0);
        uart_print(" | DEC: ");
        uart_print_uint(leituraA0);

        uart_print(" || A1 - BIN: ");
        uart_print_bin(leituraA1);
        uart_print(" | DEC: ");
        uart_print_uint(leituraA1);

        uart_print("\r\n");

        _delay_ms(200);
    }
}
