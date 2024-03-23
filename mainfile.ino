#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define SS_PIN 4
#define RST_PIN 5
#define CUSTOM_SDA_PIN 2
#define CUSTOM_SCL_PIN 0
MFRC522 mfrc522(SS_PIN, RST_PIN);

const char* ssid = "PET";
const char* password = "11223344556677";

ESP8266WebServer server(80);

const int lock = 15;
const int buzz = 16;
const int led = 1;
const int green = 3;


#define EEPROM_SIZE 1024
#define UID_CHUNK_SIZE 32 
const unsigned long wifiTimeout = 10000;
unsigned long wifiConnectStartTime = 0;
String NAME = "";
String scannedUID = "";
bool cardProcessed = false;
bool wifiConnected = false;
bool lcdDisplayed = true;
const String expectedUsername = "a";
const String expectedPassword = "a";
bool isAuthenticated = false;
String username = "a";
String userpassword = "a";
WiFiClientSecure client;
String GAS_ID = "AKfycbwWGXYihyAmtssGS2pMACbsWp8uLZPsE2q7c6ebgN6Z7p9Eokta02iKt8xCFP_kND1k";

const char* loginPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Login</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            
        }
        h1 {
            margin: 20px;
        }
        form {
            display: inline-block;
            padding: 20px;
            border: 1px solid #ccc;
            border-radius: 5px;
            text-align: left;
        }
        label, input {
            display: block;
            margin: 5px;
        }
        input {
            padding: 5px;
        }
    </style>
</head>
<body>
    <h1>Admin Login</h1>
    <form method="POST" action="/login">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Login">
    </form>
</body>
</html>

)rawliteral";

