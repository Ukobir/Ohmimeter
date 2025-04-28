#include "ssd1306.h"
#include "font.h"

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c)
{
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void ssd1306_config(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command)
{
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->port_buffer,
      2,
      false);
}

void ssd1306_send_data(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->ram_buffer,
      ssd->bufsize,
      false);
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value)
{
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

void ssd1306_fill(ssd1306_t *ssd, bool value)
{
  // Itera por todas as posições do display
  for (uint8_t y = 0; y < ssd->height; ++y)
  {
    for (uint8_t x = 0; x < ssd->width; ++x)
    {
      ssd1306_pixel(ssd, x, y, value);
    }
  }
}

void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill)
{
  for (uint8_t x = left; x < left + width; ++x)
  {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y)
  {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill)
  {
    for (uint8_t x = left + 1; x < left + width - 1; ++x)
    {
      for (uint8_t y = top + 1; y < top + height - 1; ++y)
      {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  while (true)
  {
    ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

    if (x0 == x1 && y0 == y1)
      break; // Termina quando alcança o ponto final

    int e2 = err * 2;

    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value)
{
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value)
{
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;

  // Verifica o caractere e calcula o índice correspondente na fonte
  if (c >= ' ' && c <= '~') // Verifica se o caractere está na faixa ASCII válida
  {
    index = (c - ' ') * 8; // Calcula o índice baseado na posição do caractere na tabela ASCII
  }
  else
  {
    // Caractere inválido, desenha um espaço (ou pode ser tratado de outra forma)
    index = 0; // Índice 0 corresponde ao caractere "nada" (espaço)
  }

  // Desenha o caractere na tela
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font[index + i]; // Acessa a linha correspondente do caractere na fonte
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j)); // Desenha cada pixel do caractere
    }
  }
}

// Função para desenhar uma string
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}

