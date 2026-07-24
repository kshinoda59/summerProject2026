#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <driver/i2s.h>
#include <math.h>

// --------------------------------------------------
// Microphone settings
// --------------------------------------------------

#define SAMPLE_RATE 20000

// 200 samples at 20 kHz = 10 milliseconds of audio
#define SAMPLE_BUFFER_SIZE 200

#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT

#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_6
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_7
#define I2S_MIC_SERIAL_DATA GPIO_NUM_10

// --------------------------------------------------
// MPU6050 settings
// --------------------------------------------------

#define MPU_SDA_PIN 4
#define MPU_SCL_PIN 5

Adafruit_MPU6050 mpu;

// --------------------------------------------------
// Microphone configuration
// --------------------------------------------------

i2s_config_t i2sConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = SAMPLE_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
};

i2s_pin_config_t i2sPins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
};

int32_t rawSamples[SAMPLE_BUFFER_SIZE];

// --------------------------------------------------
// Initialize the microphone
// --------------------------------------------------

bool initializeMicrophone() {
    esp_err_t installResult =
        i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);

    if (installResult != ESP_OK) {
        return false;
    }

    esp_err_t pinResult =
        i2s_set_pin(I2S_NUM_0, &i2sPins);

    return pinResult == ESP_OK;
}

// --------------------------------------------------
// Read microphone block and calculate dBFS
// --------------------------------------------------

bool readMicrophoneDbfs(double &dbfs) {
    size_t bytesRead = 0;

    esp_err_t readResult = i2s_read(
        I2S_NUM_0,
        rawSamples,
        sizeof(rawSamples),
        &bytesRead,
        portMAX_DELAY
    );

    if (readResult != ESP_OK || bytesRead == 0) {
        return false;
    }

    int samplesRead = bytesRead / sizeof(int32_t);

    // First pass: calculate the mean to remove DC offset.
    double sum = 0.0;

    for (int i = 0; i < samplesRead; i++) {
        int32_t sample = rawSamples[i] >> 8;
        sum += sample;
    }

    double mean = sum / samplesRead;

    // Second pass: calculate RMS.
    double sumOfSquares = 0.0;

    for (int i = 0; i < samplesRead; i++) {
        int32_t sample = rawSamples[i] >> 8;
        double centeredSample = sample - mean;

        sumOfSquares += centeredSample * centeredSample;
    }

    double rms = sqrt(sumOfSquares / samplesRead);

    // Prevent log10(0).
    if (rms < 1.0) {
        rms = 1.0;
    }

    const double fullScale = 8388607.0;

    dbfs = 20.0 * log10(rms / fullScale);

    return true;
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
    Serial.begin(460800);
    delay(1000);

    // Initialize MPU6050 using GPIO 4 and GPIO 5.
    Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);

    if (!mpu.begin()) {
        Serial.println("ERROR,MPU6050_not_found");

        while (true) {
            delay(100);
        }
    }

    if (!initializeMicrophone()) {
        Serial.println("ERROR,microphone_initialization_failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("STATUS,sensors_ready");

    // This header will also become the CSV header.
    Serial.println(
        "timestamp_us,ax,ay,az,gx,gy,gz,dbfs"
    );
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {
    double dbfs;

    // This blocks until one 10 ms microphone block is available.
    if (!readMicrophoneDbfs(dbfs)) {
        Serial.println("ERROR,microphone_read_failed");
        return;
    }

    sensors_event_t acceleration;
    sensors_event_t gyroscope;
    sensors_event_t temperature;

    mpu.getEvent(
        &acceleration,
        &gyroscope,
        &temperature
    );

    unsigned long timestampUs = micros();

    // Print one synchronized CSV row.
    Serial.print(timestampUs);
    Serial.print(",");

    Serial.print(acceleration.acceleration.x, 6);
    Serial.print(",");
    Serial.print(acceleration.acceleration.y, 6);
    Serial.print(",");
    Serial.print(acceleration.acceleration.z, 6);
    Serial.print(",");

    Serial.print(gyroscope.gyro.x, 6);
    Serial.print(",");
    Serial.print(gyroscope.gyro.y, 6);
    Serial.print(",");
    Serial.print(gyroscope.gyro.z, 6);
    Serial.print(",");

    Serial.println(dbfs, 2);
}