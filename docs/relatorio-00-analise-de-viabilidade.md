# Relatório 00 — Análise de Viabilidade

**Projeto:** Medição e controle de CO₂ dissolvido em aquário de água doce (1000 L)
**Data:** 19/09/2026
**Base analisada:** `docs/projeto.odt` (estudo), `legado/controle_co2.cpp`, `legado/processo_titulacao.cpp`

---

## 1. Objetivo e escopo

Este relatório avalia as duas opções propostas no estudo para medir o CO₂ dissolvido com Arduino:

- **Opção A:** titulador automático de KH (bombas peristálticas + sensor de cor TCS3200), usado junto com a fórmula pH/KH.
- **Opção B:** leitor óptico do Drop Checker (TCS3200 lendo a cor do Drop Checker pelo lado de fora do vidro), com corte da solenoide de CO₂.

O estudo também desenvolve uma terceira via, o **sensor NDIR com câmara de gás (headspace)**. Ela foi avaliada na seção 5 como alternativa e possível evolução.

Ao final (seção 9) está a recomendação de qual opção seguir.

---

## 2. Fundamentos: conferência do estudo

### 2.1 Fórmula pH/KH

`CO₂ (mg/L) = 3 × KH(dKH) × 10^(7 − pH)`

- A fórmula está correta e a tabela do estudo confere (ex.: KH 4 e pH 6,8 dão 19 mg/L).
- A constante "3" embute o pKa₁ do ácido carbônico (≈ 6,3 a 25 °C). Por isso ela só vale na faixa tropical, como o próprio estudo aponta.
- **A fórmula amplifica muito o erro do pH.** Um erro de ±0,1 no pH vira um erro de ±26% no CO₂ (10^0,1 = 1,26). Com 30 mg/L reais, um pHmetro com erro de ±0,1 indica algo entre 24 e 38 mg/L. Sondas de pH para Arduino (PH-4502C e similares) derivam ±0,1 em poucas semanas sem recalibração.

### 2.2 O problema dos taninos já existe neste aquário

O diagrama do Drop Checker no estudo indica **"Água do Aquário (com taninos dos galhos)"**. Então a limitação nº 1 da fórmula (ácidos orgânicos baixando o pH sem aumento de CO₂) **não é hipotética neste aquário, ela já está presente**. Qualquer método baseado em pH da água do aquário vai superestimar o CO₂.

### 2.3 Drop Checker

O princípio descrito está correto: só o gás atravessa a bolha de ar, então a leitura fica imune a taninos, fosfatos e KH do aquário. Com a solução-padrão de 4 dKH, a relação entre o pH interno e o CO₂ é:

| pH interno | 7,0 | 6,8 | 6,6 | 6,4 | 6,2 | 6,0 |
|---|---|---|---|---|---|---|
| CO₂ (mg/L) | 12 | 19 | **30** | 48 | 76 | 120 |
| Cor (azul de bromotimol) | azul | azul-esverdeado | verde | verde-amarelado | amarelo | amarelo |

### 2.4 Correções e observações ao estudo

1. **"Amarelo = pH abaixo de 6,0".** A 4 dKH, pH 6,0 corresponde a 120 mg/L, um nível letal. O amarelo começa a aparecer por volta de pH 6,3–6,4 (≈ 50–60 mg/L). O alarme deve disparar no **verde-amarelado**, não no amarelo pleno.
2. **Faixa segura.** A tabela chama de "perigoso" tudo acima de 30 mg/L, mas o Drop Checker "ideal" fica em ~30 mg/L. Na prática, 25–35 mg/L é o alvo comum. O limite de risco depende das espécies, do oxigênio e da agitação da superfície, e costuma ficar entre 40 e 50 mg/L.
3. **Membrana de PTFE.** O estudo diz que ela "impede totalmente o vapor de água". Isso não é verdade: o PTFE expandido barra água **líquida**, mas deixa passar **vapor** (é o princípio do Gore-Tex). O ar do circuito NDIR continua com ~100% de umidade (ver seção 5).
4. **Arquivo ausente.** O estudo cita `ciclos.cpp` (código do NDIR), que não está na pasta do projeto.

---

## 3. Opção A — Titulador automático de KH

### 3.1 O que ela mede de fato

A opção A **mede KH, não CO₂.** Para chegar ao CO₂ ainda é preciso medir o pH continuamente com uma sonda. Na prática, a opção A completa é **titulador de KH + sonda de pH + fórmula**, e herda todas as limitações da seção 2 (taninos e sensibilidade ao pH).

