# Validación de la primera entrega

Validación local realizada el 20 de octubre de 2026:

- Compilación ARMv6K con devkitARM y advertencias tratadas como errores.
- Ejecutable `.3dsx` y archivo `.smdh` generados correctamente.
- Seis grupos de pruebas aprobados con AddressSanitizer y
  UndefinedBehaviorSanitizer: 23.000 casos diferenciales contra funciones
  originales de CrossUO y comprobaciones de formato, geometría y navegación.
- Conversión privada real de 32×32: 2.042 objetos, 240 imágenes, 56 techos,
  sin descartar pisos superiores ni agregar piezas de decoración.
- Previsualización por software a partir de la lista del C++ real.
  En (1501,1612,20) el núcleo permite ocho direcciones de movimiento.
  La altura 10 situaba el personaje debajo del piso y no permitía salir;
  se fijó el inicio explícito en el piso a Z=20.

La previsualización no prueba el resultado de la GPU y conserva defectos
visuales por investigar. No se declara resuelto todo el renderizado de Britain.
Falta comparar esta misma escena con capturas de un cliente de referencia,
validar físicamente la 3DS original y medir FPS, memoria y transiciones.

Los binarios y el paquete de recursos privados se generan localmente y no se
guardan en Git. GitHub Actions genera un paquete descargable con la demo
incorporada, sin recursos originales. El resultado de Actions se consulta en
el run correspondiente al commit; este documento no presupone que pasó.
