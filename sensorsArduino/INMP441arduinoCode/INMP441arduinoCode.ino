#include <driver/i2s.h>
#include <math.h>

// Number of microphone samples processed in each block
#define SAMPLE_BUFFER_SIZE 512

// Number of samples captured each second
#define SAMPLE_RATE 8000

// L/R connected to GND means use the left I2S channel
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT

// ESP32-C3 GPIO assignments
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_6       // SCK / BCLK
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_7   // WS / LRCLK
#define I2S_MIC_SERIAL_DATA GPIO_NUM_10       // SD

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
};

i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
};

int32_t raw_samples[SAMPLE_BUFFER_SIZE];

void setup() {
  // Start communication with the Serial Monitor and Plotter
  Serial.begin(115200); //plotter and monitor should also be at 115200
  delay(1000);

  // Install and configure the ESP32 I2S driver
  esp_err_t installResult =
      i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  esp_err_t pinResult =
      i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

  if (installResult != ESP_OK || pinResult != ESP_OK) {
    Serial.println("I2S initialization failed!");

    while (true) {
      delay(100);
    }
  }

  Serial.println("Microphone initialized!");
}

void loop() {
  size_t bytesRead = 0;

  // Read one block of samples from the microphone
  esp_err_t readResult = i2s_read(
      I2S_NUM_0,
      raw_samples,
      sizeof(raw_samples),
      &bytesRead,
      portMAX_DELAY
  );

  if (readResult != ESP_OK || bytesRead == 0) {
    Serial.println("Microphone read failed!");
    return;
  }

  int samplesRead = bytesRead / sizeof(int32_t);

  /*
    First pass: calculate the mean.
    removes any DC offset (avg signal value) before RMS calculated.
    find avg signal value to remove any offset (center signal around zero using the mean) 
  */
  double sum = 0.0;

  for (int i = 0; i < samplesRead; i++) {
    // The microphone's 24-bit value stored in a 32-bit word
    int32_t sample = raw_samples[i] >> 8;
    sum += sample;
  }

  double mean = sum / samplesRead;

  /*
    Second pass: calculate the sum of squared,
    mean-centered samples.
  */
  double sumOfSquares = 0.0;

  for (int i = 0; i < samplesRead; i++) {
    int32_t sample = raw_samples[i] >> 8;
    double centeredSample = sample - mean;

    sumOfSquares += centeredSample * centeredSample;
  }

  // RMS = square root of the mean of the squared samples
  double rms = sqrt(sumOfSquares / samplesRead);

  // Prevent log10(0)
  if (rms < 1.0) {
    rms = 1.0;
  }

  /*
    Convert RMS to a relative digital decibel value. (dBFS Decibels Relative to Full Scale)
      how close is the signal to the maximum that the microphone can represent? 
    (raw microphone sample values) 8,388,607 is the largest positive signed 24-bit value., samples lie inbetween +/- 8,388,607
  */
  const double fullScale = 8388607.0;
  double dBFS = 20.0 * log10(rms / fullScale);

  // Labels make the values easy to graph in Serial Plotter
  // RMS (Root Mean Squared)
  // normal readings around 450-550 
  // super loud readings hit around 4544353 (blew super hard into mic) (same reading as the dbfs, so RMS: 4544353, dBFS: -5.32)
  Serial.print("RMS:");
  Serial.print(rms);

  // dBFS (Decibels Relative to Full Scale)
  // super loud would be like -10 to -3? (blew into mic hard and got -5.32) (normal readings typically at -84?)
  Serial.print("\tdBFS:");
  Serial.println(dBFS, 2);
}