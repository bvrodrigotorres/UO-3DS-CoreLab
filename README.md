# UO-3DS-CoreLab v0.10

Laboratorio independiente para probar en la **Nintendo 3DS original** un núcleo
de escena y movimiento adaptado de CrossUO. Este repositorio tiene historia,
ejecutable y carpeta de SD propios; no depende del repositorio UO-3DS-Offline.

**Primera prueba técnica, no un juego completo.** Caminar, alturas, orden de
dibujo, geometría del terreno y diagnóstico de rendimiento. Incluye NPC locales y una primera mochila con cofres persistentes. No incluye
combate, servidor ni recursos originales de Ultima Online.

[Novedades e instalación de v0.10](docs/v0.10.md): tercera página táctil con mochila,
cofres del mapa y transferencias guardadas automáticamente.

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

Descarga el artefacto **UO-3DS-CoreLab-v0.10** de un run verde en
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
| A | Siguiente punto de prueba en demo; volver al inicio en escena privada |
| X | Alternar demo y escena privada, si esta se cargó correctamente |
| R / deslizar horizontalmente | Recorrer diagnóstico, minimapa/personaje y mochila/cofre |
| SELECT / L | Guardar / restaurar posición, detenido |
| START | Salir |

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
