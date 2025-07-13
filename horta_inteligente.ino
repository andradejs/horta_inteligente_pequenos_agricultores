#include <Preferences.h>

/*******************************************************************
    A telegram bot for your ESP32 that demonstrates a bot
    that show bot action message

    Parts:
    ESP32 D1 Mini style Dev board* - http://s.click.aliexpress.com/e/C6ds4my
    (or any ESP32 board)

      = Affilate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Example originally written by Vadim Sinitski 

    Library written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <FirebaseClient.h>
#include <time.h>
#include <DHT.h>
#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <HTTPUpdate.h>


// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSy***************"

// User Email and password that already registered or added in your project.
#define USER_EMAIL "seu_email@gmail.com"
#define USER_PASSWORD "************"
#define DATABASE_URL "https://seu-projeto.firebaseio.com"

#define FIREBASE_PROJECT_ID "seu-projeto"

// Wifi network station credentials
#define WIFI_SSID "NOME_DA_REDE"
#define WIFI_PASSWORD "********"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "***********************"


const unsigned long BOT_MTBS = 1000;  // mean time between scan messages

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;
bool Start = false;

char *id_client = NULL;

//Pinos dos  sensores e atuadores
#define DHT_POWER 4  //por motivos de instabilidade de rede tive que colocar um pino GPIO como power para reiniciar o sensor
#define DHT_PIN 13
#define RELE 5
#define FLOW_SENSOR_PIN 21
#define CAPACITIVE_SENSOR 34

//variaveis de controle
#define SOIL_MOISTURE_LIMIT_MAX 60
#define SOIL_MOISTURE_LIMIT_MIN 30
#define CALIBRATION_SENSOR_FLOW 450.0 

//variavel de data e hora
struct tm currentTime;

//variavel do sensor de fluxo
volatile int pulseCount = 0;

//variavel para checar se houve irrigaçao
bool irrigatiionCheck = false;

//clasee DHT
DHT dht(DHT_PIN, DHT22);

//declaraçoes de funçoes utilizadas para o firebase
void authHandler();

void printResult(AsyncResult &aResult);

void printError(int code, const String &msg);

String getTimestampString(uint64_t sec, uint32_t nano);

DefaultNetwork network;

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

FirebaseApp app;

WiFiClientSecure ssl_client;

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

Firestore::Documents Docs;

AsyncResult aResult_no_callback;

//variavel para checagem de irrigaçao

unsigned long lastIrrigationCheckTime = 0;


Preferences prefs;
bool pulandoReinicio = false;
int ultimoMinutoReinicio = -1;  // novo: evita reinício repetido


void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (bot.messages[i].type == "message" && bot.messages[i].hasDocument == true) {

      httpUpdate.rebootOnUpdate(false);
      t_httpUpdate_return ret = (t_httpUpdate_return)3;

      if (bot.messages[i].file_caption == "update firmware") {
        bot.sendMessage(bot.messages[i].chat_id, "Firmware writing...", "");
        ret = httpUpdate.update(secured_client, bot.messages[i].file_path);
      }
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          bot.sendMessage(bot.messages[i].chat_id, "HTTP_UPDATE_FAILED Error (" + String(httpUpdate.getLastError()) + "): " + httpUpdate.getLastErrorString(), "");
          break;

        case HTTP_UPDATE_NO_UPDATES:
          bot.sendMessage(bot.messages[i].chat_id, "HTTP_UPDATE_NO_UPDATES", "");
          break;

        case HTTP_UPDATE_OK:
          bot.sendMessage(bot.messages[i].chat_id, "UPDATE OK.\nRestarting...", "");
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
          ESP.restart();
          break;
        default:
          break;
      }

    } else {

      if (text == "/opcoes") {
        bot.sendChatAction(chat_id, "typing");
        String option = from_name + " Digite ou toque em uma das opções: \n\n";

        option += "1- /humidade_solo \n";
        option += "2- /humidade_ar \n";
        option += "3- /temperatura \n";
        option += "4- /grafico \n";

        bot.sendMessage(chat_id, option);
      }

      else if (text == "/start") {
        String welcome = "Bem-vindo ao Horta Simples  " + from_name + ".\n";
        welcome += "Um chat bot para monitorar um protótipo de horta inteligente\n\n";
        welcome += "clique em /opcoes para começar\n";
        bot.sendMessage(chat_id, welcome);
      }

      else if (text == "/irrigar") {
        digitalWrite(RELE, LOW);
        delay(5000);
        digitalWrite(RELE, HIGH);
        bot.sendMessage(chat_id, "foi irrigado");

      }

      else if (text == "/humidade_solo") {
        float soilMoisture = getCurrentSoilMoisture();
        bot.sendMessage(chat_id, String("Umidade do Solo: ") + String(soilMoisture) + "%");
      }

      else if (text == "/humidade_ar") {
        float humidity = getCurrentHumidity();
        bot.sendMessage(chat_id, String("Umidade do Ar: ") + String(humidity) + "%");
      }

      else if (text == "/temperatura") {
        float temperature = getCurrentTemperature();
        bot.sendMessage(chat_id, String("Temperatura: ") + String(temperature) + "°C");
      }

      else if (text == "/grafico") {
        bot.sendMessage(chat_id, "https://t.me/hortainteligente_bot/HortaSimples");
      }

      else if (text == "/reiniciardht") {
        reiniciarDHT();
        bot.sendMessage(chat_id, String("reiniciou DHT22 "));
      }

      else if (text == "/temperaturaesp") {
        float temp = temperatureRead();
        bot.sendMessage(chat_id, String("Temperatura do esp32: ") + String(temp) + "°C");
      }

      else if (text == "/sinalwifi") {
        long rssi = WiFi.RSSI();
        bot.sendMessage(chat_id, String("Sinal do wifi: ") + String(rssi) + "dBm");
      }

      else if (text == "/ativarmensagem") {
        if (id_client != NULL) {
          free(id_client);  // Libera a memória antiga se já estiver alocada
        }

        id_client = (char *)malloc(strlen(chat_id.c_str()) + 1);
        strcpy(id_client, chat_id.c_str());
        bot.sendMessage(chat_id, String("mensagem habilitada"));
      }

      else if (text == "/desativarmensagem") {
        if (id_client != NULL) {
          free(id_client);   // Libera a memória
          id_client = NULL;  // Define como NULL
          printf("Nome agora é NULL\n");
        }
        bot.sendMessage(chat_id, String("mensagem habilitada"));
      }

      else {
        bot.sendChatAction(chat_id, "typing");
        String option = from_name + " Você só pode escolher uma das opções abaixo: \n\n";

        option += "1- /humidade_solo \n";
        option += "2- /humidade_ar \n";
        option += "3- /temperatura \n";
        option += "4- /grafico \n";

        bot.sendMessage(chat_id, option);
      }
    }
  }
}

void checkWiFiConnection() {


  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Tentando conectar ao WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Add root certificate for api.telegram.org
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
      Serial.print(".");
      delay(500);
    }

    Serial.println("\nWifi conectado!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    synchronize();

    connectToFirebase();
  }
}

void synchronize() {
  Serial.println("Realizando sincronia");
  configTime(-10800, 0, "pool.ntp.org");  // get UTC time via NTP


  while (!getLocalTime(&currentTime)) {
    Serial.println("obtendo data e hora");
    delay(1000);
  }
  Serial.printf("Data atual: %02d/%02d/%04d\n",
                currentTime.tm_mday, currentTime.tm_mon + 1, currentTime.tm_year + 1900);
  Serial.printf("Hora atual: %02d:%02d:%02d\n",
                currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);
}

void irrigate(float soilMoisture) {
  Serial.println("vai irrigar");

  unsigned long startIrrigation = millis();
  while (soilMoisture < SOIL_MOISTURE_LIMIT_MAX) {

    Serial.println("ta irrigando");
    //o rele funciona de maneira invertida
    digitalWrite(RELE, LOW);
    delay(1000);
    soilMoisture = getCurrentSoilMoisture();
    // soilMoisture = 60;

    if (millis() - startIrrigation >= 60000 && pulseCount < 30) {
      Serial.println("não tem agua para a irrigaçao");
      irrigatiionCheck = false;
      break;
    }
  }

  //o rele funciona de maneira invertida
  digitalWrite(RELE, HIGH);
  Serial.println("irrigou");
}

int getCurrentSoilMoisture() {

  int amountMeasurement = 50;
  int total = 0;

  for (int i = 0; i < amountMeasurement; i++) {
    total += analogRead(CAPACITIVE_SENSOR);
    delay(10);
  }

  int valueSensor = (total / amountMeasurement);

  int mappedValue = constrain(map(valueSensor, 2488, 1022, 0, 100), 0, 100);
  return mappedValue;
}

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

float measureAmountWater() {
  float totalLiters = (pulseCount / CALIBRATION_SENSOR_FLOW);
  return totalLiters;
}

float getCurrentHumidity() {
  float humidity;
  int attempts = 5;

  for (int i = 0; i < attempts; i++) {
    humidity = dht.readHumidity();
    if (!isnan(humidity)) {
      return humidity;
    }
    reiniciarDHT();  //reinicia o sensor
  }

  Serial.println("Erro: não foi possível obter um valor válido do DHT22.");
  return -1;  // Retorna um valor de erro ao invés de NaN
}


float getCurrentTemperature() {
  float temperature;
  int attempts = 5;

  for (int i = 0; i < attempts; i++) {
    temperature = dht.readTemperature();
    if (!isnan(temperature)) {
      return temperature;
    }
    reiniciarDHT();  //reinicia o sensor
  }

  Serial.println("Erro: não foi possível obter um valor válido do DHT22.");
  return -1;  // Retorna um valor de erro ao invés de NaN
}

void reiniciarDHT() {
  Serial.println("Reiniciando DHT22...");
  digitalWrite(DHT_POWER, LOW);   // Desliga sensor
  delay(2000);                    // Aguarde um tempo para reset
  digitalWrite(DHT_POWER, HIGH);  // Religa o sensor
  delay(1000);                    // Tempo para estabilizar
}

void tryCreateDocumentWithRetry(Document<Values::Value> &doc, const String &documentPath, int maxAttempts = 3) {

  for (int i = 0; i < maxAttempts; i++) {
    Serial.printf("🔁 Tentativa %d de %d...\n", i + 1, maxAttempts);

    String payload = Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath, DocumentMask(), doc);

    if (aClient.lastError().code() == 0) {

      if (id_client != NULL) {
        bot.sendMessage(id_client, "✅ Documento criado com sucesso:");
      }

      Serial.println(payload);
      return;
    }

    Firebase.printf("❌ Tentativa %d falhou. Erro: %s (código %d)\n", i + 1,
                    aClient.lastError().message().c_str(), aClient.lastError().code());

    if (id_client != NULL) {
      bot.sendMessage(id_client, aClient.lastError().message().c_str());
    }
    delay(2000);  // Espera 2 segundos antes da próxima tentativa
  }

  Serial.println("🚫 Todas as tentativas falharam.");
}


void createDocumentFirebase(
  float totalLiters,
  float soilMoistureBefore,
  float soilMoistureAfter,
  float humidityBefore,
  float humidityAfter,
  float temperatureBefore,
  float temperatureAfter,
  bool irrigationCheck) {

  authHandler();

  Docs.loop();

  if (app.ready()) {


    // We will create the documents in this parent path "test_doc_creation/doc_1/col_1/data_?"
    // (collection > document > collection > documents that contains fields).

    // Note: If new document created under non-existent ancestor documents as in this example
    // which the document "test_doc_creation/doc_1" does not exist, that document (doc_1) will not appear in queries and snapshot
    // https://cloud.google.com/firestore/docs/using-console#non-existent_ancestor_documents.

    // In the console, you can create the ancestor document "test_doc_creation/doc_1" before running this example
    // to avoid non-existent ancestor documents case.

    String documentPath = "horta/";
    time_t now = time(NULL);

    now += 10800;
    documentPath += now;

    Values::BooleanValue wasIrrigated(irrigationCheck);

    // Timestamp para a chave "data"
    Values::TimestampValue timestampV(getTimestampString(now, 999999999));

    // Valor para "agua_usada"
    Values::DoubleValue usedWater(totalLiters);

    Values::DoubleValue temperatureBeforeVariable(temperatureBefore);
    Values::DoubleValue humidityBeforeVariable(humidityBefore);
    Values::DoubleValue soilMoistureBeforeVariable(soilMoistureBefore);
    Values::DoubleValue temperatureAfterVariable(temperatureAfter);
    Values::DoubleValue humidityAfterVariable(humidityAfter);
    Values::DoubleValue soilMoistureAfterVariable(soilMoistureAfter);


    // Agora, você cria o documento e adiciona todos os valores definidos anteriormente
    Document<Values::Value> doc("data", Values::Value(timestampV));
    doc.add("temperatureBefore", Values::Value(temperatureBeforeVariable))
      .add("humidityBefore", Values::Value(humidityBeforeVariable))
      .add("soilMoistureBefore", Values::Value(soilMoistureBeforeVariable))
      .add("temperatureAfter", Values::Value(temperatureAfterVariable))
      .add("humidityAfter", Values::Value(humidityAfterVariable))
      .add("soilMoistureAfter", Values::Value(soilMoistureAfterVariable))
      .add("usedWater", Values::Value(usedWater))
      .add("wasIrrigated", Values::Value(wasIrrigated));

    if (id_client != NULL) {
      bot.sendMessage(id_client, "estado do wifi: " + String(WiFi.RSSI()));
    }
    tryCreateDocumentWithRetry(doc, documentPath);
  }
}

void connectToFirebase() {

  ssl_client.setInsecure();

  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);

  authHandler();

  // Binding the FirebaseApp for authentication handler.
  // To unbind, use Docs.resetApp();
  app.getApp<Firestore::Documents>(Docs);

  // In case setting the external async result to the sync task (optional)
  // To unset, use unsetAsyncResult().
  aClient.setAsyncResult(aResult_no_callback);
}

void restartESP32() {
  Serial.println("🔄 Reiniciando o ESP32...");
  delay(1000);    // Pequeno delay para garantir que a mensagem seja enviada ao Serial
  ESP.restart();  // Reinicia o ESP32
}


void setup() {
  secured_client.setInsecure();
  Serial.begin(115200);
  pinMode(DHT_POWER, OUTPUT);
  digitalWrite(DHT_POWER, HIGH);
  dht.begin();
  delay(2000);
  // Faz uma leitura inicial para estabilizar
  dht.readTemperature();
  dht.readHumidity();
  delay(2000);

  Serial.println();

  checkWiFiConnection();

  //configuraçao de pinos
  pinMode(CAPACITIVE_SENSOR, INPUT);
  pinMode(RELE, OUTPUT);
  digitalWrite(RELE, HIGH);

  prefs.begin("controle", false);

  // Lê se houve reinício planejado
  pulandoReinicio = prefs.getBool("pulandoReinicio", false);
  ultimoMinutoReinicio = prefs.getInt("minutoReinicio", -1);  // -1 é valor padrão

  if (pulandoReinicio) {
    Serial.println("⚠️ Reinício planejado detectado!");
    prefs.putBool("pulandoReinicio", false);  // limpa a flag
    prefs.putInt("minutoReinicio", -1);       // limpa o minuto
  }

  prefs.end();
}

void loop() {

  app.loop();

  if (WiFi.status() == WL_CONNECTED) {

    //controle de irrigação
    getLocalTime(&currentTime);

    if (((currentTime.tm_hour == 7 && currentTime.tm_min == 0)
         || (currentTime.tm_hour == 12 && currentTime.tm_min == 0)
         || (currentTime.tm_hour == 17 && currentTime.tm_min == 0))
        && ((millis() - lastIrrigationCheckTime) > 90000 || lastIrrigationCheckTime == 0)) {

      Serial.println(currentTime.tm_min);

      // variaveis para armazenamento no firebase


      Serial.println("entrou na condiçao de irrigaçao");
      float totalLiters = 0.0;
      float soilMoistureBefore = 0.0;
      float soilMoistureAfter = 0.0;
      float humidityBefore = 0.0;
      float humidityAfter = 0.0;
      float temperatureBefore = 0.0;
      float temperatureAfter = 0.0;

      reiniciarDHT();

      soilMoistureBefore = getCurrentSoilMoisture();
      humidityBefore = getCurrentHumidity();
      temperatureBefore = getCurrentTemperature();

      unsigned long startTime = millis();

      irrigatiionCheck = false;

      if (soilMoistureBefore <= SOIL_MOISTURE_LIMIT_MAX) {

        irrigatiionCheck = true;
        Serial.println("umidade esta abaixo do maximo");
        //inicia o periodo de contagem de pulsos do sensor de fluxo

        attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, RISING);

        irrigate(soilMoistureBefore);

        //termina o periodo da contagem de pulsos do sensor de fluxo
        detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));

        totalLiters = measureAmountWater();
      }

      delay(2000);
      soilMoistureAfter = getCurrentSoilMoisture();

      reiniciarDHT();
      humidityAfter = getCurrentHumidity();
      temperatureAfter = getCurrentTemperature();

      if (id_client != NULL) {
        bot.sendMessage(id_client, "umidade do solo antes: " + String(soilMoistureBefore) + "\n" + "umidade do ar antes: " + String(humidityBefore) + "\n" + "temperatura antes: " + String(temperatureBefore) + "\n" + "umidade do solo depois: " + String(soilMoistureAfter) + "\n" + "umidade do ar depois: " + String(humidityAfter) + "\n" + "temperatura depois: " + String(temperatureAfter) + "\n" + "água usada: " + String(totalLiters));
      }


      createDocumentFirebase(totalLiters,
                             soilMoistureBefore,
                             soilMoistureAfter,
                             humidityBefore,
                             humidityAfter,
                             temperatureBefore,
                             temperatureAfter,
                             irrigatiionCheck);

      pulseCount = 0;
      lastIrrigationCheckTime = millis();
      Serial.print("ultima irriigaçao: ");
      Serial.println(lastIrrigationCheckTime);
    }


    //envio de mensagem ao bot
    if (millis() - bot_lasttime > BOT_MTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      bot_lasttime = millis();
    }

  } else {

    checkWiFiConnection();
  }
}

//funcões implementadas da propria biblioteca

void authHandler() {
  // Blocking authentication handler with timeout
  unsigned long ms = millis();
  while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000) {
    // The JWT token processor required for ServiceAuth and CustomAuth authentications.
    // JWT is a static object of JWTClass and it's not thread safe.
    // In multi-threaded operations (multi-FirebaseApp), you have to define JWTClass for each FirebaseApp,
    // and set it to the FirebaseApp via FirebaseApp::setJWTProcessor(<JWTClass>), before calling initializeApp.
    JWT.loop(app.getAuth());
    printResult(aResult_no_callback);
  }
}

void printResult(AsyncResult &aResult) {
  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }

  if (aResult.isDebug()) {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()) {
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
  }
}

String getTimestampString(uint64_t sec, uint32_t nano) {
  if (sec > 0x3afff4417f)
    sec = 0x3afff4417f;

  if (nano > 0x3b9ac9ff)
    nano = 0x3b9ac9ff;

  time_t now;
  struct tm ts;
  char buf[80];
  now = sec;
  ts = *localtime(&now);

  String format = "%Y-%m-%dT%H:%M:%S";

  if (nano > 0) {
    String fraction = String(double(nano) / 1000000000.0f, 9);
    fraction.remove(0, 1);
    format += fraction;
  }
  format += "Z";

  strftime(buf, sizeof(buf), format.c_str(), &ts);
  return buf;
}

void printError(int code, const String &msg) {
  Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}