// HTML template
    const char* htmlTemplate = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>RFID CONTROLL WEBPAGE</title>
        <!-- Mobile Specific Metas -->
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
        <!-- Font-->
        <link rel="stylesheet" type="text/css" href="css/montserrat-font.css">
        <link rel="stylesheet" type="text/css" href="fonts/material-design-iconic-font/css/material-design-iconic-font.min.css">
    </head>
    <body class="form-v10">
        <div class="page-content">
            <div class="form-v10-content">
                <form class="form-detail" action="/submit" method="post" id="myform">
                    <div class="form-left">
                        <h2>RIFD SYSTEM </h2>
                        <div class="form-row">
                            <input type="text" id="inpt" name="uid" placeholder="UID">
                        </div>
                        <div class="form-row">
                            <input type="text" id="name" name="name" placeholder="Name"> <!-- New name input field -->
                        </div>
                        <div class="form-row">
                            <input type="submit" id="add-user" value="Add user">
                        </div>
                        <div class="form-row">
                            <h3 id="real">Make sure to LOGOUT</h3>
                         
                        </div>
                        <div class="form-group">
                            <div class="form-row form-row-3">
                                <button id="logout" type="button">Logout</button>
                                
                            </div>
                        </div>
                    </div>
    
                    <div class="form-right">
                        <h2>Stored Authorised Cards:</h2>
                        <ul id="stored">
                            %STORED_UIDS%
                        </ul>
                    </div>
                </form>
            </div>
        </div>
        <script>
           
    
            document.getElementById("logout").addEventListener("click", function () {
                document.getElementById("myform").action = "/logout";
                document.getElementById("myform").method = "get"; // Change the method to GET
                document.getElementById("myform").submit();
            });
    
            document.getElementById("add-user").addEventListener("click", function (e) {
                e.preventDefault(); // Prevent the default form submission
                document.getElementById("myform").action = "/submit";
                document.getElementById("myform").method = "post"; // Set it back to POST
                document.getElementById("myform").submit();
            });
        </script>
    </body>
    </html>
    
        <style>
            .remove-button {
                background-color: #c0392b; /* Red background color */
                color: #fff; /* White text color */
                border: none;
                padding: 8px 15px;
                text-align: center;
                text-decoration: none;
                display: inline-block;
                font-size: 16px;
                border-radius: 4px;
                margin-left: 180px; /* Add margin to separate buttons */
            }
    
            .remove-button:hover {
                background-color: #a93226; /* Darker red background color on hover */
            }
    
            .remove-button:active {
                background-color: #8e2b1e; /* Even darker red background color when clicked */
            }
        </style>
        <style>
            body {
        margin:  0;
    }
    #stored {
        color: whitesmoke;
        font-size: medium;
      font-family: 'Franklin Gothic Medium', 'Arial Narrow', Arial, sans-serif
    }
    span {
        color: black;
        font-size: small;
    }
   
    }
    
    #adds-user {
      background-color: #3498db;
      color: #fff;
      border: none;
      padding: 8px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      border-radius: 4px;
      margin-bottom: 30px;
    }
    
    #adds-user:hover {
      background-color: #2980b9;
    }
    
    #adds-user:active {
      background-color: #1c638d;
    }
    
    #add-user {
      background-color: #3498db;
      color: #fff;
      border: none;
      padding: 8px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      border-radius: 4px;
      margin-bottom: 30px;
    }
    
    #add-user:hover {
      background-color: #2980b9;
    }
    
    #add-user:active {
      background-color: #1c638d;
    }
    #logout {
     background-color: #3498db;
      color: #fff;
      border: none;
      padding: 8px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
      border-radius: 4px;
    }
    
    #logout:hover {
      background-color: #c0392b;
    }
    
    #logout:active {
      background-color: #a93226;
    }
    
    .page-content {
        width: 100%;
        margin:  0 auto;
        background: #75e2e9;
        display: flex;
        display: -webkit-flex;
        justify-content: center;
        -o-justify-content: center;
        -ms-justify-content: center;
        -moz-justify-content: center;
        -webkit-justify-content: center;
        align-items: center;
        -o-align-items: center;
        -ms-align-items: center;
        -moz-align-items: center;
        -webkit-align-items: center;
        background-image: -moz-linear-gradient( 136deg, rgb(149,153,226) 0%, rgb(139,198,236) 100%);
        background-image: -webkit-linear-gradient( 136deg, rgb(149,153,226) 0%, rgb(139,198,236) 100%);
        background-image: -ms-linear-gradient( 136deg, rgb(149,153,226) 0%, rgb(139,198,236) 100%);
    }
    .form-v10-content  {
        background: #fff;
        width: 1100px;
        border-radius: 10px;
        -o-border-radius: 10px;
        -ms-border-radius: 10px;
        -moz-border-radius: 10px;
        -webkit-border-radius: 10px;
        box-shadow: 0px 8px 20px 0px rgba(0, 0, 0, 0.15);
        -o-box-shadow: 0px 8px 20px 0px rgba(0, 0, 0, 0.15);
        -ms-box-shadow: 0px 8px 20px 0px rgba(0, 0, 0, 0.15);
        -moz-box-shadow: 0px 8px 20px 0px rgba(0, 0, 0, 0.15);
        -webkit-box-shadow: 0px 8px 20px 0px rgba(0, 0, 0, 0.15);
        margin: 95px 0;
        position: relative;
        font-family: 'Montserrat', sans-serif;
    }
    .form-v10-content .form-detail {
        position: relative;
        width: 100%;
        display: flex;
        display: -webkit-flex;
    }
    .form-v10-content .form-detail h2 {
        font-weight: 500;
        font-size: 25px;
        margin-bottom: 34px;
        padding: 33px 50px 0px 60px;
    }
    .form-v10-content .form-detail .form-left {
        border-top-left-radius: 10px;
        border-bottom-left-radius: 10px;
        width: 100%;
    }
    .form-v10-content .form-detail .form-left h2 {
        color: #2271dd;
    }
    .form-v10-content .form-detail .form-right {
        width: 100%;
        background: #1d0ba8;
        border-top-right-radius: 10px;
        border-bottom-right-radius: 10px;
    }
    .form-v10-content .form-detail .form-right h2 {
        color: #fff;
    }
    .form-v10-content .form-detail .form-group {
        display: flex;
        display: -webkit-flex;
    }
    .form-v10-content .form-detail .form-row {
        position: relative;
        margin-bottom: 24px;
        padding-left: 60px;
        padding-right: 50px;
    }
    .form-v10-content .form-detail .form-left .form-group .form-row.form-row-1 {
        width: 50%;
        padding: 0 12px 0 60px;
    }
    .form-v10-content .form-detail .form-left .form-group .form-row.form-row-2 {
        width: 50%;
        padding: 0 50px 0 12px;
    }
    .form-v10-content .form-detail .form-left .form-group .form-row.form-row-3 {
        width: 73%;
        padding: 0 12px 0 60px;
    }
    .form-v10-content .form-detail .form-left .form-group .form-row.form-row-4 {
        width: 50%;
        padding: 0 50px 0 12px;
    }
    .form-v10-content .form-detail .form-right .form-group .form-row.form-row-1 {
        width: 50%;
        padding: 0 12px 0 60px;
    }
    
    .form-v10-content .form-detail select,
    .form-v10-content .form-detail input {
        width: 100%;
        padding: 11.5px 15px 15px 15px;
        border: 1px solid transparent;
        background: transparent;
        appearance: unset;
        -moz-appearance: unset;
        -webkit-appearance: unset;
        -o-appearance: unset;
        -ms-appearance: unset;
        outline: none;
        -moz-outline: none;
        -webkit-outline: none;
        -o-outline: none;
        -ms-outline: none;
        font-family: 'Montserrat', sans-serif;
        font-size: 16px;
        box-sizing: border-box;
        -moz-box-sizing: border-box;
        -webkit-box-sizing: border-box;
        -o-box-sizing: border-box;
        -ms-box-sizing: border-box;
    }
    .form-v10-content .form-detail select {
        background: 0 0;
        position: relative;
        z-index: 9;
        cursor: pointer;
    }
    .form-v10-content .form-detail .form-left select {
        color: #666;
    }
    .form-v10-content .form-detail .form-right select {
        color: #f2f2f2;
    }
    .form-v10-content .form-detail .select-btn {
        z-index: 0;
        position: absolute;
        top: 30%;
        right: 11.5%;
        font-size: 18px;
    }
    
    
    .form-v10-content .form-detail .form-left input {
        color: #000;
    }
    .form-v10-content .form-detail .form-right input {
        color: #fff;
    }
    .form-v10-content .form-detail .form-left input,
    .form-v10-content .form-detail .form-left select {
        border-bottom: 1px solid #ccc;
    }
    
    /* Responsive */
    @media screen and (max-width: 1199px) {
        .form-v10-content {
            margin:  95px 20px;
        }
    }
    @media screen and (max-width: 991px) and (min-width: 768px) {
        .form-v10-content .form-detail .form-group {
            flex-direction:  column;
            -o-flex-direction:  column;
            -ms-flex-direction:  column;
            -moz-flex-direction:  column;
            -webkit-flex-direction:  column;
        }
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-1,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-2,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-3,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-4,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-1,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-2 {
            width: auto;
            padding: 0 50px 0 60px;
        }
        .form-v10-content .form-detail .select-btn,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-4 .select-btn,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-2 .select-btn {
            right: 15%;
        }
    
    }
    @media screen and (max-width: 767px) {
        .form-v10-content .form-detail {
            flex-direction:  column;
            -o-flex-direction:  column;
            -ms-flex-direction:  column;
            -moz-flex-direction:  column;
            -webkit-flex-direction:  column;
        }
        .form-v10-content .form-detail .form-right {
            border-top-right-radius: 0px;
            border-bottom-left-radius: 10px;
        }
        .form-v10-content .form-detail .form-left {
            padding-bottom: 50px;
        }
    }
    @media screen and (max-width: 575px) {
        .form-v10-content .form-detail .form-group {
            flex-direction:  column;
            -o-flex-direction:  column;
            -ms-flex-direction:  column;
            -moz-flex-direction:  column;
            -webkit-flex-direction:  column;
        }
        .form-v10-content .form-detail .form-row,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-1,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-2,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-3,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-4,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-1,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-2 {
            width: auto;
            padding: 0 30px;
        }
        .form-v10-content .form-detail .select-btn,
        .form-v10-content .form-detail .form-left .form-group .form-row.form-row-4 .select-btn,
        .form-v10-content .form-detail .form-right .form-group .form-row.form-row-2 .select-btn {
            right: 15%;
        }
        .form-v10-content .form-detail h2 {
            padding: 33px 30px 0px 30px;
        }
        .form-v10-content .form-detail .form-checkbox {
            padding: 0 30px;
        }
        .form-v10-content .form-detail .form-checkbox .checkmark {
            left: 30px;
        }
        .form-v10-content .form-detail .form-right .form-row-last {
            padding-left: 0;
            text-align: center;
            margin: 44px 0 30px;
        }
    }
        </style>
    </body>
    </html>
    )rawliteral";

