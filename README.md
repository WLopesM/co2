# CO₂ Dissolvido

Medição e controle de CO₂ dissolvido em aquário de água doce plantado (1000 L, com sump), usando Arduino.

## Estrutura

| Diretório | Conteúdo |
|---|---|
| [`docs/`](docs/) | Estudo original (`projeto.odt`) e o [relatório de viabilidade](docs/relatorio-00-analise-de-viabilidade.md) |
| [`projeto_drop_checker/`](projeto_drop_checker/) | **Projeto principal:** sensor TCS34725 lê a cor do Drop Checker, mostra o CO₂ num TFT e atua como trava de segurança da solenoide |
| [`projeto_ndir/`](projeto_ndir/) | **Projeto de comparação:** sensor NDIR 0–5% em equilibrador no sump, para medição quantitativa |
| [`legado/`](legado/) | Códigos do estudo original, só para consulta |

Cada projeto é independente: tem hardware, firmware e relatórios de fase próprios, em `<projeto>/docs/`.

## Relatórios em PDF

Os relatórios são escritos em Markdown. Para gerar os PDFs (gravados ao lado de cada `.md`):

```bash
./gerar_pdf.py                                          # todos os relatórios
./gerar_pdf.py projeto_drop_checker/docs/fase-01-bancada.md   # só um
```

O script precisa do Google Chrome (ou Chromium) e do pacote Python `markdown-it-py`. Os PDFs não vão para o git: o `.md` é a fonte.

## Status

| Projeto | Fase atual |
|---|---|
| Drop Checker óptico | Fase 1: bancada e calibração |
| NDIR | Fase 0: planejamento |

## Segurança

O CO₂ em excesso mata peixes. Nenhum firmware deste repositório deve acionar a solenoide antes da fase 4 do projeto Drop Checker. Essa fase implementa fotoperíodo por RTC, desligamento em qualquer falha e alarme.
