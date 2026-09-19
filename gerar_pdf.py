#!/usr/bin/env python3
"""Converte os relatórios Markdown do projeto em PDF.

Uso:
    ./gerar_pdf.py                      converte todos os relatórios (docs/ e <projeto>/docs/)
    ./gerar_pdf.py caminho/arquivo.md   converte só os arquivos indicados

O PDF é gravado ao lado do .md, com o mesmo nome.

Requisitos: Python 3 com markdown-it-py e o Google Chrome (ou Chromium) instalado.
"""

import argparse
import html
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from markdown_it import MarkdownIt

RAIZ = Path(__file__).resolve().parent
NAVEGADORES = ["google-chrome", "google-chrome-stable", "chromium", "chromium-browser"]

CSS = """
@page {
  size: A4;
  margin: 18mm 16mm 20mm 16mm;
  @bottom-center {
    content: counter(page) " / " counter(pages);
    font: 9pt "Noto Sans", "DejaVu Sans", sans-serif;
    color: #666;
  }
}
body {
  font: 10.5pt/1.5 "Noto Sans", "DejaVu Sans", "Liberation Sans", sans-serif;
  color: #1a1a1a;
}
h1 { font-size: 20pt; border-bottom: 2px solid #1f6f8b; padding-bottom: 4px; margin-top: 0; }
h2 { font-size: 14pt; color: #1f6f8b; margin-top: 22px; break-after: avoid; }
h3 { font-size: 12pt; margin-top: 16px; break-after: avoid; }
p, li { orphans: 3; widows: 3; }
table { border-collapse: collapse; width: 100%; margin: 10px 0; font-size: 9.5pt; }
th, td { border: 1px solid #bbb; padding: 4px 7px; text-align: left; vertical-align: top; }
th { background: #e8f1f4; }
tr { break-inside: avoid; }
code {
  font: 9pt "Noto Sans Mono", "DejaVu Sans Mono", monospace;
  background: #f2f2f2; padding: 1px 4px; border-radius: 3px;
}
pre {
  background: #f6f8fa; border: 1px solid #ddd; border-radius: 4px;
  padding: 8px 10px; white-space: pre-wrap; break-inside: avoid;
}
pre code { background: none; padding: 0; }
hr { border: none; border-top: 1px solid #ccc; margin: 18px 0; }
a { color: #1f6f8b; text-decoration: none; }
.caixa { font-family: "DejaVu Sans", sans-serif; margin-right: 6px; }
li.tarefa { list-style: none; margin-left: -1.1em; }
"""


def renderizar_html(md_texto: str, titulo: str) -> str:
    # breaks=True: cada linha do .md vira uma linha no PDF (ex.: cabeçalho Projeto/Status/Data)
    md = MarkdownIt("commonmark", {"breaks": True}).enable(["table", "strikethrough"])
    corpo = md.render(md_texto)
    # Listas de tarefas do GitHub ("- [ ]" / "- [x]") viram caixas de seleção
    corpo = re.sub(r"<li>\[ \]", '<li class="tarefa"><span class="caixa">☐</span>', corpo)
    corpo = re.sub(r"<li>\[[xX]\]", '<li class="tarefa"><span class="caixa">☑</span>', corpo)
    return (
        "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='utf-8'>"
        f"<title>{html.escape(titulo)}</title><style>{CSS}</style></head>"
        f"<body>{corpo}</body></html>"
    )


def encontrar_navegador() -> str:
    for nome in NAVEGADORES:
        caminho = shutil.which(nome)
        if caminho:
            return caminho
    sys.exit("ERRO: Google Chrome/Chromium não encontrado.")


def converter(md_arquivo: Path, navegador: str) -> Path:
    texto = md_arquivo.read_text(encoding="utf-8")
    titulo_md = re.search(r"^# (.+)$", texto, re.MULTILINE)
    titulo = titulo_md.group(1) if titulo_md else md_arquivo.stem
    pdf = md_arquivo.with_suffix(".pdf")
    pdf.unlink(missing_ok=True)  # evita confundir um PDF antigo com o resultado desta conversão

    with tempfile.TemporaryDirectory() as tmp:
        html_arquivo = Path(tmp) / "relatorio.html"
        html_arquivo.write_text(renderizar_html(texto, titulo), encoding="utf-8")
        resultado = subprocess.run(
            [
                navegador,
                "--headless=new",
                "--disable-gpu",
                "--no-pdf-header-footer",
                f"--user-data-dir={tmp}/perfil",
                f"--print-to-pdf={pdf}",
                html_arquivo.as_uri(),
            ],
            capture_output=True,
            text=True,
            timeout=120,
        )
    if resultado.returncode != 0 or not pdf.exists():
        sys.exit(f"ERRO ao converter {md_arquivo}:\n{resultado.stderr}")
    return pdf


def relatorios_padrao() -> list[Path]:
    arquivos = sorted(RAIZ.glob("docs/*.md")) + sorted(RAIZ.glob("projeto_*/docs/*.md"))
    return [a for a in arquivos if a.is_file()]


def relativo(caminho: Path) -> Path:
    return caminho.relative_to(RAIZ) if caminho.is_relative_to(RAIZ) else caminho


def main() -> None:
    parser = argparse.ArgumentParser(description="Converte relatórios Markdown em PDF.")
    parser.add_argument("arquivos", nargs="*", type=Path, help="arquivos .md (padrão: todos os relatórios)")
    args = parser.parse_args()

    arquivos = args.arquivos or relatorios_padrao()
    if not arquivos:
        sys.exit("Nenhum relatório encontrado.")

    navegador = encontrar_navegador()
    for md_arquivo in arquivos:
        if md_arquivo.suffix.lower() != ".md" or not md_arquivo.is_file():
            sys.exit(f"ERRO: {md_arquivo} não é um arquivo .md existente.")
        pdf = converter(md_arquivo.resolve(), navegador)
        print(f"{relativo(md_arquivo.resolve())} -> {relativo(pdf)}")


if __name__ == "__main__":
    main()
