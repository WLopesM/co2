// Projeto Drop Checker óptico — Fase 1: bancada
//
// Lê a cor do Drop Checker com o TCS34725 e calcula o matiz (hue). Ao mesmo
// tempo, lê o pH da solução de calibração com o módulo PH-4502C, formando os
// pares matiz × pH da curva de calibração. NÃO aciona a solenoide de CO2.
//
// Cada medida é diferencial: uma leitura com o LED de iluminação apagado (luz
// ambiente e do aquário) é subtraída de outra com o LED aceso.
//
// Comandos pelo monitor serial (115200 baud):
//   b  registra o balanço de branco (Drop Checker com água sem reagente)
//   n  calibra o pH no tampão neutro; informe o valor: n6.86
//   a  calibra o pH no tampão ácido; informe o valor: a4.00
//   m  média de AMOSTRAS_MEDIA leituras (usar em cada ponto de calibração)
//   c  liga/desliga a saída CSV contínua
//   ?  ajuda

#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_TCS34725.h>
#include "config.h"

enum Estado : uint8_t { ESTADO_OK, ESTADO_SINAL_BAIXO, ESTADO_SATURADO };

struct Leitura {
  uint16_t r, g, b, c;
};

struct Medida {
  Leitura acesa, apagada;
  int32_t dr, dg, db, dc;  // acesa − apagada
  float rn, gn, bn;        // diferencial normalizado pelo branco
  float matiz;             // graus; NAN quando estado != ESTADO_OK
  float saturacao;         // 0–1; perto de 0 = cor acinzentada, matiz pouco confiável
  Estado estado;
  float phVolts;           // tensão na saída PO do módulo de pH
  float ph;                // NAN enquanto o pH não estiver calibrado
};

const uint16_t MAGICO_BRANCO = 0xC02B;

struct ReferenciaBranco {
  uint16_t magico;
  float r, g, b;
};

const uint16_t MAGICO_PH = 0xC0B7;
const int ENDERECO_EEPROM_BRANCO = 0;
const int ENDERECO_EEPROM_PH = 32;

// Dois pontos de calibração: tampão neutro (≈ 6,86) e ácido (≈ 4,00)
struct CalibracaoPh {
  uint16_t magico;
  float voltsNeutro, phNeutro;
  float voltsAcido, phAcido;
};

Adafruit_TCS34725 tcs(TCS_TEMPO_INTEGRACAO, TCS_GANHO);
Adafruit_ST7789 tft(PINO_TFT_CS, PINO_TFT_DC, PINO_TFT_RST);

ReferenciaBranco branco;
bool brancoCalibrado = false;
CalibracaoPh calPh;
bool phCalibrado = false;
bool saidaCsv = true;
uint32_t ultimaLeitura = 0;

// ---------------------------------------------------------------------------
// Medição
// ---------------------------------------------------------------------------

Leitura lerComLed(bool acesa) {
  digitalWrite(PINO_LED_ILUMINACAO, acesa ? HIGH : LOW);
  // A integração em curso no momento da troca mistura as duas condições.
  // Esperar dois ciclos garante que a última integração completa começou
  // depois da troca.
  delay(2 * TCS_TEMPO_INTEGRACAO_MS + 10);
  Leitura l;
  tcs.getRawData(&l.r, &l.g, &l.b, &l.c);
  return l;
}

float lerVoltsPh() {
  uint32_t soma = 0;
  for (uint8_t i = 0; i < PH_AMOSTRAS_ADC; i++) {
    soma += analogRead(PINO_PH);
  }
  return (soma / (float)PH_AMOSTRAS_ADC) * VREF_ADC / 1023.0;
}

float voltsParaPh(float volts) {
  if (!phCalibrado) return NAN;
  float inclinacao = (calPh.phAcido - calPh.phNeutro) / (calPh.voltsAcido - calPh.voltsNeutro);
  return calPh.phNeutro + (volts - calPh.voltsNeutro) * inclinacao;
}

float calcularMatiz(float r, float g, float b, float *saturacao) {
  float maximo = max(r, max(g, b));
  float minimo = min(r, min(g, b));
  float delta = maximo - minimo;
  *saturacao = (maximo > 0) ? delta / maximo : 0;
  if (delta <= 0) return NAN;

  float h;
  if (maximo == r)      h = 60.0 * fmod((g - b) / delta, 6.0);
  else if (maximo == g) h = 60.0 * ((b - r) / delta + 2.0);
  else                  h = 60.0 * ((r - g) / delta + 4.0);
  if (h < 0) h += 360.0;
  return h;
}

