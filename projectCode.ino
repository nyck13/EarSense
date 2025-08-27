#include <driver/i2s.h>

// ----- CONFIGURAÇÕES -----
// Pinos do microfone I2S
#define I2S_WS   25   // L/R (word select)
#define I2S_SD   32   // data
#define I2S_SCK  33   // clock

const int vibracall = 19;          // pino de saída do motor de vibração 

// Normalização (ajuste conforme calibração do microfone)
const float DB_MIN = 49.5;
const float DB_MAX = 200.0;
const float DB_BUZINA_MIN = 120.0;

// Controle do vibracall
unsigned long vibracallStart = 0;
bool vibracallActive = false;

// ----- FUNÇÕES AUXILIARES -----
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  if (in_max == in_min) return out_min;
  float y = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  if (y < out_min) y = out_min;
  if (y > out_max) y = out_max;
  return y;
}

// ----- SETUP -----
void setup() {
  Serial.begin(115200);
  pinMode(vibracall, OUTPUT);
  digitalWrite(vibracall, LOW);

  // Configuração I2S para microfone digital
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1, // não usamos saída
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  delay(2000);
}

// ----- LOOP -----
void loop() {
  // ---- Leitura I2S (digital) ----
  int32_t buffer[128];  // <<< menor janela (antes era 256) → responde mais rápido
  size_t bytes_read;
  i2s_read(I2S_NUM_0, (void*)buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

  int samples_read = bytes_read / sizeof(int32_t);
  long sum = 0;
  for (int i = 0; i < samples_read; i++) {
    sum += abs(buffer[i] >> 12); // <<< mais sensível (antes era >> 14)
  }
  int avg = sum / samples_read;

  // converte para dB aproximado (precisa calibrar os limites!)
  float db = mapFloat(avg, 50, 10000, DB_MIN, DB_MAX); // <<< faixa mais curta para ampliar resposta

  // ---- Debug Serial ----
  Serial.print(db);
  Serial.println(" dB");

  // ---- Controle do vibracall ----
  if (db >= DB_BUZINA_MIN && !vibracallActive) {
    vibracallActive = true;
    vibracallStart = millis();
    digitalWrite(vibracall, HIGH);
  }

  if (vibracallActive && millis() - vibracallStart >= 2000) {
    digitalWrite(vibracall, LOW);
    vibracallActive = false;
  }

  delay(100);
}