Além disso, o KH de um aquário estável muda pouco (semanas). Automatizar esse teste traz pouco ganho em relação ao teste manual de gotas, que leva 2 minutos.

### 3.2 Arquitetura

- 3 bombas peristálticas 12 V (amostra, reagente, descarte) + driver (L298N ou MOSFETs com diodo de roda livre)
- Câmara de teste opaca, aerador para misturar, TCS3200
- Reagente de KH (consumível) e descarte para o ralo
- Sonda de pH com placa de interface (necessária para o CO₂)

### 3.3 Pontos fortes

- Automatiza um teste que é tradicional e bem conhecido.
- O KH medido é útil para o controle geral da química da água.
- A sonda de pH responde em minutos.

### 3.4 Pontos fracos e riscos

- **Não resolve o problema principal:** o CO₂ calculado continua falseado pelos taninos.
- **Fluídica complexa:** três bombas, mangueiras e um reservatório perto da água e da eletrônica. Um vazamento ou uma sifonagem pode esvaziar água do aquário para o ralo.
- **Precisão volumétrica ruim:** o volume da amostra e da "gota" é definido por tempo de bomba (`delay(5000)` e `delay(150)`). Peristálticas DC variam com tensão, temperatura e desgaste da mangueira.
- **Resolução baixa:** 1 gota = 1 dKH. Com KH 4, o erro de ±0,5 dKH já é ±12,5% no CO₂, somado ao erro do pH.
- Reagente consumível e descarte de química.

### 3.5 Análise do código `legado/processo_titulacao.cpp`

| # | Linha | Problema | Sugestão |
|---|---|---|---|
| 1 | 53 | O critério de virada `r < b && r < g` exige vermelho mais forte que verde, mas amarelo tem vermelho **e** verde fortes. O texto do estudo usa outro critério (`R > B * 1.5`), com sentido invertido, porque `pulseIn` mede período e não frequência. | Calibrar o balanço de branco e usar a razão R/B ou o matiz (hue) com um limiar obtido em bancada. |
| 2 | 31–36 | O volume da amostra é definido por tempo. | Usar uma câmara com **ladrão (overflow)**: enche até transbordar e o volume fica fixo pela geometria. |
| 3 | 42–44 | Uma "gota" de 150 ms não é reprodutível. | Usar uma peristáltica com **motor de passo**, onde a dose é definida por número de passos. Assim é possível dosar frações de gota e melhorar a resolução. |
| 4 | — | Não há o enxágue inicial nem a leitura de referência descritos no fluxograma. O aerador não é controlado. | Implementar as etapas conforme o fluxograma. |
| 5 | 60–63 | Ao abortar, o código descarta a amostra, mas não registra o erro de forma distinguível para quem estiver monitorando. | Retornar um código de erro e sinalizar com LED ou alarme. |

### 3.6 Custo estimado (ordem de grandeza, confirmar preços)

R$ 400–800, com as bombas, o driver, a câmara, o TCS3200, a sonda de pH e o reagente. Soma-se o custo recorrente do reagente e da recalibração da sonda de pH.

---

## 4. Opção B — Leitor óptico do Drop Checker

### 4.1 O que ela mede

A opção B mede **o CO₂ através de um intermediário que é imune a taninos**. É exatamente o problema que este aquário tem. O sensor não tem contato com a água e não há fluídica.

### 4.2 Arquitetura

- Drop Checker com solução-padrão de 4 dKH + azul de bromotimol
- TCS3200 em suporte opaco (impressão 3D) do lado de fora do vidro
- Relé da solenoide de CO₂ + RTC (DS3231) para o fotoperíodo
- Microcontrolador: Arduino Uno/Nano ou **ESP32**. O ESP32 é recomendado porque permite registro de dados e alerta no celular via Wi-Fi.

### 4.3 Pontos fortes

- Imune a taninos, fosfatos e variação de KH.
- Barata e simples: sem bombas, sem reagente consumível (além do refil do Drop Checker a cada 2–4 semanas) e sem risco de vazamento.
- Rápida de prototipar. Cada etapa pode ser validada visualmente, porque o olho humano confere o sensor.
- **Pode ir além de 3 cores:** calibrando a curva matiz → pH (seção 4.5), o leitor dá um valor contínuo de CO₂ em mg/L, não apenas azul/verde/amarelo.

### 4.4 Pontos fracos e riscos

