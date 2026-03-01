# Monitoramento de CO₂ com ESP32, ThingSpeak e Alertas via WhatsApp

Este projeto implementa um sistema de monitoramento de qualidade do ar utilizando o sensor MQ135 para medição de CO₂, juntamente com um sensor DHT22 para temperatura e umidade. Os dados são enviados periodicamente para a plataforma ThingSpeak e, em caso de níveis elevados de CO₂, um alerta é enviado automaticamente via WhatsApp.

O firmware foi desenvolvido com foco em **tolerância a falhas**, garantindo que nenhuma leitura seja perdida mesmo em situações de instabilidade de rede.

## 📋 Funcionalidades

- Leitura dos sensores:
  - CO₂ (MQ135)
  - Temperatura e umidade (DHT22)
- Envio periódico dos dados para o ThingSpeak (a cada 15 minutos)
- Alerta via WhatsApp (CallMeBot) quando o nível de CO₂ excede um limiar definido
- **Tolerância a falhas**:
  - Limite de 10 tentativas de conexão Wi-Fi; após falha, sistema aguarda 15 minutos antes de nova tentativa
  - Buffer local em memória flash (SPIFFS): cada leitura é registrada em um arquivo CSV com timestamp e status de envio
  - Reenvio automático de dados pendentes quando a conexão é restabelecida

## 🧰 Hardware Necessário

- Microcontrolador ESP32 (qualquer modelo com suporte ao Arduino core)
- Sensor MQ135
- Sensor DHT22
- Protoboard e jumpers
- Fonte de alimentação (USB ou bateria)

### Conexões

| Componente | Pino ESP32 |
|------------|------------|
| MQ135 (pino analógico) | GPIO 34 |
| DHT22 (data) | GPIO 2 |
| VCC (ambos) | 3.3V |
| GND (ambos) | GND |

## 📦 Dependências e Bibliotecas

O projeto é desenvolvido no PlatformIO com framework Arduino. As seguintes bibliotecas são necessárias:

- `WiFi.h` – nativa do ESP32
- `HTTPClient.h` – nativa do ESP32
- `SPIFFS.h` – nativa do ESP32 (sistema de arquivos)
- `UrlEncode` – para codificação de mensagens WhatsApp ([plageoj/UrlEncode](https://github.com/plageoj/UrlEncode))
- (Opcional) `DHT sensor library` da Adafruit – caso prefira usar a biblioteca em vez da leitura manual

No arquivo `platformio.ini` as dependências já estão listadas:

```ini
lib_deps = 
    mathworks/ThingSpeak@^2.0.0
    adafruit/Adafruit GFX Library@^1.11.11
    adafruit/Adafruit SSD1306@^2.5.13
    plageoj/UrlEncode@^1.0.1
    adafruit/DHT sensor library@^1.4.6
