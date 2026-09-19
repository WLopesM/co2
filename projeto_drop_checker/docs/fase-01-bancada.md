# Fase 1 — Bancada: sensor e curva de calibração

**Projeto:** Drop Checker óptico
**Status:** em andamento (plano pronto, resultados pendentes)
**Firmware:** `../firmware/fase1_bancada/`

## 1. Objetivo

Provar, fora do aquário, que o TCS34725 distingue a cor do Drop Checker com resolução suficiente, e levantar a **curva matiz → pH** que permite converter a leitura em CO₂ (mg/L). O pH de cada ponto é medido pelo módulo PH-4502C, lido pelo mesmo Arduino:

`CO₂ = 12 × 10^(7 − pH)` (solução interna de 4 dKH)

Nesta fase **não há controle da solenoide**.

## 2. Hardware

### 2.1 Disponível

- Arduino Uno
- Módulo TCS34725
- LED branco 5 mm
- Display TFT 2,4" 240×320 ST7789 (SPI)
- Módulo de pH com conector BNC e sonda. Tratado aqui como **PH-4502C**, o modelo mais comum; conferir se a pinagem da placa bate com a seção 2.4.

### 2.2 Comprar para esta fase

| Item | Uso | Obs. |
|---|---|---|
| Drop Checker de vidro + solução de referência 4 dKH + azul de bromotimol | Objeto da medição | Refil comercial, ou 4 dKH de bicarbonato + indicador |
| Pós tampão de pH 4,00 e 6,86 (9,18 opcional) | Calibrar a sonda de pH | O tampão 6,86 cai bem no meio da faixa do Drop Checker |
| Solução de armazenamento KCl 3 M | Guardar a sonda | A sonda **não pode secar** |
| CD4050 (ou 74HC4050), ou 10 resistores (5 × 1 kΩ + 5 × 2 kΩ) | Converter os sinais de 5 V do Uno para 3,3 V do display | Ver 2.4 |
| Resistor de 150 Ω | LED de iluminação | ≈ 13 mA |
| Cartão ou plástico branco fosco | Fundo atrás do Drop Checker | |
| Protoboard e jumpers | | |

### 2.3 Por que o TCS34725 é melhor que o TCS3200 do estudo

O TCS34725 tem leitura digital de 16 bits por I2C, **filtro de infravermelho**, ganho e tempo de integração ajustáveis, e usa apenas 2 pinos. O TCS3200 media largura de pulso, o que foi a origem da confusão "frequência × período" no código legado.

### 2.4 Ligações

| Componente | Pino do componente | Arduino Uno |
|---|---|---|
| TCS34725 | VIN | 5V |
| | GND | GND |
| | SDA | A4 |
| | SCL | A5 |
| | LED | **GND** (apaga o LED da placa) |
| Módulo de pH PH-4502C | V+ | 5V |
| | G (o GND ao lado do V+) | GND |
| | PO | A0 |
| | TO, DO | não conectar (não usados nesta fase) |
| LED branco 5 mm | ânodo (perna longa) | D5, via resistor de 150 Ω |
| | cátodo | GND |
| TFT ST7789 | VCC | 3,3V (ver aviso) |
| | GND | GND |
| | SCL / SCK | D13 ⚠ via conversor |
| | SDA / MOSI | D11 ⚠ via conversor |
| | RES | D8 ⚠ via conversor |
| | DC | D9 ⚠ via conversor |
| | CS | D10 ⚠ via conversor |
| | BLK / BL | 3,3V |

⚠ **Display:** a maioria dos módulos ST7789 trabalha com lógica de **3,3 V**, e o Uno manda 5 V nos pinos. Ligar direto pode danificar o display. Use:
- **CD4050** alimentado em 3,3 V: entradas nos pinos do Uno, saídas no display. É a opção recomendada.
- Ou **divisores resistivos** em cada um dos 5 sinais: 1 kΩ em série vindo do Uno e 2 kΩ do sinal para o GND. Se a imagem sair corrompida, reduza `TFT_SPI_HZ` em `config.h`.

Se o módulo tiver regulador próprio (componente de 3 pinos marcado "662K" ou "LDO"), o VCC pode ir em 5V, mas os sinais continuam precisando de conversão.

**Módulo de pH:**
- **Ajuste do zero:** com a sonda no tampão 6,86, gire o trimpot de *offset* até a tela mostrar ~2,5 V. Assim a leitura fica centrada na faixa do ADC. Na maioria das placas, é o trimpot mais próximo do conector BNC; o outro ajusta o limiar da saída DO. Confirme girando e observando a tensão.
- **Mesma alimentação:** o Uno converte a tensão usando os 5 V como referência. Calibre e meça **com a mesma fonte**: se calibrar pelo USB do computador, meça pelo USB do computador.
- A leitura do pH aparece no display **em volts** até a calibração ser feita (`n` e `a`), e em pH depois disso.

**Pinos reservados para a fase 4:** D6 (MOSFET da solenoide), D7 (buzzer) e o RTC DS3231 no mesmo barramento I2C (A4/A5).

## 3. Montagem de bancada

```
  [LED 5 mm]  ↘ ~35°
                 [vidro]  [água]  (Drop Checker)  [fundo branco]
  [TCS34725]  →  │        ~~~~       (  ●  )           ▌
                 │
      (caixa escura cobrindo sensor + LED)
```

