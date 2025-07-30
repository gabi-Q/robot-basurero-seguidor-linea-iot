# Sistema Robótico Móvil IoT para la Gestión Automatizada de Residuos con Navegación Autónoma a un Punto de Acopio

## Descripción del Proyecto

Este proyecto presenta el diseño y la construcción de un prototipo de robot móvil para la gestión automatizada de residuos sólidos en interiores. El sistema integra conceptos del **Internet de las Cosas (IoT)** para permitir el monitoreo y control del dispositivo en tiempo real.

El robot funciona como un contenedor estacionario que, al detectar que ha alcanzado su capacidad máxima, se desplaza de forma autónoma siguiendo una línea designada hasta un punto de acopio central. La principal innovación es que invierte la logística tradicional: en lugar de que el personal de limpieza revise cada contenedor, el contenedor se mueve hacia el personal cuando necesita ser vaciado.

## 👥 Autores

| Nombre | Usuario GitHub |
|--------|----------------|
| René Pongo Calderón | Rene-Pongo |
| Gabriel Omar Quispe Llanque | gabi-Q |
| Juan Julio Argote Huancapaza | argote314 |
| Paola Celeste Romucho Sullcahuaman | CelesteSky16 |


## Funcionalidades Principales

- **Apertura sin contacto:** La tapa se abre automáticamente al detectar la proximidad de una persona, usando un sensor ultrasónico.

- **Monitoreo IoT en Tiempo Real:** Un sensor láser de Tiempo de Vuelo (ToF) mide el nivel de llenado y, a través del microcontrolador ESP32 con Wi-Fi, transmite los datos a un dashboard web a través de Firebase. Esta es una funcionalidad clave del sistema IoT.

- **Navegación Autónoma:** Al llenarse, el robot activa un modo de navegación guiada por sensores infrarrojos y un controlador PID para seguir una ruta hasta el punto de acopio.

- **Retorno a la Base:** Tras ser vaciado, el sistema retorna a su estación de origen para reiniciar el ciclo de recolección.
---
## Dashboard

Puedes acceder al dashboard en [Smart Bin Dashboard](https://robot-basurero-seguidor-linea-iot.vercel.app/).
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



## Estructura del Proyecto

```
robot-basurero-seguidor-linea-iot/
├── README.md
├── docs/  # Documentación del proyecto
│   ├── articulo.pdf
│   ├── informe.pdf
│   └── video_url.txt
├── Src/  # Código fuente principal
│   ├── App/  # Dashboard web (aplicación React)
│   │   ├── App.tsx
│   │   ├── constants.ts
│   │   ├── firebaseConfig.ts
│   │   ├── index.html
│   │   ├── index.tsx
│   │   ├── package.json
│   │   ├── package-lock.json
│   │   ├── tsconfig.json
│   │   ├── types.ts
│   │   ├── vite.config.ts
│   │   └── avatar.jpg
│   ├── src/  # Código principal ESP32 (firmware Arduino)
│   │   └── main.ino
│   ├── platformio.ini  # Configuración PlatformIO
│   ├── include/  # Archivos de cabecera
│   │   └── README
│   ├── lib/  # Librerías
│   │   └── README
│   ├── test/  # Pruebas
│   │   └── README
│   ├── .gitignore
│   └── .vscode/  # Configuración VS Code
│       └── extensions.json
├── data/  # Datos y resultados
│   └── resultados.md
├── Imagenes/  # Imágenes y diagramas
│   └── diagrama de actividades.png
├── guia_git_equipo.md  # Guía de Git para el equipo
└── requeriments.txt  # Requisitos y dependencias
```

Esta estructura organiza el código fuente en Src, documentación en docs, datos en data e imágenes en Imagenes, con descripciones de las subcarpetas principales.