- **Resposta lenta (1–2 h).** Isso a torna inadequada como controle liga/desliga fino: a solenoide oscilaria. Ela funciona bem como **supervisor e trava de segurança**, com a injeção guiada por fotoperíodo. Num aquário de 1000 L a dinâmica do próprio CO₂ também é lenta (horas), o que atenua esse problema.
- **Óptica através de dois vidros.** O TCS3200 é um sensor de reflexão. O reflexo especular do vidro do aquário e do bulbo pode dominar a leitura.
- **Luz do próprio aquário.** O suporte externo bloqueia a luz da sala, mas a iluminação do aquário chega ao sensor **por dentro**, através do Drop Checker. O estudo não trata disso.
- Algas no vidro e no bulbo alteram a cor com o tempo.
- Se a bolha de ar do Drop Checker se perder, a água do aquário entra e a leitura perde o sentido.

Mitigações:

| Problema | Mitigação |
|---|---|
| Reflexo nos vidros | Colocar um **fundo branco fosco atrás do Drop Checker**, dentro do aquário. A luz atravessa o líquido duas vezes, o que aumenta o contraste. Inclinar levemente o sensor evita o reflexo direto. |
| Luz do aquário | **Leitura diferencial:** ler com os LEDs do sensor ligados e depois desligados, e subtrair. Isso cancela a luz ambiente. Exige um módulo com pino de controle dos LEDs (LED/OE). |
| Algas e deriva | Recalibrar o branco periodicamente. Detectar a deriva comparando com a leitura logo após a limpeza. |
| Bolha perdida | Detectar saturação ou cor fora da curva e sinalizar "Drop Checker inválido", com CO₂ desligado. |

### 4.5 Como obter CO₂ em mg/L (e não só 3 cores)

1. Preparar **soluções de referência**: tampões de pH 6,0 / 6,2 / 6,4 / 6,6 / 6,8 / 7,0 / 7,2 com a mesma concentração de azul de bromotimol do Drop Checker.
2. Colocá-las no mesmo Drop Checker, na mesma montagem, e registrar o RGB normalizado e o matiz.
3. Ajustar a curva matiz → pH e aplicar `CO₂ = 12 × 10^(7 − pH)` (fórmula com KH fixo em 4 dKH, válida aqui porque a solução interna é controlada).

Os tampões de pH são baratos e reutilizáveis, e esse procedimento vira o protocolo de recalibração.

### 4.6 Análise do código `legado/controle_co2.cpp`

| # | Linha | Problema | Gravidade |
|---|---|---|---|
| 1 | 39–53 | **Falha do sensor liga o CO₂.** Se o TCS3200 desconectar, `pulseIn` retorna 0 nos três canais, nenhuma das duas condições é verdadeira e o código cai no `else` ("CO₂ ideal"), que mantém a solenoide **aberta**. | **Crítica** |
| 2 | — | **Sem fotoperíodo.** O CO₂ fica ligado a noite toda. Sem fotossíntese, o CO₂ acumula e, com 1–2 h de atraso do Drop Checker, pode matar os peixes antes do alarme. Essa é a causa clássica de mortandade em aquários com CO₂. | **Crítica** |
| 3 | 29 | O CO₂ inicia **ligado** no boot. Muitos módulos de relé para Arduino são **ativos em LOW**, e nesse caso toda a lógica fica invertida. | Alta |
| 4 | 39–48 | Compara os canais brutos sem balanço de branco. Os fotodiodos R, G e B do TCS3200 têm sensibilidades diferentes, então "qual canal é menor" não corresponde à cor real. | Alta |
| 5 | — | Sem histerese e sem limite de tempo máximo de solenoide aberta. | Média |
| 6 | 56 | `delay(300000)` bloqueia tudo por 5 min: nada de alarme, botão ou watchdog nesse intervalo. | Média |
| 7 | 12–14 | As variáveis se chamam `*Frequency`, mas guardam período em µs. Isso confunde, e o texto do estudo já inverteu um critério por causa disso. | Baixa |

### 4.7 Custo estimado (ordem de grandeza, confirmar preços)

R$ 150–300: Drop Checker, TCS3200, ESP32 ou Arduino, relé, RTC, suporte impresso e tampões de pH. Não inclui o cilindro, o regulador e a solenoide, que já fazem parte do sistema de CO₂.

---

## 5. Alternativa do estudo: sensor NDIR com câmara de gás

### 5.1 Avaliação

O princípio é sólido e é usado em oceanografia: medidores de pCO₂ em navios usam um **equilibrador água-ar + NDIR em circuito fechado**. É a única das três vias que mede o gás diretamente, com resposta em minutos e sem depender de reagentes.

A conversão pode ser feita **por cálculo, pela Lei de Henry**, sem depender só do Drop Checker para calibrar:

