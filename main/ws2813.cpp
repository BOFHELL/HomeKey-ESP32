#include "ws2813.hpp"
#include "HardwareManager.hpp"
#include "esp_timer.h"
#include <cmath>

WsLedPixel::WsLedPixel(HardwareManager* owner, int pin, const char* type, int numLeds)
    : Pixel(pin, type), m_owner(owner), m_numLeds(numLeds), m_brightness(0.8f) {
    m_buf.resize(m_numLeds);
}

// Globale Helligkeit setzen (0–100 %)
void WsLedPixel::setBrightness(float percent) {
    m_brightness = std::clamp(percent / 100.0f, 0.0f, 1.0f);
}

// zentrale Funktion zum Schreiben auf LEDs
// Zeigt den aktuellen Buffer mit globaler Helligkeit an
void WsLedPixel::show() {
    // temporärer Buffer, damit wir m_buf nicht direkt ändern
    std::vector<Pixel::Color> scaled(m_numLeds);

    for(int i=0; i<m_numLeds; i++) {
        const Pixel::Color& c = m_buf[i];
        Pixel::Color s;

        // RGB-Werte skalieren
        s.col[0] = (uint8_t)(c.col[0] * m_brightness);  // R
        s.col[1] = (uint8_t)(c.col[1] * m_brightness);  // G
        s.col[2] = (uint8_t)(c.col[2] * m_brightness);  // B

        // W/C unverändert oder optional skalieren, falls du willst
        s.col[3] = c.col[3];
        s.col[4] = c.col[4];

        scaled[i] = s;
    }

    // HomeSpan setzen
    set(scaled.data(), m_numLeds, true);
}

void WsLedPixel::fill(Color c) {
    for (int i = 0; i < m_numLeds; i++) {
        m_buf[i] = c;
    }
    show();
}

// Off
void WsLedPixel::off() {
    fill(RGB(0,0,0));
}

// On
void WsLedPixel::on() {
    fill(RGB(m_r, m_g, m_b));
}

// Glow
void WsLedPixel::glow() {
    float t = (esp_timer_get_time()/1000ULL) % (int)m_speed;
    float brightness = (sin((t/m_speed)*2*M_PI) + 1.0f)/2.0f;
    fill(RGB(m_r*brightness, m_g*brightness, m_b*brightness));
}

// Puls
void WsLedPixel::puls() {
    float t = (esp_timer_get_time()/1000ULL) % (int)m_speed;
    float phase = t/m_speed;
    float brightness = (phase < 0.5f) ? (phase*2) : (2-phase*2);
    fill(RGB(m_r*brightness, m_g*brightness, m_b*brightness));
}

// Moving Spots
void WsLedPixel::movingSpots() {
    int pos = (millis() / (int)m_speed) % m_numLeds;
    Color base = RGB(m_r*0.5, m_g*0.5, m_b*0.5);
    for (int i=0;i<m_numLeds;i++) m_buf[i]=base;
    int pos2 = (pos + m_numLeds/2) % m_numLeds;
    m_buf[pos]  = RGB(m_r*0.8, m_g*0.8, m_b*0.8);
    m_buf[pos2] = RGB(m_r*0.8, m_g*0.8, m_b*0.8);
    show();
}

// Ambient
void WsLedPixel::ambient() {
    float factor = std::clamp(m_ambientBrightness / 100.0f, 0.0f, 1.0f);
    fill(RGB((int)(m_ambientR * factor),
             (int)(m_ambientG * factor),
             (int)(m_ambientB * factor)));
}

// Rainbow
void WsLedPixel::rainbow(uint8_t hueOffset, uint8_t sat, uint8_t val) {
    for (int i = 0; i < m_numLeds; i++) {
        uint8_t hue = (uint8_t)((i * 256 / m_numLeds) + hueOffset);
        m_buf[i].HSV(hue * 360.0 / 255.0,
                     sat / 255.0 * 100.0,
                     val / 255.0 * 100.0,
                     0);
    }
    show();
}

