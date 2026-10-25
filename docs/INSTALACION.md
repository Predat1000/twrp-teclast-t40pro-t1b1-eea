# 2. Instalar TWRP conservando Android

[Empezar desde stock](EMPEZAR.md) · [Desbloqueo](DESBLOQUEO.md) · [Root opcional](ROOT-OPCIONAL.md)

**No necesitas root de Android.** La entrada inicial utiliza fastbootd con bootloader ya desbloqueado. Una vez arrancado TWRP, su propio ADB root permite leer y verificar los bloques.

Solo **Teclast T40 Pro T1B1 EEA, Android stock 11, UMS512 1H10**, con Android en A y la distribución de particiones descrita en [EMPEZAR](EMPEZAR.md). Instalar este recovery escribe **boot_b**. No requiere escribir boot_a, formatear data, cambiar vbmeta ni tocar el bootloader.

## Alcance de la ruta inicial

Los primeros TWRP se instalaron antes de rootear Android. El registro del 18/10/2026 aporta una corrección importante: la lectura íntegra confirmó que los flashes anteriores de stock/V45 sí habían persistido. La aparición de Android Recovery tras `reboot recovery` no demostraba lo contrario. Seleccionar B y hacer **reboot normal** arrancó V45 residente con ADB root.

CP10 se instaló y aceptó mediante escritura directa y hash de B. **La secuencia completa desde stock bloqueado hasta CP10 no se ha repetido de extremo a extremo.** La ruta inicial siguiente se basa en la entrada histórica; solo se acepta en tu unidad cuando la imagen arrancada y el hash real de B quedan comprobados. Un OKAY no basta. `fastboot boot` no se utiliza para acreditar qué imagen se ejecuta.

## Preparar imagen y rescate

