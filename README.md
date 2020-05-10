# Práctica 1 - Servidor Web.
Iris Álvarez Nieto y Carmen Díez Menéndez. Pareja 3.

## Decisiones de diseño y funcionamiento

Se incluyen en la [Wiki](../../wikis/home).

## Instrucciones para ejecutar el servidor
### Compilación y demonización del proceso

1. Añadir la carpeta **media** a **www** del [zip proporcionado](https://moodle.uam.es/mod/resource/view.php?id=1318802).
1. `make` en la carpeta principal.
2. Crear en **/etc/systemd/system/** un archivo **.service** como este, modificando `User`, `ExecStart` y `WorkingDirectory`.
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
WorkingDirectory=/home/dmcarmen/Desktop/Redes2/practica1

[Install]
WantedBy=multi-user.target
```

3. Correr el proceso y pararlo eventualmente.
```console
foo@bar:~$ sudo systemctl start server
foo@bar:~$ sudo systemctl stop server
```
Todas las notificaciones aparecerán en el System Log.
