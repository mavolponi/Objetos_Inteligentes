#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <PubSubClient.h>

// ---------- DADOS DO WI-FI ----------
const char* ssid = "Rafita";
const char* password = "25060528";

// ---------- DADOS DO BROKER MQTT ----------
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* topic_sensor = "alarme/queda";    // envio de alerta
const char* topic_comando = "alarme/comando"; // comando remoto

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- MPU ----------
MPU6050 mpu;

// ---------- PINOS ----------
const int pinoBotao = 4;
const int pinoBuzzer = 5;

// ---------- SISTEMA ----------
bool sistemaAtivo = false;
bool quedaDetectada = false;
float limiteQueda = 1.4;

// ---------- TEMPO ----------
unsigned long ultimoEnvio = 0;

// ---------- FUNÇÕES ----------
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Recebe comando via MQTT
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Mensagem recebida: ");
  Serial.println(msg);

  if (String(topic) == topic_comando) {
    if (msg == "ATIVAR") {
      sistemaAtivo = true;
      quedaDetectada = false;
      digitalWrite(pinoBuzzer, LOW);
      Serial.println("✅ Sistema ATIVADO via MQTT");
    } else if (msg == "DESATIVAR") {
      sistemaAtivo = false;
      digitalWrite(pinoBuzzer, LOW);
      Serial.println("⛔ Sistema DESATIVADO via MQTT");
    } else if (msg == "BUZZER_ON") {
      digitalWrite(pinoBuzzer, HIGH);
      Serial.println("🔊 Buzzer LIGADO via MQTT");
    } else if (msg == "BUZZER_OFF") {
      digitalWrite(pinoBuzzer, LOW);
      Serial.println("🔇 Buzzer DESLIGADO via MQTT");
    }
  }
}

void conectarMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("ESP32Alarme")) {
      Serial.println("✅ Conectado ao broker!");
      client.subscribe(topic_comando);
      client.publish(topic_sensor, "ESP32 Online e pronto!"); // mensagem inicial
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s");
      delay(5000);
    }
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(pinoBotao, INPUT_PULLUP);
  pinMode(pinoBuzzer, OUTPUT);
  digitalWrite(pinoBuzzer, LOW);

  mpu.initialize();

  conectarWiFi();
  conectarMQTT();

  Serial.println("=== SISTEMA DE ALARME DE QUEDA ===");
  Serial.println("Aperte o botão para ATIVAR/DESATIVAR");
  Serial.println("=================================");
}

// ---------- LOOP ----------
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();

  // ---------- BOTÃO FÍSICO ----------
  static bool ultimoEstado = HIGH;
  bool estadoAtual = digitalRead(pinoBotao);

  if (ultimoEstado == HIGH && estadoAtual == LOW) {
    sistemaAtivo = !sistemaAtivo;
    quedaDetectada = false;
    digitalWrite(pinoBuzzer, LOW);

    if (sistemaAtivo) {
      Serial.println("✅ Sistema ATIVADO pelo botão");
      client.publish(topic_sensor, "Sistema ATIVADO pelo botão");
    } else {
      Serial.println("⛔ Sistema DESATIVADO pelo botão");
      client.publish(topic_sensor, "Sistema DESATIVADO pelo botão");
    }
    delay(400); // debounce
  }
  ultimoEstado = estadoAtual;

  // ---------- DETECÇÃO DE QUEDA ----------
  if (sistemaAtivo) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    float x = ax / 16384.0;
    float y = ay / 16384.0;
    float z = az / 16384.0;

    float forca = sqrt(x * x + y * y + z * z);
    Serial.print("Força: ");
    Serial.println(forca);

    if (forca > limiteQueda && !quedaDetectada) {
      quedaDetectada = true;
      digitalWrite(pinoBuzzer, HIGH);

      unsigned long agora = millis();
      if (agora - ultimoEnvio > 5000) {
        client.publish(topic_sensor, "🚨 ALERTA! QUEDA DETECTADA!");
        Serial.println("📨 Mensagem enviada via MQTT!");
        ultimoEnvio = agora;
      }
    }
  }

  delay(200);
}

