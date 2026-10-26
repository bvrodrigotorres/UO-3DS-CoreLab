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

Salida: `dist/CoreLab-v0.47.cia`. Title ID `000400000D0C4C00`, distinto del
lanzador anterior `000400000D0C4B00`, para evitar que una selección homebrew de
Luma aún asociada a ese título intercepte el juego nativo. Instala otro icono
llamado `CoreLab 0.47`. La versión interna CIA es 3.0.0; el juego está en 0.47.
Memoria: aplicación Old 3DS 64 MB, pila principal 128 KiB, sin reloj New 3DS.

Instalar con FBI y el QR servido por el Mac. Reiniciar la consola antes de la
primera instalación para descartar una selección homebrew temporal del antiguo
lanzador. En las siguientes actualizaciones usar `Actualizar juego HOME (CIA)`
en la tienda local. Actualizar un 3DSX no modifica un juego instalado como CIA.

No borrar recursos ni partidas. Abrir el nuevo icono `CoreLab 0.47`, no el viejo.
La CIA no contiene el mapa ni recursos privados. La salida de una aplicación
nativa vuelve a HOME; el 3DSX conserva el retorno normal al Homebrew Launcher.
La estructura y hashes de CIA se verifican con ctrtool, pero la instalación y
el ciclo HOME deben comprobarse en la consola real.
