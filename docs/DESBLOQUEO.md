# 1. Desbloquear el bootloader

[Preparación y compatibilidad](EMPEZAR.md) · [Siguiente: TWRP](INSTALACION.md)

**Esta operación inicial borra userdata.** Copia tus datos fuera de la tablet antes de ejecutarla. Si ya está desbloqueada, no vuelvas a desbloquearla ni a bloquearla: pasa a la instalación de TWRP.

## Qué se conoce

El historial del proyecto registra desbloqueo mediante `unisoc-unlock` el 14/10/2026 y confirmación posterior de Android con `ro.boot.flash.locked=0`, `ro.boot.vbmeta.device_state=unlocked` y Verified Boot orange.

La instalación conservada contiene `unisoc-unlock 0.0.2`, `libusb1 3.4.0` y `pycryptodome 3.24.1`, sobre Python 3.12. Los módulos cli/fastboot/common instalados coinciden con el wheel 0.0.2 de PyPI. Existe además una copia de auditoría con cambios para paquetes SN, pero no se ha recuperado evidencia suficiente de que esa copia fuera la ejecutada ni su parche de transporte completo. No se distribuye como una solución validada.

Por tanto, los comandos siguientes documentan la ruta upstream y las comprobaciones del proyecto; **no acreditan una nueva prueba de desbloqueo de fábrica ni todas las combinaciones de drivers de Windows**.

## Preparar la herramienta

Descarga solo desde el [proyecto unisoc-unlock](https://github.com/patrislav1/unisoc-unlock) / [PyPI](https://pypi.org/project/unisoc-unlock/0.0.2/). En una carpeta de trabajo nueva:

```powershell
py -3.12 -m venv .venv-unisoc
$unlockPython = (Resolve-Path '.venv-unisoc\Scripts\python.exe').Path
& $unlockPython -m pip install unisoc-unlock==0.0.2 libusb1==3.4.0 pycryptodome==3.24.1
& $unlockPython -m unisoc_unlock --version
```

Referencia del wheel auditado: `unisoc_unlock-0.0.2-py3-none-any.whl`, SHA-256 `66933e899916744c070fc0476c807f250f1b07213d3db3736aace4ee8cce8864`. El repositorio de TWRP no redistribuye sus claves.

## Entrar en fastboot clásico

En las opciones de desarrollador, habilita **Desbloqueo OEM**. Ese interruptor permite la operación, pero todavía no desbloquea el dispositivo.

Con las variables de [EMPEZAR](EMPEZAR.md):

```powershell
& $adb -s $serial reboot bootloader
& $fb devices
& $fb -s $serial getvar is-userspace
```

Debe aparecer tu tablet y `is-userspace: no`. `yes` identifica fastbootd: no ejecutar el desbloqueo desde ahí. Un valor vacío tampoco confirma el modo. En este bootloader clásico algunos getvar de slot/unlocked pueden estar vacíos; no interpretar la línea `Finished` como su valor.

## Desbloquear una sola vez

Asegúrate de que solo está conectada la tablet que quieres desbloquear y de haber guardado los datos. Este es el comando que inicia el desbloqueo:

```powershell
& $unlockPython -m unisoc_unlock unlock
```

Cuando la tablet pida confirmación, confirma **Unlock bootloader / Yes** siguiendo el texto de su pantalla. El desbloqueo y borrado son parte de esa acción. No sustituirlo por un flasheo de PAC ni por modificaciones de SPL/FDL/U-Boot.

Si aparece `unknown header`, `SN`, un token vacío, `Unlock bootloader fail` o un error de libusb, conserva el error y detente. Los errores de acceso USB requieren comprobar el driver de la interfaz fastboot real; no sustituir drivers de todas las interfaces ni usar download mode como atajo. La copia SN de auditoría no es un parche completo publicado.

## Verificar desde Android

Después del desbloqueo, reinicia a sistema si la herramienta no lo hace y completa la configuración inicial. Es normal tener que volver a habilitar depuración USB y autorizar el PC tras el borrado inicial.

```powershell
& $adb -s $serial shell getprop ro.boot.flash.locked
& $adb -s $serial shell getprop ro.boot.vbmeta.device_state
& $adb -s $serial shell getprop ro.boot.verifiedbootstate
& $adb -s $serial shell getprop ro.boot.slot_suffix
& $adb -s $serial shell getprop sys.boot_completed
```

Para la ruta probada: `0`, `unlocked`, `orange`, `_a` y `1`, respectivamente. La confirmación de la pantalla o del programa por sí sola no sustituye esas lecturas.

**No vuelvas a bloquear el bootloader con TWRP o un boot modificado instalado.** Continúa con [TWRP sin root de Android](INSTALACION.md).

Fuentes: [unisoc-unlock](https://github.com/patrislav1/unisoc-unlock), [reglas de bloqueo/desbloqueo AOSP](https://source.android.com/docs/core/architecture/bootloader/locking_unlocking). La confirmación en T1B1 procede del historial del proyecto; upstream describe sus propias pruebas en otro dispositivo T618.
