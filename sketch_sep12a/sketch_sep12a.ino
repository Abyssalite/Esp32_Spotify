#include <FS.h>
//#include <U8g2lib.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <string>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <JPEGDEC.h>

#include "env.h"
#include "cd_image.h"
#include "Bluetooth.h"
#include "WebSocket.h"

#define UPDATE_OLED_PERIOD 2000
#define UPDATE_TFT_PERIOD 500
#define RECONNECT_PERIOD 1000
#define NOTIFY_PERIOD 1000
#define API_PERIOD 10000
#define IMAGE_SIZE 160
#define DISPLAY_WIDTH 240
#define SCROLL_INTERVAL 500
#define SCROLL_STEP 8
#define RESET_BUTTON 9
#define RESET_DELAY 5000 // 5s debounce

unsigned long resetPress = 0;
unsigned long updateOledTimer = 0;
unsigned long updateTftTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long apiTimer = 0;
unsigned long lastNotify = 0;
unsigned long lastScrollTime = 0;

uint8_t spin = 0;
uint16_t *imageBuffer = nullptr;
float imageAngle = 0.0f;
int scrollOffset = 0;
bool scrollDirection = true;   // true = moving left, false = moving right
volatile bool isReset = false;
volatile bool canSetInterrupt = true;

String wifiSsid     = "";
String wifiPassword = "";
String wsStatus   = "";
String message = "";
String btStatus   = "";

String apiKey   = "";
String userName = "";

String imageUrl   = "";
String songName   = "";
String artistName = ""; 
String currentSongName = "";
bool isPlaying = false;
bool isWifiConnect = false;
bool isIpRequest = false;
String isSetup = "false";
String isReady = "false";

//JsonDocument telemetryJson;
Preferences preferences;

//U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;
Bluetooth bluetooth;
WebSocket websocket;

bool debounceReset(uint8_t btn, uint8_t isHigh) {
  if (isHigh) return false;
  if (canSetInterrupt) {
    resetPress = millis();
    canSetInterrupt = false;
  }
  if (millis() - resetPress > RESET_DELAY) {
    canSetInterrupt = true;
    resetPress = 0;
    return true;
  }
  return false;
}

void IRAM_ATTR resetButtonISR() {
  if (debounceReset(RESET_BUTTON, digitalRead(RESET_BUTTON))) {
    isReset = true;
  }
}

void saveUser(String user, String key) {
  apiKey = user;
  userName = key;
  preferences.begin("last.fm", false); //read-write
  preferences.putString("key", key);
  preferences.putString("user", user);
  preferences.end();
}

void saveWifi(String ssid, String pass) {
  wifiSsid = ssid;
  wifiPassword = pass;
  preferences.begin("wifi", false); //read-write
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
}

void saveSetup(String setup, String ready) {
  isSetup = setup;
  isReady = ready;
  preferences.begin("esp", false); //read-write
  preferences.putString("setup", setup);
  preferences.putString("ready", ready);
  preferences.end();
}

String loadApiKey() {
  preferences.begin("last.fm", true); //read-only
  String key = preferences.getString("key", "");
  preferences.end();
  return key;
}

String loadUserName() {
  preferences.begin("last.fm", true); //read-only
  String user = preferences.getString("user", "");
  preferences.end();
  return user;
}

String loadSsid() {
  preferences.begin("wifi", true); //read-only
  String ssid = preferences.getString("ssid", "");
  preferences.end();
  return ssid;
}

String loadWifiPass() {
  preferences.begin("wifi", true); //read-only
  String pass = preferences.getString("pass", "");
  preferences.end();
  return pass;
}

String loadEspSetup() {
  preferences.begin("esp", true); //read-only
  String setup = preferences.getString("setup", "false");
  preferences.end();
  return setup;
}

String loadEspReady() {
  preferences.begin("esp", true); //read-only
  String ready = preferences.getString("ready", "false");
  preferences.end();
  return ready;
}

/*void drawOled(String a, String b, String c) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.setCursor(4, 8);
    u8g2.print(a);

    u8g2.setCursor(4, 18);
    u8g2.print(b);

    u8g2.setCursor(4, 28);
    u8g2.print(c);
  } while (u8g2.nextPage()); 
}*/

