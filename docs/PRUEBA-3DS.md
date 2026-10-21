# Primera prueba: UO-3DS-CoreLab v0.14

Esta es una aplicación distinta a UO-3DS-Offline. Mantén ambas carpetas.

## Instalación

1. Copia la carpeta `UO-3DS-CoreLab` a `SD:/3ds/`.
2. Abre `UO-3DS-CoreLab.3dsx` desde Homebrew Launcher.
3. El título inferior debe decir `BRITANNIA` y `CORELAB V0.9.1`.

El paquete público contiene la demo incorporada. Si el paquete fue preparado
localmente con los recursos privados, también incluye `scene.bin`, ocho o más
atlas según la exportación y, opcionalmente, tu animación original.
La escena privada aparece al inicio solo si todos los recursos validan.
Si falta un archivo, se informa el error y se abre la demo.

## Recorrido corto

1. Pulsa X para elegir `SYNTHETIC TEST`.
2. Camina alrededor de la habitación, entra por los huecos y prueba los escalones.
3. Pulsa A para recorrer los cuatro puntos de diagnóstico.
4. Pulsa Y para recorrer AUTO, HIDDEN y FULL. AUTO recorta el interior y
   transparenta techos/muros que ocultan al personaje desde fuera; HIDDEN quita
   todas las piezas de plantas superiores; FULL muestra todas las alturas. Los obstáculos
   conservan sus colisiones.
5. En la pendiente comprueba que no haya huecos entre casillas.
6. Si hay escena privada, pulsa X y revisa la habitación y alfombra cerca de
   (1501,1612). La altura inicial debe ser 20. A vuelve al inicio.
7. Camina/corre durante unos minutos y anota FPS, CPU/GPU, memoria libre y
   cualquier detención, desaparición de sprites o error del HUD.

No reemplaces archivos en `SD:/3ds/UO-3DS-Offline/`.

## Qué falta comprobar

La primera entrega está compilada y tiene pruebas de núcleo y formato en Mac.
La consola debe confirmar imagen, funcionamiento de la GPU, lectura de los
atlas, controles y rendimiento. Objetivo inicial: 30 FPS en este recorrido,
con memoria libre estable. No es un rendimiento ya verificado.

La demo y la escena privada no son un juego terminado. Un fallo de oclusión
se registra con coordenadas, Z, modo de techo y una foto de ambas pantallas;
no debe corregirse inventando datos en el exportador.

Para el parche v0.12 y sus controles nuevos, consulta [v0.12.md](v0.12.md).
La página 3 permite abrir un cofre cercano y transferir pilas con el táctil.

Página 3: MODO: EQUIPAR permite ponerse daga, túnica o botas de la mochila.
Página 2: M/R/B quitan cada pieza y la devuelven a la mochila.

Al iniciar se muestra el menú. Pad y A o táctil para elegir.
Nuevo Juego pide confirmar; Continuar conserva el inventario y equipo guardados.

Prueba v0.14: mapa privado 256x256 y equipo caminando.
Consultar [v0.14.md](v0.14.md) para instalación y partidas separadas.

Actualización v0.15: consultar [v0.15.md](v0.15.md).

Actualización v0.16: consultar [v0.16.md](v0.16.md).

Actualización v0.17: consultar [v0.17.md](v0.17.md); START ahora abre la pausa.


## v0.18: texto y sombreros

Consultar [instalación y cambios](v0.18.md). Probar nombres amarillos/grises sobre madera, hierba y piedra; acercarse dos segundos hasta que se oculten y alejarse para recuperarlos. Comprobar sombreros de Emilio, Verity y bard al caminar y girar. La pausa no debe consumir el tiempo de proximidad.


## v0.20: combate y estado original

Consultar [instalación y controles](v0.20.md). A inicia/detiene el ataque y X cambia de criatura; acercarse con el pad. La segunda página muestra el gump de estado original. Desde la tercera página se abre el cadáver cercano con el mismo botón que los cofres. Verificar daño y barra sobre el jugador, botín persistente, reaparición a los 20 segundos, pausa de los temporizadores y retorno al punto inicial al morir.


## v0.21: mochila y botín automático

Consultar [v0.21.md](v0.21.md). Acercarse a un cofre o cadáver y abrir la tercera página: el contenido debe aparecer directamente. Probar SACAR TODO, mochila llena sin pérdida, 12 casillas por hoja y equipo desde la segunda hoja. Verificar ESTADO DEL PERSONAJE con el nuevo status.bin. Continuar debe conservar todo el equipo e inventario anterior.

## v0.22: personalización

Consultar [v0.22.md](v0.22.md) para instalar la carpeta character completa. Probar
crear/cancelar, teclado para nombre, ambos cuerpos, pelo/barba y colores, giro,
las tres vistas de equipo, guardar desde pausa y Continuar tras reiniciar.
Verificar que el mundo siga detenido al editar y que no cambien mochila ni
equipo. Probar Salir del juego desde el menú principal.

## v0.23: vestimenta real

Consultar [v0.23.md](v0.23.md). Comprobar confirmación de nuevo juego, cancelar
sin reiniciar, vistas previas y nombre medieval. Continuar una partida de v0.22,
quitar camisa/pantalón/zapatos y volver a equiparlos desde la mochila. Buscar
gorros, gorjales y guantes en los cofres; verificar las ocho ranuras y el pelo
bajo el gorro. Personalizar desde pausa permite colores pero bloquea cambiar
los tipos de prendas, incluso usando Y. Reiniciar y verificar todo el equipo.
