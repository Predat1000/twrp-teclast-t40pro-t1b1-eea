# 3. Root de Android con APatch — opcional

[Volver a empezar](EMPEZAR.md) · [Instalar TWRP sin root de Android](INSTALACION.md)

**Puedes omitir toda esta página.** TWRP ya tiene su propio ADB root y no necesita APatch para funcionar. Esta etapa cambia el boot de Android en **A**; es una decisión separada de instalar TWRP en B.

## Configuración probada

CP01 quedó aceptado con APatch Manager **11282** (versionName `e8e4651`), **KernelPatch 0.13.9** y **AndroidPatch FULL**. Se verificaron uid=0, `apd` activo, SELinux Enforcing, escritura en `/data/adb` y tres reinicios completos de 47,5 / 47,5 / 47,4 segundos. El root estable se conservó después de instalar CP10.

Descarga la [release oficial APatch 11282](https://github.com/bmax121/APatch/releases/tag/11282). El APK usado fue `APatch_11282_e8e4651_HEAD-release-signed.apk`, 6716426 bytes, SHA-256:

```text
d6bf0dbcab95fb9a3aa111c35efd945267bcce3a41377c3297c3c1a9eab421a5
```

El hash local coincide con el digest del asset oficial. Esta validación no se transfiere automáticamente a otras versiones, módulos KPM o métodos de root.

**No se distribuye el boot rooteado personal de CP01 ni una SuperKey común.** Cada usuario debe parchear su propio boot stock y mantener privada su imagen resultante.

## Preparación y backup propio

Primero termina la instalación/verificación de TWRP y comprueba que puedes volver a Android A. No uses root como requisito para entrar en TWRP.

Desde TWRP B con ADB uid=0 estable, comprueba bootloader desbloqueado, slot B, enlaces por nombre y tamaño de **ambos** boot. B debe tener el SHA CP10 publicado. Guarda los hashes originales de A y B.

Con las variables de [EMPEZAR](EMPEZAR.md), crea una copia de A en RAM y cópiala al PC:

```powershell
& $adb -s $serial shell "dd if=/dev/block/by-name/boot_a of=/tmp/boot_a-original.img bs=4M && sync"
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_a
& $adb -s $serial shell sha256sum /tmp/boot_a-original.img
& $adb -s $serial pull /tmp/boot_a-original.img .\boot_a-original.img
Get-FileHash .\boot_a-original.img -Algorithm SHA256
(Get-Item .\boot_a-original.img).Length
```

Los tres hashes deben coincidir y el tamaño ser 67108864 bytes. Guarda la copia fuera de la tablet. Haz lo mismo con B usando otro nombre; es tu recovery de rescate. No capturar binarios mediante redirección textual de PowerShell o de `su`.

La receta AVB de abajo se limita al stock usado como rollback de CP01:

```text
5f2aab689c1f5de0927e67834e8797a9d99e43a32830cad5409bacf6eb0a3c8a
```

**Si tu boot original no coincide, no aplicar esa receta de firma ni suponer que es el mismo firmware.** Conserva el original y analiza su header, DTB y metadatos AVB antes de preparar otra variante.

## Preparar tu imagen

Vuelve a Android A e instala el APK verificado. En APatch Manager selecciona tu copia stock para parchear `boot.img`, configura una **SuperKey propia, fuerte y privada**, y guarda el archivo resultante en el PC. No parchees `init_boot`, el boot de TWRP ni una imagen modificada por otro gestor.

El procedimiento de parcheo está en la [documentación oficial de APatch](https://apatch.dev/install.html). Evita poner la SuperKey en capturas, repositorios o logs; puede quedar incorporada al boot. La imagen parcheada por el Manager todavía debe pasar la firma y comprobación específica de esta tablet.

## Firmar y comprobar offline

Se necesita Python, OpenSSL accesible y una **clave externa compatible con el stock**. El archivo `reproduce/avbtool.py` está en este repositorio. La clave privada no se incluye. La procedencia registrada de la clave usada es [vendor_sprd_proprietories-source_packimage](https://github.com/TomKing062/vendor_sprd_proprietories-source_packimage), archivo `config-unisoc/rsa4096_boot.pem`; verifica su identidad criptográfica, no solo el nombre.

No continuar sin la clave apropiada. No sustituir la firma por desactivar AVB ni escribir vbmeta.

En una carpeta nueva para la firma, ajusta las rutas. `$stock` es tu backup original; `$patched` es el resultado de APatch. La salida debe ser un archivo nuevo llamado `boot.img`, separado de ambos:

```powershell
$stock = 'C:\T40Pro\boot_a-original.img'
$patched = 'C:\T40Pro\apatch-patched-personal.img'
$avb = 'C:\T40Pro\twrp-teclast-t40pro-t1b1-eea\reproduce\avbtool.py'
$key = 'C:\T40Pro\claves-privadas\rsa4096_boot.pem'
$work = 'C:\T40Pro\firma-apatch-nueva'
if (Test-Path $work) { throw 'Usa una carpeta de salida nueva' }
if ((Get-FileHash $stock -Algorithm SHA256).Hash.ToLowerInvariant() -ne '5f2aab689c1f5de0927e67834e8797a9d99e43a32830cad5409bacf6eb0a3c8a') { throw 'Stock distinto al probado' }
New-Item -ItemType Directory $work
$out = Join-Path $work 'boot.img'
$env:PATH = 'C:\Program Files\Git\mingw64\bin;' + $env:PATH
py -3.12 $avb extract_public_key --key $key --output "$work\boot.avbpubkey"
if ($LASTEXITCODE -ne 0) { throw 'No se pudo comprobar la clave' }
if ((Get-FileHash "$work\boot.avbpubkey" -Algorithm SHA1).Hash.ToLowerInvariant() -ne 'ea410c1b46cdb2e40e526880ff383f083bd615d5') { throw 'Clave incompatible' }
Copy-Item $patched $out
py -3.12 $avb erase_footer --image $out
if ($LASTEXITCODE -ne 0) { throw 'Revisar el formato de la imagen parcheada' }
py -3.12 $avb add_hash_footer --image $out --partition_size 67108864 --partition_name boot --algorithm SHA256_RSA4096 --key $key --salt d85454e69ecebd81e2d9e0bb6bcc5d93a00b571a460ec7c35f059bec96985fcd --prop 'com.android.build.boot.fingerprint:Teclast/T40Pro_EEA/T40Pro_EEA:11/RP1A.201005.001/20210924:user/release-keys' --prop 'com.android.build.boot.os_version:11'
if ($LASTEXITCODE -ne 0) { throw 'Fallo de firma AVB' }
py -3.12 $avb info_image --image $out
py -3.12 $avb verify_image --image $out --key $key
if ($LASTEXITCODE -ne 0) { throw 'AVB no verificado' }
if ((Get-Item $out).Length -ne 67108864) { throw 'Tamano incorrecto' }
Get-FileHash $out -Algorithm SHA256
```

Conserva el hash de **tu** salida como `HASH_PERSONAL`; será diferente del checkpoint porque has usado tu propia SuperKey. Verifica también que el parche conserva el boot header v2, página 2048 y DTB del stock. Los parámetros de firma anteriores proceden de la receta conservada de CP01; la guía reordena el procedimiento, no afirma una nueva prueba de una SuperKey diferente.

## Instalar únicamente tu boot de Android

Arranca TWRP B y vuelve a comprobar ADB uid=0, slot B, estado unlocked, enlaces, 64 MiB, hash original de A y hash CP10 de B. No continuar si cambió algo desde el backup. Copia la imagen personal firmada y verifica su tamaño y SHA dentro de TWRP:

```powershell
& $adb -s $serial push $out /tmp/android-root-personal.img
& $adb -s $serial shell stat -c %s /tmp/android-root-personal.img
& $adb -s $serial shell sha256sum /tmp/android-root-personal.img
```

Solo si son 67108864 bytes y tu `HASH_PERSONAL`, esta escritura optativa sustituye **A**, conservando TWRP en B:

```powershell
& $adb -s $serial shell "dd if=/tmp/android-root-personal.img of=/dev/block/by-name/boot_a bs=4M && sync && blockdev --flushbufs /dev/block/by-name/boot_a"
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_a
& $adb -s $serial shell sha256sum /dev/block/by-name/boot_b
```

A debe coincidir exactamente con `HASH_PERSONAL` y B debe seguir siendo CP10. **No reiniciar si no coinciden.** No se usa fastboot flash A como prueba de instalación.

## Completar AndroidPatch y verificar

En TWRP, **Reboot > System** selecciona A. Tras arrancar Android, abre APatch Manager con tu SuperKey y completa la instalación de AndroidPatch hasta obtener el estado FULL. Concede permisos root únicamente a las aplicaciones que decidas usar; para la comprobación siguiente, autoriza el shell ADB del PC de confianza.

```powershell
& $adb -s $serial shell getprop ro.boot.slot_suffix
& $adb -s $serial shell getprop sys.boot_completed
& $adb -s $serial shell su -c id
& $adb -s $serial shell pidof apd
& $adb -s $serial shell getenforce
```

Se espera `_a`, `1`, uid=0, un PID de apd y `Enforcing`. Verifica también acceso de lectura/escritura a `/data/adb`, Wi-Fi e Internet. Realiza tres reinicios completos, comprobando esos resultados después de cada uno, antes de considerar tu root estable. No instalar módulos durante esa validación.

No se ha validado aquí instalar el APK renombrado a ZIP desde TWRP ni un instalador que elija automáticamente el slot. Esos métodos pueden no respetar la firma y distribución A/B de este proyecto.

## Rollback

Si Android no arranca, vuelve al TWRP B comprobado mediante la [ruta de rescate](INSTALACION.md). Desde su ADB root, restaura **tu backup original de boot_a** con el mismo procedimiento de copia, verificación, dd, sync y SHA completo. Comprueba que B sigue intacto y vuelve a System/A.

Restaurar el boot original revierte ese cambio de arranque; no equivale a borrar todos los archivos del Manager de userdata. No hacer wipe, no restaurar claves de cifrado de otra instalación y no bloquear el bootloader como intento de recuperación.
