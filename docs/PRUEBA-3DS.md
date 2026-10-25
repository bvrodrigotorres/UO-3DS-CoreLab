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

## v0.24: orden y equipo rápido

Consultar [v0.24.md](v0.24.md). Reemplazar status.bin y character completos.
Comprobar las cuatro pestañas y sus nuevos campos; crear con túnica Sí/No;
comprobar colores de accesorios deshabilitados al crear y al quitar la prenda.
Verificar sombrero entero y título de estado centrado. Tocar dos veces cada
botón de equipo, reiniciar con una ranura vacía y volver a equipar su última
prenda. Depositarla en un cofre y comprobar que el botón no crea una copia.

## v0.25: colores y recortes

Consultar [v0.25.md](v0.25.md). Copiar character completo. Probar afro con ambos
cuerpos y sombrero equipado; comprobar ambas vistas y la segunda página del
juego. Seleccionar el primer y último color de pelo y barba; deben ser tonos
naturales. Cambiar color calzado, guardar, continuar y comprobar zapatos/botas.
Recorrer las siete filas de Colores con pad y táctil y verificar cada flecha.

## v0.26: guardarropa propio

Consultar [v0.26.md](v0.26.md). Desde pausa, probar las ocho ranuras de Ropa:
quitar, alternar prendas de la mochila, guardar y continuar tras reiniciar.
Cancelar debe conservar el inventario anterior. Probar mochila llena y verificar
que no desaparece ningún objeto. Revisar sombrero teñido en ambas vistas,
cambiar el peinado y volver a quitar el sombrero.

## v0.27: combate animado

Consultar [v0.27.md](v0.27.md). Reemplazar character completo, incluido combat/.
Probar A sin arma y con daga, ocho direcciones, ropa y túnica. Caminar durante
el combate y pausar a mitad de golpe. Atacar una criatura a varias casillas:
debe acercarse y responder; probar pared, esquina y otro piso. Revisar el
sombrero coloreado tanto en personalización como en la página de equipo.

## v0.28: daño, vida y sangre

Consultar [v0.28.md](v0.28.md). Reemplazar programa y añadir blood.bin/blood.t3x.
Recibir daño y esperar: la barra debe seguir visible mientras recupera 13/50
puntos, o hasta llenarse si falta menos. Comprobar números amarillos al golpear
y rojos al recibir daño, gotas, charco mortal de tres segundos y botín intacto.
Pausar durante el efecto y comprobar que su tiempo queda detenido. Probar
pisos superiores y paredes delante de los efectos.

## v0.29: Britain ampliado

Consultar [v0.29.md](v0.29.md). Añadir britain-expanded y conservar britain-large.
Al iniciar debe ofrecer Continuar con la campaña importada, equipo, mochila y
colores; comprobar un cofre que ya se hubiera saqueado y no se debe rellenar.
Salir y continuar sin que se importe de nuevo. Caminar al cementerio y exteriores
del norte/oeste, comprobar agresión, colisiones, muertes, botín y reaparición.
Revisar mapa, puertas, pisos y la primera carga de nuevas especies desde SD.

## Actualización v0.39: caballo y pausa inmediata

Copiar **toda** la carpeta `3ds` del ZIP privado sobre la SD. Además del ejecutable
actualizado, se agrega `3ds/UO-3DS-CoreLab/character/mounted/` (68 capas originales
más su manifiesto). No borrar `character`, mapas, objetos ni partidas. No hace
falta Nuevo Juego. El arte privado del caballo no se incluye en GitHub Actions.

En la página del minimapa/personaje, tocar el icono de caballo en Estado del
personaje estando detenido: montar; tocarlo de nuevo: desmontar. Pad para caminar,
B + pad para galopar. Se conserva la montura al continuar una partida. Al morir,
se regresa a pie al punto inicial y se puede volver a montar desde el icono.

START muestra la pausa antes de iniciar el guardado. «GUARDANDO PARTIDA...» cambia
a «PARTIDA GUARDADA» cuando termina la cola. En pausa, Opciones ocupa el lugar de
Desarrollador; mantener L+R dos segundos revela Desarrollador hasta cerrar la pausa.

Pruebas en consola: andar/galopar en ocho direcciones, atacar con/sin armas,
montar con distintas prendas y cuerpos, escaleras/puertas, equipar y personalizar
montado, guardar/salir/continuar, y varios enemigos simultáneos. Ver también
`docs/v0.39.md` para alcance y validación.

## v0.40

Actualizar ejecutable y SMDH sobre v0.39, conservando la carpeta de animaciones
`character/mounted`. Probar paso/galope y ataque en las ocho direcciones: el
caballo debe mirar hacia donde avanza. El minimapa añade puntos rojos para
hostiles, grises para criaturas neutrales, amarillos para humanos y celeste para
el jugador. Probar movimiento, zoom, hostilidad al atacar, muerte y respawn.

## v0.41

Copiar toda la carpeta `3ds` del ZIP privado, incluidos `britain-expanded/npc-poses.bin`
y `npc-poses/`. No borrar mapas ni partidas. Diagnóstico: `NPC SPRITES: CACHE 4 MIB`.
Repetir bosque, persecución, curación y saqueo durante una sesión prolongada.
Detalles: [v0.41](v0.41.md).

## v0.41.1

Corrección del arranque: se restaura la reserva RAM de las pantallas anterior
a v0.41, conservando la nueva carga de poses de NPC y cadáveres. Instalar el ZIP
v0.41.1 sin borrar partidas. [Detalles](v0.41.1.md).

## v0.41.2

Corregidas las lecturas de terreno que no terminaban y las poses de NPC que se
descartaban al avanzar la animación. Colas independientes y ciclos de caminar
completos con caché GPU fija. [Instalación y validación](v0.41.2.md).
