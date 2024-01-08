#include <Arduino.h>

#define FIRE_THRESHOLD_TEMP 30.0
#define FIRE_THRESHOLD_HUMIDITY 70.0
#define FIRE_THRESHOLD_GAS 1 // Ngưỡng gas (0 hoặc 1)
#define FIRE_THRESHOLD_PM 35.0 // Ngưỡng bụi mịn
#define DST_THRESHOLD 0.6
#define NUM_SAMPLES 20

float temperatureSamples[NUM_SAMPLES];
float humiditySamples[NUM_SAMPLES];
float gasSamples[NUM_SAMPLES]; // Mẫu đo khí gas
float pmSamples[NUM_SAMPLES]; // Mẫu đo bụi mịn
int sampleIndex = 0;

// Định nghĩa điểm và giá trị tin tưởng cho nhiệt độ
#define NUM_POINTS_TEMP 5
float temperaturePoints[NUM_POINTS_TEMP] = {0.0, 20.0, 30.0, 35.0, 45.0};
float temperatureMassValues[NUM_POINTS_TEMP] = {0.0, 0.0, 0.2, 0.5, 1.0};

// Định nghĩa điểm và giá trị tin tưởng cho độ ẩm
#define NUM_POINTS_HUMI 4
float humidityPoints[NUM_POINTS_HUMI] = {30.0, 50.0, 70.0, 100.0};
float humidityMassValues[NUM_POINTS_HUMI] = {1.0, 0.5, 0.1, 0.0};

// Định nghĩa điểm và giá trị tin tưởng cho PM
#define NUM_POINTS_PM 3
float pmPoints[NUM_POINTS_PM] = {0.0, 20.0, 50.0};
float pmMassValues[NUM_POINTS_PM] = {0.0, 0.5, 1.0};

float lagrangeInterpolation(float x, const float xPoints[], const float yPoints[], int numPoints) {
    float result = 0.0;

    for (int i = 0; i < numPoints; i++) {
        float term = yPoints[i];
        for (int j = 0; j < numPoints; j++) {
            if (j != i) {
                term *= (x - xPoints[j]) / (xPoints[i] - xPoints[j]);
            }
        }
        result += term;
    }

    return result;
}

float calculateDempstersRule(float mass1, float mass2) {
    return (mass1 * (1 - mass2) + mass2 * (1 - mass1)) / (1 - (mass1 * mass2));
}

bool isFireDetected(float temperature, float humidity, int gas, float pm) {
    float mass1 = lagrangeInterpolation(temperature, temperaturePoints, temperatureMassValues, NUM_POINTS_TEMP);
    float mass2 = lagrangeInterpolation(humidity, humidityPoints, humidityMassValues, NUM_POINTS_HUMI);
    float mass3 = (gas >= FIRE_THRESHOLD_GAS) ? 1.0 : 0.0;
    float mass4 = lagrangeInterpolation(pm, pmPoints, pmMassValues, NUM_POINTS_PM);

    float combinedMass = calculateDempstersRule(mass1, mass2);
    combinedMass = calculateDempstersRule(combinedMass, mass3);
    combinedMass = calculateDempstersRule(combinedMass, mass4);

    return combinedMass >= DST_THRESHOLD;
}