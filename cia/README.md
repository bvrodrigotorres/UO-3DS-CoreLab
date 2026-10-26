# CoreLab como aplicación HOME

La CIA nativa incluye el ejecutable del juego. No llama a hb:ldr, no reinicia
un título homebrew y no depende de boot.3dsx. El arranque y la salida usan el
ciclo normal de una aplicación con libctru. Los recursos y partidas conservan
su ruta `sdmc:/3ds/UO-3DS-CoreLab/`.

Compilación:

```
make -j4
make -C cia MAKEROM=/ruta/makerom BANNERTOOL=/ruta/bannertool
```

Salida: `dist/CoreLab-v0.46.cia`. Title ID `000400000D0C4B00`, igual que el
lanzador anterior para reemplazar su icono sin borrar datos. La versión interna
CIA es 2.0.0 (mayor que 1.0.0 del lanzador); la versión del juego es 0.46.
Memoria: aplicación Old 3DS 64 MB, pila principal 128 KiB, sin reloj New 3DS.

Instalar con FBI y el QR servido por el Mac. Reiniciar la consola antes de la
primera instalación para descartar una selección homebrew temporal del antiguo
lanzador. En las siguientes actualizaciones usar `Actualizar juego HOME (CIA)`
en la tienda local. Actualizar un 3DSX no modifica un juego instalado como CIA.

No borrar recursos, partidas ni el título existente antes de actualizar.
La CIA no contiene el mapa ni recursos privados. La salida de una aplicación
nativa vuelve a HOME; el 3DSX conserva el retorno normal al Homebrew Launcher.
La estructura y hashes de CIA se verifican con ctrtool, pero la instalación y
el ciclo HOME deben comprobarse en la consola real.
