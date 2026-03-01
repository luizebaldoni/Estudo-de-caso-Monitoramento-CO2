# Monitoramento de CO₂, Temperatura e Umidade com ESP32

Este projeto consiste em um sistema robusto de monitoramento ambiental focado na detecção de níveis de **Dióxido de Carbono (CO₂)**. O software foi desenvolvido com foco em **tolerância a falhas**, garantindo que, mesmo em quedas de conectividade, os dados sejam armazenados localmente e sincronizados posteriormente.

## 🚀 Funcionalidades

* **Leitura de Sensores:** Monitoramento de CO₂ (MQ135), Temperatura e Umidade (DHT22).
* **Integração IoT:** Envio de dados em tempo real para o dashboard do **ThingSpeak**.
* **Alertas Instantâneos:** Notificações automáticas via **WhatsApp** (CallMeBot) quando o sistema está ativo.
* **Resiliência de Dados:** Sistema de buffer local utilizando **SPIFFS** (memória flash interna) para armazenar leituras caso o Wi-Fi esteja indisponível.
* **Sincronização NTP:** Registro de data e hora (timestamp) real em todas as medições.
* **Recuperação Automática:** Processamento de logs pendentes assim que a conexão é restabelecida.

## 🛠️ Hardware Necessário

* **Microcontrolador:** ESP32 DevKit V1
* **Sensor de Gases:** MQ135 (CO₂)
* **Sensor de Clima:** DHT22 (Temperatura e Umidade)
* **Conectividade:** Rede Wi-Fi 2.4GHz



## 📋 Pré-requisitos e Bibliotecas

O projeto utiliza as seguintes bibliotecas (configuradas no `platformio.ini` fornecido):

* `ThingSpeak`: Comunicação com a plataforma de IoT.
* `UrlEncode`: Formatação de mensagens para a API do WhatsApp.
* `DHT sensor library`: Comunicação com o sensor de temperatura.
* `SPIFFS` & `WiFi`: Bibliotecas nativas do framework Arduino para ESP32.

## ⚙️ Configuração

Para rodar o projeto, edite as seguintes variáveis no código:

1.  **Wi-Fi:** `SSID_REDE` e `SENHA_REDE`.
2.  **ThingSpeak:** `chave_escrita_thingspeak`.
3.  **WhatsApp:** `phoneNumber1` (ex: +55...) e `apiKey` (obtida com o bot).

## 📉 Lógica de Funcionamento

O sistema opera em um ciclo de **15 minutos** (900.000 ms) para evitar sobrecarga de dados:

1.  **Coleta:** Realiza a leitura dos sensores e gera um timestamp via NTP.
2.  **Conexão:** Tenta conectar ao Wi-Fi (limite de 10 tentativas).
3.  **Fluxo de Sucesso:**
    * Envia os dados para o ThingSpeak.
    * Dispara o alerta de WhatsApp.
    * Salva no log local como `enviado`.
    * Verifica se existem dados `pendentes` no histórico e tenta sincronizá-los.
4.  **Fluxo de Contingência:**
    * Em caso de falha de Wi-Fi, salva o registro como `pendente` no arquivo `dados.csv`.
    * Entra em modo de espera de rede por 15 minutos para economizar recursos antes da próxima tentativa.

## 📁 Estrutura do Log (SPIFFS)

Os dados são salvos no formato CSV:
`Timestamp, CO2(ppm), Temperatura(°C), Umidade(%), Status`

Exemplo:
`2025-06-11 14:30:05, 450.20, 25.5, 60.0, enviado`

---

## 👩‍💻 Informações do Projeto
* **Última Atualização:** 11/06/2025.
* **Ambiente de Desenvolvimento:** PlatformIO / CLion JetBrains.