void initDisplay(ssd1306_t *ssd)
{
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                   // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                   // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                       // Pull up the data line
  gpio_pull_up(I2C_SCL);                                       // Pull up the clock line
  ssd1306_init(ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(ssd);                                         // Configura o display
  ssd1306_send_data(ssd);                                      // Envia os dados para o display
}

//  Atualiza o conteúdo do display com animações
void tela1(ssd1306_t *ssd, char ADC[5], char Rx[5])
{
  bool cor = 1;
  ssd1306_fill(ssd, !cor);                          // Limpa o display
  ssd1306_rect(ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
  ssd1306_line(ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
  ssd1306_line(ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
  ssd1306_draw_string(ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
  ssd1306_draw_string(ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
  ssd1306_draw_string(ssd, "  Ohmimetro", 10, 28);  // Desenha uma string
  ssd1306_draw_string(ssd, "ADC", 13, 41);          // Desenha uma string
  ssd1306_draw_string(ssd, "Resisten.", 50, 41);    // Desenha uma string
  ssd1306_line(ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
  ssd1306_draw_string(ssd, ADC, 8, 52);             // Desenha uma string
  ssd1306_draw_string(ssd, Rx, 59, 52);             // Desenha uma string
  ssd1306_send_data(ssd);                           // Atualiza o display
}

// Variável do código de cores
const char codigo[10][15] = {"Preto", "Marrom", "Vermelho", "Laranja",
                             "Amarelo", "Verde", "Azul", "Violeta",
                             "Cinza", "Branco"};

//  Atualiza o conteúdo do display com animações
void tela2(ssd1306_t *ssd, char res[5], int um, int dois, int tres)
{
  //Desenho feito a mão
  // ssd1306_line(ssd, 0, 5, 12, 5, true);
  // ssd1306_line(ssd, 14, 7, 14, 15, true);
  // ssd1306_line(ssd, 12, 17, 9, 17, true);
  // ssd1306_line(ssd, 10, 18, 10, 38, true);
  // ssd1306_line(ssd, 9, 39, 12, 39, true);
  // ssd1306_line(ssd, 14, 41, 14, 48, true);
  // ssd1306_line(ssd, 0, 50, 12, 50, true);
  // ssd1306_line(ssd, 6, 51, 6, 57, true);
  // ssd1306_line(ssd, 0, 59, 4, 59, true);
  // ssd1306_line(ssd, 2, 54, 2, 51, true);
  // ssd1306_line(ssd, 0, 55, 1, 55, true);
  // ssd1306_line(ssd, 2, 4, 2, 0, true);
  // ssd1306_line(ssd, 6, 0, 6, 4, true);
  // ssd1306_rect(ssd, 10, 0, 13, 3, true, true);
  // ssd1306_rect(ssd, 20, 0, 9, 3, true, true);
  // ssd1306_rect(ssd, 31, 0, 9, 3, true, true);
  // ssd1306_pixel(ssd, 12, 6, true);
  // ssd1306_pixel(ssd, 13, 6, true);
  // ssd1306_pixel(ssd, 13, 7, true);
  // ssd1306_pixel(ssd, 13, 15, true);
  // ssd1306_pixel(ssd, 13, 16, true);
  // ssd1306_pixel(ssd, 12, 16, true);
  // ssd1306_pixel(ssd, 12, 40, true);
  // ssd1306_pixel(ssd, 13, 40, true);
  // ssd1306_pixel(ssd, 13, 41, true);
  // ssd1306_pixel(ssd, 13, 48, true);
  // ssd1306_pixel(ssd, 13, 49, true);
  // ssd1306_pixel(ssd, 12, 49, true);
  // ssd1306_pixel(ssd, 5, 57, true);
  // ssd1306_pixel(ssd, 5, 58, true);
  // ssd1306_pixel(ssd, 4, 58, true);
  // ssd1306_pixel(ssd, 1, 54, true);
  // ssd1306_line(ssd, 22, 9, 23, 9, true);
  // ssd1306_line(ssd, 22, 12, 23, 12, true);
  // ssd1306_line(ssd, 22, 19, 23, 19, true);
  // ssd1306_line(ssd, 22, 22, 23, 22, true);
  // ssd1306_line(ssd, 22, 31, 23, 31, true);
  // ssd1306_line(ssd, 22, 34, 23, 34, true);
  // ssd1306_pixel(ssd, 22, 8, true);
  // ssd1306_pixel(ssd, 22, 13, true);
  // ssd1306_pixel(ssd, 22, 18, true);
  // ssd1306_pixel(ssd, 22, 23, true);
  // ssd1306_pixel(ssd, 22, 30, true);
  // ssd1306_pixel(ssd, 22, 35, true);
  // ssd1306_line(ssd, 19, 10, 24, 10, true);
  // ssd1306_line(ssd, 19, 11, 24, 11, true);
  // ssd1306_line(ssd, 19, 20, 24, 20, true);
  // ssd1306_line(ssd, 19, 21, 24, 21, true);
  // ssd1306_line(ssd, 19, 32, 24, 32, true);
  // ssd1306_line(ssd, 19, 33, 24, 33, true);

  
 // Desenho feito ao exportar o arquivo no Piskelapp automatizado
  for (int j = 0; j < 64; j++)
  {
    for (int i = 0; i < 128; i++)
    {
      int a = j * 128 + i;
      if (new_piskel_data[a] >= 0xff000000){
        ssd1306_pixel(ssd, i, j, true);
      }
    }
  }


  ssd1306_draw_string(ssd, codigo[um], 31, 6);    // Desenha uma string
  ssd1306_draw_string(ssd, codigo[dois], 31, 16); // Desenha uma string
  ssd1306_draw_string(ssd, codigo[tres], 31, 30); // Desenha uma string
  ssd1306_draw_string(ssd, "Comercial", 50, 47);  // Desenha uma string
  ssd1306_draw_string(ssd, res, 59, 55);          // Desenha uma string
  ssd1306_send_data(ssd);                         // Atualiza o display
}

void desenho()
{


}