| CO₂ na água | 22 °C | 25 °C | 28 °C |
|---|---|---|---|
| 15 mg/L | 9.300 ppm | 10.100 ppm | 10.900 ppm |
| 30 mg/L | 18.500 ppm | 20.100 ppm | 21.800 ppm |
| 45 mg/L | 27.800 ppm | 30.200 ppm | 32.700 ppm |

A tabela confirma a conclusão do estudo: é **obrigatório** um sensor de 0–5% (50.000 ppm). Ela mostra também que é preciso **compensar a temperatura** (≈ 3% por °C), com um sensor DS18B20 na água.

### 5.2 Pontos a corrigir no desenho do estudo

- **Umidade:** como a membrana de PTFE deixa passar vapor, a sílica gel num circuito fechado vai saturar rápido, porque está secando um ar que se reumidifica continuamente. A solução mais robusta é manter a **caixa do sensor alguns graus acima da temperatura da água** (sem condensação não há dano) e usar um coletor de condensado. A sílica fica como opcional.
- **Calibração automática (ABC):** os MH-Z19/MH-Z16 vêm com a calibração automática de linha de base **ligada**. Ela assume que o menor valor do dia é 400 ppm, o que corrompe as medições nesta aplicação. É obrigatório desligá-la via comando UART.
- **Diluição com ar ambiente** (o "ajuste via software" do estudo): descartar. A razão de diluição por agulha não é estável.
- **Pressão atmosférica:** o sensor lê a fração molar. Em altitude (ex.: 800 m, ≈ 920 hPa), o erro é de ~9% se não houver correção.

### 5.3 Custo e risco

R$ 400–800. Sensores de 0–5% (MH-Z16 na versão de 5%, ExplorIR-W, SprintIR) costumam ser importados. O **risco técnico é o maior das três vias**, por causa da umidade, da vedação e da estanqueidade do circuito de ar.

---

## 6. Matriz comparativa

Notas de 1 (pior) a 5 (melhor), com a ponderação refletindo as prioridades deste aquário: taninos presentes, 1000 L de volume e peixes em risco.

| Critério | Peso | A: KH + pH | B: Drop Checker óptico | NDIR headspace |
|---|---|---|---|---|
| Mede CO₂ de forma específica | 20% | 2 | 4 | 5 |
| Imunidade a taninos/ácidos | 15% | 1 | 5 | 5 |
| Simplicidade de montagem | 15% | 1 | 4 | 2 |
| Risco técnico (baixo = melhor) | 15% | 2 | 4 | 2 |
| Custo | 10% | 2 | 5 | 2 |
| Manutenção e consumíveis | 10% | 2 | 4 | 3 |
| Tempo de resposta | 15% | 3 | 2 | 5 |
| **Nota ponderada** | | **1,85** | **3,95** | **3,60** |

---

## 7. Requisitos de segurança (valem para qualquer opção)

1. **Fotoperíodo por RTC:** o CO₂ liga 1–2 h antes das luzes e desliga 1 h antes de apagá-las. **Nunca injetar à noite.** O sensor apenas corta o CO₂ antes do horário, nunca estende a injeção.
2. **Falha segura (fail-safe):** qualquer falha (sensor sem leitura, valor fora de faixa, microcontrolador travado ou sem energia) deve resultar em **CO₂ desligado**. Para isso: solenoide normalmente fechada, contato NA do relé, watchdog ativo e verificação da polaridade do módulo de relé.
3. **Tempo máximo de injeção por dia**, independente do sensor.
4. **Alarme** sonoro e, com ESP32, notificação no celular. Como ação de emergência, ligar uma bomba de ar/aeração.
5. **Solenoide de 220 V:** o relé chaveia a rede elétrica perto da água. Usar caixa fechada e DR (disjuntor diferencial residual), ou preferir uma solenoide de 12 V DC.

---

## 8. Pendências para as próximas fases

- Enviar o `ciclos.cpp` citado no estudo, se ele ainda existir.
- Confirmar: tensão da solenoide (12 V DC ou 220 V AC), existência de sump, tipo de iluminação e fotoperíodo, e se já existe um controlador de CO₂ ou timer.
- Definir o microcontrolador (recomendação: ESP32).

---

## 9. Conclusão e recomendação

**A opção mais viável é a B: leitor óptico do Drop Checker.**

Justificativa:

