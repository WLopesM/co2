# Fase 0 — Planejamento do medidor NDIR

**Projeto:** NDIR headspace
**Status:** planejamento
**Objetivo:** medir o CO₂ dissolvido de forma quantitativa e rápida (minutos), e **comparar os resultados** com o Drop Checker óptico (`../../projeto_drop_checker/`).

## 1. Princípio

A água e um pequeno volume de ar trocam gás até o equilíbrio (Lei de Henry). Um sensor NDIR mede o CO₂ nesse ar, e o valor na água sai por cálculo:

```
C (mg/L) = x · P · kH(T) · 44010

x      = leitura do NDIR (fração molar; 20.000 ppm = 0,020)
P      = pressão atmosférica (atm)
kH(T)  = 0,0339 · exp(2400 · (1/T − 1/298,15))   mol/(L·atm), T em kelvin
```

A 25 °C e 1 atm, **30 mg/L ≈ 20.100 ppm**. A seção 5 do relatório 00 traz a tabela com 15/30/45 mg/L de 22 a 28 °C. Por isso o sensor precisa de **faixa 0–5% (50.000 ppm)**.

Esse é o mesmo princípio dos medidores de pCO₂ usados em oceanografia (equilibrador + NDIR).

## 2. Arquitetura proposta (aproveitando o sump)

```
 bomba de retorno ──┬──► aquário
                    │ (derivação com registro)
                    ▼
          ┌──────────────────┐   ar ──► [coletor de condensado] ──► [NDIR na caixa aquecida] ─┐
          │  equilibrador    │◄── ar ◄── [microbomba de diafragma] ◄──────────────────────────┘
          │  (chuveiro)      │
          └───────┬──────────┘
                  ▼ dreno de volta ao sump
```

- **Equilibrador tipo chuveiro:** tubo vertical fechado (PVC ou acrílico, ~50 mm × 30 cm). Um fio de água da bomba de retorno cai em gotas através do ar interno e sai por baixo para o sump. O selo d'água no fundo mantém o ar preso. A área de troca é enorme, então o equilíbrio leva minutos, **sem membrana**.
- **Circuito de ar fechado:** microbomba de diafragma 5 V faz o ar circular entre o equilibrador e o sensor.
- **Umidade:** o ar sai saturado. Duas medidas protegem o sensor:
  1. **Coletor de condensado:** frasco baixo antes do sensor, onde a água condensada se acumula.
  2. **Caixa do sensor aquecida 3–5 °C acima da água:** resistor de potência ou tapete térmico, com termostato no Arduino. Sem superfície fria, não há condensação nas óticas.
- **Temperatura da água:** DS18B20 à prova d'água, no equilibrador.
- **Pressão:** BMP280 (I2C) na caixa do sensor.

## 3. Hardware (comprar)

| Item | Obs. | Ordem de preço |
|---|---|---|
| Sensor NDIR **0–5%** com saída UART | Confirmar com o vendedor a faixa de **50.000 ppm**. Opções: Winsen MH-Z16/MH-Z19 em versão de faixa estendida; Gas Sensing Solutions ExplorIR-W 5% | R$ 250–600 |
| Microcontrolador independente | Arduino Nano ou 2º Uno (recomendado: mantém o mesmo ferramental) | R$ 40–90 |
| DS18B20 à prova d'água + resistor 4,7 kΩ | Temperatura da água | R$ 15–25 |
| BMP280 | Pressão atmosférica | R$ 10–20 |
| Microbomba de ar de diafragma 5 V | Circulação | R$ 20–40 |
| Tubo, tampas, conexões e mangueira de silicone | Equilibrador | R$ 50–100 |
| Resistor de aquecimento + MOSFET | Caixa aquecida | R$ 15–30 |
| RTC DS3231 | Carimbo de hora para a comparação | R$ 15–25 |

## 4. Cuidados obrigatórios

1. **Desligar o ABC** (calibração automática de linha de base) via comando UART antes de qualquer medição. Com o ABC ligado, o sensor assume que o menor valor do dia é 400 ppm e corrompe a leitura.
2. **Estanqueidade do circuito de ar:** qualquer entrada de ar da sala dilui a amostra e dá leitura baixa. Teste com o circuito pressurizado e água com sabão.
3. **Sump × aquário:** a queda d'água do overflow libera CO₂, então o sump tende a ter **menos CO₂** que o aquário principal. Para comparar de forma justa, instale **um segundo Drop Checker (visual) no sump**, ao lado do equilibrador, e considere essa diferença ao comparar com o Drop Checker óptico do aquário.
4. Aquecimento controlado e desligado em falha: o resistor não pode ficar ligado sem controle.

## 5. Fases previstas

| Fase | Entrega | Relatório |
|---|---|---|
| 1 | Bancada do sensor: comunicação UART, ABC desligado, conferência no ar ambiente (~420 ppm) | `fase-01-bancada-sensor.md` |
| 2 | Equilibrador e circuito de ar: estanqueidade, tempo de resposta, controle de condensação | `fase-02-equilibrador.md` |
| 3 | Medição: conversão para mg/L com compensação de T e P, tela e registro | `fase-03-medicao.md` |
| 4 | **Comparação** NDIR × Drop Checker óptico × Drop Checker visual do sump × pH (sonda PH-4502C) | `fase-04-comparacao.md` |

## 6. Método de comparação (fase 4)

- Os dois sistemas registram CSV com **data e hora** (RTC em cada placa, ou ambos ligados por USB a um computador que carimba a hora).
- O período mínimo é **7 dias**, cobrindo o ciclo completo: CO₂ ligando e desligando, dia e noite.
- Métricas:
  - diferença média e dispersão entre os métodos (gráfico de Bland-Altman);
  - atraso de resposta: quanto tempo cada método leva para registrar a subida do CO₂ depois que a solenoide abre;
  - estabilidade ao longo dos dias.
- É esperado que o Drop Checker óptico mostre as mesmas tendências com **1–2 h de atraso**. O NDIR deve responder em minutos.
- **pH como terceiro método:** a sonda do projeto Drop Checker registra o pH no mesmo período. Dois cálculos entram na comparação:
  - fórmula pH/KH, que deve dar valores **acima** do NDIR por causa dos taninos; a diferença mede o tamanho desse viés neste aquário;
  - queda de pH (ΔpH), que deve acompanhar o NDIR em tempo e formato.
  Esse é o teste que confirma (ou não) o uso da queda de pH como alarme rápido.

## 7. Pendências

- [ ] Escolher o sensor NDIR (confirmar a faixa de 0–5% antes de comprar).
- [ ] Confirmar a vazão disponível na bomba de retorno para a derivação.
- [ ] Definir a placa: Nano ou 2º Uno.
