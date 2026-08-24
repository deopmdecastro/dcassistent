# frontend-preview — apenas serve o mockup HTML/CSS/JS estático (Echo Mind UI)
# para pré-visualização no browser. NÃO é o firmware real: o ecrã LVGL só
# corre na placa ESP32-S3 (ES3C28P) física — ver firmware/README.md.
FROM nginx:1.27-alpine

COPY index.html /usr/share/nginx/html/index.html

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
    CMD wget -qO- http://localhost/ >/dev/null || exit 1
