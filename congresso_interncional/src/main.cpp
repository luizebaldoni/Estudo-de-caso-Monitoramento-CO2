/*
 * MONITORAMENTO DE CO2, ENVIO DE DADOS PARA THINGSPEAK E ALERTAS VIA WHATSAPP
 * Autor: omitido para revisão
 * Academica de Engenharia de Computação
 * Ultima atualização: 11/06/2025
 *
 * DESCRIÇÃO:
 * Este código utiliza o sensor MQ135 para monitorar níveis de CO2, enviando os dados para a plataforma ThingSpeak e alertas automáticos via WhatsApp.
 * Ele realiza a leitura de sensores de CO2, temperatura e umidade, e envia os dados em intervalos regulares.
 * Caso os níveis de CO2 sejam elevados, um alerta é enviado via WhatsApp para um número pré-configurado.
 *
 * REQUISITOS:
 * - Sensor MQ135 para CO2
 * - Sensor DHT22 para temperatura e umidade
 * - Conexão Wi-Fi para envio de dados ao ThingSpeak e envio de alertas via WhatsApp
 * - Chave de API do serviço WhatsApp (CallMeBot)
 *
 * MODIFICAÇÕES PARA TOLERÂNCIA A FALHAS:
 * - Limite de 10 tentativas de conexão Wi-Fi; após falha, espera 15 minutos e reinicia o ciclo.
 * - Ciclo de amostragem a cada 15 minutos.
 * - Buffer local em SPIFFS: cada leitura gera um registro em arquivo de log com timestamp, valores e status da transmissão.
 * - Em caso de falha de conectividade, os dados permanecem armazenados localmente e são reenviados quando a conexão for restabelecida.
 */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <SPIFFS.h>
#include <time.h>

////// DEFINIÇÕES DE REDE E SENSOR //////

#define SSID_REDE "SSID"                 // Nome da rede Wi-Fi
#define SENHA_REDE "Senha"                // Senha da rede Wi-Fi
#define INTERVALO_AMOSTRAGEM 900000       // 15 minutos em milissegundos
#define valor_sensorMQ 34                  // Pino analógico conectado ao sensor MQ135
#define DHTPIN 2                           // Pino onde o sensor DHT está conectado
#define MAX_TENTATIVAS_WIFI 10             // Número máximo de tentativas de conexão Wi-Fi
#define TEMPO_ESPERA_FALHA 900000          // 15 minutos de espera após falha de conexão

////// CONFIGURAÇÕES DE SENSORES, REDE E API //////

char endereco_api_thingspeak[] = "api.thingspeak.com"; // Endereço do servidor ThingSpeak
String chave_escrita_thingspeak = "WRITE_KEY";  // Chave de escrita da API ThingSpeak

// Configuração do envio de alertas via WhatsApp
String phoneNumber1 = "+55559000000";  // Número de celular no formato internacional
String apiKey = "000000";              // Chave de autenticação para o serviço de mensagens WhatsApp

// Arquivo de log
const char* arquivo_log = "/dados.csv";

// Variáveis de controle de tempo
unsigned long ultima_leitura = 0;
unsigned long tempo_inicio_falha = 0;
bool em_espera_por_falha = false;

////// DECLARAÇÃO DE FUNÇÕES //////

bool conecta_wifi_limitado();
void init_spiffs();
void getTimestamp(char* buffer, size_t len);
bool enviar_para_thingspeak(float co2, float temperatura, float umidade);
void enviar_alertas_whatsapp(float co2, float temperatura, float umidade);
void appendLog(float co2, float temperatura, float umidade, const char* status);
void processar_pendentes();
float lerTemperatura();
float lerUmidade();
void lerDHT22(float &temperatura, float &umidade);
void sincronizarNTP();

////// FUNÇÃO PARA CONECTAR WI-FI COM LIMITE DE TENTATIVAS //////
bool conecta_wifi_limitado() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.print("Conectando-se à rede Wi-Fi: ");
    Serial.println(SSID_REDE);
    WiFi.begin(SSID_REDE, SENHA_REDE);

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < MAX_TENTATIVAS_WIFI) {
        delay(1000);
        Serial.print(".");
        tentativas++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conectado com sucesso!");
        return true;
    } else {
        Serial.println("Falha na conexão após " + String(MAX_TENTATIVAS_WIFI) + " tentativas.");
        return false;
    }
}

////// INICIALIZA O SPIFFS //////
void init_spiffs() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Erro ao montar SPIFFS");
        return;
    }
    Serial.println("SPIFFS montado com sucesso");
}

////// OBTÉM TIMESTAMP VIA NTP (SE CONECTADO) //////
void getTimestamp(char* buffer, size_t len) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        strftime(buffer, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        snprintf(buffer, len, "N/A");
    }
}

////// SINCRONIZAÇÃO COM NTP //////
void sincronizarNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Sincronizando NTP");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Hora atual: ");
    Serial.println(asctime(&timeinfo));
}

