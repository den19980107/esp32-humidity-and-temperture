#include "Server.h"
#include "oled.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
std::string index_html PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>Connect to Wifi</h1></p>
  <form method="POST" action="/connectWifi" enctype="multipart/form-data" style="display:flex; flex-direction:column; margin-bottom: 1rem;">
    <label>WIFI SSID:</label>
    <select name="ssid" id="ssid-select">
      <option value=""> Please choose a wifi </option>
      {{options}}
    </select>

    <label>WIFI PASSWORD:</label>
    <input type="password" name="password"/>
    <input type="submit" name="connect" value="connect" title="connect">
  </form>
</body>
</html>
)rawliteral";
std::vector<String> scanedWifi;
bool fillUpWifiInfo = false;
String ssid;
String password;

void server_init() {
  _server_setup();

  _server_scan_wifis();

  _server_config_web_server();

  _server_wait_till_wifi_info_filled();

  _server_connect_to_user_select_wifi();
}

void _server_setup() {
  String ssid = "esp-captive";
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid);
  delay(100);
  IPAddress Ip(192, 168, 7, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  oled_print_ap_info(ssid, Ip, NMask);
}

void replaceAll(std::string &str, const std::string &from,
                const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing
                              // 'x' with 'yx'
  }
}

void _server_config_web_server() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    std::string anchor = "{{options}}";
    std::string optionsHtml = "";

    for (int i = 0; i < scanedWifi.size(); i++) {
      char buff[500];
      int n = sprintf(buff, "<option value=\"%s\">%s</option>", scanedWifi[i],
                      scanedWifi[i]);
      optionsHtml += buff;
    }

    replaceAll(index_html, anchor, optionsHtml);

    request->send(200, "text/html", String(index_html.c_str()));
  });

  server.on("/connectWifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    int paramsNr = request->params();

    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);

      if (p->name() == "ssid") {
        ssid = p->value();
      } else if (p->name() == "password") {
        password = p->value();
      } else {
        continue;
      }
    }

    fillUpWifiInfo = true;
    request->redirect("/");
  });

  server.begin();
}

void _server_scan_wifis() {
  Serial.println("Scanning for WiFi networks...");

  // Perform the Wi-Fi scan
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("No networks found");
    return;
  }

  for (int i = 0; i < n; ++i) {
    char buf[500];
    std::sprintf(buf, "%s", WiFi.SSID(i).c_str());
    scanedWifi.push_back(String(buf));

    // Print SSID and RSSI for each network found
    Serial.printf(buf);
  }

  // Clear the results from the scan to free up memory
  WiFi.scanDelete();
}

void _server_wait_till_wifi_info_filled() {
  while (!fillUpWifiInfo) {
    Serial.println("waiting for fill up wifi info");
    delay(1000);
  }
}

void _server_connect_to_user_select_wifi() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  Serial.print("WiFi connecting");

  // 當WiFi連線時會回傳WL_CONNECTED，因此跳出迴圈時代表已成功連線
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("IP位址:");
  Serial.println(WiFi.localIP()); // 讀取IP位址
  Serial.print("WiFi RSSI:");
  Serial.println(WiFi.RSSI()); // 讀取WiFi強度
  Serial.print("MAC Address:");
  Serial.println(WiFi.macAddress()); // 讀取WiFi強度

  oled_print_connect_wifi_info(ssid, WiFi.localIP(), WiFi.RSSI(),
                               WiFi.macAddress());
}