Medida medir() {
  Medida m;
  m.apagada = lerComLed(false);
  m.acesa = lerComLed(true);
  digitalWrite(PINO_LED_ILUMINACAO, LOW);

  m.dr = (int32_t)m.acesa.r - m.apagada.r;
  m.dg = (int32_t)m.acesa.g - m.apagada.g;
  m.db = (int32_t)m.acesa.b - m.apagada.b;
  m.dc = (int32_t)m.acesa.c - m.apagada.c;

  m.rn = max(m.dr, 0L) / branco.r;
  m.gn = max(m.dg, 0L) / branco.g;
  m.bn = max(m.db, 0L) / branco.b;

  if (m.acesa.c >= LIMITE_SATURACAO) m.estado = ESTADO_SATURADO;
  else if (m.dc < SINAL_MINIMO)      m.estado = ESTADO_SINAL_BAIXO;
  else                               m.estado = ESTADO_OK;

  float matiz = calcularMatiz(m.rn, m.gn, m.bn, &m.saturacao);
  m.matiz = (m.estado == ESTADO_OK) ? matiz : NAN;

  m.phVolts = lerVoltsPh();
  m.ph = voltsParaPh(m.phVolts);
  return m;
}

const __FlashStringHelper *nomeEstado(Estado e) {
  switch (e) {
    case ESTADO_OK:          return F("OK");
    case ESTADO_SINAL_BAIXO: return F("SINAL BAIXO");
    case ESTADO_SATURADO:    return F("SATURADO");
  }
  return F("?");
}

// ---------------------------------------------------------------------------
// Balanço de branco (EEPROM)
// ---------------------------------------------------------------------------

void carregarBranco() {
  EEPROM.get(ENDERECO_EEPROM_BRANCO, branco);
  brancoCalibrado = (branco.magico == MAGICO_BRANCO && branco.r > 0 && branco.g > 0 && branco.b > 0);
  if (!brancoCalibrado) {
    branco = {MAGICO_BRANCO, 1.0, 1.0, 1.0};
  }
}

void registrarBranco() {
  Serial.println(F("# Balanco de branco: mantenha o Drop Checker com agua sem reagente..."));
  float somaR = 0, somaG = 0, somaB = 0;
  uint8_t validas = 0;
  for (uint8_t i = 0; i < AMOSTRAS_MEDIA; i++) {
    Medida m = medir();
    if (m.estado != ESTADO_OK) continue;
    somaR += m.dr;
    somaG += m.dg;
    somaB += m.db;
    validas++;
  }
  if (validas < AMOSTRAS_MEDIA / 2 || somaR <= 0 || somaG <= 0 || somaB <= 0) {
    Serial.println(F("# ERRO: poucas leituras validas. Verifique ganho/posicao e repita."));
    return;
  }
  branco = {MAGICO_BRANCO, somaR / validas, somaG / validas, somaB / validas};
  EEPROM.put(ENDERECO_EEPROM_BRANCO, branco);
  brancoCalibrado = true;
  Serial.print(F("# Branco salvo: R="));
  Serial.print(branco.r, 1);
  Serial.print(F(" G="));
  Serial.print(branco.g, 1);
  Serial.print(F(" B="));
  Serial.println(branco.b, 1);
}

// ---------------------------------------------------------------------------
// Calibração do pH (EEPROM)
// ---------------------------------------------------------------------------

bool calibracaoPhValida(const CalibracaoPh &c) {
  if (c.magico != MAGICO_PH) return false;
  if (fabs(c.phNeutro - c.phAcido) < 1.0) return false;
  float inclinacao = (c.voltsAcido - c.voltsNeutro) / (c.phAcido - c.phNeutro);
  return fabs(inclinacao) >= PH_INCLINACAO_MINIMA;
}

void carregarCalibracaoPh() {
  EEPROM.get(ENDERECO_EEPROM_PH, calPh);
  phCalibrado = calibracaoPhValida(calPh);
  if (calPh.magico != MAGICO_PH) {
    calPh = {MAGICO_PH, NAN, NAN, NAN, NAN};
  }
}

