#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "TPA Darul Hikmah 2.4";
const char* password = "1sampe10";
const char* websocket_server_host = "192.168.18.50";
const uint16_t websocket_server_port = 8888;

using namespace websockets;
WebsocketsClient client;
bool isWebSocketConnected;

void onEventsCallback(WebsocketsEvent event, String data){
  if(event == WebsocketsEvent::ConnectionOpened){
    Serial.println("Connection Opened");
    isWebSocketConnected = true;
  }else if(event == WebsocketsEvent::ConnectionClosed){
    Serial.println("Connection Closed");
    isWebSocketConnected = false;
    webSocketConnect();
  }
}

void setup() {
  isWebSocketConnected = false;
  Serial.begin(115200);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 60;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  client.onEvent(onEventsCallback);
  webSocketConnect();
}

int reconnectDelay = 1000;
void webSocketConnect(){
   while(!client.connect(websocket_server_host, websocket_server_port, "/")){
    delay(reconnectDelay);
    reconnectDelay *= 2;
    Serial.print(".");
  }
  Serial.println("Websocket Connected!");
  isWebSocketConnected = true;
}

void loop() {

  if(client.available()){
    client.poll();
  }
  
  if(!isWebSocketConnected){
    webSocketConnect();
    return;
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }
  
  fb->buf[12] = 0x01; //Kamera Kiri
  //fb->buf[12] = 0x02; //Kamera Tengah
  //fb->buf[12] = 0x03; //Kamera Kanan

  bool sendSuccess = false;
  for(int i = 0; i < 3 && !sendSuccess; i++){
    sendSuccess = client.sendBinary((const char*) fb->buf, fb->len);
    delay(100); // Delay between send attempts
  }

  esp_camera_fb_return(fb);
}
