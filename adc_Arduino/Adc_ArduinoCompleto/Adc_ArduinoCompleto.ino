#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD - 1

#define LED_INICIO PB0
#define LED_FIM PB1
#define SAIDA PD6

void uart_init(unsigned int ubrr) {
    // Baud rate
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;

    // Habilita transmissão
    UCSR0B = (1 << TXEN0);

    // Formato: 8 bits, 1 stop, sem paridade
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));  // Espera buffer livre
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
    itoa(val, buffer, 10);  // Converte para string decimal
    uart_print(buffer);
}

void adc_init(void) {
    ADMUX = (1 << REFS0); // AVcc, canal ADC0  entrada analogica A0
    // ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) |
            //  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
    
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) |
         (1 << ADPS2) | (1 << ADPS1); // Prescaler 64
    ADCSRB = 0; // Free Running
}

void io_init(void) {
    DDRB |= (1 << LED_INICIO) | (1 << LED_FIM);
    DDRD |= (1 << SAIDA);
}

int main(void) {
    io_init();
    uart_init(MYUBRR);
    adc_init();

    uint16_t leitura = 0;

    while (1) {
        // LED início da conversão
        PORTB |= (1 << LED_INICIO);

        // Espera a conversão (flag ADIF)
        while (!(ADCSRA & (1 << ADIF)));

        // LED fim da conversão
        PORTB |= (1 << LED_FIM);

        // Leitura
        uint8_t low = ADCL;
        uint8_t high = ADCH;
        leitura = (high << 8) | low;

        // Zera a flag
        ADCSRA |= (1 << ADIF);

        // Atualiza saída digital com base no valor
        if (leitura > 512) {
            PORTD |= (1 << SAIDA);
        } else {
            PORTD &= ~(1 << SAIDA);
        }

        // Envia via serial
        uart_print("BIN: ");
        uart_print_bin(leitura);
        uart_print(" | DEC: ");
        uart_print_uint(leitura);
        uart_print("\r\n");

        // Piscar LED
        _delay_ms(100);
        PORTB &= ~((1 << LED_INICIO) | (1 << LED_FIM));
    }
}