void connectToWiFi() {
    wifiConnected = false;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(ssid, password);
        wifiConnectStartTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Connecting to WiFi...");
            lcd.setCursor(0, 0);
            lcd.print("Connecting....");

            if (millis() - wifiConnectStartTime >= wifiTimeout) {
                wifiConnected = false;
                Serial.println("Wi-Fi connection unsuccessful (timeout)");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Connection");
                lcd.setCursor(0, 1);
                lcd.print("unsuccessful");
                break;
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Connection");
            lcd.setCursor(0, 1);
            lcd.print("Established");
            Serial.println("Connected to WiFi");
        }
    }
}

const char* host = "script.google.com";
const int httpsPort = 443;

void setup() {
    Serial.begin(115200);

    Wire.begin(CUSTOM_SDA_PIN, CUSTOM_SCL_PIN);
    pinMode(CUSTOM_SDA_PIN, INPUT_PULLUP);
    pinMode(CUSTOM_SCL_PIN, INPUT_PULLUP);
    client.setInsecure();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    SPI.begin();
    mfrc522.PCD_Init();
    connectToWiFi();
    delay(500);
    EEPROM.begin(EEPROM_SIZE);
    pinMode(lock, OUTPUT);
    digitalWrite(lock, LOW);
    pinMode(buzz, OUTPUT);
    digitalWrite(buzz, LOW);
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
    pinMode(green, OUTPUT);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.on("/clear", HTTP_GET, handleClear);
    server.on("/logout", HTTP_GET, handleLogout);
    server.on("/remove", HTTP_GET, handleRemove);
    server.on("/login", HTTP_POST, handleLogin);
    server.begin();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RFID Door Lock");
    lcd.setCursor(0, 1);
    lcd.print("System");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Made by");
    lcd.setCursor(0, 1);
    lcd.print("Peter Basumatary");
    delay(2000);
    lcd.clear();
}

