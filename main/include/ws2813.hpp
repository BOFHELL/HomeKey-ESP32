#pragma once
#include "Pixel.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>
#include <algorithm> // für std::clamp

class HardwareManager; // Forward declaration

enum class LedEffect {
    NONE,           // 0
    OFF,            // 1
    ON,             // 2
    GLOW,           // 3
    PULS,           // 4
    RAINBOW,        // 5
    MOVING_SPOTS,   // 6
    AMBIENT         // 7
};

class WsLedPixel : public Pixel {
public:
    WsLedPixel(HardwareManager* owner, int pin, const char* type, int numLeds);

    // Dauerbetrieb
    void setEffect(LedEffect effect,
                   uint8_t r = 0,
                   uint8_t g = 0,
                   uint8_t b = 0,
                   float speed = 1000.0f,
                   float brightness=50.0f);// 50% default

    void startTask();

    // Preview (zeitlich begrenzt)
    void startPreview(LedEffect effect,
                      uint8_t r, uint8_t g, uint8_t b,
                      float speed,
                      uint32_t durationMs = 6000,
                      float brightness=80.0f); // 50% default

    // Grundfunktionen
    void rainbow(uint8_t hueOffset, uint8_t sat = 255, uint8_t val = 255);
    void fill(Color c);
    void on();
    void off();
    void glow();
    void puls();
    void movingSpots();
    void ambient();

    // Globale Helligkeit
    void setBrightness(float percent);
private:
    void show(); // skaliert Buffer mit m_brightness und setzt LEDs

    // Dauerbetrieb
    static void taskEntry(void* pvParameter);
    void taskLoop();

    // Preview
    static void previewTaskEntry(void* pvParameter);
    void previewLoop();

    HardwareManager* m_owner;
    
    LedEffect m_currentEffect = LedEffect::NONE;
    LedEffect m_previewEffect = LedEffect::NONE;
    
    int m_numLeds;
    std::vector<Color> m_buf;

    // Parameter für Effekte
    uint8_t m_r = 0, m_g = 0, m_b = 0;
    float   m_speed = 1000.0f;
    uint32_t m_previewDuration = 0;
    
    bool m_paused = false;
    // Zwischenspeicher für Ambient
    uint8_t m_ambientR = 0;
    uint8_t m_ambientG = 0;
    uint8_t m_ambientB = 0;
    float   m_ambientBrightness = 80.0f; // Prozent
    // temporäre Speicherung für Preview
    uint8_t m_prevAmbientR = 0;
    uint8_t m_prevAmbientG = 0;
    uint8_t m_prevAmbientB = 0;
    float   m_prevAmbientBrightness = 80.0f; // Prozent
    
    
    // Globale Helligkeit
    float m_brightness;
};