void reconnectSpinner() {
  const char spinner[] = { '|', '/', '-', '\\' }; 

  drawScrollingText("Connecting", 120, TFT_WHITE, 2);
  drawScrollingText(String(spinner[spin]), 160, TFT_WHITE, 4);
}

int savePixel(JPEGDRAW *pDraw) {
  for (int y = 0; y < pDraw->iHeight; y++) {
    int destY = pDraw->y + y;

    if (destY < 0 || destY >= IMAGE_SIZE)
      continue;

    int srcX = 0;
    int destX = pDraw->x;
    int width = pDraw->iWidth;

    // Clip left
    if (destX < 0) {
        srcX = -destX;
        width -= srcX;
        destX = 0;
    }

    // Clip right
    if (destX + width > IMAGE_SIZE) {
      width = IMAGE_SIZE - destX;
    }

    if (width <= 0)
        continue;

    memcpy(
      imageBuffer + destY * IMAGE_SIZE + destX,
      pDraw->pPixels + y * pDraw->iWidth + srcX,
      width * sizeof(uint16_t)
    );
  }
  return 1;
}

void drawTftJPEG() {
  const float srcCenter = 79.5f;
  const float dstCenter = 119.5f;

  const int radius = DISPLAY_WIDTH / 2;
  const int radius2 = radius * radius;

  const float scale = 1.5f;

  float rad = imageAngle * PI / 180.0f;
  float cosA = cos(rad);
  float sinA = sin(rad);

  for (int y = 0; y < DISPLAY_WIDTH; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {

      // Circular mask
      int dxScreen = x - 120;
      int dyScreen = y - 120;

      if (dxScreen * dxScreen + dyScreen * dyScreen > radius2)
        continue;

      // Position relative to destination center
      float dx = (x - dstCenter) / scale;
      float dy = (y - dstCenter) / scale;

      // Reverse rotation to find source pixel
      float srcX = dx * cosA + dy * sinA;
      float srcY = -dx * sinA + dy * cosA;

      srcX += srcCenter;
      srcY += srcCenter;

      int ix = round(srcX);
      int iy = round(srcY);

      // Outside source image
      if (ix < 0 || ix >= IMAGE_SIZE ||
          iy < 0 || iy >= IMAGE_SIZE)
        continue;

      uint16_t pixel = imageBuffer[iy * IMAGE_SIZE + ix];

      tft.drawPixel(x, y, pixel);
    }
  }
}

bool downloadAndSaveImage(int x, int y) {
  if (imageUrl == "") {
    Serial.println("No Image url");
    return false;    
  }
  
  HTTPClient http;
  http.begin(imageUrl);
  http.setTimeout(15000);

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("Download Image failed: %d\n", httpCode);
    http.end();
    return false;
  }

  // Get the image data
  int totalLen = http.getSize();
  if (totalLen <= 0) return false;

  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  uint8_t *buffer = (uint8_t *)malloc(totalLen);
  if (!buffer) {
    Serial.println("Not enough memory");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  int bytesRead = 0;
  unsigned long timeout = millis() + 10000;

  while (http.connected() && (bytesRead < totalLen) && millis() < timeout) {
    size_t avail = stream->available();
    if (avail) {
      int toRead = min((int)avail, totalLen - bytesRead);
      int got = stream->readBytes(buffer + bytesRead, toRead);
      if (got > 0) bytesRead += got;
    } else {
      delay(2);
    }
  }

  http.end();

  if (bytesRead != totalLen) {
    Serial.println("Incomplete download");
    free(buffer);
    return false;
  }
  // ===== Decode with JPEGDEC =====
  if (jpeg.openRAM(buffer, totalLen, savePixel)) {
    //Serial.printf("JPEG size: %d x %d\n", jpeg.getWidth(), jpeg.getHeight());
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);

    if(jpeg.getWidth() != IMAGE_SIZE || jpeg.getHeight() != IMAGE_SIZE) {
      free(buffer);
      jpeg.close();
      return false;
    }
    if (!jpeg.decode(x, y, 0)) {
      free(buffer);
      jpeg.close();
      return false;
    }
    jpeg.close();

  } else {
    Serial.println("JPEGDEC failed to open image");
    free(buffer);
    return false;
  }

  free(buffer);
  return true;
}

