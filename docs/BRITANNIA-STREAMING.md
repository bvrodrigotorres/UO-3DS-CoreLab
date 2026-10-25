# Preparación del mundo completo — etapa de geometría

**Estado: la geometría se integra en el modo experimental de exploración v0.44.**
Ver `v0.44.md`: la campaña sigue separada; no hay población/loot global.
El lector por sí solo sigue sin ser un paquete instalable; v0.44 añade un
catálogo gráfico privado y un paquete opcional por el servidor local.

## Auditoría real del cliente

- Map0: 7168×4096, incluidas las áreas adicionales del cliente.
- 5.246.724 estáticos, 13.409 gráficos estáticos distintos.
- La estructura residente actual requeriría como mínimo 1076 MiB para todo el
  mundo. Es una cota conservadora, no una medición de RAM en la consola.
- Los generadores de Felucca referencian 580 clases de contenido; todavía hay
  que separar móviles/objetos y ampliar estilos/IA. No son 580 tipos de NPC ya
  implementados y no equivale al estado de un servidor en ejecución.

## Implementado y verificado

`tools/export_world_archive.py` exporta TODO el terreno y los estáticos de map0,
con las diferencias del cliente y los flags, alturas y tonos de tiledata.
No filtra por visibilidad ni descarta objetos para entrar en memoria.

`WorldArchive` lee ese archivo por sectores de 64×64, con halos de terreno de
una casilla para conservar las alturas compartidas en los bordes. Prepara una
ventana de hasta 128×128 compatible con la geometría y colisiones existentes.
Mantiene IDs de objetos estables en el archivo y una caché LRU de cuatro
sectores, con un máximo combinado de 512 KiB de payload. Metadatos y la ventana
son memoria adicional; 512 KiB NO es el consumo total del motor.

El archivo privado generado tiene 7168 sectores y ocupa 128.736.604 bytes.
El mayor sector contiene 106.342 bytes. El recorrido nativo de las 1792 ventanas
alineadas de 128×128 recuperó exactamente los 5.246.724 objetos, con un máximo de
38.935 objetos en una ventana. El visor no conserva un World global en RAM.

Validaciones:

- Checksums del catálogo y de cada sector, tamaños, conteos, coordenadas y EOF.
- Archivo completo recorrido en C++ con ASAN/UBSAN; máximo de caché comprobado.
- Comparación independiente contra el cliente en cinco ventanas: Britain,
  3620/2100, 0/0, extremo 7104/4032 y ventana 3000/800 de 128×128. Coinciden
  terreno, alturas, flags, modo de pendiente, estáticos y tonos.
- Pruebas sintéticas de continuidad, IDs, LRU, corrupción tardía, índices
  inválidos y conservación de la ventana anterior ante error.
- 53 grupos de pruebas nativas y compilación ARMv6K con auditoría de pila.

## Integración y trabajo pendiente

Los `Land.image` y `Object.image` de las ventanas nuevas permanecen en NoImage:
la geometría no contiene el mapeo al banco gráfico del renderer. WorldGraphics
lo resuelve antes de activar una ventana de exploración. No debe
usarse como sustituto directo de `scene.bin` ni como mapa supuestamente jugable.

Falta implementar y comprobar conjuntamente:

1. Catálogo gráfico global y resolución de imágenes por ventana, respetando el
   límite de imágenes y la caché de texturas de Old 3DS.
2. Precarga asíncrona, intercambio de ventanas y coordenadas globales, sin que
   el hilo de render haga lecturas síncronas de `WorldArchive` durante un frame.
3. Separación del personaje/campaña global respecto al estado de cofres,
   suelo, puertas y cadáveres de cada sector. Migración de la partida v0.43.
4. Decoración dinámica de ServUO, partición de generadores y NPCs/IA cercanos.
   El archivo de esta etapa conserva los estáticos del cliente, NO incorpora
   toda la decoración o población del servidor.
5. Minimapa por sectores, teletransportes, regreso a NPCs de misiones que estén
   fuera de la ventana y pruebas reales de trayectos prolongados en Old 3DS.

No se eleva el límite de 512 de `loadWorld`, no se sustituye la campaña actual
y no se reescriben sus partidas. v0.44 permite explorar la geografía completa
como modo aparte, con posición separada y cargas visibles.

## Reproducir con archivos privados

```sh
python3 tools/export_world_archive.py --client /ruta/cliente --out /ruta/nueva
c++ -std=c++17 -O2 -Iinclude tools/world_archive_probe.cpp source/world_archive.cpp source/core.cpp -o world-probe
./world-probe /ruta/nueva/britannia.uow 1480 1592 64
```

El exportador requiere el SDK/Pillow utilizados por las herramientas anteriores.
No sobrescribe una carpeta de salida existente. El sondeo es nativo y no abre
el juego ni modifica una SD. Los archivos del cliente y el `.uow` son privados;
solo código, documentación y fixtures sintéticos pueden subirse a GitHub.
