# Validación CP10

Estado: **ACCEPTED_FOR_CURRENT_STOCK_CONFIGURATION**, 2026-10-25.

Se distingue la imagen limpia instalada de la referencia V53B. Las pruebas heredadas se enumeran expresamente para no atribuir a la imagen limpia observaciones físicas que no se repitieron.

| Prueba | Resultado y procedencia |
|---|---|
| Build nativa | Salida inicialmente vacía `out-clean-v53`; 17.083 objetivos. Pruebas Go habilitadas. |
| Reanudación | Último comando 32 min 54 s. Un intento anterior se detuvo deliberadamente para obtener una pila de Go/APEX; se reanudó con GOGC=100. No se presenta ese tiempo como la duración total. |
| ELF | 120 interfaces sin cambios; once cierres de dependencias sin bibliotecas ni símbolos fuertes pendientes. |
| Diferencias respecto a referencia | 83/120 ELF idénticos; 33 cambian únicamente build-id; cuatro contienen rutas de protobuf `out/` frente a `out-clean-v53/`. La comparación de instrucciones conserva tipos de operación y destinos de saltos. |
| Escritura de imagen limpia | Solo boot_b mediante dd + sync. SHA completo de boot_b idéntico al archivo. boot_a se volvió a leer y permaneció igual. |
| Arranque limpio | Nuevo arranque real; ADB root en 26,5 s y MTP + crypto en 41,2 s. Los 120 payloads vivos coinciden con la exportación limpia. |
| Datos | /data F2FS descifrada RW; /metadata ext4 RO + norecovery. Claves existentes conservadas. |
| Fastbootd limpio | is-userspace=yes; slot B; tamaño boot 64 MiB; selección de A con lectura íntegra de misc y cambios limitados a boot control. |
| Regreso a Android | Slot A; boot_completed=1 en 46,8 s; su uid=0, apd activo, SELinux Enforcing, Wi-Fi e Internet. |
| Sistema de archivos | fsck stock termina con “No error was reported”; checkpoint de desmontaje limpio. |
| GUI y táctil | El usuario informó que el menú aparecía después de tocar Keep Read Only en la referencia V53B. El registro de página no corroboró independientemente el toque. GUI/DRM tienen bytes idénticos en la imagen limpia. |
| Estabilidad prolongada | Referencia V53B: 605 s, mismo PID recovery, sin crash, USB estable y monitor desactivado. No se atribuye una nueva prueba de 605 s a la reconstrucción. |
| Montajes dinámicos | Referencia V53B: montaje, lectura y desmontaje de las cuatro particiones A; sin crash incluso al observar EBUSY de DRM. |
| Escritura de almacenamiento | Referencia: canary RW comprobado posteriormente desde Android. |
| Reinicio a recovery | Ruta nativa probada en referencia y arranque limpio; actualización de intentos B y lectura completa de misc. |
| Panic | No observado en las pruebas aceptadas; Native Hang Monitor desactivado en recovery. |

La imagen final **no** es byte idéntica a la referencia, por lo que se probó de verdad:

- Referencia V53B: `66e148317575181b9542fc08428081934b9822e6c3c5435b4570b44bfd6941c0`.
- CP10 limpio: `b029e36391ca919feef66d608eb5964562433d1b205ee0ebf8ba8579a0af4451`.

## Alcance de las guías para nuevos usuarios

| Etapa | Evidencia disponible | Límite |
|---|---|---|
| Desbloqueo | Historial del 14/10: unisoc-unlock, confirmación física y propiedades unlocked en Android. Auditoría actual del paquete 0.0.2. | No se repitió el desbloqueo ni se certifica la copia SN de auditoría o cualquier driver de Windows. |
| TWRP sin root Android previo | V42 se flasheó en B por fastbootd antes de APatch. El 18/10, SHA de bloques confirmó stock/V45 escritos y V45 residente arrancó al seleccionar B y hacer reboot normal. | Esta ruta histórica no es una nueva instalación completa de CP10 desde stock bloqueado. El arranque y SHA de B deben comprobarse en cada unidad. |
| CP10 | Imagen exacta instalada por dd y leída íntegramente, con arranque real. | No se infiere persistencia de una respuesta OKAY. |
| Root opcional | CP01: APatch Manager 11282/e8e4651, KernelPatch 0.13.9, AndroidPatch FULL; tres reinicios y comprobaciones funcionales PASS. | El boot personal no se distribuye. Una imagen con otra SuperKey requiere su propia firma y validación. |

La lectura del 18/10 corrigió la hipótesis inicial de que los flashes fastbootd no habían persistido. Una pantalla stock después de `reboot recovery` no demostraba el contenido de boot_b. La conclusión que se conserva es **verificar lo escrito**, no afirmar que fastboot nunca escribe.

## No probado / no prometido

- Secuencia completa desde bootloader bloqueado de fábrica hasta CP10 y root opcional siguiendo las nuevas guías.
- Cambios de PIN o contraseña y otras configuraciones de FBE.
- ZIP arbitrarios o restauración completa de backups.
- Otras placas, firmware, revisiones de hardware o ROM.
- Persistencia de imágenes escritas por fastboot solo porque responda OKAY.
- Una segunda comprobación física independiente del táctil en la imagen reconstruida.
- Reproducción independiente por terceros ni compilación del kernel stock desde fuentes.

La compilación limpia verificada es la de los componentes nativos del recovery. El kernel, DTB y parte del runtime stock son entradas congeladas, y la firma exacta requiere la clave externa correspondiente. Esto no es una build enteramente desde fuentes de todos los componentes de la tablet.

El checkpoint privado conserva pruebas completas, hashes y rollback. La publicación contiene un resumen técnico sin identificadores personales ni claves de usuario.
