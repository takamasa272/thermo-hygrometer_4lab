# thermo-hygrometer
ESP32 + AHT25
(+ SSD1306 OLED Display)

Wi-Fi接続し，AHT25から温湿度を取得，AmbientやGoogle Apps Script にロギングしていく．

# Wi-Fi/ambient/Google Apps Script接続について
SSIDとパスワードなどのクレデンシャル情報は`wifi_credentials.h`などのクレデンシャル用ヘッダファイルに定義しておく．
企業/大学等のネットワークに接続できるように，個人の`ID`と`password`を用いて接続する`WPA2-enterprise` (`EPA-PEAP`)の認証方式に対応している．

このファイルは`.gitignore`で同期しないようになっている．（インシデントは良くないので）

雛形は `wifi_credentials.h.sample` 等の`.sample`に掲示するので，適宜内容を変更の上，`wifi_credentials.h`にリネームすることで使用可能と思われる．

# ファイル構造
```
thermo-hygrometer_4lab/
│
├── ambient_credentials.h
├── gas_credentials.h
├── wifi_credentials.h
│
├── thermo-hygrometer_4lab.ino
├── oled_ssd1306.ino
├── aht25.ino
└── network.ino

また，`GoogleAppsScript_code.gs` をgoogle Spreadsheetにてgoogle Apps Scriptにコピーし，デプロイし，urlを`gas_credentials.h`にしるすをとでスプレッドシートにロギングできる．
```