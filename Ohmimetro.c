/*
 * Por: Leonardo Romão e Wilton Lacerda Silva
 *    Ohmímetro utilizando o ADC da BitDogLab
 *
 *
 * Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 * desconhecido, utilizando um divisor de tensão com dois resistores.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 * Se apertarmos o botão A o aparelho mostra no display o valor comercial
 * e seu código de cores
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/ws2812.h"

#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A

#define LED_PIN 7

int R_conhecido = 9980;    // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)
float c_angular = 1.035;

ssd1306_t ssd;
bool flag_tela = false; // Flag da tela
// Variáveis utilizada para o debouncing
static volatile uint32_t passado = 0;
uint32_t tempo_atual;

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  tempo_atual = to_ms_since_boot(get_absolute_time());
  if (tempo_atual - passado > 2e2)
  {
    passado = tempo_atual;
    if (gpio == botaoB)
    {
      reset_usb_boot(0, 0);
    }
    else if (gpio == Botao_A)
    {
      flag_tela = !flag_tela;
    }
  }
}
// Declaração das funções para converter a resistência e adequala ao código de cores
int multiplicador(float Rx);
void resto(int i, int *pri, int *segu, int *terc);
void definido(int valor, int posi);

int main()
{
  // Inicializa o sistema.
  stdio_init_all();

  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  // Aqui termina o trecho para modo BOOTSEL com botão B

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // I2C Initialisation. Using it at 400Khz.
  initDisplay(&ssd);

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string
  char str_z[5]; // Buffer para armazenar a string

  // Inicializa matriz de LEDs NeoPixel.
  npInit(LED_PIN);
  npClear(); // Garante que todos os LEDs comecem apagados.
  npWrite();

  int linha1, linha2, linha3;

  bool ca = flag_tela;  // Variável para definir o circuito aberto
  bool one_time = true; // Variável para ligar o led apenas uma vez a fim de evitar leitura errada (com queda de tensão)
  int valor;            // Valor que será armazenado a posição da resistência comercial

  while (true)
  {

    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = c_angular * ((float)R_conhecido * media) / ((float)ADC_RESOLUTION - media);

    // Comparação para não exibir resistências acima de 100k ou abaixo de 510
    if (R_x > 10e4 || R_x < 510)
    {
      flag_tela = false;
      sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
      memcpy(str_y, "ERRO", 5);
    }
    else
    {
      sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
      sprintf(str_y, "%1.0f", R_x); // Converte o float em string
    }

    // Toda vez que aperta o botão A limpa o display e LEDs
    if (ca != flag_tela)
    {
      ca = flag_tela;
      // Limpa o display e desliga a matrix de leds para não influenciar os valores.
      npClear();
      npWrite();
      ssd1306_fill(&ssd, false);
      ssd1306_send_data(&ssd);
    }
    // Tela 1 é o frame que o Professor Wilton fez

    if (!flag_tela)
    {

      tela1(&ssd, str_x, str_y);
      valor = multiplicador(R_x);
      sprintf(str_z, "%d", res_val[valor]); // Converte o float em string

      one_time = true;
    }

    else
    {
      if (one_time)
      {
        resto(valor, &linha1, &linha2, &linha3);
        one_time = false;
      }

      printf("%d  %d   %d\n", linha1, linha2, linha3);
      definido(linha1, 1);
      definido(linha2, 3);
      definido(linha3, 5);
      tela2(&ssd, str_z, linha1, linha2, linha3);
    }

    sleep_ms(8);
  }
}

// Função para converter a resistência desconhecida em uma comercial.
int multiplicador(float Rx)
{

  uint32_t menorVal;
  uint32_t maiorVal;

  for (int i = 1; i < 55; i++)
  {
    if (Rx < res_val[i])
    {
      maiorVal = res_val[i];
      menorVal = res_val[i - 1];
      maiorVal = maiorVal - Rx;
      menorVal = Rx - menorVal;
      if (maiorVal > menorVal)
      {
        return (i - 1);
      }

      else
      {
        return i;
      }
    }
  }
};

// Função para separar a resistência comercial em unidades
void resto(int i, int *pri, int *segu, int *terc)
{

  int x;
  int b;
  if (i < 7)
  {
    *terc = 1;
    x = res_val[i] / 10;
  }
  else if (i > 6 && i < 31)
  {
    *terc = 2;
    x = res_val[i] / 100;
  }
  else
  {
    *terc = 3;
    x = res_val[i] / 1000;
  }

  b = x % 10;
  *segu = b;
  *pri = (x - b % 10) / 10.0;
}
