# Sistema Robótico Móvil IoT para la Gestión Automatizada de Residuos con Navegación Autónoma a un Punto de Acopio

## Descripción del Proyecto

Este proyecto presenta el diseño y la construcción de un prototipo de robot móvil para la gestión automatizada de residuos sólidos en interiores. El sistema integra conceptos del **Internet de las Cosas (IoT)** para permitir el monitoreo y control del dispositivo en tiempo real.

El robot funciona como un contenedor estacionario que, al detectar que ha alcanzado su capacidad máxima, se desplaza de forma autónoma siguiendo una línea designada hasta un punto de acopio central. La principal innovación es que invierte la logística tradicional: en lugar de que el personal de limpieza revise cada contenedor, el contenedor se mueve hacia el personal cuando necesita ser vaciado.

## Funcionalidades Principales

- **Apertura sin contacto:** La tapa se abre automáticamente al detectar la proximidad de una persona, usando un sensor ultrasónico.

- **Monitoreo IoT en Tiempo Real:** Un sensor láser de Tiempo de Vuelo (ToF) mide el nivel de llenado y, a través del microcontrolador ESP32 con Wi-Fi, transmite los datos a un dashboard web a través de Firebase. Esta es una funcionalidad clave del sistema IoT.

- **Navegación Autónoma:** Al llenarse, el robot activa un modo de navegación guiada por sensores infrarrojos y un controlador PID para seguir una ruta hasta el punto de acopio.

- **Retorno a la Base:** Tras ser vaciado, el sistema retorna a su estación de origen para reiniciar el ciclo de recolección.

## Autores

- Pongo Calderón, René  
- Quispe Llanque, Gabriel Omar  
- Argote Huancapaza, Juan Julio  
- Romucho Sullcahuaman, Paola Celeste
