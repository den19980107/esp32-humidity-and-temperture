#ifndef HARDWARE_OLED_DISPLAY_H
#define HARDWARE_OLED_DISPLAY_H

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "../core/interfaces.h"
#include "../core/logger.h"

class OLEDDisplay : public IDisplayDriver {
public:
    OLEDDisplay(int width, int height, int sdaPin, int sclPin)
        : display(width, height, &Wire, -1), width(width), height(height),
          sdaPin(sdaPin), sclPin(sclPin), initialized(false) {}
    
    ErrorCode initialize() override {
        Wire.begin(sdaPin, sclPin);
        
        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            LOG_ERROR("SSD1306 allocation failed");
            return ErrorCode::DISPLAY_INIT_FAILED;
        }
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.display();
        
        initialized = true;
        LOG_INFO("OLED display initialized");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode show(const DisplayData& data) override {
        if (!initialized) {
            return ErrorCode::DISPLAY_INIT_FAILED;
        }
        
        display.clearDisplay();
        
        if (data.showLedStatus) {
            showLedStatus(data.ledStatus);
        } else {
            showSensorData(data.sensorData);
        }
        
        // Show LED timer countdown in top right if active
        if (data.showLedTimer && data.ledTimerRemaining > 0) {
            showLedCountdown(data.ledTimerRemaining);
        }
        
        display.display();
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode clear() override {
        if (!initialized) {
            return ErrorCode::DISPLAY_INIT_FAILED;
        }
        
        display.clearDisplay();
        display.display();
        return ErrorCode::SUCCESS;
    }
    
private:
    Adafruit_SSD1306 display;
    int width, height;
    int sdaPin, sclPin;
    bool initialized;
    
    void showSensorData(const SensorData& data) {
        // Temperature
        display.setCursor(0, 0);
        display.printf("Temp: %.1fC", data.temperture);
        
        // Humidity
        display.setCursor(0, 16);
        display.printf("Humidity: %.1f%%", data.humidity);
        
        // Light level bar
        display.setCursor(0, 32);
        display.print("Light:");
        
        // Draw light level bar (0-4095 ADC range)
        int barWidth = 80;
        int barHeight = 8;
        int barX = 0;
        int barY = 48;
        
        // Border
        display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);
        
        // Fill based on light level
        int fillWidth = map(data.photoresisterValue, 0, 4095, 0, barWidth - 2);
        if (fillWidth > 0) {
            display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);
        }
        
        // Light value
        display.setCursor(85, 48);
        display.printf("%d", data.photoresisterValue);
    }
    
    void showLedStatus(bool ledOn) {
        display.setTextSize(2);
        
        // Center the text
        const char* text = ledOn ? "LED ON" : "LED OFF";
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        
        int x = (width - w) / 2;
        int y = (height - h) / 2;
        
        display.setCursor(x, y);
        display.print(text);
        
        display.setTextSize(1);  // Reset text size
    }
    
    void showLedCountdown(unsigned long remainingSeconds) {
        // Calculate minutes and seconds
        unsigned long minutes = remainingSeconds / 60;
        unsigned long seconds = remainingSeconds % 60;
        
        // Format the countdown string
        char countdownStr[16];
        if (minutes > 0) {
            snprintf(countdownStr, sizeof(countdownStr), "%lum%lus", minutes, seconds);
        } else {
            snprintf(countdownStr, sizeof(countdownStr), "%lus", seconds);
        }
        
        // Position in top right corner
        display.setTextSize(1);
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(countdownStr, 0, 0, &x1, &y1, &w, &h);
        
        int x = width - w - 2;  // 2 pixels from right edge
        int y = 0;              // Top of screen
        
        display.setCursor(x, y);
        display.print(countdownStr);
    }
};

#endif