# UO-3DS-CoreLab v0.55

Laboratorio independiente para probar en la **Nintendo 3DS original** un núcleo
de escena y movimiento adaptado de CrossUO. Este repositorio tiene historia,
ejecutable y carpeta de SD propios; no depende del repositorio UO-3DS-Offline.

**Primera prueba técnica, no un juego completo.** Caminar, alturas, orden de
dibujo, geometría del terreno y diagnóstico de rendimiento. Incluye NPC locales y una primera mochila con cofres persistentes. Incluye combate local experimental con cadáveres y botín. No incluye servidor
ni recursos originales de Ultima Online.

[Novedades e instalación de v0.55](docs/v0.55.md): menú inicial antes del mundo, personalización antes del mapa, entrada directa a alcantarillas y eliminación de relecturas del mapa.

[Novedades e instalación de v0.54](docs/v0.54.md): pociones apilables, Y para vendas y mochila de 36 espacios en tres páginas.

[Novedades e instalación de v0.53](docs/v0.53.md): comercio pausado en la pantalla superior, compras y ventas por cantidad y entrenamiento de habilidades por profesión hasta 30.0.

[Novedades e instalación de v0.12](docs/v0.12.md): carga inicial, título en la
pantalla superior y menú táctil con Nuevo Juego, Continuar y Opciones.

[Novedades v0.17](docs/v0.17.md): nombres y profesiones, menú de pausa y
tejados escalonados que se ocultan como un conjunto.

[Novedades v0.18](docs/v0.18.md): fuente original con colores, borde y sombra;
etiquetas que se ocultan al acercarse y sombreros por encima del pelo.

[Ajustes v0.18.1](docs/v0.18.1.md): borde de nombres más fino, alineación
a píxeles enteros y encabezado PAUSA.

[Novedades v0.19](docs/v0.19.md): atravesar NPCs al insistir y panel de estado
debajo del mapa y personaje.

[Novedades v0.20](docs/v0.20.md): estado original de UO, combate local,
cadáveres con botín y reaparición.

[Novedades v0.21](docs/v0.21.md): botín automático, Sacar todo, mochila de
24 espacios en dos hojas y Estado del personaje en español.

[Novedades v0.22](docs/v0.22.md): creador humano en ambas pantallas, apariencia
persistente, personalización desde pausa y Salir del juego en el menú principal.

[Novedades v0.23](docs/v0.23.md): confirmación de nuevo juego, vista previa
reorganizada, ropa inicial real y ocho ranuras de equipo persistentes.

[Novedades v0.24](docs/v0.24.md): pestañas reorganizadas, túnica inicial,
sombreros completos y botones para quitar o recuperar el último equipo.

[Novedades v0.25](docs/v0.25.md): color del calzado, filas en una línea,
paleta de pelo/barba corregida y paperdolls sin recorte fijo.

[Novedades v0.26](docs/v0.26.md): prendas propias desde pausa y ajuste del
sombrero en las vistas previas.

[Novedades v0.27](docs/v0.27.md): poses y golpes originales del jugador,
persecución de criaturas y teñido completo del sombrero.

[Novedades v0.28](docs/v0.28.md): barra de vida hasta recuperar un 25%,
números de daño, partículas y charcos temporales de sangre.

[Novedades v0.29](docs/v0.29.md): Britain 512×512, criaturas hostiles del
entorno, texturas de NPC por cercanía e importación de la partida anterior.

## Qué cambia respecto al prototipo anterior

- Se conservan todos los estáticos de la región exportada, con ID, X/Y/Z, hue,
  altura, flags y orden original. No se borran pisos ni techos al convertir.
- Varias superficies y obstáculos pueden coexistir en la misma casilla.
- Una lista intercalada ordena terrenos, estáticos y actor por casilla y prioridad,
  adaptando reglas de CrossUO. No hay máscara de pisos contra paredes exteriores.
- El terreno usa cuatro alturas y triángulos texturizados; conserva las pendientes.
- La resolución normal de apoyos y huecos adapta CrossUO; las diagonales comprueban
  ambos lados y pueden deslizarse hacia una dirección lateral.
- El núcleo no incluye SDL, OpenGL, red ni Unity. La salida gráfica es citro3d.

## Probar sin archivos de UO

