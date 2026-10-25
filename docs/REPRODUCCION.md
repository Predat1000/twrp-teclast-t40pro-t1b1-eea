# Reproducción de CP10

La release incluye `CP10-reproduction-inputs.zip`. Descomprimirlo en una ruta nueva: contiene `reproduce/` con las entradas exactas y `INPUTS-SHA256.json`. No incluye la clave privada de firma.

## Dos niveles

1. **Reensamblaje de artefactos conservados:** reproduce el contenido de la build limpia aceptada usando sus 120 ELF, stock runtime, kernel y DTB congelados.
2. **Nueva compilación nativa:** sincronizar los commits del manifest, aplicar parches, compilar en una salida vacía y exportar los nuevos ELF. Deben repetirse las auditorías y probar cualquier imagen cuyo hash cambie. No se promete identidad de bytes entre hosts.

El kernel stock se reutiliza como prebuilt. La distribución no contiene el código fuente específico de ese kernel del fabricante. El manifiesto y parches describen el userspace Android 11 / TWRP, no una compilación completa del firmware.

## Reensamblaje

Python 3.12, OpenSSL en PATH y pyelftools 0.32:

```sh
cd reproduce
python -m pip install -r requirements.txt
python assemble_clean.py --output ../assembled-new --avbtool avbtool.py --key /ruta/externa/rsa4096_boot.pem
```

La carpeta de salida debe ser nueva. El ensamblador comprueba entradas, interfaces ELF, arquitectura, símbolos, parches exactos de init, ConfigFS/etiqueta de build, header y AVB. No conecta con ADB ni escribe particiones.

La firma de CP10 espera clave pública SHA-1 `ea410c1b46cdb2e40e526880ff383f083bd615d5`. La clave privada se mantiene fuera del proyecto público. Si no se dispone de la clave apropiada, no se puede generar la misma firma aceptada por el dispositivo. No eliminar ese control para obtener un PASS.

Los estados históricos de `inputs-manifest.json` describen cuándo se prepararon las entradas; la aceptación posterior está en [VALIDACION.md](VALIDACION.md).

## Fuentes nativas

`source/manifest.xml` fija los commits. Origen: `minimal-manifest-twrp/platform_manifest_twrp_aosp`; base Android 11 r29 y rama TeamWin android-11.

En un directorio de trabajo vacío, inicializar repo con ese manifest y sincronizar los commits fijados. Una ruta habitual:

```sh
repo init -u https://github.com/Predat1000/twrp-teclast-t40pro-t1b1-eea.git -b main -m source/manifest.xml
repo sync -j4
```

Aplicar dentro de cada repositorio su parche, primero con `git apply --check`:

| Parche | Directorio |
|---|---|
| bootable_recovery.patch | bootable/recovery |
| frameworks_base.patch | frameworks/base |
| system_libvintf.patch | system/libvintf |

Copiar `source/untracked/` respetando las rutas y `source/device/teclast/T40Pro_EEA` a `device/teclast/T40Pro_EEA`.

Los tres prebuilts del device tree se conservan en la cápsula original incluida en el ZIP: `reproduce/source-capsule-v53.tar.gz`. Extraer de ella únicamente `device/teclast/T40Pro_EEA/prebuilt/kernel`, `dtb.img` y `dtbo.img` y comprobarlos contra `source/prebuilt-hashes.json`. Los binarios no se duplican en el historial Git.

El kernel prebuilt de la cápsula nativa tiene hash `81a0e8fd…`; pertenece al boot.img intermediario. El ensamblador final utiliza `kernel.bin` de hash `7e9413ae…`, comprobado contra el rescate stock vigente. No confundir esas dos entradas ni instalar el intermediario.

La cápsula original está congelada por hash. Su README antiguo menciona 12.1 y otro kernel: esos textos históricos **no** son instrucciones de CP10. Tampoco ejecutar `prebuilt/sourcecode/patch.sh`: conserva un overlay histórico, no sustituye los tres parches Android 11 actuales. La documentación pública de este repositorio es la vigente.

En Ubuntu 22.04, con dependencias AOSP y las toolchains del manifest:

```sh
test ! -e out-clean-v53
export OUT_DIR=out-clean-v53 GOGC=100 GOMAXPROCS=4
export BUILD_DATETIME=1792107360
source build/envsetup.sh
lunch twrp_T40Pro_EEA-eng
mka bootimage t40_keyprobe t40_keymaster_ready t40_install_keyring -j4
```

Conservar la salida y el código de retorno de la build. Una reanudación puede usar los objetos de esa misma salida inicialmente vacía; nunca copiar los de un `out` anterior. No desactivar las pruebas de Go/APEX.

Recompilar el helper con clang-r383902b:

```sh
C=prebuilts/clang/host/linux-x86/clang-r383902b/bin
"$C/clang.real" --target=aarch64-linux-gnu -nostdlib -static -ffreestanding -fno-stack-protector -fno-builtin -O2 -Wall -Wextra -fuse-ld=lld -Wl,--build-id=none -Wl,-e,_start /ruta/source/extra-source/t40wm_init_v43.c -o /ruta/nuevo-helper
sha256sum /ruta/nuevo-helper
```

Hash esperado del helper: `93ca5f53a0d737d140e473d0b6ca7765ae61609f19bf55ccdc692458a050219d`.

## Exportar una nueva build

Crear una copia nueva de las entradas y usar `tools/export-native.py` con el árbol y helper compilados. El exportador solo lee `out-clean-v53`; exige candidatos equivalentes, ELF válidos y el helper exacto. El archivo de resultado de build debe ser el generado por la ejecución real y contener `status: PASS`; no fabricar ese estado si falla la compilación.

```sh
python tools/export-native.py --tree /ruta/twrp-11 --inputs /ruta/reproduce --helper /ruta/nuevo-helper --build-result /ruta/build-result.json --output /ruta/export-new
```

En una copia de `reproduce/`, sustituir los tres resultados exportados: `native-artifacts-v53.tar.gz`, `native-transfer-manifest.json` y `pc-clean-build-result.json`. Ejecutar después el ensamblador y revisar las auditorías. El script portable publicado reorganiza el exportador usado en CP10; se ejecutó contra el out-clean-v53 real conservado y produjo exactamente el archivo de 120 artefactos de CP10 (SHA-256 9ea63cbc6f716df8366a1afca21da7bd009692c56839ec4b76e03f0e39dc7cce). No se atribuye a esta exportación una nueva build completa.

**El boot.img nativo del árbol es un intermediario, no la imagen final que debe instalarse.**

CP10 final SHA-256: `b029e36391ca919feef66d608eb5964562433d1b205ee0ebf8ba8579a0af4451`. Una imagen distinta exige investigación y prueba controlada nueva; no hereda automáticamente la aceptación del checkpoint.

## Verificación del paquete público

Las 23 entradas del ZIP publicado se verificaron y se reensamblaron offline. El resultado fue byte idéntico a la imagen aceptada de CP10, incluido su SHA-256. Esta comprobación no escribió ni reinició la tablet.
