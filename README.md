# Server Monitor IoT

Projeto dividido em duas partes:

- `backend/`: API HTTP em Python para métricas do servidor Debian.
- `firmware/esp8266_monitor/`: sketch para ESP8266 com display OLED SSD1306.

## Backend

### Dependências

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Execução

```bash
export CPU_TDP_WATTS=65
python app.py
```

API padrão: `http://0.0.0.0:8000/metrics`

## Firmware ESP8266

### Bibliotecas da Arduino IDE

- `ESP8266WiFi`
- `ESP8266HTTPClient`
- `ArduinoJson`
- `Adafruit GFX Library`
- `Adafruit SSD1306`

### Conversão da sua logo para XBM / PROGMEM

1. Crie uma imagem monocromática com o tamanho do seu bitmap, por exemplo `32x32` ou `64x64`.
2. Exporte em XBM usando o GIMP, ImageMagick ou um conversor online.
3. Copie o array gerado e substitua `logoBitmap`.
4. Ajuste o `drawBitmap(x, y, logoBitmap, largura, altura, SSD1306_WHITE)`.

## Git

### Inicializar e publicar

```bash
git init
git branch -M main
git add .
git commit -m "Initial commit: backend API and ESP8266 firmware"
git remote add origin https://github.com/SEU_USUARIO/SEU_REPOSITORIO.git
git push -u origin main
```
