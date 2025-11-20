#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <driver/i2s.h>

#define TFT_CS    5
#define TFT_RST   17
#define TFT_DC    16

#define I2S_WS 25
#define I2S_SD 22
#define I2S_SCK 26

#define I2S_NUM I2S_NUM_0



Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  tft.init(135, 240); // width, height
  tft.setRotation(1);  // 0,1,2,3 are valid
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Hello ESP32!");


  i2s_config_t i2s_config = {
      (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),    // mode
      44100,                             // sample_rate
      I2S_BITS_PER_SAMPLE_32BIT,         // bits_per_sample
      I2S_CHANNEL_FMT_ONLY_LEFT,         // channel_format
      (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_I2S_MSB), // communication_format
      0,                                 // intr_alloc_flags
      4,                                 // dma_buf_count
      256,                               // dma_buf_len
      false,                             // use_apll
      false,                             // tx_desc_auto_clear
      0                                  // fixed_mclk
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);

  Serial.println("I2S mic initialized.");
}

void loop() {
  int32_t buffer[1024];
  size_t bytes_read = 0;

  // Read audio samples from mic
  esp_err_t result = i2s_read(I2S_NUM, &buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
  if (result == ESP_OK && bytes_read > 0) {
    // Convert bytes_read to number of 32-bit samples
    size_t samples_read = bytes_read / 4;

    // Simple summary: print max amplitude in buffer
    for (int i = 0; i < 16; i++) {
      Serial.println(buffer[i], HEX);
    }

    int32_t max_val = 0;
    for (size_t i = 0; i < samples_read; i++) {
      int32_t sample = buffer[i] >> 8; // shift to get actual audio value
      if (abs(sample) > max_val) max_val = abs(sample);
    }

    Serial.print("Samples read: ");
    Serial.print(samples_read);
    Serial.print(" | Max amplitude: ");
    Serial.println(max_val);
  }

  delay(200);

}