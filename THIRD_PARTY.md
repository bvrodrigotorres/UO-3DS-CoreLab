# Procedencia y alcance

El núcleo adapta un subconjunto de archivos marcados MIT de
[CrossUO, revisión 79d0f94a3645154dc1795045d9d89df554d6cd2a](https://github.com/crossuo/crossuo/tree/79d0f94a3645154dc1795045d9d89df554d6cd2a).
Autores originales identificados en esos archivos: Copyright (c) 2016 Hotride.
La licencia MIT está incluida en LICENSE.

| Origen | Adaptación |
|---|---|
| [MapBlock.cpp: AddRender](https://github.com/crossuo/crossuo/blob/79d0f94a3645154dc1795045d9d89df554d6cd2a/src/GameObjects/MapBlock.cpp) | `priority` y orden estable por casilla. Solo terreno, estáticos normales y actor. |
| [LandObject.cpp](https://github.com/crossuo/crossuo/blob/79d0f94a3645154dc1795045d9d89df554d6cd2a/src/GameObjects/LandObject.cpp) | Cuatro alturas, media por diagonal y altura según dirección. |
| [PathFinder.cpp](https://github.com/crossuo/crossuo/blob/79d0f94a3645154dc1795045d9d89df554d6cd2a/src/Walker/PathFinder.cpp) | Candidatos de apoyo, puentes, límites de altura, hueco de 16 unidades, paso diagonal y deslizamiento lateral. |
| [GameScreen.cpp](https://github.com/crossuo/crossuo/blob/79d0f94a3645154dc1795045d9d89df554d6cd2a/src/ScreenStages/GameScreen.cpp) | Recorrido diagonal de casillas; no se importa la clase de escritorio. |

`tests/upstream/*.inc` contienen dos funciones originales sin cambios de cuerpo.
`tests/reference.cpp` aporta objetos simulados y entradas de prueba, sin SDL,
jugador conectado ni UI. Se suprime el destructor virtual de CPathObject en el
adaptador para poder ejecutar el qsort original sobre un registro trivial.
No se usa ese adaptador en la consola.

Las comparaciones comprueban `AddRender` en columnas con un terreno, estáticos
y un actor, y `CalculateNewZ` con candidatos y límites ya suministrados.
No demuestran equivalencia de la construcción completa del mundo, de todos los
estados del jugador, de movimiento interpolado ni de la GPU.

Cambios deliberados: registros compactos; vectores en lugar de listas enlazadas;
desempates estables; enteros con signo explícito; exclusión de monturas, vuelo,
muerte, GM, casas personalizadas, red, efectos y búsqueda automática de rutas.
Las puertas conservan sus flags, pero no tienen interacción en v0.1.
El recorte de techos es nuevo y experimental: no reproduce todavía la búsqueda
de continuidad de techos de CrossUO.

MobileUO/ClassicUO se consultó como segunda referencia. No se incorpora su código
ni Unity en esta entrega. El conversor aplica parches de archivos por orden de
lista, con la última entrada para un bloque prevaleciendo. Se aplican todos los
parches locales disponibles; un servidor podría anunciar otro número de parches.

Dependencias externas (no se incluyen sus fuentes): devkitARM, libctru,
citro2d, citro3d y tex3ds de devkitPro; Python y compilador C++ para tests;
Pillow y Ultima SDK Python para conversión privada y previsualización.
Cada dependencia conserva su propia licencia.

No se distribuyen mapas, arte, sonidos o animaciones originales de Ultima Online.
El usuario aporta los datos locales. Este proyecto no está afiliado a sus titulares.

El conversor tools/export_avatar.py procede del trabajo previo del usuario en UO-3DS-Offline, adaptado aquí con acción 2 correcta y metadata. No se modifica el conversor ni los recursos del repositorio anterior.

El formato RLE de los gumps en tools/export_journal.py se contrastó con
[ClassicUO GumpsLoader.Decode](https://github.com/ClassicUO/ClassicUO/blob/main/src/ClassicUO.Assets/GumpsLoader.cs): offsets DWORD relativos al inicio de la tabla y pares color/longitud.
El conversor es propio; no incorpora arte del cliente al repositorio.

Para v0.8 se consultan los datos y convenciones de ServUO pub57
(commit d76bf4443cf76d081ddaf8f57c87ff33749256af): decoración de Britain,
carteles, marcos y estados de puertas. ServUO conserva su licencia GPL-2.0.
No se incorporan sus archivos de configuración ni sus fuentes C# al repositorio;
el importador propio acepta referencias aportadas localmente y registra sus hashes.
Las posiciones y gráficos exportados permanecen en el paquete privado del usuario.
Ver [procedencia y límites](docs/v0.8.md).