void loop() {
    if (wifiConnected) {
        static bool lcdDisplayed = false;

        if (!lcdDisplayed) {
            lcd.clear();
            lcd.print("ROBOTICS CLUB");
            lcdDisplayed = true;
        }

        server.handleClient();

        if (mfrc522.PICC_IsNewCardPresent()) {
            if (!cardProcessed && mfrc522.PICC_ReadCardSerial()) {
                scannedUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);

                if (checkStoredUID(scannedUID)) {
                    Serial.println("Authorised");
                    Serial.println("TRIGGERING PIN");
                    Serial.println(NAME);
                    Serial.println(scannedUID);
                    digitalWrite(lock, HIGH);
                    lcd.clear();
                    abuzz();
                    lcd.setCursor(0, 0);
                    lcd.print("Access Granted");
                    lcd.setCursor(0, 1);
                    lcd.print("Name : " + NAME);
                    sendData(NAME, scannedUID);
                    digitalWrite(lock, LOW);
                   
                    
                    
                } else {
                    Serial.println("NOT Authorised");
                    NAME = "INTRUDER";
                    ubuzz();  
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("ACCESS DENIED");
                    lcd.setCursor(0, 1);
                    lcd.print("UID: " + scannedUID);
                    Serial.println(scannedUID);
                    sendData(NAME, scannedUID);
                    
                }
                    cardProcessed = true;
                    mfrc522.PICC_HaltA();
                    delay(1000);
                    lcdDisplayed = false;
                    scannedUID = "";
                    server.sendHeader("Location", "/");
                    server.send(303);
            }
        } else {
            cardProcessed = false;
        }
    }

    if (!wifiConnected) {
        static bool lcdDisplayed = false;

        if (!lcdDisplayed) {
            lcd.clear();
            lcd.print("ROBOTICS CLUB");
            lcdDisplayed = true;
        }
        scannedUID = "";
        if (mfrc522.PICC_IsNewCardPresent()) {
            if (!cardProcessed && mfrc522.PICC_ReadCardSerial()) {
                scannedUID = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
                if (checkStoredUID(scannedUID)) {
                    Serial.print("Authorised");
                    Serial.println("TRIGGERING PIN");
                    digitalWrite(lock, HIGH); 
                    abuzz();  
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Access Granted");
                    lcd.setCursor(0, 1);
                    lcd.print("UID: " + scannedUID);
                    delay(1000);
                    Serial.println(NAME);
                    Serial.println(scannedUID);
                    digitalWrite(lock, LOW);
                } else {
                    Serial.print("NOT Authorised");
                    ubuzz();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("ACCESS DENIED");
                    lcd.setCursor(0, 1);
                    lcd.print("UID: " + scannedUID);
                    delay(2000);
                    NAME = "INTRUDER";
                    Serial.println(scannedUID);
                }
                    cardProcessed = true;
                    mfrc522.PICC_HaltA();
                    delay(1000);
                    lcdDisplayed = false;
                    server.sendHeader("Location", "/");
                    server.send(303);
            }
        } else {
            cardProcessed = false;
        }
    }
}

