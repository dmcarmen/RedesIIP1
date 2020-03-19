# Práctica 1 - Servidor Web.
Iris Álvarez Nieto y Carmen Díez Menéndez. Pareja 3.


## Instrucciones
### Compilación

1. Añadir el path absoluto al archivo **server.conf** en **main.c** en el define `PATH_CONF`. Por ejemplo:
```cpp
#define PATH_CONF "/home/dmcarmen/Desktop/Redes2/practica1/server.conf"
```
2. Añadir el path absoluto al root del server en **server.conf**. Por ejemplo:
```
server_root = "/home/dmcarmen/Desktop/Redes2/practica1/www"
```
3. `make` en la carpeta principal.

### Preparar el proceso daemon del sistema
Creamos en **/etc/systemd/system/** un archivo **.service** como este, modificando *User* y *ExecStart*.
```console
foo@bar:~$ sudo cat /etc/systemd/system/server.service
[Unit]
Description=Server redes II
After=network.target
StartLimitIntervalSec=0
[Service]
Type=simple
Restart=on-failure
RestartSec=1
User=dmcarmen
ExecStart=/home/dmcarmen/Desktop/Redes2/practica1/main

[Install]
WantedBy=multi-user.target
```

### Correr y parar el proceso
```console
foo@bar:~$ sudo systemctl start server
foo@bar:~$ sudo systemctl stop server
```
Todas las notificaciones aparecerán en el System Log.
