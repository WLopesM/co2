const int S0 = 2; const int S1 = 3; const int S2 = 4; const int S3 = 5; const int sensorOut = 6;
const int BOMBA_AQUARIO = 8;
const int BOMBA_REAGENTE = 9;
const int BOMBA_DESCARTE = 10;

int r = 0; int g = 0; int b = 0;
int contagemGotas = 0;
bool titulacaoConcluida = false;

void setup() {
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT); pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(BOMBA_AQUARIO, OUTPUT); pinMode(BOMBA_REAGENTE, OUTPUT); pinMode(BOMBA_DESCARTE, OUTPUT);
  
  digitalWrite(S0, HIGH); digitalWrite(S1, LOW); // Escala 20%
  Serial.begin(9600);
  
  // Executa uma titulação ao iniciar (para teste)
  executarTitulacaoKH();
}

void loop() {
  // Fica em espera. Poderia ser acionado por um botão ou timer (RTC) aqui.
}

void executarTitulacaoKH() {
  Serial.println("Iniciando ciclo de teste de KH...");
  titulacaoConcluida = false;
  contagemGotas = 0;

  // 1. Coleta a água do aquário (calibre o tempo para dar exatos 5ml ou 10ml)
  Serial.println("Coletando amostra de água...");
  digitalWrite(BOMBA_AQUARIO, HIGH);
  delay(5000); // Exemplo: 5 segundos ligada
  digitalWrite(BOMBA_AQUARIO, LOW);
  delay(2000); // Espera estabilizar

  // 2. Loop de gotejamento do reagente
  while (!titulacaoConcluida) {
    // Injeta 1 gota (pulso muito curto calibrado na sua bomba peristáltica)
    Serial.println("Injetando 1 gota...");
    digitalWrite(BOMBA_REAGENTE, HIGH);
    delay(150); // Ajuste este tempo milimetricamente para corresponder a 1 gota
    digitalWrite(BOMBA_REAGENTE, LOW);
    
    contagemGotas++;
    delay(4000); // Espera 4 segundos para misturar bem (ideal usar um aerador ligado aqui)
    
    lerRGB_Titulador();
    
    // Critério de virada: Quando o Vermelho (Red) fica muito mais forte que o Azul (Blue)
    // Lembre-se: Menor frequência = cor mais forte no TCS3200
    if (r < b && r < g) { 
      titulacaoConcluida = true;
      Serial.print("Mudança de cor detectada (Azul -> Amarelo)! KH final: ");
      Serial.println(contagemGotas); // Em testes padrão, 1 gota = 1 dKH
    }
    
    // Trava de segurança para não esvaziar o frasco de reagente se o sensor falhar
    if (contagemGotas > 25) {
      Serial.println("Erro: Limite de gotas excedido. Abortando.");
      break;
    }
  }

  // 3. Limpeza e Descarte
  Serial.println("Esvaziando câmara de teste...");
  digitalWrite(BOMBA_DESCARTE, HIGH);
  delay(7000); // Tempo suficiente para esvaziar tudo
  digitalWrite(BOMBA_DESCARTE, LOW);
  Serial.println("Ciclo finalizado.");
}

void lerRGB_Titulador() {
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  r = pulseIn(sensorOut, LOW); delay(20);
  
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  g = pulseIn(sensorOut, LOW); delay(20);
  
  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  b = pulseIn(sensorOut, LOW); delay(20);
}
