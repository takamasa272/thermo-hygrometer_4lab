# thermo-hygrometer
ESP32 + AHT25

Wi-Fi接続し，AHT25から温湿度を取得，AmbientやGoogle Apps Script にロギングしていく．

# Wi-Fi/ambient/Google Apps Script接続について
SSIDとパスワードなどのクレデンシャル情報は`wifi_credentials.h`などのクレデンシャル用ヘッダファイルに定義しておく．
企業/大学等のネットワークに接続できるように，個人の`ID`と`password`を用いて接続する`WPA2-enterprise` (`EPA-PEAP`)の認証方式に対応している．

このファイルは`.gitignore`で同期しないようになっている．（インシデントは良くないので）

雛形は `wifi_credentials.h.sample` 等の`.sample`に掲示するので，適宜内容を変更の上，`wifi_credentials.h`にリネームすることで使用可能と思われる．
