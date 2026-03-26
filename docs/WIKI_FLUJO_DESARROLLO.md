# Flujo de Desarrollo

## 🔄 Workflow Típico

```
┌─────────────────────┐
│  Tarea / Feature    │
└──────────┬──────────┘
           │
    ┌──────▼──────────┐
    │  Feature Branch │ (git checkout -b feature/*)
    └──────┬──────────┘
           │
    ┌──────▼──────────────────┐
    │  Editar código           │
    │  - src/*.c               │
    │  - lib/*.c               │
    └──────┬──────────────────┘
           │
    ┌──────▼──────────────────┐
    │  Compilar & Testar      │
    │  - Native (local)       │
    │  - ARM (cross-compile)  │
    └──────┬──────────────────┘
           │
    ┌──────▼──────────────────┐
    │  Deploy Remoto          │
    │  (test en device)       │
    └──────┬──────────────────┘
           │
    ┌──────▼──────────────────┐
    │  Commit & Push          │
    │  (git commit/push)      │
    └──────┬──────────────────┘
           │
    ┌──────▼──────────────────┐
    │  Pull Request / Review  │
    └──────┬──────────────────┘
           │
    ┌──────▼──────────────────┐
    │  Merge a main           │
    └─────────────────────────┘
```

## 🎯 Flujo de una Feature Típica

### 1️⃣ Crear Branch

```bash
git checkout main
git pull origin main
git checkout -b feature/add-json-logging
```

### 2️⃣ Editar y Codificar

```bash
# Editar archivo
code src/main.c

# Guardar cambios
git add src/main.c
git commit -m "feat: Agregar logging en JSON"
```

### 3️⃣ Compilar Localmente (Native)

```bash
Cmd+Shift+B → "Compilar binario Native para Pruebas"
```

Verificar:
- ✅ Sin errores de compilación
- ✅ Sin warnings críticos
- ✅ Tests pasan: `./test/main_test`

### 4️⃣ Compilar ARM

```bash
Cmd+Shift+B → "1. Compilar ARM"
```

Verificar:
- ✅ Binario generado
- ✅ Tamaño razonable
- ✅ Arquitectura ARM: `file sensor_trident_modbus_ARM`

### 5️⃣ Deploy y Test Remoto

```bash
Cmd+Shift+B → "2. Enviar por SCP"
Cmd+Shift+B → "3. Ejecutar en Dispositivo"
```

Verificar:
- ✅ Se envía correctamente
- ✅ Se ejecuta sin errores
- ✅ Output es correcto

### 6️⃣ Push y PR

```bash
git push origin feature/add-json-logging

# Crear Pull Request en GitHub
# Asignar revisores
# Esperar feedback
```

### 7️⃣ Merge

```bash
# Después de aprobación:
git checkout main
git pull origin main
git merge feature/add-json-logging
git push origin main

# Borrar branch
git branch -d feature/add-json-logging
```

## 🚀 Atajos Útiles para VS Code

| Acción | Shortcut |
|--------|----------|
| Compilar (Ask task) | `Cmd+Shift+B` |
| Ir a definición | `F12` |
| Find usages | `Shift+F12` |
| Quick fix | `Cmd+.` |
| Format document | `Shift+Alt+F` |
| Open integrated terminal | `` Ctrl+` `` |
| Git commit | `Ctrl+Shift+G` → `Ctrl+Enter` |

## 📋 Checklist Pre-commit

Antes de hacer `git commit`:

- [ ] Código compila: `gcc ... -Wall`
- [ ] Tests pasan: `./test/main_test`
- [ ] Sin memory leaks: `valgrind`
- [ ] Formateado correctamente
- [ ] Comentarios/docs actualizados
- [ ] Mensaje commit claro: `feat:`, `fix:`, `refactor:` etc.

## 🐛 Debugging Workflow

### Escenario: Código falla en remoto

```
1. Copiar binario con símbolos (-g flag)
   scp -P 2122 sensor_*_debug root@192.168.188.39:/SD/

2. SSH y ejecutar con strace
   ssh -p 2122 root@192.168.188.39 \
       "strace -o /SD/trace.log /SD/sensor_*_debug"

3. Descargar logs
   scp -P 2122 root@192.168.188.39:/SD/trace.log ./

4. Analizar output
   cat trace.log | grep -i error
```

### Escenario: Memory corruption

```bash
# Compilar con AddressSanitizer
gcc ... -fsanitize=address -fsanitize=undefined ...

# Ejecutar y ver output
./test/main_test
# ASAN report

# Verificar stack trace en GDB
gdb ./test/main_test
# (gdb) run
```

## 📝 Convenciones de Commits

```
<tipo>(<scope>): <descripción corta>

<descripción larga opcional>

<referencias a issues>
```

### Tipos

- `feat`: Nueva funcionalidad
- `fix`: Corrección de bug
- `refactor`: Cambio sin nueva funcionalidad
- `docs`: Cambios en documentación
- `test`: Agregar o mejorar tests
- `ci`: Cambios en CI/CD

### Ejemplos

```
feat(modbus): Agregar timeout configurable

fix(sensor): Corregir parsing de FLOAT

refactor: Modularizar funciones principales

docs(wiki): Actualizar Setup guide
```

## 🔄 Proceso de Review (PR)

### Para el Autor

1. Crear PR con descripción clara
2. Asignar revisores
3. Responder comentarios
4. Re-request review después de cambios

### Para el Revisor

1. Verificar que compila
2. Revisar lógica de código
3. Sugerir mejoras
4. Aprobar o pedir cambios

### Criterios de Aceptación

- ✅ Código compila sin warnings
- ✅ Tests pasan
- ✅ Tested en remoto (si afecta funcionalidad)
- ✅ Documentación actualizada
- ✅ No hay regressions

## 📊 Histórico de Cambios

### Ver logs

```bash
# Últimos 10 commits
git log --oneline -10

# Por autor
git log --author="Name"

# Por archivo
git log -- src/main.c

# Gráfico visual
git log --graph --oneline --all
```

### Ver diferencias

```bash
# Cambios no commiteados
git diff

# Entre commits
git diff abc123 def456

# Entre branches
git diff main feature/nueva-feature
```

## 🔐 Buenas Prácticas

### ✅ Hacer

- ✓ Commits pequeños y enfocados
- ✓ Escribir buenos mensajes de commit
- ✓ Testear antes de push
- ✓ Hacer PR con descripción clara
- ✓ Rebase antes de merge (mantener historia limpia)

### ❌ Evitar

- ✗ Commits enormes con múltiples cambios
- ✗ Mensajes vaguos: "fix stuff"
- ✗ Push directo a main
- ✗ Mezclar cambios sin relación en un commit
- ✗ Olvidar actualizar documentación

## 🆘 Situaciones Comunes

### "Olvidé hacer un cambio antes de commitar"

```bash
# Hacer el cambio y amend
git add nuevo_cambio.txt
git commit --amend --no-edit
git push origin feature/x -f  # Force (solo si no hay PR merged)
```

### "Necesito descartar cambios"

```bash
# Descartar un archivo
git checkout -- src/main.c

# Descartar todo
git reset --hard
```

### "Commiteé en la rama equivocada"

```bash
# Crear rama nueva con commits
git branch feature/nueva-feature

# Retroceder en main
git reset --hard origin/main

# Cambiar a nueva rama
git checkout feature/nueva-feature
```