1. **Resolve o problema real deste aquário.** Os taninos dos galhos invalidam qualquer método baseado no pH da água, e a opção A depende exatamente disso. A opção B é imune por princípio físico.
2. **A opção A não mede CO₂.** Ela automatiza a medição de KH, um parâmetro que muda pouco. Para virar medição de CO₂ precisa de uma sonda de pH e continua falseada pelos taninos. Tem a fluídica mais complexa e o maior risco de vazamento.
3. **A opção B tem o menor custo e o menor risco técnico**, e cada etapa pode ser conferida a olho nu.
4. **A limitação da B (resposta lenta) é contornável** tratando o leitor como supervisor e trava de segurança, com a injeção comandada pelo fotoperíodo. O volume de 1000 L joga a favor, porque a química do tanque também muda devagar.
5. **O código atual da opção B precisa de correções antes de controlar a solenoide.** As duas falhas críticas da seção 4.6 (falha do sensor mantendo o CO₂ ligado e ausência de fotoperíodo) têm de ser resolvidas antes de ligar o relé ao cilindro.

O **NDIR fica como evolução futura**: a nota ficou próxima e ele oferece medição quantitativa e rápida. Pode ser integrado depois, reaproveitando o mesmo microcontrolador, o relé, o RTC e a lógica de segurança. Com o Drop Checker óptico já funcionando, ele ainda serve de referência cruzada para validar o NDIR.

### Roteiro proposto (substituído pela seção 10)

| Fase | Entrega | Relatório |
|---|---|---|
| 1 | Bancada: TCS3200 + Drop Checker fora do aquário; balanço de branco; curva matiz → pH com tampões | `relatorio-01-bancada-sensor.md` |
| 2 | Montagem mecânica: suporte opaco, fundo branco, leitura diferencial (LED ligado/desligado) | `relatorio-02-montagem.md` |
| 3 | Firmware de **monitoramento apenas** (sem relé): registro de dados por 1–2 semanas, comparado ao olho | `relatorio-03-monitoramento.md` |
| 4 | Atuação segura: RTC, fotoperíodo, fail-safe, watchdog, alarme | `relatorio-04-controle-seguro.md` |
| 5 | (Opcional) Módulo NDIR como segunda medição | `relatorio-05-ndir.md` |

---

## 10. Decisão (19/09/2026)

- **Opção B aprovada.** Ela segue como projeto principal em `projeto_drop_checker/`.
- **O NDIR vira um projeto independente** em `projeto_ndir/`, com o objetivo de comparar os resultados com o Drop Checker óptico.
- **Mudanças de hardware em relação ao estudo:**
  - O sensor passa a ser o **TCS34725** (I2C, 16 bits, filtro de IR) em vez do TCS3200.
  - A iluminação é feita por um **LED branco de 5 mm** controlado pelo Arduino, o que permite a leitura diferencial.
  - O microcontrolador é o **Arduino Uno**. Como ele não tem Wi-Fi, o alerta no celular foi substituído por **buzzer e TFT ST7789 2,4"**.
  - A solenoide é de **12 V DC**, o que elimina o risco de chavear 220 V perto da água.
  - O aquário **tem sump**, onde fica o equilibrador do NDIR.
- **Os códigos originais foram arquivados em `legado/`.** Os relatórios de cada fase ficam em `<projeto>/docs/`, conforme o `README.md` de cada projeto.

---

## 11. Inclusão do módulo de pH (19/09/2026)

O módulo de pH com conector BNC (PH-4502C) foi incorporado ao projeto Drop Checker. Isso **não contradiz** a seção 2: lá foi descartado usar o pH para calcular o **valor absoluto** do CO₂ pela fórmula pH/KH, que os taninos distorcem. O módulo entra em dois papéis onde esse problema não existe:

1. **Instrumento de calibração (fase 1).** Mede o pH das soluções de referência, que não têm taninos, para montar a curva matiz → pH do Drop Checker óptico.
2. **Alarme rápido por queda de pH (fases 3 e 4).** A referência é uma amostra da água do próprio aquário aerada até perder o CO₂ extra, e o alarme dispara pela **diferença** de pH em relação a ela. Os taninos estão presentes nas duas leituras e se cancelam. A sonda responde em minutos, o que cobre a maior fraqueza da opção B: a demora de 1–2 h do Drop Checker para detectar excesso de CO₂.

Cuidados: ruído elétrico das bombas e aquecedores com a sonda no aquário (prever isolador de sinal), recalibração mensal, vida útil da sonda de ~1 ano em imersão contínua, e a referência de 5 V do ADC do Uno.

O pH também entra como **terceiro método na comparação** do projeto NDIR, tanto pela fórmula pH/KH quanto pela queda de pH.