Descarga el artefacto **UO-3DS-CoreLab-v0.24** de un run verde en
[Actions](https://github.com/bvrodrigotorres/UO-3DS-CoreLab/actions).
También se puede compilar localmente con devkitPro y `make -j2`.

Copia la carpeta `UO-3DS-CoreLab` a `/3ds/` en la SD y abre
`UO-3DS-CoreLab.3dsx` desde Homebrew Launcher.
La escena de diagnóstico está incorporada en el ejecutable: habitación con
pisos, muros, techos, escalones, dos superficies superpuestas y una pendiente.

| Control | Acción |
|---|---|
| Circle Pad / cruceta | Caminar |
| B mantenido | Correr |
| Y | Techos: AUTO (transparencia) / HIDDEN (quitar plantas superiores) / FULL (mostrar todo) |
| A | Iniciar/detener ataque contra una criatura cercana |
| X | Cambiar el objetivo entre criaturas cercanas |
| R / deslizar horizontalmente | Recorrer diagnóstico, minimapa/personaje y mochila/cofre |
| SELECT / L | Guardar / restaurar posición, detenido |
| START | Pausa: Continuar / Menú principal / Salir |

La pantalla inferior muestra FPS, tiempo CPU/GPU, objetos enviados, tamaño de
atlas, memoria lineal libre y memoria de aplicación libre. Esos datos tienen
que medirse en hardware: compilar no prueba rendimiento ni corrección de la GPU.

## Probar Britain con datos locales

Usa tu instalación de UO, Python con Pillow y
[Ultima SDK Python](https://github.com/UltimaWorks/ultima-sdk-python), y `tex3ds`.
El conversor acepta `map0.mul` o `map0LegacyMUL.uop`, `tiledata.mul`,
`staidx0.mul`, `statics0.mul`, arte MUL/UOP y texturas de terreno clásicas.
Aplica los parches locales `mapdif*` / `stadif*` cuando existen. No interpreta
un shard ni asume que sus datos sean iguales a los de esta instalación.

```sh
python3 tools/export_scene.py --client /ruta/a/UO_Client \
  --out private-assets/britain-v01
```

Por defecto conserva la región de 32×32 desde (1488,1596) y coloca el personaje
en (1501,1612,20), sobre el piso interior. `--spawn-z` es la altura inicial:
debe corresponder a un apoyo real. Los argumentos permiten cambiar región e inicio.
El directorio de salida debe ser nuevo. Los fallos de recursos producen un error;
no se sustituyen silenciosamente texturas ni se inventan objetos.

Copia `scene.bin` y todos los `atlas_XX.t3x` generados a
`SD:/3ds/UO-3DS-CoreLab/`, junto al ejecutable. Opcionalmente copia tu
`player_anim.t3x` de 40 o más cuadros a esa misma carpeta. El programa no usa
la carpeta de SD del juego anterior.

`report.json`, `placements.json` y `_private_sources/` son diagnósticos del Mac;
no hacen falta en la SD. **Los datos originales y derivados permanecen privados**
y están excluidos de Git. No los subas en commits, issues ni artefactos públicos.

## Validación reproducible

```sh
python3 tests/run_tests.py
make -j2
```

Los tests compilan el núcleo con AddressSanitizer y UndefinedBehaviorSanitizer.
Comparan **20.000 conjuntos de candidatos** contra `CalculateNewZ` original y
**3.000 columnas** contra `MapBlock::AddRender` original. Las funciones de
referencia están fijadas a una revisión y se conservan sin modificar su cuerpo;
los adaptadores simulan únicamente las dependencias necesarias.

También verifican integridad del formato, rechazo de archivos inválidos,
parches repetidos, geometría, recorte y algunos pasos. Esto prueba los
subconjuntos indicados; no demuestra equivalencia de todo CrossUO.
La compilación automática ejecuta los tests antes de generar el artefacto 3DS.

Para una previsualización en Mac de la lista del núcleo C++:

```sh
python3 tools/render_preview.py private-assets/britain-v01/scene.bin \
  --out private-assets/britain-v01/preview.png
```

Esa imagen es una **simulación por software**, no una captura de la consola ni
una captura de CrossUO. La comparación visual con un cliente original y la
validación física siguen pendientes. Consulta [la prueba en consola](docs/PRUEBA-3DS.md).

## Límites de v0.1

- Región máxima 32×32, 8.192 estáticos y 512 imágenes; todos los atlas de esa
  región se cargan juntos. No hay streaming del mundo completo todavía.
- El recorte superior usa un techo de altura sobre el personaje; no es la
  detección completa de interiores y continuidad de techos de CrossUO.
- Se retiene hue pero no se aplica su recoloración. No hay luces ni sombras UO.
- El movimiento interpolado es propio del laboratorio y necesita comparación
  visual, especialmente al pasar por delante/detrás de paredes.
- Los objetos dinámicos, multis, puertas interactivas y reglas avanzadas de
  movimiento se desarrollarán aparte.
- No se afirma compatibilidad completa con las reglas de The Second Age.

Licencia MIT; procedencia y diferencias documentadas en [THIRD_PARTY.md](THIRD_PARTY.md).

Cambios e instalación de esta entrega: [v0.2](docs/v0.2.md).

**v0.4:** carrera desarmada corregida a acción 2, atlas con procedencia comprobable, guardado de posición y mapa de orientación. [Instalación y controles](docs/v0.4.md).

**v0.16:** puertas originales interactivas sin duplicados y auditoría de los 60 spawners regionales. [Detalles y compatibilidad de partidas](docs/v0.16.md).

**v0.17:** nombres y profesiones con notoriedad, pausa con START y tejados conectados. [Instalación](docs/v0.17.md).

**v0.18:** fuente original con colores y borde, ocultación de nombres tras dos segundos cerca y sombreros encima del pelo. [Instalación](docs/v0.18.md).

## v0.30

Armas con daño/cadencia y armaduras con protección física; botín por criatura y
comerciantes cercanos en la cuarta página táctil. [Instalación y reglas](docs/v0.30.md).

## v0.31

Menú de Equipamiento en pausa, once ranuras con selección de objetos propios,
mangas y joyas con efectos. [Instalación y controles](docs/v0.31.md).

## v0.32

L/R navegan las ventanas; el paperdoll abre Equipamiento pausado. Transferencias
a cadáveres, arrastre, objetos temporales en Suelo y precios/cantidades claros.
[Instalación y controles](docs/v0.32.md).

## v0.33

Corrección del desbordamiento de pila al abrir Equipamiento y menú Desarrollador
con teletransporte seguro a puntos de interés. [Instalación y controles](docs/v0.33.md).

## v0.34

Defensa automática con foco en el primer agresor y curación mediante pociones,
vendas, pan y manzanas. [Instalación y controles](docs/v0.34.md).

## v0.35

SELECT cambia techos, Y repite la última curación y pausar guarda la partida.
Corrección de foco defensivo ante objetivos anteriores y agresores alejados del
origen. [Instalación y controles](docs/v0.35.md).

## v0.36

Progresión por uso, límites 100.0/700.0, modos Subir/Bajar/Bloquear y pantalla
de habilidades. Ocho habilidades conectadas a combate, vendas y meditación;
catálogo clásico de 49 con sistemas pendientes identificados.
[Instalación y reglas](docs/v0.36.md).

## v0.37

MISS amarillo/rojo para los fallos de ambos bandos y curación real en celeste
sobre el jugador. [Instalación y detalles](docs/v0.37.md).

## v0.38

Perfil Old 3DS con objetivo de 30 FPS, inventario y recursos con I/O en segundo
plano, colisiones incrementales, menos reservas y cachés de ropa/minimapa.
[Informe de optimización, medidas e instalación](docs/v0.38.md).

## v0.39

Caballo clásico desde el icono de Estado del personaje: paso, galope con B y
combate montado con equipo visible. Pausa visible antes del guardado asíncrono;
Opciones ocupa el lugar de Desarrollador, que se revela con L+R durante dos
segundos en pausa. Se optimizan composición de ropa, alcance de NPC y esperas de
atlas. [Instalación, controles, límites y validación](docs/v0.39.md).

## v0.40

Caballo orientado con el mismo ajuste que el personaje a pie. Puntos dinámicos
en el minimapa: hostiles rojos, criaturas neutrales grises, NPC humanos amarillos
y jugador celeste. Conserva la caché y el refresco de la pantalla inferior.
[Instalación y comprobaciones](docs/v0.40.md).

## v0.41

Caché fija para poses de NPC y cadáveres, lecturas protegidas frente al terreno,
menor presión de memoria y buffers de pantalla aislados en VRAM. Instalar también
los nuevos recursos privados de `npc-poses`. Ver [instrucciones y alcance de la
corrección](docs/v0.41.md).

## v0.41.1

Corrección del arranque: se restaura la reserva RAM de las pantallas anterior
a v0.41, conservando la nueva carga de poses de NPC y cadáveres. Instalar el ZIP
v0.41.1 sin borrar partidas. [Detalles](docs/v0.41.1.md).

## v0.41.2

Corregidas las lecturas de terreno que no terminaban y las poses de NPC que se
descartaban al avanzar la animación. Colas independientes y ciclos de caminar
completos con caché GPU fija. [Instalación y validación](docs/v0.41.2.md).
