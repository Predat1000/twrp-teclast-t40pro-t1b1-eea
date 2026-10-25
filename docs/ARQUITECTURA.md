# Arquitectura de la imagen aceptada

- Kernel y DTB stock del checkpoint de rescate vigente, boot header v2 y página de 2048 bytes.
- Helper estático V43 como PID 1, con init Android 11 stock parcheado en dos instrucciones verificadas. Conserva el manejo de first-stage/force_normal_boot necesario en este firmware.
- Recovery TWRP Android 11 nativo; runtime y namespaces seleccionados mediante DT_NEEDED y símbolos reales. No se usa la mezcla incompatible TWRP 12.1 + librerías Android 11.
- Keymaster stock en un entorno aislado; preparación de binder y keyring controlada por la etiqueta `-native-crypto-`.
- Política de claves existentes: no generar, actualizar o borrar las claves de usuario durante las operaciones de descifrado probadas.
- `/metadata` en solo lectura; data F2FS RW permitida expresamente en B.
- USB ConfigFS con controlador `musb-hdrc.0.auto`.
- BootControl 1.1 y Health 2.1 para fastbootd, imports RC y servicios comprobados.
- Reinicio a System dirige a A; Recovery a B, actualizando intentos y verificando boot control.
- Native Hang Monitor desactivado en recovery, con diagnóstico acotado en RAM/consola.
- DRM: reintento acotado ante EBUSY, conservación de una superficie válida y eliminación de liberaciones duplicadas. Evita el acceso nulo desde gr_flip.

## Identidades de las entradas

| Entrada | SHA-256 |
|---|---|
| Kernel stock vigente | `7e9413ae8ffcbe92f44dce11b3cd46d5a3f01a07418e208dd9c39f32954f2043` |
| DTB | `e6b1acb0b52a51c9042bd2059f695f199fa6a8687ce6df6457b22111bc503187` |
| Helper V43 reconstruido | `93ca5f53a0d737d140e473d0b6ca7765ae61609f19bf55ccdc692458a050219d` |
| Init stock original | `b5a14d9af76af64223a866d68bd5799444a488eca5bf2702008a458dc8cf4273` |
| Init stock parcheado | `36e06777c037cb1ee914c2c373e18fed52e9bafc0395cd850a8f1471f22d5d93` |
| libminuitwrp.so | `e324ee4265867509ce4fe51500f2ebc18ca2328e38eac9507f95af1bb0f09109` |

Parche de init: offsets `0x75a5c` (antes `0x540012ea`) y `0x75ac4` (antes `0x54000aca`) pasan a `0xd503201f`. El ensamblador verifica los bytes anteriores y el hash completo posterior; no aplicar esos offsets a otro init.

El paquete reproduce una composición concreta validada, no una sustitución indiscriminada de todas las bibliotecas stock.