// setEffect
void WsLedPixel::setEffect(LedEffect effect, uint8_t r, uint8_t g, uint8_t b, float speed, float brightness) {
    m_currentEffect = effect;
    m_r = r; m_g = g; m_b = b;
    m_speed = speed;
    setBrightness(brightness);

    if (effect == LedEffect::AMBIENT) {
        m_ambientR = r;
        m_ambientG = g;
        m_ambientB = b;
        m_ambientBrightness = brightness;
    }
}

// Start Task
void WsLedPixel::startTask() {
    xTaskCreate(&WsLedPixel::taskEntry, "LedMainTask", 4096, this, 5, nullptr);
}

void WsLedPixel::taskEntry(void* pvParameter) {
    WsLedPixel* self = static_cast<WsLedPixel*>(pvParameter);
    self->taskLoop();
}

void WsLedPixel::taskLoop() {
    for (;;) {
        if (m_paused) { 
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        switch (m_currentEffect) {
            case LedEffect::ON: on(); break;
            case LedEffect::OFF: off(); break;
            case LedEffect::GLOW: glow(); break;
            case LedEffect::PULS: puls(); break;
            case LedEffect::RAINBOW: rainbow((millis()/10)%256); break;
            case LedEffect::MOVING_SPOTS: movingSpots(); break;
            case LedEffect::AMBIENT: ambient(); break;
            default: break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Preview
void WsLedPixel::startPreview(LedEffect effect, uint8_t r, uint8_t g, uint8_t b,
                              float speed, uint32_t durationMs, float brightness) {
    m_previewEffect = effect;
    m_r = r; m_g = g; m_b = b;
    m_speed = speed;
    m_previewDuration = durationMs;
    
    
    setBrightness(brightness);

    if (effect == LedEffect::AMBIENT) {
        // save ambiet preview
        m_prevAmbientR = m_ambientR;
        m_prevAmbientG = m_ambientG;
        m_prevAmbientB = m_ambientB;
        m_prevAmbientBrightness = m_ambientBrightness;
        ESP_LOGI("WsLedPixel", "Save ambient color (%d,%d,%d), brightness %.1f%%",
                 m_ambientR, m_ambientG, m_ambientB, m_ambientBrightness);
        m_ambientR = r;
        m_ambientG = g;
        m_ambientB = b;
        m_ambientBrightness = brightness;
    }
    m_paused = true;
    
    ESP_LOGI("WsLedPixel", "Starting preview effect %d with ambient override (%d,%d,%d), speed %.1f, duration %d ms, brightness %.1f%%",
             (int)effect, r, g, b, speed, durationMs, brightness);

    xTaskCreate(&WsLedPixel::previewTaskEntry, "LedPreview", 4096, this, 6, nullptr);
}

void WsLedPixel::previewTaskEntry(void* pvParameter) {
    WsLedPixel* self = static_cast<WsLedPixel*>(pvParameter);
    self->previewLoop();
    vTaskDelete(nullptr);
}

void WsLedPixel::previewLoop() {
    uint64_t start = esp_timer_get_time() / 1000ULL;
    while ((esp_timer_get_time() / 1000ULL) - start < m_previewDuration) {
        switch (m_previewEffect) {
            case LedEffect::ON: on(); break;
            case LedEffect::OFF: off(); break;
            case LedEffect::GLOW: glow(); break;
            case LedEffect::PULS: puls(); break;
            case LedEffect::RAINBOW: rainbow((millis()/10)%256); break;
            case LedEffect::MOVING_SPOTS: movingSpots(); break;
            case LedEffect::AMBIENT: ambient(); break;
            default: break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    off();
    m_paused = false;

    // reset ambiente color from nvm
    m_ambientR = m_prevAmbientR;
    m_ambientG = m_prevAmbientG;
    m_ambientB = m_prevAmbientB;
    m_ambientBrightness = m_prevAmbientBrightness;
    ESP_LOGI("WsLedPixel", "Reset ambient color, brithness: (%d,%d,%d), brightness %.1f%%",
             m_ambientR, m_ambientG, m_ambientB, m_ambientBrightness);
}