// Registra um ponto de tampão. A calibração só passa a valer quando os dois
// pontos formam uma reta com inclinação plausível.
void registrarTampao(bool neutro) {
  float valor = Serial.parseFloat();
  if (valor < 1.0 || valor > 13.0) {
    Serial.println(F("# ERRO: informe o valor do tampao junto do comando, ex.: n6.86 ou a4.00"));
    return;
  }
  Serial.println(F("# Aguardando a sonda estabilizar (30 s)..."));
  delay(30000);

  float volts = 0;
  for (uint8_t i = 0; i < AMOSTRAS_MEDIA; i++) {
    volts += lerVoltsPh();
    delay(100);
  }
  volts /= AMOSTRAS_MEDIA;

  if (neutro) {
    calPh.voltsNeutro = volts;
    calPh.phNeutro = valor;
  } else {
    calPh.voltsAcido = volts;
    calPh.phAcido = valor;
  }
  calPh.magico = MAGICO_PH;
  EEPROM.put(ENDERECO_EEPROM_PH, calPh);

  Serial.print(neutro ? F("# Tampao neutro ") : F("# Tampao acido "));
  Serial.print(valor, 2);
  Serial.print(F(" = "));
  Serial.print(volts, 4);
  Serial.println(F(" V"));

  phCalibrado = calibracaoPhValida(calPh);
  if (isnan(calPh.voltsNeutro) || isnan(calPh.voltsAcido)) {
    Serial.println(F("# Falta o outro tampao para concluir a calibracao."));
  } else if (phCalibrado) {
    Serial.print(F("# pH calibrado. Inclinacao: "));
    Serial.print(1000.0 * (calPh.voltsAcido - calPh.voltsNeutro) / (calPh.phAcido - calPh.phNeutro), 1);
    Serial.println(F(" mV/pH"));
  } else {
    Serial.println(F("# ERRO: inclinacao muito baixa. Verifique a sonda (hidratada? conectada?) e repita."));
  }
}

// ---------------------------------------------------------------------------
// Média para pontos de calibração
// ---------------------------------------------------------------------------

void medirMedia() {
  Serial.println(F("# Media: aguarde..."));
  // Welford: média e desvio padrão do matiz sem guardar as amostras
  float media = 0, m2 = 0, somaR = 0, somaG = 0, somaB = 0, somaPh = 0;
  uint8_t n = 0;
  for (uint8_t i = 0; i < AMOSTRAS_MEDIA; i++) {
    Medida m = medir();
    if (m.estado != ESTADO_OK || isnan(m.matiz)) continue;
    n++;
    float d = m.matiz - media;
    media += d / n;
    m2 += d * (m.matiz - media);
    somaR += m.rn;
    somaG += m.gn;
    somaB += m.bn;
    somaPh += m.ph;  // NAN se o pH não estiver calibrado
  }
  if (n < 2) {
    Serial.println(F("# ERRO: menos de 2 leituras validas."));
    return;
  }
  Serial.print(F("# MEDIA n="));
  Serial.print(n);
  Serial.print(F(" matiz="));
  Serial.print(media, 2);
  Serial.print(F(" desvio="));
  Serial.print(sqrt(m2 / (n - 1)), 2);
  Serial.print(F(" rn="));
  Serial.print(somaR / n, 4);
  Serial.print(F(" gn="));
  Serial.print(somaG / n, 4);
  Serial.print(F(" bn="));
  Serial.print(somaB / n, 4);
  Serial.print(F(" pH="));
  if (phCalibrado) Serial.println(somaPh / n, 2);
  else Serial.println(F("nao_calibrado"));
}

// ---------------------------------------------------------------------------
// Saídas
// ---------------------------------------------------------------------------

void imprimirCabecalhoCsv() {
  Serial.println(F("ms,r_acesa,g_acesa,b_acesa,c_acesa,r_apagada,g_apagada,b_apagada,c_apagada,dr,dg,db,dc,matiz,saturacao,estado,ph_volts,ph"));
}

void imprimirCsv(const Medida &m) {
  const uint16_t valores[] = {m.acesa.r, m.acesa.g, m.acesa.b, m.acesa.c,
                              m.apagada.r, m.apagada.g, m.apagada.b, m.apagada.c};
  Serial.print(millis());
  for (uint16_t v : valores) {
    Serial.print(',');
    Serial.print(v);
  }
  const int32_t difs[] = {m.dr, m.dg, m.db, m.dc};
  for (int32_t v : difs) {
    Serial.print(',');
    Serial.print(v);
  }
  Serial.print(',');
  if (!isnan(m.matiz)) Serial.print(m.matiz, 2);
  Serial.print(',');
  Serial.print(m.saturacao, 3);
  Serial.print(',');
  Serial.print(nomeEstado(m.estado));
  Serial.print(',');
  Serial.print(m.phVolts, 4);
  Serial.print(',');
  if (!isnan(m.ph)) Serial.print(m.ph, 2);
  Serial.println();
}

// Posições da tela (retrato 240x320)
const int16_t Y_AMOSTRA = 50, ALT_AMOSTRA = 80;
const int16_t X_VALOR = 120, Y_LINHA0 = 140, ALT_LINHA = 24;

