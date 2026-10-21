# Primera prueba: UO-3DS-CoreLab v0.7

Esta es una aplicación distinta a UO-3DS-Offline. Mantén ambas carpetas.

## Instalación

1. Copia la carpeta `UO-3DS-CoreLab` a `SD:/3ds/`.
2. Abre `UO-3DS-CoreLab.3dsx` desde Homebrew Launcher.
3. El título inferior debe decir `BRITANNIA` y `CORELAB V0.7`.

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

Para el parche v0.7 y sus controles nuevos, consulta [v0.7.md](v0.7.md).
