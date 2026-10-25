# Empezar desde Android stock

El objetivo es poder **desbloquear el bootloader, instalar TWRP y, solo si lo deseas, obtener root en Android**. Puedes terminar al instalar TWRP: APatch no es un requisito.

| Etapa | Para qué sirve | Estado de la evidencia |
|---|---|---|
| [1. Desbloquear](DESBLOQUEO.md) | Permitir arrancar imágenes personalizadas | Éxito histórico en esta T1B1; no se ha vuelto a bloquear/desbloquear para probar esta guía. |
| [2. Instalar TWRP](INSTALACION.md) | Recovery en boot_b, conservando Android en A | CP10 probado con escritura y lectura íntegra de B. Entrada inicial por fastbootd y reinicio normal demostrada con versiones anteriores; no se ha repetido CP10 desde una tablet bloqueada de fábrica. |
| [3. Root opcional](ROOT-OPCIONAL.md) | Permisos de administrador dentro de Android | APatch 11282 / KernelPatch 0.13.9 / AndroidPatch FULL validado en CP01. Cada usuario debe preparar su propia imagen. |

**El desbloqueo inicial borra los datos.** Haz antes una copia externa de tus archivos y de lo necesario para recuperar tus cuentas. Si el bootloader ya está desbloqueado, omite esa etapa: instalar TWRP no requiere otro restablecimiento.

## Comprobar compatibilidad

Esta guía se limita a **Teclast T40 Pro, placa T1B1 EEA, Unisoc T618 / UMS512 1H10, Android stock 11**. No basta con que otra tablet tenga el mismo procesador.

La configuración probada tiene Android en A, particiones boot de 64 MiB y particiones dinámicas B vacías. TWRP monta las particiones dinámicas A explícitamente. Si tu Android arranca en B, tu firmware es distinto o utilizas PIN/contraseña de bloqueo, esa situación queda fuera de lo validado: no aplicar estos comandos sin resolver la diferencia. No eliminar credenciales como atajo para forzar compatibilidad.

Antes de empezar, conserva en el PC el firmware stock exacto de tu unidad y una vía de recuperación conocida. Una imagen de otro modelo o revisión no sirve como rollback. Si ya tienes acceso de bloques, guarda también copias verificadas de ambos boot. Si partes de stock sin root, todavía no podrás leer esos bloques con ADB normal; no instales root para salvar ese requisito ni presentes una extracción del firmware como una lectura de la tablet.

## Preparar el PC

Las guías utilizan PowerShell en Windows, [Android Platform Tools oficiales](https://developer.android.com/tools/releases/platform-tools) y, para desbloqueo/firma, [Python](https://www.python.org/downloads/windows/). Extrae Platform Tools en una carpeta local y ajusta estas rutas:

```powershell
$adb = 'C:\platform-tools\adb.exe'
$fb = 'C:\platform-tools\fastboot.exe'
& $adb devices -l
$serial = 'SUSTITUYE_POR_EL_SERIAL_DE_TU_TABLET'
```

Activa las opciones de desarrollador y la depuración USB en Android. Autoriza tu PC en la pantalla de la tablet. El estado ADB debe ser `device`, no `unauthorized`. Conecta solo la tablet objetivo al usar la herramienta de desbloqueo, que selecciona el dispositivo USB por su cuenta.

Mantén estas variables en la misma sesión de PowerShell para las guías siguientes. Comprueba el resultado de cada comando antes de pasar al siguiente; no pegues todos los bloques seguidos.

```powershell
& $adb -s $serial shell getprop ro.product.model
& $adb -s $serial shell getprop ro.product.device
& $adb -s $serial shell getprop ro.build.fingerprint
& $adb -s $serial shell getprop ro.boot.slot_suffix
& $adb -s $serial shell getprop ro.boot.flash.locked
& $adb -s $serial shell getprop ro.boot.vbmeta.device_state
```

El modelo/producto observado fue `T40_Pro_EEA` / `T40Pro_EEA`. Confirma también T1B1 en la identificación física/firmware: las propiedades por sí solas no acreditan la revisión de placa.

## Qué significa root en cada etapa

- **Bootloader desbloqueado:** permite arrancar imágenes personalizadas; no convierte las aplicaciones de Android en root.
- **ADB root de TWRP:** `adb shell id` devuelve uid=0 dentro del recovery. Permite instalar/verificar B sin haber rooteado Android.
- **Root de Android:** se obtiene aparte con APatch. Se comprueba con `su -c id` dentro del sistema. Puedes prescindir de él.

[Continuar con el desbloqueo](DESBLOQUEO.md) · [Ya estoy desbloqueado: instalar TWRP](INSTALACION.md) · [Límites de validación](VALIDACION.md)
