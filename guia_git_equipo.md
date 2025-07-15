
# 🛠️ GUÍA DE TRABAJO EN EQUIPO CON GIT & GITHUB

## 🌐 Repositorio del Proyecto
🔗 [robot-basurero-seguidor-linea-iot](https://github.com/gabi-Q/robot-basurero-seguidor-linea-iot)

---

## 👥 Integrantes y Ramas Asignadas

| Integrante       | Rama Personal |
|------------------|---------------|
| Gabriel (Líder)  | `gabriel`     |
| Celeste          | `celeste`     |
| Argote           | `argote`      |
| Pongo            | `pongo`       |
| Rama Principal   | `main`        |

---

## ✅ Primera vez: Clonar el repositorio

Cada integrante debe abrir una terminal y ejecutar:

```bash
git clone https://github.com/gabi-Q/robot-basurero-seguidor-linea-iot.git
cd robot-basurero-seguidor-linea-iot
```

---

## 🔁 Flujo de Trabajo Diario

> Este ciclo se repite cada vez que quieras hacer cambios.

---

### 🔹 Paso 1: Asegurarte que `main` está actualizado

```bash
git checkout main
git pull origin main
```

> ✅ Esto asegura que tenés la última versión del proyecto principal.

---

### 🔹 Paso 2: Cambiar a tu rama personal

```bash
git checkout <tu_rama>
# Ejemplo: git checkout celeste
```

---

### 🔹 Paso 3: Traer los últimos cambios del `main` a tu rama

```bash
git merge main
```

> 🔄 Esto mezcla lo nuevo del proyecto en tu propia rama para que trabajes con la versión más actual.

---

### 🔹 Paso 4: Hacer tus cambios y subirlos

```bash
git add .
git commit -m "feat: descripción clara del cambio"
git push origin <tu_rama>
```

📌 Ejemplos de mensajes:

- `feat: agregado sensor ultrasónico`
- `fix: corregido error en motor`
- `docs: actualizado README`

---

### 🔹 Paso 5: Crear un Pull Request (PR)

✅ **Solo Gabriel (el líder)** debe hacer este paso.

1. Ir al repositorio en GitHub.
2. Ir a la pestaña **Pull Requests**.
3. Crear uno nuevo: de tu rama → hacia `main`.
4. Agregar título claro y una descripción.
5. Revisar y aprobar si todo funciona.

---

## 🔄 ¿Qué pasa después?

Una vez que el Pull Request fue aprobado y fusionado, **todos deben comenzar de nuevo desde el Paso 1** para mantenerse sincronizados.

---

## 📌 Resumen del Ciclo de Trabajo

```bash
# Paso 1
git checkout main
git pull origin main

# Paso 2
git checkout <tu_rama>

# Paso 3
git merge main

# Paso 4
git add .
git commit -m "mensaje"
git push origin <tu_rama>

# Paso 5
# Gabriel crea y aprueba el Pull Request
```

---

> ⚠️ Seguí siempre este flujo para evitar conflictos y asegurar que el proyecto avance bien sincronizado.
