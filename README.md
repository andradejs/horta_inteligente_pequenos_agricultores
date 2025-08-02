---

# 🌱 Horta Inteligente com ESP32, Firebase e Telegram Bot

Este projeto implementa um **sistema de irrigação automatizado** utilizando **ESP32**, sensores de solo e umidade, integração com o **Firebase** para armazenamento dos dados e um **bot do Telegram** para interação e controle remoto.

## 🚀 Funcionalidades

* Medição de:

  * Umidade do solo (sensor capacitivo)
  * Umidade e temperatura do ar (sensor DHT22)
  * Fluxo de água (sensor de fluxo com interrupção)
* Irrigação automática em horários programados (7h, 12h, 17h)
* Atualização dos dados no **Firebase Firestore**
* Comandos via **Telegram bot**
* Suporte a **atualização remota de firmware**
* Leitura e controle via comandos no Telegram
* Reboot programado e seguro do ESP32

---

## 📦 Hardware Necessário

* ESP32 Dev Board
* Sensor DHT22
* Sensor capacitivo de umidade do solo
* Sensor de fluxo de água (hall effect)
* Relé 1 canal (para bomba ou válvula)
* Resistores, jumpers, fonte de alimentação

---

## 🔧 Bibliotecas Necessárias

Certifique-se de instalar as seguintes bibliotecas:

* `WiFi.h`
* `WiFiClientSecure.h`
* `UniversalTelegramBot`
* `FirebaseClient` (nova biblioteca da Firebase para ESP)
* `DHT sensor library`
* `Preferences`
* `HTTPUpdate`

---

## ⚙️ Configuração

### 1. **Credenciais**

Edite os seguintes campos no código:

```cpp
#define WIFI_SSID "NOME_DA_REDE"
#define WIFI_PASSWORD "SENHA_WIFI"

#define BOT_TOKEN "TOKEN_DO_SEU_BOT"

#define API_KEY "SUA_FIREBASE_API_KEY"
#define USER_EMAIL "SEU_EMAIL_CADASTRADO"
#define USER_PASSWORD "SUA_SENHA"
#define DATABASE_URL "https://seu-projeto.firebaseio.com"
#define FIREBASE_PROJECT_ID "seu-projeto"
```

### 2. **Certificado Telegram**

Certifique-se de adicionar o certificado raiz do Telegram (via `TELEGRAM_CERTIFICATE_ROOT`) se for necessário no seu código.

---

## 🤖 Comandos disponíveis via Telegram

| Comando              | Ação                                             |
| -------------------- | ------------------------------------------------ |
| `/start`             | Boas-vindas e apresentação                       |
| `/opcoes`            | Mostra os comandos disponíveis                   |
| `/humidade_solo`     | Retorna a umidade atual do solo                  |
| `/humidade_ar`       | Retorna a umidade do ar                          |
| `/temperatura`       | Retorna a temperatura do ar                      |
| `/grafico`           | Link para gráfico externo (ex: Telegram Channel) |
| `/irrigar`           | Aciona a irrigação por 5 segundos manualmente    |
| `/reiniciardht`      | Reinicia o sensor DHT22                          |
| `/temperaturaesp`    | Mostra temperatura interna do ESP32              |
| `/sinalwifi`         | Mostra a força do sinal WiFi                     |
| `/ativarmensagem`    | Ativa notificações automáticas no Telegram       |
| `/desativarmensagem` | Desativa notificações automáticas                |

---

## ☁️ Firebase

Os dados enviados para o Firestore incluem:

* Timestamp
* Umidade e temperatura antes e depois da irrigação
* Quantidade de água utilizada
* Indicador se houve irrigação ou não

---

## 🔄 Atualização OTA

O bot aceita **documentos com firmware**. Se o arquivo enviado tiver a legenda "update firmware", o ESP32 tentará atualizar automaticamente, sem reiniciar imediatamente.

---

## 🧠 Lógica de Irrigação

* Executa 3 vezes ao dia (7h, 12h, 17h)
* Verifica se a umidade do solo está abaixo de um limite
* Se estiver, aciona a irrigação até atingir o valor ideal
* Mede a água usada via sensor de fluxo
* Salva dados no Firebase após cada irrigação

---

## 📝 Observações

* O sensor DHT pode travar em longos períodos. O sistema inclui reinicialização via GPIO.
* O ESP32 pode ser reiniciado automaticamente se necessário, mas o código tenta evitar reinicializações desnecessárias com controle por `Preferences`.
* O código está preparado para reconectar ao Wi-Fi automaticamente.

---

## 📷 Exemplo de Interface com o Bot

```
🤖 Horta Simples
Digite ou toque em uma das opções:

1- /humidade_solo
2- /humidade_ar
3- /temperatura
4- /grafico
```

---

## 🤝 Agradecimentos

* [@witnessmenow](https://github.com/witnessmenow) pela biblioteca do Telegram
* [Firebase Arduino Client Library for ESP](https://github.com/mobizt/Firebase-ESP-Client)

---

## 📄 Licença

Este projeto é livre para uso educacional e pessoal. Para usos comerciais, entre em contato com o autor.

---

