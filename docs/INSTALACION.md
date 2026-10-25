# Instalación y vía de recuperación

**Solo Teclast T40 Pro T1B1 EEA, Android stock 11, UMS512 1H10.** No se ofrece un instalador automático. Este procedimiento presupone acceso root de bloques ya operativo y un rescate comprobado; no explica desbloqueo, root inicial ni unbrick.

## Condiciones

1. Confirmar el modelo, firmware, serial objetivo y modo real; nunca operar sobre el primer dispositivo sin identificarlo.
2. Bootloader desbloqueado, Android/rescate funcional en A, backup propio de boot_a y boot_b fuera de la tablet.
3. Confirmar las particiones de 64 MiB y resolver sus enlaces `/dev/block/by-name/boot_a` y `boot_b`. No copiar números de mmcblk de otra tablet.
4. Comprobar que las particiones dinámicas A son las pobladas. Esta imagen fija los montajes en A.
5. Verificar archivo, tamaño y SHA-256 de la release y conservar una copia de rollback.
6. Tener ADB root operativo en un recovery conocido. Si solo hay fastboot, **detenerse**: en este proyecto su respuesta OKAY no basta para verificar una escritura. La imagen no resuelve por sí sola el acceso inicial a un recovery.

No borrar userdata, formatear data, modificar SPL/FDL/U-Boot, flashear un PAC completo ni escribir boot_a para instalar este recovery.

## Verificar el archivo en el PC

```sh
python tools/verify-release.py /ruta/a/archivos-descargados
```

El directorio debe contener `SHA256SUMS`, la imagen y el ZIP de reproducción publicados. También puede verificarse el hash de la imagen con `sha256sum` o `Get-FileHash -Algorithm SHA256`.

## Escritura directa desde un recovery con ADB root

Son comandos para una sesión controlada, **no un bloque para pegar a ciegas**. Sustituir SERIAL por el objetivo comprobado. Guardar antes una copia de ambas particiones con `adb pull` desde ese recovery y verificar tamaño y hash en el PC.

```sh
adb -s SERIAL shell id
adb -s SERIAL shell getprop ro.boot.slot_suffix
adb -s SERIAL shell getprop ro.bootmode
adb -s SERIAL shell getprop ro.boot.flash.locked
adb -s SERIAL shell getprop ro.boot.vbmeta.device_state
adb -s SERIAL shell readlink -f /dev/block/by-name/boot_b
adb -s SERIAL shell blockdev --getsize64 /dev/block/by-name/boot_b
adb -s SERIAL shell sha256sum /dev/block/by-name/boot_a
adb -s SERIAL shell sha256sum /dev/block/by-name/boot_b
```

Se requiere uid=0 y 67108864 bytes para B. Registrar el hash original de A. Ante cualquier discrepancia, detenerse. Verificar el estado unlocked desde fastboot cuando las propiedades no lo acrediten; no asumirlo.

```sh
adb -s SERIAL push TWRP-T40Pro-T1B1-EEA-CP10-V53B.img /tmp/cp10.img
adb -s SERIAL shell stat -c %s /tmp/cp10.img
adb -s SERIAL shell sha256sum /tmp/cp10.img
```

Solo tras confirmar 67108864 bytes y el SHA exacto indicado en README:

```sh
adb -s SERIAL shell "dd if=/tmp/cp10.img of=/dev/block/by-name/boot_b bs=4M && sync"
adb -s SERIAL shell sha256sum /dev/block/by-name/boot_b
adb -s SERIAL shell sha256sum /dev/block/by-name/boot_a
```

**No reiniciar si el hash completo de B no coincide o el de A ha cambiado.** Conservar los logs y el acceso actual. La recuperación consiste en restaurar el backup propio de B mediante acceso root ya verificado y volver a comprobar el hash, manteniendo A.

## Selección de slot y arranque

La ruta ensayada utiliza fastbootd real (`getvar is-userspace` devuelve `yes`) para seleccionar B, comprobar `current-slot` y reiniciar. Identificar siempre el dispositivo con `fastboot devices`; verificar `getvar unlocked`, `getvar current-slot`, `getvar partition-size:boot_b` y `getvar slot-unbootable:b` antes de actuar. La ruta exacta de entrada a ese modo depende del recovery que ya esté arrancado.

La selección de B debe llevar a TWRP, identificado por `ro.t40.build=V53B-native-crypto-drm-guard`, ADB uid=0 y `ro.twrp.version`. Una pantalla stock o un OKAY de tempboot no acredita la ejecución de esta imagen.

En esta build, el reinicio nativo de TWRP a **System selecciona A** y el reinicio a **Recovery selecciona B**. Las particiones dinámicas B están vacías: no intentar arrancar Android normal en B. Si TWRP no funciona, utilizar el modo fastboot ya comprobado para seleccionar A y recuperar Android. No continuar con escrituras a ciegas si tampoco se puede comprobar ese rescate.

Validar ADB, GUI/táctil, USB, montajes y regreso a Android antes de usar funciones que escriban datos. No cambiar PIN ni ejecutar restauraciones completas basándose en esta validación limitada.