String getUIDString(byte* uid, byte size) {
    unsigned long decimalUID = strtoul(getUIDHex(uid, size).c_str(), NULL, 16);
    String result = String(decimalUID);
    while (result.length() < 6) {
        result = "0" + result;
    }
    return result;
}

String getUIDHex(byte* uid, byte size) {
    String result = "";
    for (byte i = 0; i < size; i++) {
        result += "0x";
        if (uid[i] < 0x10) {
            result += "0";
        }
        result += String(uid[i], HEX);
        if (i < size - 1) {
            result += " ";
        }
    }
    result.trim();
    return result;
}

bool checkStoredUID(String scannedUID) {
    for (int i = 0; i < EEPROM_SIZE; i += UID_CHUNK_SIZE) {
        char dataBuffer[UID_CHUNK_SIZE + 1];
        for (int j = 0; j < UID_CHUNK_SIZE; j++) {
            dataBuffer[j] = EEPROM.read(i + j);
        }
        dataBuffer[UID_CHUNK_SIZE] = '\0';
        String storedData = String(dataBuffer);
        storedData.trim();
        if (storedData[0] != '\0') {
            int delimiterPos = storedData.indexOf(',');
            if (delimiterPos != -1) {
                String storedUID = storedData.substring(0, delimiterPos);
                String storedName = storedData.substring(delimiterPos + 1);
                int spaceIndex = storedName.indexOf(' ');
                String firstName;
                if (spaceIndex != -1) {
                    firstName = storedName.substring(0, spaceIndex);
                } else {
                    firstName = storedName;
                }
                NAME = firstName;
                if (scannedUID == storedUID) {
                    Serial.print("Authorised. uid: ");
                    Serial.println(storedUID);
                    Serial.print("Authorised. Name: ");
                    Serial.println(storedName);
                    return true;
                }
            }
        }
    }
    return false;
}

void abuzz(){
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(150);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
  delay(150);
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(150);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
  delay(150);
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(600);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
}
void ubuzz(){
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(150);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
  delay(150);
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(150);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
  delay(150);
  digitalWrite(led, HIGH);
  digitalWrite(buzz, HIGH);
  delay(150);
  digitalWrite(led, LOW);
  digitalWrite(buzz, LOW);
}

  void handleClear() {
        for (int i = 0; i < EEPROM_SIZE; i++) {
            EEPROM.write(i, 0); // Clear EEPROM
        }
        EEPROM.commit();
        // Redirect back to the root page
        server.sendHeader("Location", "/");
        server.send(303);
    }
