#include <Arduino.h>
#include <driver/i2s.h>

// Pin definitions
#define I2S_SCK 26   // BCLK
#define I2S_WS  25   // LRCL / WS
#define I2S_SD  22   // DOUT from mic

#define I2S_NUM I2S_NUM_0

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("SPH0645 I2S Right Channel Test (SEL -> VDD)");

  // I2S configuration
  i2s_config_t i2s_config = {
      (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      44100,                               // sample_rate
      I2S_BITS_PER_SAMPLE_32BIT,
      I2S_CHANNEL_FMT_ONLY_RIGHT,          // Read only right channel
      (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_I2S_MSB),
      0,
      4,
      256,
      false,
      false,
      0
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);

  Serial.println("I2S mic initialized. Speak near the mic!");
}

void loop() {
  int32_t buffer[8];      // small buffer for testing
  size_t bytes_read = 0;

  // Read audio samples
  esp_err_t result = i2s_read(I2S_NUM, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
  if (result != ESP_OK || bytes_read == 0) {
    Serial.println("I2S read error or no data");
    delay(500);
    return;
  }

  // Print samples as amplitude values
  Serial.println("Raw right channel samples (signed 24-bit):");
  for (int i = 0; i < 8; i++) {
    // SPH0645 outputs 24-bit left-aligned in 32-bit word, shift down 8 bits
    int32_t sample = buffer[i];
    Serial.println(sample);
  }

  delay(500);
}