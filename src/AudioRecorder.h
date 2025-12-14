#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <SPIFFS.h>
#include <FS.h>
#include "PinConfig.h"
#include <Adafruit_ST7789.h> 

struct WavHeader {
  char riff_tag[4];  uint32_t riff_length; 
  char wave_tag[4];  char fmt_tag[4];   
  uint32_t fmt_length; uint16_t audio_format; uint16_t num_channels;
  uint32_t sample_rate; uint32_t byte_rate; uint16_t block_align;
  uint16_t bits_per_sample; char data_tag[4]; uint32_t data_length;
};

void initMic() {
  // Uninstall first to prevent the "register failed" error
  i2s_driver_uninstall(I2S_PORT);
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // Mic sends 32-bit
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, 
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8, 
    .dma_buf_len = 512,
    .use_apll = true, 
    .tx_desc_auto_clear = false, 
    .fixed_mclk = 0
  };
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void recordAudio(Adafruit_ST7789 &tft) {
  LittleFS.remove(FILE_NAME);
  File file = LittleFS.open(FILE_NAME, FILE_WRITE);
  if(!file){ Serial.println("File Error"); return; }

  // UI Setup
  tft.fillScreen(ST77XX_RED);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 50); tft.println("RECORDING...");
  tft.drawRect(20, 100, 200, 20, ST77XX_WHITE);

  // Write Placeholder Header
  WavHeader header;
  file.write((uint8_t*)&header, sizeof(WavHeader)); 

  // BUFFERS
  // We read 32-bit samples but write 16-bit samples (2x Faster!)
  size_t bytes_read;
  int32_t* i2s_read_buff = (int32_t*) calloc(256, sizeof(int32_t));
  int16_t* i2s_write_buff = (int16_t*) calloc(256, sizeof(int16_t));

  size_t total_samples = SAMPLE_RATE * RECORD_TIME_SECONDS;
  size_t samples_recorded = 0;

  unsigned long last_draw = 0;
  int progress_width = 0;

  while (samples_recorded < total_samples) {
    // 1. Read Raw Data (32-bit)
    i2s_read(I2S_PORT, (void*)i2s_read_buff, 256 * sizeof(int32_t), &bytes_read, portMAX_DELAY);
    
    int samples_in_batch = bytes_read / 4; // 4 bytes per 32-bit sample

    // 2. Convert to 16-bit (Compression Step)
    // SPH0645 data is in the top 24 bits. We shift right by 14 to fit into 16 bits cleanly.
    for(int i=0; i<samples_in_batch; i++) {
        i2s_write_buff[i] = (i2s_read_buff[i] >> 14); 
    }

    // 3. Write Compressed Data
    file.write((uint8_t*)i2s_write_buff, samples_in_batch * 2); // 2 bytes per 16-bit sample
    samples_recorded += samples_in_batch;

    // 4. Smooth Animation
    if(millis() - last_draw > 300) {
        last_draw = millis();
        float percent = (float)samples_recorded / (float)total_samples;
        int w = (int)(percent * 196);
        if(w > progress_width) {
            tft.fillRect(22 + progress_width, 102, w - progress_width, 16, ST77XX_WHITE);
            progress_width = w;
        }
    }
  }

  free(i2s_read_buff);
  free(i2s_write_buff);

  // Update Header for 16-BIT Audio
  memcpy(header.riff_tag, "RIFF", 4); memcpy(header.wave_tag, "WAVE", 4);
  memcpy(header.fmt_tag, "fmt ", 4);  memcpy(header.data_tag, "data", 4);
  header.riff_length = file.size() - 8;
  header.fmt_length = 16; header.audio_format = 1; header.num_channels = 1;
  header.sample_rate = SAMPLE_RATE; 
  header.bits_per_sample = 16;       // <--- CHANGED TO 16
  header.byte_rate = SAMPLE_RATE * 2; // <--- CHANGED TO 2 BYTES
  header.block_align = 2;
  header.data_length = file.size() - 44;

  file.seek(0);
  file.write((uint8_t*)&header, sizeof(WavHeader));
  file.close();

  Serial.println("Done.");
  tft.fillScreen(ST77XX_BLACK); 
}

#endif