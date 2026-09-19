# Legado

Códigos que acompanham o estudo original (`../docs/projeto.odt`). Estão aqui **só para consulta** ao reler esse material e não devem ser usados no aquário.

| Arquivo | O que era | Por que foi substituído |
|---|---|---|
| `controle_co2.cpp` | Leitor do Drop Checker com TCS3200 e corte da solenoide | Com o sensor desconectado, a solenoide ficava aberta, e não havia controle de fotoperíodo. Ver seção 4.6 do relatório 00. O hardware atual usa o TCS34725. |
| `processo_titulacao.cpp` | Titulador automático de KH (opção A) | A opção A foi descartada. Ver seções 3 e 9 do relatório 00. |

O estudo também cita um `ciclos.cpp` (NDIR), que não foi encontrado.
