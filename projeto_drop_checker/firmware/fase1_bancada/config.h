#pragma once

// ---------------------------------------------------------------------------
// Pinagem (Arduino Uno)
// ---------------------------------------------------------------------------
// TCS34725: I2C em A4 (SDA) e A5 (SCL). O pino LED do módulo vai ao GND para
// manter o LED da placa apagado; a iluminação é feita pelo LED de 5 mm abaixo.
const uint8_t PINO_LED_ILUMINACAO = 5;  // LED branco 5 mm, com resistor de 150 ohm

// TFT ST7789 (SPI por hardware: SCK = D13, MOSI = D11)
const uint8_t PINO_TFT_CS  = 10;
const uint8_t PINO_TFT_DC  = 9;
const uint8_t PINO_TFT_RST = 8;
const uint16_t TFT_LARGURA = 240;
const uint16_t TFT_ALTURA  = 320;
// Reduza (ex.: 4000000) se a imagem sair corrompida usando divisores resistivos.
const uint32_t TFT_SPI_HZ = 8000000;

// ---------------------------------------------------------------------------
// Sensor de cor
// ---------------------------------------------------------------------------
#define TCS_TEMPO_INTEGRACAO TCS34725_INTEGRATIONTIME_154MS
const uint16_t TCS_TEMPO_INTEGRACAO_MS = 154;  // deve corresponder à linha acima
#define TCS_GANHO TCS34725_GAIN_4X

// Canal Clear diferencial (LED ligado − desligado) abaixo disso: sinal fraco
// demais para confiar no matiz. Suba o ganho ou aproxime o LED.
const uint16_t SINAL_MINIMO = 300;
// Canal Clear com LED ligado acima disso: sensor saturado. Baixe o ganho.
const uint16_t LIMITE_SATURACAO = 60000;

// ---------------------------------------------------------------------------
// Módulo de pH (PH-4502C, conector BNC)
// ---------------------------------------------------------------------------
// PO (saída analógica de pH) em A0. TO e DO não são usados nesta fase.
const uint8_t PINO_PH = A0;
const uint8_t PH_AMOSTRAS_ADC = 64;  // leituras do ADC por medida (média)
// A conversão usa 5,0 V nominais. O pH só é confiável se a calibração e o uso
// forem feitos com a mesma fonte de alimentação (não trocar USB por fonte).
const float VREF_ADC = 5.0;
// Inclinação mínima aceitável na calibração, em volts por unidade de pH.
// Abaixo disso a sonda está gasta, seca ou mal conectada.
const float PH_INCLINACAO_MINIMA = 0.05;

// ---------------------------------------------------------------------------
// Operação
// ---------------------------------------------------------------------------
const uint32_t INTERVALO_LEITURA_MS = 2000;
const uint8_t AMOSTRAS_MEDIA = 10;  // leituras por comando 'm' e 'b'