void desenharTelaFixa() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print(F("CO2 Drop Checker"));
  tft.setTextSize(1);
  tft.setCursor(10, 30);
  tft.print(F("Fase 1 - bancada (sem controle)"));

  tft.setTextSize(2);
  const char *rotulos[] = {"Matiz", "Satur.", "Sinal", "Ambiente", "pH", "Estado", "Calib."};
  for (uint8_t i = 0; i < 7; i++) {
    tft.setCursor(10, Y_LINHA0 + i * ALT_LINHA);
    tft.print(rotulos[i]);
  }
}

void escreverValor(uint8_t linha, uint16_t cor) {
  int16_t y = Y_LINHA0 + linha * ALT_LINHA;
  tft.fillRect(X_VALOR, y, TFT_LARGURA - X_VALOR, 16, ST77XX_BLACK);
  tft.setCursor(X_VALOR, y);
  tft.setTextColor(cor);
}

void atualizarTela(const Medida &m) {
  // Amostra da cor normalizada, escalada para o canal mais forte ficar em 255
  float maior = max(m.rn, max(m.gn, m.bn));
  uint16_t cor = ST77XX_BLACK;
  if (maior > 0) {
    cor = tft.color565(255 * m.rn / maior, 255 * m.gn / maior, 255 * m.bn / maior);
  }
  tft.fillRect(20, Y_AMOSTRA, TFT_LARGURA - 40, ALT_AMOSTRA, cor);
  tft.drawRect(19, Y_AMOSTRA - 1, TFT_LARGURA - 38, ALT_AMOSTRA + 2, ST77XX_WHITE);

  escreverValor(0, ST77XX_WHITE);
  if (isnan(m.matiz)) tft.print(F("--"));
  else tft.print(m.matiz, 1);

  escreverValor(1, ST77XX_WHITE);
  tft.print(m.saturacao, 2);

  escreverValor(2, ST77XX_WHITE);
  tft.print(m.dc);

  escreverValor(3, ST77XX_WHITE);
  tft.print(m.apagada.c);

  // Sem calibração, mostra a tensão da sonda (útil para ajustar o trimpot)
  if (phCalibrado) {
    escreverValor(4, ST77XX_WHITE);
    tft.print(m.ph, 2);
  } else {
    escreverValor(4, ST77XX_YELLOW);
    tft.print(m.phVolts, 3);
    tft.print('V');
  }

  escreverValor(5, m.estado == ESTADO_OK ? ST77XX_GREEN : ST77XX_RED);
  tft.print(nomeEstado(m.estado));

  // B = balanço de branco, P = pH; "--" = pendente
  escreverValor(6, (brancoCalibrado && phCalibrado) ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(F("B:"));
  tft.print(brancoCalibrado ? F("OK") : F("--"));
  tft.print(F(" P:"));
  tft.print(phCalibrado ? F("OK") : F("--"));
}

void imprimirAjuda() {
  Serial.println(F("# Comandos: b = balanco de branco | n6.86 / a4.00 = tampoes de pH | m = media para calibracao | c = CSV liga/desliga | ? = ajuda"));
}

void tratarSerial() {
  while (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'b': registrarBranco(); break;
      case 'n': registrarTampao(true); break;
      case 'a': registrarTampao(false); break;
      case 'm': medirMedia(); break;
      case 'c':
        saidaCsv = !saidaCsv;
        if (saidaCsv) imprimirCabecalhoCsv();
        break;
      case '?': imprimirAjuda(); break;
      default: break;  // ignora \r, \n e caracteres desconhecidos
    }
  }
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED_ILUMINACAO, OUTPUT);
  digitalWrite(PINO_LED_ILUMINACAO, LOW);

  tft.init(TFT_LARGURA, TFT_ALTURA);
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.setRotation(0);
  desenharTelaFixa();

  if (!tcs.begin()) {
    Serial.println(F("# ERRO: TCS34725 nao encontrado. Verifique SDA=A4, SCL=A5, VIN e GND."));
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, Y_AMOSTRA);
    tft.print(F("TCS34725 AUSENTE"));
    while (true) delay(1000);
  }

  carregarBranco();
  carregarCalibracaoPh();
  imprimirAjuda();
  if (!brancoCalibrado) {
    Serial.println(F("# Balanco de branco pendente: use o comando 'b'."));
  }
  if (!phCalibrado) {
    Serial.println(F("# pH nao calibrado: use n6.86 e a4.00 com a sonda nos tampoes."));
  }
  imprimirCabecalhoCsv();
}

void loop() {
  tratarSerial();
  if (millis() - ultimaLeitura < INTERVALO_LEITURA_MS) return;
  ultimaLeitura = millis();

  Medida m = medir();
  atualizarTela(m);
  if (saidaCsv) imprimirCsv(m);
}
