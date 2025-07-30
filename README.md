# Sistema Robótico Móvil IoT para la Gestión Automatizada de Residuos con Navegación Autónoma a un Punto de Acopio

## Descripción del Proyecto

Este proyecto presenta el diseño y la construcción de un prototipo de robot móvil para la gestión automatizada de residuos sólidos en interiores. El sistema integra conceptos del **Internet de las Cosas (IoT)** para permitir el monitoreo y control del dispositivo en tiempo real.

El robot funciona como un contenedor estacionario que, al detectar que ha alcanzado su capacidad máxima, se desplaza de forma autónoma siguiendo una línea designada hasta un punto de acopio central. La principal innovación es que invierte la logística tradicional: en lugar de que el personal de limpieza revise cada contenedor, el contenedor se mueve hacia el personal cuando necesita ser vaciado.

## Funcionalidades Principales

- **Apertura sin contacto:** La tapa se abre automáticamente al detectar la proximidad de una persona, usando un sensor ultrasónico.

- **Monitoreo IoT en Tiempo Real:** Un sensor láser de Tiempo de Vuelo (ToF) mide el nivel de llenado y, a través del microcontrolador ESP32 con Wi-Fi, transmite los datos a un dashboard web a través de Firebase. Esta es una funcionalidad clave del sistema IoT.

- **Navegación Autónoma:** Al llenarse, el robot activa un modo de navegación guiada por sensores infrarrojos y un controlador PID para seguir una ruta hasta el punto de acopio.

- **Retorno a la Base:** Tras ser vaciado, el sistema retorna a su estación de origen para reiniciar el ciclo de recolección.
---

## 🧭 Diagrama de Actividades

![Diagrama de Actividades](Imagenes/diagrama%20de%20actividades.png)

---
| Nombre                                   | Tipo de Dato | Descripción                                               | Ejemplo               |
| ---------------------------------------- | ------------ | --------------------------------------------------------- | --------------------- |
| `porcentajeLlenado` | Decimal      | Nivel de llenado calculado en %                           | `62.5`                |
| `timestamp`         | Entero (int) | Marca de tiempo en formato UNIX                           | `1753633565`          |
| `tapaAbierta`       | Booleano     | Estado de la tapa (abierta/cerrada)                       | `false`               |
| `personaDetectada`  | Booleano     | Presencia detectada mediante sensor de proximidad         | `true`                |
| `calibrado`         | Booleano     | Indica si el sensor ha sido calibrado correctamente       | `true`                |
| `enMovimiento`      | Booleano     | Indica si el contenedor está en movimiento                | `false`               |
| `estado`            | Texto        | Estado del sistema           | `"ESPERANDO_EN_BASE"` |
| `giroCalibrado`     | Booleano     | Indica si el giro está correctamente calibrado            | `true`                |
| `estaLleno`         | Booleano     | Indica si el sistema detecta que el contenedor está lleno | `false`               |
| `historialLlenado`                | Objeto       | JSON con registros históricos de nivel de llenado         | Ver abajo             |


### 🔁 Estructura Firebase (Realtime Database)

```json
"sensor": {
  "currentStatus": {
    "calibrado": true,
    "enMovimiento": false,
    "estaLleno": false,
    "estado": "ESPERANDO_EN_BASE",
    "giroCalibrado": true,
    "personaDetectada": false,
    "porcentajeLlenado": 3.84615,
    "tapaAbierta": false,
    "timestamp": 1753633565
  },
  "historialLlenado": {
    "-OWBh4q9X32-1okz0ifV": {
      "porcentajeLlenado": 0,
      "timestamp": 1753633545
    }
  }
}


```

## 👥 Autores

- René Pongo Calderón  
- Gabriel Omar Quispe Llanque  
- Juan Julio Argote Huancapaza  
- Paola Celeste Romucho Sullcahuaman
