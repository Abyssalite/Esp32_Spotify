#include <FS.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
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
//#include "Bluetooth.h"
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

unsigned long updateOledTimer = 0;
unsigned long updateTftTimer = 0;
unsigned long reconnectTimer = 0;
unsigned long apiTimer = 0;
unsigned long lastNotify = 0;
unsigned long lastScrollTime = 0;

//uint8_t count = 0;
uint16_t *imageBuffer = nullptr;
float imageAngle = 0.0f;
int scrollOffset = 0;
bool scrollDirection = true;   // true = moving left, false = moving right

char* ssid     = "";
char* password = "";
char* status   = "";
String message = "";

String apiKey   = "";
String userName = "";

String imageUrl   = "";
String songName   = "";
String artistName = ""; 
String currentSongName = "";
bool isPlaying = false;

JsonDocument telemetryJson;
Preferences preferences;

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;
//Bluetooth bluetooth;
WebSocket websocket;

void saveUser(String key, String user) {
  preferences.begin("last.fm", false); //read-write
  preferences.putString("key", key);
  preferences.putString("user", user);
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

void drawOled () {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.setCursor(4, 8);
    u8g2.print("Connected to");

    u8g2.setCursor(4, 18);
    u8g2.print(WiFi.localIP());

    u8g2.setCursor(4, 28);
    u8g2.print(status);
  } while (u8g2.nextPage()); 
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
    Serial.println("Download Image failed.");
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  // Get the image data
  int totalLen = http.getSize();
  if (totalLen <= 0) return false;

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
    Serial.println("Incomplete download - abort");
    free(buffer);
    return false;
  }
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

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
    imageUrl = getItunesArtwork(artistName, songName);

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

String getItunesArtwork(String artist, String song) {
  if (artist == "" || song == "") {
    Serial.println("Missing song datas.");
    return "";
  }

  HTTPClient http;
  String term = artist + " " + song;
  term.replace(" ", "+");

  String url = "https://itunes.apple.com/search?term=" + term + "&entity=song&limit=1";

  http.begin(url);
  int code = http.GET();

  if (code != 200) {
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
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("Get Song Data failed.");
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

void onWebSocketMessage(const String& message)
{
    Serial.printf("WS RX: %s\n", message);
}
void onWebSocketStatus(const String& status)
{
    Serial.println(status);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(SSID, PASSWORD);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(2); // 180° flip

  u8g2.begin();
  u8g2.enableUTF8Print();

  // #define in env.h
  if (LASTFM_API_KEY == "" || LASTFM_USERNAME == "") {
    apiKey = loadApiKey();
    userName = loadUserName();
  }
  else {
    apiKey = LASTFM_API_KEY;
    userName = LASTFM_USERNAME;
    saveUser(apiKey, userName);
  }
  
  delay(500);
  imageBuffer = (uint16_t*)malloc(IMAGE_SIZE * IMAGE_SIZE * sizeof(uint16_t));
  if (!imageBuffer)
      Serial.println("Failed to allocate image buffer");
  if(!loadDefaultPicture())
      Serial.println("Failed to load default image");

  delay(500);
  websocket.setMessageHandler(onWebSocketMessage, onWebSocketStatus);
  websocket.begin();
  delay(500);
  //bluetooth.begin();
}

void loop() {
  unsigned long now = millis();

  if (now - updateOledTimer >= UPDATE_OLED_PERIOD) {
    updateOledTimer = now;
    drawOled();
  }

  if (now - updateTftTimer >= UPDATE_TFT_PERIOD && isPlaying) {
    updateTftTimer = now;
    drawTftJPEG();
    
    imageAngle += 1.0f;
    if (imageAngle >= 360.0f)
      imageAngle = 0.0f;
  }

  if (now - apiTimer >= API_PERIOD && 0)//(WiFi.status() == WL_CONNECTED)) 
  {
    apiTimer = now;
    if (!getNowPlaying())
      Serial.println("Failed to get current song");
  }

  if (now - lastScrollTime >= SCROLL_INTERVAL) {
    lastScrollTime = now;
    showNowPlaying();
  }

  if (now - lastNotify >= NOTIFY_PERIOD) {
    lastNotify = now;
  }

}