////// ENVIA DADOS PARA THINGSPEAK VIA HTTP //////
bool enviar_para_thingspeak(float co2, float temperatura, float umidade) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    String url = "http://api.thingspeak.com/update";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "api_key=" + chave_escrita_thingspeak +
                      "&field1=" + String(co2) +
                      "&field2=" + String(temperatura) +
                      "&field3=" + String(umidade);

    int httpCode = http.POST(postData);
    bool sucesso = (httpCode == 200);
    if (sucesso) {
        Serial.println("Dados enviados ao ThingSpeak com sucesso!");
    } else {
        Serial.print("Falha no envio ao ThingSpeak. Código HTTP: ");
        Serial.println(httpCode);
    }
    http.end();
    return sucesso;
}

////// ENVIA ALERTA VIA WHATSAPP //////
void enviar_alertas_whatsapp(float co2, float temperatura, float umidade) {
    String message = "⚠ *AVISO!* ⚠ \n \n Nível de CO²: " + String(co2) + " ppm \nTemperatura: " + String(temperatura) + " ºC \nUmidade: " + String(umidade) + "% \n\n  > Mensagem gerada automaticamente pelo sistema de monitoramento após detecção.";

    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber1 + "&apikey=" + apiKey + "&text=" + urlEncode(message);
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(url);
    if (httpResponseCode == 200) {
        Serial.println("Mensagem WhatsApp enviada com sucesso!");
    } else {
        Serial.println("Erro ao enviar mensagem WhatsApp. Código HTTP: " + String(httpResponseCode));
    }
    http.end();
}

////// ADICIONA REGISTRO AO ARQUIVO DE LOG //////
void appendLog(float co2, float temperatura, float umidade, const char* status) {
    File file = SPIFFS.open(arquivo_log, FILE_APPEND);
    if (!file) {
        Serial.println("Erro ao abrir arquivo de log para escrita");
        return;
    }

    char timestamp[20];
    getTimestamp(timestamp, sizeof(timestamp));

    file.print(timestamp);
    file.print(",");
    file.print(co2);
    file.print(",");
    file.print(temperatura);
    file.print(",");
    file.print(umidade);
    file.print(",");
    file.println(status);
    file.close();
    Serial.println("Registro adicionado ao log: " + String(timestamp));
}

////// PROCESSA DADOS PENDENTES (REENVIA E ATUALIZA STATUS) //////
void processar_pendentes() {
    if (!SPIFFS.exists(arquivo_log)) {
        return; // Arquivo não existe, nada a processar
    }

    File file = SPIFFS.open(arquivo_log, FILE_READ);
    if (!file) {
        Serial.println("Erro ao abrir arquivo de log para leitura");
        return;
    }

    // Ler todas as linhas para memória
    String linhas[100]; // Limite de 100 linhas pendentes para evitar estouro
    int numLinhas = 0;
    while (file.available() && numLinhas < 100) {
        linhas[numLinhas] = file.readStringUntil('\n');
        linhas[numLinhas].trim();
        if (linhas[numLinhas].length() > 0) {
            numLinhas++;
        }
    }
    file.close();

    // Reescrever o arquivo com status atualizados
    File outFile = SPIFFS.open(arquivo_log, FILE_WRITE);
    if (!outFile) {
        Serial.println("Erro ao reabrir arquivo para escrita");
        return;
    }

    for (int i = 0; i < numLinhas; i++) {
        String linha = linhas[i];
        // Formato esperado: timestamp,co2,temp,umid,status
        int primeiroVirgula = linha.indexOf(',');
        int segundoVirgula = linha.indexOf(',', primeiroVirgula + 1);
        int terceiroVirgula = linha.indexOf(',', segundoVirgula + 1);
        int quartoVirgula = linha.indexOf(',', terceiroVirgula + 1);

        if (primeiroVirgula == -1 || segundoVirgula == -1 || terceiroVirgula == -1 || quartoVirgula == -1) {
            // Linha mal formatada, ignorar
            continue;
        }

        String timestamp = linha.substring(0, primeiroVirgula);
        float co2 = linha.substring(primeiroVirgula + 1, segundoVirgula).toFloat();
        float temp = linha.substring(segundoVirgula + 1, terceiroVirgula).toFloat();
        float umid = linha.substring(terceiroVirgula + 1, quartoVirgula).toFloat();
        String status = linha.substring(quartoVirgula + 1);

        if (status == "pendente") {
            // Tentar enviar
            if (enviar_para_thingspeak(co2, temp, umid)) {
                // Enviado com sucesso, marcar como enviado
                outFile.print(timestamp);
                outFile.print(",");
                outFile.print(co2);
                outFile.print(",");
                outFile.print(temp);
                outFile.print(",");
                outFile.print(umid);
                outFile.println(",enviado");
                Serial.println("Regudo reenviado: " + timestamp);
            } else {
                // Falha no envio, manter pendente
                outFile.println(linha);
            }
        } else {
            // Já enviado, manter
            outFile.println(linha);
        }
    }
    outFile.close();
}

