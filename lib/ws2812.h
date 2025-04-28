#include "ws2818b.pio.h"
#include "hardware/pio.h"

#ifndef WS2812_H
#define WS2812_H

#define LED_COUNT 25
#define LED_PIN 7
#define MATRIX_ROWS 5
#define MATRIX_COLS 5
#define MATRIX_DEPTH 3

// Definição de pixel GRB
struct pixel_t
{
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin)
{

    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

// Modificado do github: https://github.com/BitDogLab/BitDogLab-C/tree/main/neopixel_pio
// Função para converter a posição do matriz para uma posição do vetor.
int getIndex(int x, int y)
{
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0)
    {
        return 24 - (y * 5 + x); // Linha par (esquerda para direita).
    }
    else
    {
        return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

void desenhaMatriz(int matriz[5][5][3])
{
    float intensidade = 0.01;
    for (int linha = 0; linha < 5; linha++)
    {
        for (int coluna = 0; coluna < 5; coluna++)
        {
            int posicao = getIndex(linha, coluna);
            npSetLED(posicao, ((float)matriz[coluna][linha][0] * intensidade), ((float)matriz[coluna][linha][1] * intensidade), ((float)matriz[coluna][linha][2] * intensidade));
        }
    }
    npWrite();
    npClear();
}

// Função para ligar os LEDs de acordo com o código de cores.
void definido(int valor, int posi)
{
    //   printf("valor: %d posi: %d\n", valor, posi);
    switch (valor)
    {
    case 0:
        npSetLED(25 - posi, 0, 0, 0);
        npSetLED(14 + posi, 0, 0, 0);
        npSetLED(15 - posi, 0, 0, 0);
        npSetLED(4 + posi, 0, 0, 0);
        npSetLED(5 - posi, 0, 0, 0);
        break;
    case 1:
        npSetLED(25 - posi, 6, 1, 0);
        npSetLED(14 + posi, 6, 1, 0);
        npSetLED(15 - posi, 6, 1, 0);
        npSetLED(4 + posi, 6, 1, 0);
        npSetLED(5 - posi, 6, 1, 0);
        break;
    case 2:
        npSetLED(25 - posi, 1, 0, 0);
        npSetLED(14 + posi, 1, 0, 0);
        npSetLED(15 - posi, 1, 0, 0);
        npSetLED(4 + posi, 1, 0, 0);
        npSetLED(5 - posi, 1, 0, 0);
        break;
    case 3:
        npSetLED(25 - posi, 255, 118, 0);
        npSetLED(14 + posi, 255, 118, 0);
        npSetLED(15 - posi, 255, 118, 0);
        npSetLED(4 + posi, 255, 118, 0);
        npSetLED(5 - posi, 255, 118, 0);
        break;
    case 4:
        npSetLED(25 - posi, 1, 1, 0);
        npSetLED(14 + posi, 1, 1, 0);
        npSetLED(15 - posi, 1, 1, 0);
        npSetLED(4 + posi, 1, 1, 0);
        npSetLED(5 - posi, 1, 1, 0);
        break;
    case 5:
        npSetLED(25 - posi, 0, 1, 0);
        npSetLED(14 + posi, 0, 1, 0);
        npSetLED(15 - posi, 0, 1, 0);
        npSetLED(4 + posi, 0, 1, 0);
        npSetLED(5 - posi, 0, 1, 0);
        break;
    case 6:
        npSetLED(25 - posi, 0, 0, 1);
        npSetLED(14 + posi, 0, 0, 1);
        npSetLED(15 - posi, 0, 0, 1);
        npSetLED(4 + posi, 0, 0, 1);
        npSetLED(5 - posi, 0, 0, 1);
        break;
    case 7:
        npSetLED(25 - posi, 1, 0, 1);
        npSetLED(14 + posi, 1, 0, 1);
        npSetLED(15 - posi, 1, 0, 1);
        npSetLED(4 + posi, 1, 0, 1);
        npSetLED(5 - posi, 1, 0, 1);
        break;
    case 8:
        npSetLED(25 - posi, 31, 29, 33);
        npSetLED(14 + posi, 31, 29, 33);
        npSetLED(15 - posi, 31, 29, 33);
        npSetLED(4 + posi, 31, 29, 33);
        npSetLED(5 - posi, 31, 29, 33);
        break;
    case 9:
        npSetLED(25 - posi, 1, 1, 1);
        npSetLED(14 + posi, 1, 1, 1);
        npSetLED(15 - posi, 1, 1, 1);
        npSetLED(4 + posi, 1, 1, 1);
        npSetLED(5 - posi, 1, 1, 1);
        break;
    }
    npWrite();
};

#endif