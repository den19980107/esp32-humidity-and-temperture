#ifndef SERVER_H
#define SERVER_H

void server_init();

void _server_setup();
void _server_config_web_server();
void _server_scan_wifis();
void _server_wait_till_wifi_info_filled();
void _server_connect_to_user_select_wifi();

#endif