bool loadDefaultPicture() {
   // ===== Decode with JPEGDEC =====
  if (jpeg.openFLASH((uint8_t *)cdImage, cdImageSize, savePixel)) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);

    if (!jpeg.decode(0, 0, 0)) {
      jpeg.close();
      return false;
    }
    jpeg.close();

  } else {
    return false;
  }

  return true;
}

void drawScrollingText(String text, int y, uint16_t color, uint8_t textSize) {
  tft.setTextSize(textSize);
  tft.setTextDatum(TL_DATUM);    // Top-Left

  int textWidth = tft.textWidth(text);
  int areaHeight = (textSize == 2) ? 20 : 12;

  // Clear text area
  tft.fillRect(0, y - 4, 240, areaHeight + 4, TFT_BLACK);

  if (textWidth <= DISPLAY_WIDTH) {
    tft.setTextDatum(MC_DATUM); // Middle-Center
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(text, 120, y);
    return;
  }

  // Scroll text
  if (scrollDirection) {
    scrollOffset -= SCROLL_STEP;
    if (scrollOffset <= -(textWidth - DISPLAY_WIDTH + 10)) {
      scrollDirection = false;      // reverse direction
    }
  } else {
    scrollOffset += SCROLL_STEP;
    if (scrollOffset >= 10) {
      scrollDirection = true;       // reverse direction
    }
  }

  tft.setViewport(0, y - 4, 240, areaHeight);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(scrollOffset, 4);   // relative to viewport
  tft.print(text);
  tft.resetViewport();
}

void showNowPlaying() {
  // Draw album art
  if (songName != currentSongName) {
    imageUrl = getArtwork(artistName, songName);

    if (!downloadAndSaveImage(0, 0))
      if(!loadDefaultPicture())
        Serial.println("Failed to load default image");   

    currentSongName = songName;
    imageAngle = 0.0f;
    scrollOffset = 0;
    scrollDirection = true;  
  }

  tft.fillRect(0, 241, 240, 80, TFT_BLACK);
  // Song name
  drawScrollingText(songName, 260, TFT_WHITE, 2);
  // Artist name
  drawScrollingText(artistName, 295, TFT_CYAN, 1);
}

