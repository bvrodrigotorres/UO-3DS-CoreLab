# UCL1, versión 1

Enteros little endian. El lector valida longitud exacta y rechaza datos fuera
de límites antes de aceptar la escena. Formato distinto a las escenas del
proyecto anterior; no se reutilizan ni sobrescriben esos archivos.

| Registro | Formato Python struct | Contenido |
|---|---|---|
| Cabecera 32 bytes | `<4sHHHHIiiHHhH` | magic UCL1, versión, ancho, alto, imágenes, objetos, origen X/Y, inicio X/Y local, inicio Z, reservado=0 |
| Vértices | `(ancho+1)*(alto+1)` int8 | Z por vértice; fila de ancho+1; incluye vecinos derecho e inferior |
| Terreno 13 bytes | `<HHQB` | ID de arte, índice de imagen, flags64, modo 0=diamante/1=textura cuadrada |
| Estático 26 bytes | `<HHbBHHHQIH` | X/Y local, Z, altura, ID arte, hue, imagen, flags64, orden original, reservado=0 |

El índice de imagen 65535 indica ausencia explícita (por ejemplo terreno 2).
Las otras imágenes corresponden a `atlas_{indice//32:02d}.t3x`, subimagen
`indice%32`. Los atlas se empaquetan con tex3ds en RGBA5551. El renderer usa
los helpers de coordenadas Tex3DS para respetar imágenes rotadas en el atlas.

Pisos y techos permanecen en la lista original. La visibilidad es una decisión
de ejecución. `order` conserva la relación de empate de los estáticos. No
incluye entidades ni multis de un servidor.

Límites v0.1: ancho/alto 1..32; imágenes <=512; objetos <=8192;
archivo <=512 KiB. Son límites del experimento, no del mapa original de UO.
