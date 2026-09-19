// Pinos do TCS3200
const int S0 = 2;
const int S1 = 3;
const int S2 = 4;
const int S3 = 5;
const int sensorOut = 6;

// Pino de Controle do CO2
const int RELAY_CO2 = 7;

// Variáveis para armazenar a intensidade das cores
int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;

void setup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(RELAY_CO2, OUTPUT);
  
  // Configura a escala de frequência do sensor para 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  
  // Inicia o CO2 ligado por padrão de segurança (ou desligado se preferir)
  digitalWrite(RELAY_CO2, HIGH); 
  Serial.begin(9600);
}

void loop() {
  lerRGB();
  
  // LÓGICA DE DECISÃO BASEADA EM FREQUÊNCIA
  // Nota: No TCS3200, QUANTO MENOR a frequência lida, MAIS INTENSA é a cor.
  
  if (redFrequency < blueFrequency && greenFrequency < blueFrequency) {
    // AMARELO: Frequências de Red e Green são menores (mais fortes) que Blue
    Serial.println("ALERTA: Excesso de CO2! Desligando Solenóide.");
    digitalWrite(RELAY_CO2, LOW); // Desliga a solenoide
  } 
  else if (blueFrequency < redFrequency && blueFrequency < greenFrequency) {
    // AZUL: Frequência de Blue é a menor (mais forte)
    Serial.println("Status: Pouco CO2. Solenóide Ativa.");
    digitalWrite(RELAY_CO2, HIGH);
  } 
  else {
    // VERDE: Equilíbrio ideal
    Serial.println("Status: CO2 Ideal. Sistema Seguro.");
    digitalWrite(RELAY_CO2, HIGH);
  }

  // O Drop Checker responde lentamente (1-2 horas), ler a cada 5 minutos é suficiente
  delay(300000); 
}

void lerRGB() {
  // Configura para ler filtro Vermelho (Red)
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  redFrequency = pulseIn(sensorOut, LOW);
  delay(20);
  
  // Configura para ler filtro Verde (Green)
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  greenFrequency = pulseIn(sensorOut, LOW);
  delay(20);
  
  // Configura para ler filtro Azul (Blue)
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  blueFrequency = pulseIn(sensorOut, LOW);
  delay(20);
}