- Coloque o Drop Checker dentro de um pote de vidro com água. O vidro do pote simula o vidro do aquário; se possível, use espessura parecida.
- Instale o **fundo branco atrás do bulbo**: a luz atravessa o líquido, bate no branco e volta. Isso reforça a cor.
- Coloque o **LED inclinado (~30–45°)** e o sensor de frente. O reflexo do vidro sai no ângulo espelhado e não entra no sensor.
- Instale a caixa escura sobre o conjunto sensor + LED. A luz que vem pelo lado da água é compensada pela leitura diferencial.

## 4. Como o firmware mede

1. Faz uma leitura com o LED **apagado** (luz ambiente e do aquário).
2. Faz uma leitura com o LED **aceso**.
3. Subtrai as duas. O resultado é só a luz do LED que passou pelo líquido.
4. Normaliza pelo **balanço de branco** e calcula o **matiz** (0–360°). Azul fica em ~200°+, verde em ~120–160° e amarelo em ~50–60°.
5. Indica `SINAL BAIXO` quando o sinal diferencial é fraco e `SATURADO` quando o sensor estoura. Nesses casos, ajuste `TCS_GANHO` em `config.h`.

A troca do LED aguarda **dois ciclos de integração** antes de ler. Assim, nenhuma leitura mistura as duas condições.

## 5. Procedimento de calibração

Instale as bibliotecas pelo gerenciador da IDE Arduino: *Adafruit TCS34725*, *Adafruit ST7735 and ST7789 Library* e *Adafruit GFX Library*. Depois, carregue `fase1_bancada.ino` e abra o monitor serial em **115200 baud**.

0. **Calibração do pH:** enxágue a sonda com água destilada e mergulhe no tampão 6,86. Envie `n6.86` (o firmware espera 30 s para a sonda estabilizar). Enxágue, mergulhe no tampão 4,00 e envie `a4.00`. O monitor mostra a inclinação em mV/pH. Se ela sair baixa demais, o firmware recusa a calibração. Refaça no início de cada sessão de bancada.
1. **Balanço de branco:** encha o Drop Checker com **água destilada sem reagente**, monte na posição final e envie `b`. O valor fica salvo na EEPROM. Refaça sempre que mudar a geometria.
2. **Pontos de pH:** para cada alvo (7,2 – 7,0 – 6,8 – 6,6 – 6,4 – 6,2 – 6,0):
   1. Em um copo, pegue a solução de referência (4 dKH + indicador) e ajuste o pH: **gotas de vinagre** baixam o pH e **uma pitada de bicarbonato** sobe.
   2. Mantenha a **sonda de pH no copo** e acompanhe o pH no display enquanto ajusta.
   3. Encha o Drop Checker com a solução do copo, monte e envie `m` **dentro de 5 minutos**, com a sonda ainda no copo. O comando registra o matiz do Drop Checker e o pH do copo **na mesma medição**. Não demore: dentro do pote, o Drop Checker vai trocando CO₂ com a água e a cor deriva devagar.
   4. Anote o resultado na tabela da seção 6.
3. **Teste de luz ambiente:** com um ponto montado (ex.: pH 6,6), envie `m` com a luz da sala acesa e depois apagada. Faça o mesmo com uma lanterna apontada para o pote pelo lado da água.
4. **Teste de repetibilidade:** desmonte e remonte o Drop Checker no mesmo ponto e repita `m` três vezes.

A cor do indicador depende só do pH, e não de como ele foi alcançado. Por isso vinagre e bicarbonato servem para montar a curva.

**Limitação:** a sonda com o ADC de 10 bits do Uno tem precisão de ±0,05–0,1 de pH, e esse erro entra na curva. Para reduzir o impacto: calibre no mesmo dia, use vários pontos e mantenha o tampão 6,86 como âncora. Se não for suficiente, um ADC externo ADS1115 (16 bits, no mesmo I2C) resolve.

## 6. Resultados (preencher)

**Configuração usada:** ganho ___ · integração ___ ms · distância sensor–vidro ___ mm · espessura do vidro ___ mm

**Calibração do pH:** tampão neutro ___ V · tampão ácido ___ V · inclinação ___ mV/pH · temperatura ___ °C

| pH alvo | pH medido (sonda) | CO₂ equivalente (mg/L) | Matiz médio (°) | Desvio (°) | Sinal (dc) | Observação |
|---|---|---|---|---|---|---|
| 7,2 | | 8 | | | | |
| 7,0 | | 12 | | | | |
| 6,8 | | 19 | | | | |
| 6,6 | | 30 | | | | |
| 6,4 | | 48 | | | | |
| 6,2 | | 76 | | | | |
| 6,0 | | 120 | | | | |

| Teste | Resultado |
|---|---|
| Luz da sala acesa × apagada (Δ matiz) | |
| Lanterna pelo lado da água (Δ matiz) | |
| Repetibilidade após remontagem (Δ matiz) | |

## 7. Critérios de aceite

- [ ] A sonda de pH calibra com inclinação estável: repetir a calibração muda menos de 5%.
- [ ] O matiz varia de forma **monotônica** com o pH entre 6,0 e 7,2.
- [ ] A diferença de matiz entre pontos vizinhos (0,2 de pH) é **≥ 4× o desvio padrão**. Isso equivale a uma resolução de ~0,05 de pH, ou ~±12% no CO₂.
- [ ] Acender e apagar a luz da sala e a lanterna muda o matiz **menos que 1 desvio padrão**.
- [ ] A remontagem muda o matiz **menos que metade** da diferença entre pontos vizinhos.

## 8. Conclusão (preencher ao final)

_Pendente._
