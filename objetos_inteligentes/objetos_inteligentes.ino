#include "MPU6050_tockn.h"
#include "Wire.h"

MPU6050 mpu6050(Wire);

// Definição dos pinos
const int botaoPin = 2;     // Botão de emergência no D2
const int buzzerPin = 15;   // Buzzer no D15

// Variáveis para detecção de queda
float accelThreshold = 2.5; // Limite para detecção de queda (ajustável)
bool quedaDetectada = false;
unsigned long tempoQueda = 0;
bool sistemaAtivo = true;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Inicializa componentes
  pinMode(botaoPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  // Sinal de início
  tone(buzzerPin, 1000, 200);
  delay(1000);
  
  Serial.println();
  Serial.println("🟢 === SISTEMA DE MONITORAMENTO PARA IDOSOS ===");
  Serial.println("✅ Sistema inicializado com sucesso!");
  Serial.println("🎯 Botão de emergência: Pino D2");
  Serial.println("🔊 Buzzer: Pino D15");
  Serial.println("📡 Sensor MPU-6050: Inicializando...");
  Serial.println("=============================================");
  
  // Inicializa sensor MPU-6050
  bool sensorOK = false;
  for(int i = 0; i < 3; i++) {
    try {
      mpu6050.begin();
      mpu6050.calcGyroOffsets(true);
      sensorOK = true;
      break;
    } catch (...) {
      Serial.println("⚠️ Tentativa " + String(i+1) + " - Sensor não respondendo...");
      delay(1000);
    }
  }
  
  if(sensorOK) {
    Serial.println("✅ Sensor MPU-6050 calibrado e pronto!");
  } else {
    Serial.println("❌ ERRO: Sensor MPU-6050 não detectado!");
    Serial.println("🔧 Verifique conexões: VCC, GND, SDA(21), SCL(22)");
    sistemaAtivo = false;
  }
  
  Serial.println("📊 Iniciando monitoramento...");
  Serial.println();
}

void loop() {
  // Controle do botão de emergência com anti-ressalto
  static unsigned long ultimoBotao = 0;
  int estadoBotao = digitalRead(botaoPin);
  
  if(estadoBotao == LOW && (millis() - ultimoBotao > 1000)) {
    ultimoBotao = millis();
    Serial.println("🚨🚨🚨 BOTÃO DE EMERGÊNCIA PRESSIONADO! 🚨🚨🚨");
    ativarAlerta(3, 1000); // 3 bips longos
    delay(2000); // Evita múltiplas ativações
  }
  
  // Monitoramento do sensor MPU-6050 (se estiver ativo)
  if(sistemaAtivo) {
    mpu6050.update();
    
    float accelX = mpu6050.getAccX();
    float accelY = mpu6050.getAccY(); 
    float accelZ = mpu6050.getAccZ();
    
    float accelTotal = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
    
    // Detecção de queda
    if(accelTotal > accelThreshold && !quedaDetectada) {
      quedaDetectada = true;
      tempoQueda = millis();
      
      Serial.println();
      Serial.println("⚠️⚠️⚠️ QUEDA DETECTADA! ⚠️⚠️⚠️");
      Serial.print("📊 Aceleração: "); Serial.println(accelTotal);
      Serial.print("📍 X:"); Serial.print(accelX);
      Serial.print(" Y:"); Serial.print(accelY); 
      Serial.print(" Z:"); Serial.println(accelZ);
      
      ativarAlerta(5, 500); // 5 bips rápidos
    }
    
    // Reset da detecção após 10 segundos
    if(quedaDetectada && (millis() - tempoQueda > 10000)) {
      quedaDetectada = false;
      Serial.println("✅ Sistema resetado - pronto para nova detecção");
    }
    
    // Mostra status a cada 5 segundos
    static unsigned long ultimoStatus = 0;
    if(millis() - ultimoStatus > 5000) {
      ultimoStatus = millis();
      Serial.print("📡 Sistema OK | ");
      Serial.print("Queda: "); Serial.print(quedaDetectada ? "SIM" : "não");
      Serial.print(" | Botão: "); Serial.print(estadoBotao ? "SOLTO" : "PRESSIONADO");
      Serial.print(" | Tempo: "); Serial.print(millis() / 1000); Serial.println("s");
    }
  } else {
    // Modo de emergência - só botão funciona
    static unsigned long ultimoErro = 0;
    if(millis() - ultimoErro > 10000) {
      ultimoErro = millis();
      Serial.println("🔴 MODO EMERGÊNCIA - Apenas botão funciona");
      Serial.println("🔧 Verifique conexão do sensor MPU-6050");
    }
  }
  
  delay(100); // Pequeno delay para estabilidade
}

// Função para ativar alertas sonoros
void ativarAlerta(int vezes, int duracao) {
  for(int i = 0; i < vezes; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(duracao);
    digitalWrite(buzzerPin, LOW);
    if(i < vezes - 1) delay(200);
  }
}