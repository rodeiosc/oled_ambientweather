#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// === Wi-Fi ===
const char* ssid = "NOME WIFI";
const char* password = "SENHA WIFI";

// === Display com U8G2 ===
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R2, /* clock=*/2, /* data=*/0, /* reset=*/ U8X8_PIN_NONE);


// === NTP ===
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

// === Chaves AmbientWeather ===
const String apiKey = "DEFINA AQUI SEU apikey";
const String appKey = "defina aqui sei apikey";

// === Variáveis de clima ===
String tempExt = "--", humExt = "--", vento = "--", chuva = "--", pressao = "--", uv = "--";
String tempInt = "--", umidInt = "--", radiacao = "--", gust = "--", feels = "--";
String dew = "--", dirVento = "--", pressaoAbs = "--", chuvaDia = "--";

unsigned long ultimaTroca = 0;
int telaAtual = 0;
int intervaloTela = 2000;

// === Conversão para pontos cardeais ===
String direcaoCardeal(int graus) {
  const char* direcoes[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                            "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int index = (int)((graus + 11.25) / 22.5) % 16;
  return String(direcoes[index]);
}

void buscarDadosAmbientWeather() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String url = "https://api.ambientweather.net/v1/devices?apiKey=" + apiKey + "&applicationKey=" + appKey;
  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode == 200) {
      String payload = https.getString();
      StaticJsonDocument<8192> doc;
      if (!deserializeJson(doc, payload)) {
        JsonObject lastData = doc[0]["lastData"];
        tempExt = String((lastData["tempf"].as<float>() - 32.0) * 5.0 / 9.0, 1);
        humExt = String(lastData["humidity"].as<int>());
        vento = String(lastData["windspeedmph"].as<float>() * 1.60934, 1);
        chuva = String(lastData["hourlyrainin"].as<float>() * 25.4, 1);
        pressao = String(lastData["baromrelin"].as<float>() * 33.8639, 1);

        // Substituição do índice UV numérico por texto
        int uvIndex = lastData["uv"].as<int>();
        if (uvIndex <= 2) uv = "Baixo";
        else if (uvIndex <= 5) uv = "Moderado";
        else if (uvIndex <= 7) uv = "Alto";
        else if (uvIndex <= 10) uv = "Muito Alto";
        else uv = "Extremo";

        tempInt = String((lastData["tempinf"].as<float>() - 32.0) * 5.0 / 9.0, 1);
        umidInt = String(lastData["humidityin"].as<int>());
        radiacao = String(lastData["solarradiation"].as<float>(), 0);
        gust = String(lastData["windgustmph"].as<float>() * 1.60934, 1);
        feels = String((lastData["feelsLike"].as<float>() - 32.0) * 5.0 / 9.0, 1);
        dew = String((lastData["dewPoint"].as<float>() - 32.0) * 5.0 / 9.0, 1);
        dirVento = direcaoCardeal(lastData["winddir"].as<int>());
        pressaoAbs = String(lastData["baromabsin"].as<float>() * 33.8639, 1);
        chuvaDia = String(lastData["dailyrainin"].as<float>() * 25.4, 1);
      }
    }
    https.end();
  }
}

void desenharRodape() {
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  struct tm* ti = localtime(&now);

  char hora[9];
  sprintf(hora, "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);

  char data[11];
  sprintf(data, "%02d/%02d/%04d", ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900);

  display.drawHLine(0, 55, 128);  // linha separadora

  display.setFont(u8g2_font_6x12_tr);
  display.setCursor(0, 64);
  display.print(hora);

  int dataWidth = display.getStrWidth(data);
  display.setCursor(128 - dataWidth, 64);
  display.print(data);
}

void telaTexto(String titulo, String valor, String unidade) {
  display.clearBuffer();
  display.drawFrame(0, 0, 128, 55); // quadro fora do rodapé

  display.setFont(u8g2_font_9x18B_tr);
  int tituloWidth = display.getStrWidth(titulo.c_str());
  display.setCursor((128 - tituloWidth) / 2, 14);
  display.print(titulo);

  display.setFont(u8g2_font_logisoso18_tr);
  String valorUn = valor + " " + unidade;
  int valorWidth = display.getStrWidth(valorUn.c_str());
  display.setCursor((128 - valorWidth) / 2, 44);
  display.print(valorUn);

  desenharRodape();
  display.sendBuffer();
}

void telaHoraGrande() {
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  struct tm* ti = localtime(&now);

  char hora[9];
  sprintf(hora, "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);

  display.clearBuffer();
  display.drawFrame(0, 0, 128, 55); // quadro fora do rodapé
  display.setFont(u8g2_font_logisoso24_tr);
  display.setCursor(10, 40);
  display.print(hora);
  desenharRodape();
  display.sendBuffer();
}

void telaData() {
  timeClient.update();
  time_t rawtime = timeClient.getEpochTime();
  struct tm* ti = localtime(&rawtime);

  String dias[] = {"DOMINGO", "SEGUNDA", "TERCA", "QUARTA", "QUINTA", "SEXTA", "SABADO"};
  String meses[] = {"JAN", "FEV", "MAR", "ABR", "MAI", "JUN", "JUL", "AGO", "SET", "OUT", "NOV", "DEZ"};

  String semana = dias[ti->tm_wday];
  String data = String(ti->tm_mday) + " " + meses[ti->tm_mon] + " " + String(1900 + ti->tm_year);

  display.clearBuffer();
  display.drawFrame(0, 0, 128, 55); // quadro fora do rodapé
  display.setFont(u8g2_font_9x18B_tr);
  display.setCursor((128 - display.getStrWidth(semana.c_str())) / 2, 20);
  display.print(semana);
  display.setCursor((128 - display.getStrWidth(data.c_str())) / 2, 42);
  display.print(data);
  desenharRodape();
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  display.begin();
  display.clearDisplay();
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(10, 30, "Inicializando...");
  display.sendBuffer();

  timeClient.begin();
  timeClient.update();
  buscarDadosAmbientWeather();
}

void loop() {
  if (millis() - ultimaTroca > intervaloTela) {
    telaAtual = (telaAtual + 1) % 17;
    ultimaTroca = millis();

    if (telaAtual == 0) {
      buscarDadosAmbientWeather();
      intervaloTela = 8000;
    } else {
      intervaloTela = 2000;
    }
  }

  switch (telaAtual) {
    case 0: telaHoraGrande(); break;
    case 1: telaData(); break;
    case 2: telaTexto("Temperatura", tempExt, "°C"); break;
    case 3: telaTexto("Umidade", humExt, "%"); break;
    case 4: telaTexto("Chuva", chuva, "mm/h"); break;
    case 5: telaTexto("Vento", vento, "km/h"); break;
    case 6: telaTexto("Pressao", pressao, "hPa"); break;
    case 7: telaTexto("Indice UV", uv, ""); break;
    case 8: telaTexto("Temp Interna", tempInt, "°C"); break;
    case 9: telaTexto("Umid Interna", umidInt, "%"); break;
    case 10: telaTexto("Radiacao", radiacao, "W/m²"); break;
    case 11: telaTexto("Rajada Vento", gust, "km/h"); break;
    case 12: telaTexto("Sens. Termica", feels, "°C"); break;
    case 13: telaTexto("Ponto Orvalho", dew, "°C"); break;
    case 14: telaTexto("Dir. Vento", dirVento, ""); break;
    case 15: telaTexto("Pressao Abs", pressaoAbs, "hPa"); break;
    case 16: telaTexto("Chuva/hoje", chuvaDia, "mm"); break;
  }

  delay(300);
}
