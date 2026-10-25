# TWRP para Teclast T40 Pro T1B1 EEA

**TWRP 3.7.0_11-0 · Android 11 nativo · CP10 / V53B**

Recovery no oficial validado en una Teclast T40 Pro **T1B1 EEA**, Unisoc T618 / UMS512 `1H10`. Publicación del checkpoint de ingeniería aceptado el **25 de octubre de 2026**.

[**Empezar desde stock**](docs/EMPEZAR.md) · [Desbloquear bootloader](docs/DESBLOQUEO.md) · [Root opcional](docs/ROOT-OPCIONAL.md)

[Descargar CP10](https://github.com/Predat1000/twrp-teclast-t40pro-t1b1-eea/releases/tag/cp10-v53b) · [Instalación y recuperación](docs/INSTALACION.md) · [Pruebas y límites](docs/VALIDACION.md) · [Reproducción](docs/REPRODUCCION.md)

## Qué puedes hacer

1. **Desbloquear el bootloader** siguiendo la guía específica de Unisoc. El desbloqueo inicial borra los datos; se omite si ya está desbloqueado.
2. **Instalar TWRP en boot_b**, conservando Android en A. No requiere root previo de Android.
3. **Opcionalmente, rootear Android con APatch**, usando tu propio boot y una SuperKey privada. Puedes terminar en el paso 2.

El ADB root de TWRP y el root de Android son cosas distintas. La imagen CP10 no instala APatch. La ruta completa desde una tablet bloqueada de fábrica no se ha repetido con CP10: las guías distinguen los pasos históricos probados y lo pendiente de validar de extremo a extremo.

## Resultado

- Arranque real de la imagen reconstruida; ADB root en 26,5 s y MTP + descifrado en 41,2 s.
- Interfaz TWRP y táctil confirmados por el usuario en V53B de referencia. Las bibliotecas GUI/DRM son idénticas en la reconstrucción; véase el alcance exacto de esa comprobación.
- USB ConfigFS `musb-hdrc.0.auto`, almacenamiento interno y `/data` F2FS descifrada en lectura/escritura.
- Fastbootd auténtico, HAL de BootControl y selección de slot comprobada leyendo `misc`.
- Reinicio de recovery y regreso a Android A conservando root APatch y SELinux Enforcing.
- Compilación nativa desde una salida inicialmente vacía; 120 payloads ELF auditados y comprobados en la tablet.
- Sin panic ni reinicio por Native Hang Monitor durante las pruebas documentadas.

**Distribución de slots de esta configuración:** `boot_a` mantiene Android y su rescate; `boot_b` contiene TWRP. Las particiones dinámicas B están vacías y TWRP monta explícitamente `system_a`, `system_ext_a`, `vendor_a` y `product_a`.

## Descarga verificada

Archivo: `TWRP-T40Pro-T1B1-EEA-CP10-V53B.img`  
Tamaño: **67.108.864 bytes (64 MiB)**  
SHA-256:

```text
b029e36391ca919feef66d608eb5964562433d1b205ee0ebf8ba8579a0af4451
```

AVB: `SHA256_RSA4096`, boot header v2, página 2048. Es exactamente la imagen que se escribió con `dd`, se leyó completa y se arrancó en CP10. No es un instalador de root para Android.

## Antes de usarla

Esta publicación está limitada al modelo y configuración probados; no es una imagen genérica para T618. La [guía de inicio](docs/EMPEZAR.md) cubre la preparación desde stock y el desbloqueo previo. **Para instalar TWRP se escribe solo boot_b: no escribir boot_a, no hacer wipe y no relockear el bootloader con esta imagen.** La modificación de A para root opcional se trata por separado. Recovery ofrece acceso root y a almacenamiento descifrado: conectar solo a equipos de confianza.

Un `fastboot flash … OKAY` por sí solo no demuestra persistencia. La lectura posterior confirmó que los flashes históricos stock/V45 sí habían persistido; seleccionar B y hacer reboot normal arrancó TWRP residente. CP10 se aceptó tras escritura directa, `sync` y SHA-256 completo de B. `fastboot boot` tampoco acredita qué imagen se ejecuta. La [guía](docs/INSTALACION.md) explica la entrada inicial por fastbootd, el reinicio correcto y la comprobación de bloques.

## Alcance

Se probaron las claves FBE existentes **sin PIN ni contraseña de bloqueo**. No se validaron cambios de credencial, ZIP arbitrarios, restauraciones completas, otras revisiones de placa ni otras ROM. La confirmación física de GUI/táctil corresponde a V53B de referencia; no se afirma una segunda prueba táctil independiente de la imagen limpia.

El repositorio conserva fuentes, parches y herramientas de ensamblaje; los archivos de la release incluyen la imagen, entradas de reproducción y hashes. Los registros privados completos permanecen en el checkpoint original: no se publican números de serie, redes Wi-Fi, claves de cifrado, claves de firma ni copias de userdata.

## Contenido

- `source/`: manifest fijado, device tree, parches y helpers. Las fuentes heredadas se conservan con sus avisos.
- `reproduce/`: ensamblador y auditor ELF originales, mapas y manifiestos.
- `tools/`: exportación portable de artefactos y verificación de los archivos publicados.
- `docs/`: inicio desde stock, desbloqueo, instalación de TWRP, root opcional, arquitectura, reproducción y matriz de pruebas.
- Release `cp10-v53b`: imagen exacta y `CP10-reproduction-inputs.zip`.

## Créditos y licencias

TeamWin / TWRP, Android Open Source Project, LineageOS, SebaUbuntu / twrpdtgen y autores de los componentes Unisoc. Véase [NOTICE](NOTICE.md). Este proyecto no está afiliado ni respaldado por TeamWin, Teclast o Unisoc. Los avisos y licencias de los componentes se conservan; no se atribuye una licencia única a los binarios stock.
