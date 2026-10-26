# Acceso directo HOME de CoreLab

`dist/CoreLab-HOME.cia` instala un lanzador, no una segunda copia del juego.
Title ID reservado para este proyecto: `000400000D0C4B00`; versión del lanzador
1.0.0. Requiere Luma3DS y el soporte de retorno del lanzador en `source/main.cpp`.
El juego sigue usando `sdmc:/3ds/UO-3DS-CoreLab/UO-3DS-CoreLab.3dsx` y sus
recursos/partidas existentes. No borra archivos ni necesita reinstalar mapas.

Instalación inalámbrica: actualizar solo el juego en la tienda local, después
FBI > Remote Install > Scan QR Code, usando la URL local de CoreLab-HOME.cia.
Las siguientes actualizaciones conservan el mismo acceso directo.

Compilar con devkitARM/libctru, makerom 0.19.0 y bannertool:

```
make -C forwarder MAKEROM=/ruta/makerom BANNERTOOL=/ruta/bannertool
```

Adaptado de ZallaxDev/emerald-3ds, revisión b198c08defb94081ae995796cf75ab8cdb51f2d0,
`3ds_port/forwarder` (MIT; licencia incluida en LICENSE-REFERENCE.md).
No utiliza recursos de Pokémon. Banner derivado de la ilustración de inicio de
CoreLab; icono propio; sonido silencioso. La disposición de la página compartida
es una interfaz privada de Luma, por lo que un cambio futuro de Luma podría
requerir actualizar este lanzador.

El lanzador entrega como argumento el título previamente elegido para homebrew.
CoreLab lo restaura al entrar en main, antes de cargar recursos; no guarda ese
cambio en la configuración de Luma. Sin este soporte en el 3DSX, el acceso puede
alterar temporalmente la selección homebrew hasta reiniciar la consola.

Comprobaciones: compilación ARM con avisos como errores; contenedor CIA y hashes
de contenido, ExHeader y ExeFS comprobados con ctrtool 1.3.0. Las firmas de
Nintendo de ticket/TMD/AccessDescriptor no son válidas, como es habitual en un
homebrew para CFW. La instalación y el arranque todavía requieren prueba real.