void sendData(String scannedUID, String NAME) {
    Serial.println("==========");
    Serial.print("connecting to ");
    Serial.println(host);

    if (!client.connect(host, httpsPort)) {
        Serial.println("connection failed");
        client.stop();
        return;
    }

    String url = "/macros/s/" + GAS_ID + "/exec?name=" + NAME + "&uid=" + scannedUID;
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("request sent");

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("headers received");
            break;
        }
    }
    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
        Serial.println("Data sending failed");
    } else {
        Serial.println("Data sent successfully");
    }
    Serial.println("closing connection");
    Serial.println("==========");

    client.stop();
}
void handleLogin() {
    String inputUsername = server.arg("username");
    String inputPassword = server.arg("password");

    if (inputUsername.equals(username) && inputPassword.equals(userpassword)) {
        isAuthenticated = true;
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "");
    } else {
        isAuthenticated = false;
        server.send(200, "text/html", loginPage);
    }
}
void handleRoot() {
    if (!isAuthenticated) {
        // If not authenticated, serve the login page
        server.send(200, "text/html", loginPage);
    } else {
        // If authenticated, serve the RFID control page
        String storedUIDsHtml = "";
        for (int i = 0; i < EEPROM_SIZE; i += UID_CHUNK_SIZE) {
            char uidBuffer[UID_CHUNK_SIZE + 1];
            for (int j = 0; j < UID_CHUNK_SIZE; j++) {
                uidBuffer[j] = EEPROM.read(i + j);
            }
            uidBuffer[UID_CHUNK_SIZE] = '\0';

            String storedUID = String(uidBuffer);
            storedUID.trim();

            if (storedUID[0] != '\0') {
                storedUIDsHtml += "<li>";
                storedUIDsHtml += storedUID;
                storedUIDsHtml += " <a href='/remove?uid=";
                storedUIDsHtml += storedUID;
                storedUIDsHtml += "' class='remove-button'>Remove</a>";
                storedUIDsHtml += "</li>";
            }
        }

        String html = String(htmlTemplate);
        html.replace("%STORED_UIDS%", storedUIDsHtml);
        html.replace("%SCANNED_UID%", scannedUID);

        server.send(200, "text/html", html);
    }
}


void handleLogout() {
    server.sendHeader("Location", "/login");
    server.send(303); 
    delay(3000);
    ESP.restart();
}


void handleRemove() {
        if (server.hasArg("uid")) {
            String uidToRemove = server.arg("uid");
            int uidSize = uidToRemove.length();
            uidSize = min(uidSize, UID_CHUNK_SIZE); // Ensure the UID size doesn't exceed UID_CHUNK_SIZE
    
            for (int i = 0; i < EEPROM_SIZE; i += UID_CHUNK_SIZE) {
                char uidBuffer[UID_CHUNK_SIZE + 1]; // +1 for null-terminator
                for (int j = 0; j < UID_CHUNK_SIZE; j++) {
                    uidBuffer[j] = EEPROM.read(i + j);
                }
                uidBuffer[UID_CHUNK_SIZE] = '\0'; // Ensure null-termination
    
                String storedUID = String(uidBuffer);
                storedUID.trim(); // Remove leading/trailing spaces
    
                if (storedUID == uidToRemove) {
                    // Found the UID to remove, clear it from EEPROM
                    for (int j = 0; j < UID_CHUNK_SIZE; j++) {
                        EEPROM.write(i + j, ' ');
                    }
                    EEPROM.commit();
                    break;
                }
            }
        }
        // Redirect back to the root page
        server.sendHeader("Location", "/");
        server.send(303);
    }
    

int findEmptyEEPROMAddress() {
    for (int i = 0; i < EEPROM_SIZE; i += UID_CHUNK_SIZE) {
        char dataBuffer[UID_CHUNK_SIZE + 1];
        for (int j = 0; j < UID_CHUNK_SIZE; j++) {
            dataBuffer[j] = EEPROM.read(i + j);
        }
        dataBuffer[UID_CHUNK_SIZE] = '\0';
        String storedData = String(dataBuffer);
        storedData.trim();
        if (storedData[0] == '\0') {
            return i;
        }
    }
    return -1;  // EEPROM is full
}
void handleSubmit() {
    if (server.hasArg("uid") && server.hasArg("name")) {
        String uid = server.arg("uid");
        String name = server.arg("name");
        String NAME = name;
        String dataToStore = uid + "," + name;
        int eepromAddress = findEmptyEEPROMAddress();
        if (eepromAddress != -1) {
            int dataSize = dataToStore.length();
            dataSize = min(dataSize, UID_CHUNK_SIZE);
            for (int i = 0; i < UID_CHUNK_SIZE; i++) {
                if (i < dataSize) {
                    EEPROM.write(eepromAddress + i, dataToStore[i]);
                } else {
                    EEPROM.write(eepromAddress + i, '\0');
                }
            }
            EEPROM.commit();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(NAME + " written");
            lcd.setCursor(0, 1);
            lcd.print("Successfully");

        } else {
            // EEPROM is full, can't store more entries.
        }
        server.sendHeader("Location", "/");
        server.send(303);
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}
