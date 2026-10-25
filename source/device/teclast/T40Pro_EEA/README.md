# Device tree T40Pro_EEA — CP10 Android 11

Device tree y helpers de la build nativa Android 11. La documentación vigente está en el README principal y docs/REPRODUCCION.md.

El boot.img del árbol es un intermediario. La imagen final usa el ensamblador y entradas congeladas de CP10.

Los prebuilts kernel/dtb.img/dtbo.img se obtienen de la cápsula de la release y se verifican con source/prebuilt-hashes.json.

Los directorios prebuilt/sourcecode/files y original son material heredado conservado. **No ejecutar patch.sh ni aplicar ese overlay encima de los parches Android 11 de CP10.** Las instrucciones antiguas de TWRP 12.1 no son aplicables.
