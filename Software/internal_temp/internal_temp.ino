#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// -----------------------------
// UART CONFIG
// -----------------------------
#define UART_TX_PIN PB4
#define BAUD 9600
#define BIT_DELAY_US (1000000UL / BAUD)

// Initialize TX pin
void uart_init(void)
{
    DDRB |= (1 << UART_TX_PIN);  // Set PB4 as output
}

// Send single byte (8N1)
void uart_tx_byte(uint8_t data)
{
    // Start bit
    PORTB &= ~(1 << UART_TX_PIN);
    _delay_us(BIT_DELAY_US);

    // 8 data bits
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x01)
            PORTB |= (1 << UART_TX_PIN);
        else
            PORTB &= ~(1 << UART_TX_PIN);

        _delay_us(BIT_DELAY_US);
        data >>= 1;
    }

    // Stop bit
    PORTB |= (1 << UART_TX_PIN);
    _delay_us(BIT_DELAY_US);
}

// Send string
void uart_print(const char *str)
{
    while (*str)
    {
        uart_tx_byte(*str++);
    }
}

// -----------------------------
// ADC (Temperature)
// -----------------------------
void adc_init(void)
{
    // 1.1V reference, temp sensor
    ADMUX = (1 << REFS1) | (1 << MUX3);

    ADCSRA =
        (1 << ADEN)  |
        (1 << ADPS2) |
        (1 << ADPS1);  // prescaler 64

    DIDR0 = 0xFF;  // disable digital inputs
}

uint16_t adc_read(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

uint16_t adc_read_avg(uint8_t samples)
{
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += adc_read();
        _delay_ms(2);
    }

    return sum / samples;
}

// -----------------------------
// MAIN
// -----------------------------
int main(void)
{
    char buffer[32];

    uart_init();
    adc_init();

    // stabilize ADC
    adc_read();
    _delay_ms(100);

    while (1)
    {
        uint16_t adc_val = adc_read_avg(16);

        // VERY rough temperature estimate
        int16_t temp = adc_val;

        // Format string
        snprintf(buffer, sizeof(buffer), "Temp: %d\r\n", temp);

        // Send via UART
        uart_print(buffer);

        _delay_ms(1000);
    }
}