////// LEITURA DO SENSOR DHT22 (IMPLEMENTAÇÃO MANUAL) //////
void lerDHT22(float &temperatura, float &umidade) {
    uint8_t laststate = HIGH;
    uint8_t j = 0;
    uint8_t data[5] = {0};

    // Inicia a comunicação com o sensor (sinal de start)
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(20);

    pinMode(DHTPIN, INPUT); // Muda para leitura

    // Verifica a resposta do sensor
    laststate = digitalRead(DHTPIN);
    if (laststate == LOW) {
        delayMicroseconds(80);
        laststate = digitalRead(DHTPIN);
    }

    // Lê os 40 bits de dados
    for (j = 0; j < 40; j++) {
        while (digitalRead(DHTPIN) == LOW);
        delayMicroseconds(40);
        if (digitalRead(DHTPIN) == HIGH) {
            data[j / 8] |= (1 << (7 - (j % 8))); // Armazena o bit
        }
        while (digitalRead(DHTPIN) == HIGH);
    }

    // Verifica a soma de verificação
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (data[4] == sum) {
        umidade = (float)((data[0] << 8) + data[1]) / 10; // Umidade
        temperatura = (float)((data[2] << 8) + data[3]) / 10; // Temperatura
    } else {
        Serial.println("Falha na leitura do sensor DHT22.");
        temperatura = -1;
        umidade = -1;
    }
}

////// CONFIGURAÇÃO INICIAL //////
void setup() {
    Serial.begin(9600);
    init_spiffs();

    // Tenta conectar Wi-Fi com limite de tentativas
    if (conecta_wifi_limitado()) {
        sincronizarNTP(); // Se conectou, obtém hora
    } else {
        Serial.println("Wi-Fi não disponível. Operando sem conectividade.");
        em_espera_por_falha = true;
        tempo_inicio_falha = millis();
    }

    ultima_leitura = millis(); // Inicializa o contador
}

////// LOOP PRINCIPAL //////
void loop() {
    unsigned long agora = millis();

    // Se estiver em espera por falha, verifica se já passou 15 minutos
    if (em_espera_por_falha) {
        if (agora - tempo_inicio_falha >= TEMPO_ESPERA_FALHA) {
            em_espera_por_falha = false;
            Serial.println("Fim do período de espera. Tentando reconectar...");
        } else {
            // Ainda em espera, não faz nada
            delay(1000);
            return;
        }
    }

    // Verifica se é hora de fazer uma nova leitura (a cada 15 minutos)
    if (agora - ultima_leitura >= INTERVALO_AMOSTRAGEM) {
        ultima_leitura = agora;

        // Leitura dos sensores
        float temperatura = 0;
        float umidade = 0;
        lerDHT22(temperatura, umidade);

        double MQ_bruto = analogRead(valor_sensorMQ);
        double voltagem = (MQ_bruto / 4095.0) * 4.95;
        double Rs = (5.0 - voltagem) / voltagem;
        double ratio = Rs / 10.0;
        double CO2 = 110.47 * pow(ratio, -2.862);

        // Exibe dados no monitor serial
        Serial.print("CO2: ");
        Serial.print(CO2);
        Serial.print(" ppm | Temp: ");
        Serial.print(temperatura);
        Serial.print(" °C | Umid: ");
        Serial.println(umidade);

        // Gera timestamp
        char timestamp[20];
        getTimestamp(timestamp, sizeof(timestamp));
        Serial.print("Timestamp: ");
        Serial.println(timestamp);

        // Tenta conectar Wi-Fi (com limite)
        bool wifiConectado = conecta_wifi_limitado();

        if (wifiConectado) {
            // Se conseguiu conectar, tenta enviar os dados atuais
            bool envioOk = enviar_para_thingspeak(CO2, temperatura, umidade);
            if (envioOk) {
                // Enviado com sucesso: registra como enviado e envia alerta WhatsApp
                appendLog(CO2, temperatura, umidade, "enviado");
                enviar_alertas_whatsapp(CO2, temperatura, umidade);
            } else {
                // Falha no envio: registra como pendente
                appendLog(CO2, temperatura, umidade, "pendente");
            }

            // Processa dados pendentes do arquivo (reenvia)
            processar_pendentes();
        } else {
            // Não conseguiu conectar: registra como pendente e entra em espera por falha
            appendLog(CO2, temperatura, umidade, "pendente");
            Serial.println("Falha na conexão Wi-Fi. Entrando em espera por 15 minutos.");
            em_espera_por_falha = true;
            tempo_inicio_falha = millis();
        }
    }

    delay(1000); // Pequeno delay para evitar uso excessivo da CPU
}