Usa las variables `$adb`, `$fb` y `$serial` de [EMPEZAR](EMPEZAR.md). Descarga la imagen de la [release CP10](https://github.com/Predat1000/twrp-teclast-t40pro-t1b1-eea/releases/tag/cp10-v53b) y ajusta su ruta:

```powershell
$img = 'C:\T40Pro\TWRP-T40Pro-T1B1-EEA-CP10-V53B.img'
$expected = 'b029e36391ca919feef66d608eb5964562433d1b205ee0ebf8ba8579a0af4451'
if ((Get-Item $img).Length -ne 67108864) { throw 'Tamano incorrecto' }
if ((Get-FileHash $img -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) { throw 'SHA256 incorrecto' }
```

Ese SHA identifica la imagen de 64 MiB cuya firma AVB se verificó: SHA256_RSA4096, header v2 y página 2048. No usar el boot.img intermediario de la build. Para verificar también el ZIP y sus entradas puedes usar `python tools/verify-release.py RUTA_DESCARGAS`.

Conserva el firmware stock exacto y tu rescate de A fuera de la tablet. Si ya tienes recovery con acceso de bloques, guarda además ambos boot antes de escribir. Si es la primera instalación y no puedes leerlos, registra esa limitación: una imagen stock extraída del firmware no demuestra cuál era el contenido previo de B.

## Ruta A: primera entrada, sin root de Android

Desde Android A operativo y desbloqueado:

```powershell
& $adb -s $serial reboot fastboot
& $fb devices
& $fb -s $serial getvar is-userspace
& $fb -s $serial getvar current-slot
& $fb -s $serial getvar unlocked
& $fb -s $serial getvar has-slot:boot
& $fb -s $serial getvar partition-size:boot_a
& $fb -s $serial getvar partition-size:boot_b
& $fb -s $serial getvar slot-successful:a
& $fb -s $serial getvar slot-unbootable:a
```

Requisitos: tu serial, `is-userspace: yes`, slot `a`, `unlocked: yes`, `has-slot:boot: yes`, ambos boot `0x4000000`, A successful `yes` y unbootable `no`. Si una lectura falta o difiere, detenerse. No usar valores recordados de un reinicio anterior.

Después de verificar modo, slot, desbloqueo, partición, archivo, tamaño y hash:

```powershell
& $fb -s $serial flash boot_b $img
if ($LASTEXITCODE -ne 0) { throw 'Flash fallo; conservar el modo actual y el log' }
```

OKAY solo permite pasar a la comprobación, no declarar instalado el recovery. Selecciona B y comprueba el resultado:

```powershell
& $fb -s $serial set_active b
& $fb -s $serial getvar current-slot
& $fb -s $serial getvar slot-unbootable:b
```

Solo si devuelve B y unbootable `no`, reinicia normalmente:

```powershell
& $fb -s $serial reboot
```

**No usar `reboot recovery` para esta entrada.** En la configuración probada, Android normal no existe en B; el helper de esta imagen inicia TWRP al arrancar boot_b.

Si aparece Android Recovery o no hay TWRP/ADB, comprueba el modo real antes de otra acción. No encadenar flashes ni esperar a que varios fallbacks arreglen el problema. La sección de rescate explica cómo volver a A desde fastboot comprobado.

## Identificar TWRP y verificar lo escrito

El aviso inicial puede cerrarse con **Keep Read Only**, que se refiere al sistema. No hace falta deslizar para permitir cambios en system.

```powershell
& $adb devices -l
& $adb -s $serial shell id
& $adb -s $serial shell getprop ro.boot.slot_suffix
& $adb -s $serial shell getprop ro.bootmode
& $adb -s $serial shell getprop ro.t40.build
& $adb -s $serial shell getprop ro.twrp.version
& $adb -s $serial shell readlink -f /dev/block/by-name/boot_b
& $adb -s $serial shell blockdev --getsize64 /dev/block/by-name/boot_b
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_b
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_a
```

Se requiere uid=0, slot `_b`, etiqueta `V53B-native-crypto-drm-guard`, TWRP `3.7.0_11-0`, B de 67108864 bytes y **SHA completo de B igual a $expected**. La etiqueta se comparte con la referencia V53B: el hash distingue CP10. Registrar también A y compararlo con su lectura previa si se disponía de ella.

Si B coincide, no lo vuelvas a escribir. Si no coincide, no declares CP10 instalado ni reinicies a ciegas. Conserva el acceso y los logs; solo usa la ruta directa si el recovery actual es conocido y fiable.

## Ruta B: escritura directa desde un recovery conocido

Esta ruta también sirve para futuras actualizaciones. No requiere APatch en Android. Antes de escribir, se exige recovery con ADB uid=0 estable, estado unlocked acreditado, slot conocido, enlaces y tamaños de ambos boot comprobados. Guarda las imágenes completas en `/tmp`, léelas al PC con `adb pull` y compara los SHA de los archivos con los de los bloques. No uses números mmcblk de otra tablet.

Registra el SHA previo de A y de B. Comprueba de nuevo el archivo del PC con el bloque anterior y después:

```powershell
& $adb -s $serial push $img /tmp/cp10.img
& $adb -s $serial shell stat -c %s /tmp/cp10.img
& $adb -s $serial shell sha256sum /tmp/cp10.img
```

Solo tras confirmar 67108864 bytes y el SHA de CP10:

```powershell
& $adb -s $serial shell "dd if=/tmp/cp10.img of=/dev/block/by-name/boot_b bs=4M && sync && blockdev --flushbufs /dev/block/by-name/boot_b"
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_b
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_a
```

**No reiniciar si B no coincide o A ha cambiado.** El rollback consiste en restaurar tu copia de B desde el acceso root conservado y verificarla de la misma manera. Para arrancar lo escrito, entra en fastbootd, verifica `is-userspace=yes`, desbloqueo, tamaño y slot; selecciona B y haz reboot normal como en la ruta A.

## Validar y volver a Android

Comprueba que ves TWRP, que el táctil abre un menú y que ADB/USB se mantienen estables. Confirma almacenamiento y montajes sin formatear ni cambiar credenciales. El alcance de descifrado probado es el de claves existentes sin PIN/contraseña; una pantalla de contraseña o data vacía no se resuelve haciendo Format Data.

En esta build, **Reboot > System selecciona A** y **Reboot > Recovery selecciona B**. Tras volver a Android, comprueba:

```powershell
& $adb -s $serial shell getprop ro.boot.slot_suffix
& $adb -s $serial shell getprop sys.boot_completed
```

Deben ser `_a` y `1`. Android puede seguir sin root: eso es correcto y completa el objetivo de instalar TWRP.

Para entrar otra vez desde Android, usa `adb reboot fastboot`, verifica fastbootd, selecciona B, comprueba el slot y haz reboot normal. El comando `adb reboot recovery` desde Android A no garantiza entrar en TWRP B.

Comprueba también **Reboot > Fastboot** en TWRP y `getvar is-userspace=yes`. Para salir de ese modo a Android, selecciona A, verifica `current-slot=a` y `slot-unbootable:a=no`, y reinicia normalmente.

## Rescate si TWRP no arranca

Comprueba ADB, fastboot y el dispositivo USB real antes de pulsar botones. Cuando hayas recuperado fastbootd con tu serial, modo y slot comprobados:

```powershell
& $fb -s $serial set_active a
& $fb -s $serial getvar current-slot
& $fb -s $serial getvar slot-unbootable:a
```

Solo con A y unbootable `no`, ejecuta `& $fb -s $serial reboot`. Esto selecciona el Android conservado; no reescribe boot_a.

Si el modo/slot no puede comprobarse o Android A tampoco vuelve, detente y conserva los logs. Un PAC completo o un factory reset no son pasos de instalación de TWRP.

Puedes terminar aquí. [Root de Android es opcional](ROOT-OPCIONAL.md).