String getArtwork(String artist, String song) {
  if (artist == "" || song == "") {
    Serial.println("Missing song datas.");
    return "";
  }

  HTTPClient http;
  String term = artist + " " + song;
  term.replace(" ", "+");

  String url = "https://itunes.apple.com/search?term=" + term + "&entity=song&limit=1";

  http.begin(url);
  http.setTimeout(15000);

  int code = http.GET();

  if (code != 200) {
    Serial.printf("ITune error: %d\n", code);

    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();

  JsonDocument imgJson;
  deserializeJson(imgJson, payload);

  if (imgJson["resultCount"] == 0) return "";

  String art = imgJson["results"][0]["artworkUrl100"].as<String>(); 
  art.replace("100x100bb", "160x160bb");
  art.replace("http://", "https://");

  return art;
}

bool getNowPlaying() {
  if (userName == "" || apiKey == "") {
    Serial.println("Missing login datas.");
    return false;
  }
  HTTPClient http;

  String url = "http://ws.audioscrobbler.com/2.0/?method=user.getrecenttracks";
  url += "&user=" + userName;
  url += "&api_key=" + apiKey;
  url += "&limit=1&format=json";

  http.begin(url);
  http.setTimeout(15000);

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("Last.fm error: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument songJson;
  deserializeJson(songJson, payload);

  JsonObject track = songJson["recenttracks"]["track"][0];
  artistName = ""; 
  songName   = "";

  // Check if something is currently playing
  isPlaying = (track["@attr"]["nowplaying"].as<String>() == "null") ? false : true;
  songName   = track["name"].as<String>();
  artistName = track["artist"]["#text"].as<String>();
  
  return true;
}

void onWebSocketMessage(const String& message) {
    Serial.printf("WS RX: %s\n", message);
}
void onWebSocketStatus(const String& status) {
    wsStatus = status;
}

void onBluetoothMessage(const String& message) {
    JsonDocument btJson;
    deserializeJson(btJson, message);

    if (btJson["Ssid"].as<String>() != "null") {
      WiFi.disconnect(true);
      saveWifi(btJson["Ssid"].as<String>(), btJson["Password"].as<String>());
      isWifiConnect = false;
      return;
    }
    if (btJson["Ip"].as<String>() == "REQUEST") {
      isIpRequest = true;
      return;
    }
    if (btJson["Setup"].as<String>() == "TRUE") {
      saveSetup("true", "false");
      return;
    }
}
void onBluetoothStatus(const String& status) {
    btStatus = status.substring(0, 9);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); 
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(2); // 180° flip

  //u8g2.begin();
  //u8g2.enableUTF8Print();

  pinMode(RESET_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonISR, FALLING);

  // #define in env.h
  if (LASTFM_API_KEY == "" || LASTFM_USERNAME == "") {
    apiKey = loadApiKey();
    userName = loadUserName();
  }
  else {
    saveUser(LASTFM_USERNAME, LASTFM_API_KEY);
  }

  if (SSID == "" || PASSWORD == "") {
    wifiSsid = loadSsid();
    wifiPassword = loadWifiPass();
  }
  else {
    saveWifi(SSID, PASSWORD);
  }

  isSetup = loadEspSetup();
  isReady = loadEspReady();

  delay(500);
  websocket.begin();
  websocket.setMessageHandler(onWebSocketMessage, onWebSocketStatus);
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  delay(500);
  if (isReady == "false") {
    bluetooth.begin();
    bluetooth.setMessageHandler(onBluetoothMessage, onBluetoothStatus);    
  } 
  else if (isReady == "true") {
    imageBuffer = (uint16_t*)malloc(IMAGE_SIZE * IMAGE_SIZE * sizeof(uint16_t));
    if (!imageBuffer)
      Serial.println("Failed to allocate image buffer");
    if (!loadDefaultPicture())
        Serial.println("Failed to load default image");
  }

}

void loop() {
  unsigned long now = millis();

  if (now - updateOledTimer >= UPDATE_OLED_PERIOD) {
    updateOledTimer = now;
    //drawOled("Connected to", WiFi.localIP().toString(), wsStatus);
  }

  if (WiFi.status() == WL_CONNECTED && isReady == "true") {
    if (now - updateTftTimer >= UPDATE_TFT_PERIOD) {
      updateTftTimer = now;
      drawTftJPEG();
      
      if (isPlaying) {
        imageAngle += 1.0f;
        if (imageAngle >= 360.0f)
          imageAngle = 0.0f;      
      }
    }

    if (now - apiTimer >= API_PERIOD) 
    {
      apiTimer = now;
      if (!getNowPlaying())
        Serial.println("Failed to get current song");
    }

    if (now - lastScrollTime >= SCROLL_INTERVAL) {
      lastScrollTime = now;
      showNowPlaying();
    }
  } 
  else {
    if (now - updateTftTimer >= UPDATE_TFT_PERIOD) {
      updateTftTimer = now;
      spin = (spin + 1) % 4;
      reconnectSpinner();
    }
  }

  if (now - reconnectTimer >= RECONNECT_PERIOD) {
    reconnectTimer = now;

    if (!isWifiConnect && WiFi.status() != WL_CONNECTED) {
      if (wifiSsid != "" && wifiPassword != "") {
        WiFi.disconnect(true);
        WiFi.begin(wifiSsid, wifiPassword);
        isWifiConnect = true;
      }
    }

    if (isIpRequest && btStatus == "CONNECTED" && WiFi.status() == WL_CONNECTED) {
      JsonDocument json; 
      String jsonString = "";

      json["Ip"] = WiFi.localIP().toString();
      serializeJson(json, jsonString);
      bluetooth.send(jsonString);
      isIpRequest = false;
    }

    if (isSetup == "true" && isReady == "false" && WiFi.status() == WL_CONNECTED) {
      btStatus = "";
      saveSetup("true", "true");
      ESP.restart();
    }
  }

  if (now - lastNotify >= NOTIFY_PERIOD) {
    lastNotify = now;
  }

  if (isReset) {
      Serial.println("Resetting");
      saveWifi("", "");
      saveSetup("false", "false");
      ESP.restart();
  }
}
