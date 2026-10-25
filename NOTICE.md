# Procedencia y licencias de componentes

Esta es una integración no oficial de componentes de varios proyectos. Los avisos de copyright y licencias presentes en las fuentes se conservan.

- **TeamWin / TWRP:** manifest fijado en `source/manifest.xml`, proyecto `android_bootable_recovery` commit `8bd48201c3ed825a0acecc0e6dcf125449ab1ec2`, con parches locales. Consultar las licencias de ese commit y los encabezados de cada archivo.
- **Android Open Source Project:** bibliotecas, init, utilidades y herramientas, incluidas partes bajo Apache-2.0 y otras licencias propias de cada componente. `avbtool.py` conserva su cabecera original.
- **LineageOS, SebaUbuntu / twrpdtgen y autores de overlays Unisoc:** avisos conservados en el device tree.
- **Linux y componentes stock Teclast/Unisoc:** kernel, DTB, init y runtime extraídos del firmware del dispositivo. Se proporcionan sus hashes y procedencia técnica. No se reclaman derechos exclusivos ni se relicencian los componentes del fabricante.

No se declara que los binarios stock sean íntegramente software libre. Este repositorio no contiene el árbol fuente específico del kernel stock del fabricante. La reproducibilidad indicada corresponde al userspace nativo y al ensamblaje con entradas congeladas, no a reconstruir desde fuentes todos los prebuilts.

Las fuentes nuevas, sus parches y herramientas de integración se publican para facilitar la auditoría y reproducción del proyecto. Los cambios sobre código de terceros conservan la licencia de ese código. No hay una licencia única que sustituya las de todos los componentes de la imagen.

Los nombres y marcas identifican compatibilidad y procedencia; no implican afiliación, certificación o respaldo oficial.
