# Projeto Drop Checker óptico

Monitor e trava de segurança de CO₂ para aquário de água doce de 1000 L. Um sensor TCS34725 lê, pelo lado de fora do vidro, a cor do Drop Checker e converte o resultado em CO₂ (mg/L).

A escolha desta abordagem está justificada em [`../docs/relatorio-00-analise-de-viabilidade.md`](../docs/relatorio-00-analise-de-viabilidade.md).

## Hardware

| Item | Status |
|---|---|
| Arduino Uno | disponível |
| Sensor de cor TCS34725 (I2C) | disponível |
| LED branco 5 mm (iluminação) | disponível |
| TFT 2,4" 240×320 ST7789 (SPI) | disponível |
| Módulo de pH BNC (PH-4502C) + sonda | disponível |
| Conversor de nível CD4050 (5 V → 3,3 V para o TFT) | comprar (fase 1) |
| Drop Checker + solução 4 dKH, tampões de pH 4,00/6,86, KCl 3 M | comprar (fase 1) |
| Isolador de sinal analógico para o pH | comprar (fase 3) |
| RTC DS3231 | comprar (fase 4) |
| Módulo MOSFET *logic-level* + diodo 1N4007 + solenoide 12 V | comprar (fase 4) |
| Fonte 12 V + regulador *buck* 12→5 V, buzzer | comprar (fase 4) |

## Papel do módulo de pH

1. **Fase 1:** instrumento de calibração. Mede o pH de cada ponto da curva matiz → pH ao mesmo tempo que o sensor lê a cor.
2. **Fases 3 e 4:** alarme rápido pelo método da **queda de pH (ΔpH)**. A sonda fica no aquário e a referência é o pH de uma amostra da água do próprio aquário deixada aerando até perder o CO₂ extra. Uma queda maior que o limite corta o CO₂ **em minutos**, enquanto o Drop Checker leva 1–2 h. Por ser uma medida relativa, os taninos entram nas duas leituras e se cancelam.

Com a sonda dentro do aquário, bombas e aquecedores induzem tensões que desviam a leitura. Por isso a fase 3 prevê um isolador de sinal.

Para a solenoide, use um MOSFET de nível lógico (IRLZ44N, AOD4184). **Evite os módulos IRF520**: com 5 V no gate, eles não conduzem totalmente.

## Fases

| Fase | Entrega | Relatório | Status |
|---|---|---|---|
| 1 | Bancada: leitura diferencial, balanço de branco, curva matiz → pH | [fase-01-bancada.md](docs/fase-01-bancada.md) | em andamento |
| 2 | Montagem no aquário: suporte opaco, fundo branco, LED inclinado | `docs/fase-02-montagem.md` | — |
| 3 | Monitoramento sem atuação: CO₂ (Drop Checker) e ΔpH (sonda) na tela, registro por 1–2 semanas | `docs/fase-03-monitoramento.md` | — |
| 4 | Controle seguro: RTC, fotoperíodo, fail-safe, watchdog, alarme e corte rápido por ΔpH | `docs/fase-04-controle-seguro.md` | — |

## Firmware

Cada fase tem seu próprio sketch em `firmware/`. O sketch abre direto na IDE Arduino.

- `firmware/fase1_bancada/`: leitura e calibração. Não aciona a solenoide.

Bibliotecas (gerenciador da IDE): **Adafruit TCS34725**, **Adafruit ST7735 and ST7789 Library**, **Adafruit GFX Library**.
