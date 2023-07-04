#pragma once
static std::string getMac() {
  uint8_t mac_id[6];
  esp_efuse_mac_get_default(mac_id);
  char esp_mac[13];
  sprintf(esp_mac, "%02x%02x%02x%02x%02x%02x", mac_id[0], mac_id[1], mac_id[2], mac_id[3], mac_id[4], mac_id[5]);
  return std::string(esp_mac);
}
