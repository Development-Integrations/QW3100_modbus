# Cómo Usar esta Wiki en GitHub

## 📋 Resumen

Se han creado 7 documentos de wiki exhaustivos, listos para importar a tu GitHub Wiki:

1. **Home** - Overview del proyecto
2. **Arquitectura** - Diseño y componentes
3. **Setup** - Configuración del entorno
4. **Compilación** - Build ARM & Native
5. **Deployment** - Envío y ejecución remota
6. **Testing** - Validación y debugging
7. **Flujo de Desarrollo** - Workflow con Git y Convenciones

## 📂 Ubicación de Archivos

Todos están en: `/docs/`

```
docs/
├── WIKI_HOME.md                  ← Home/Index
├── WIKI_ARQUITECTURA.md          ← Arquitectura
├── WIKI_SETUP.md                 ← Setup del Entorno
├── WIKI_COMPILACION.md           ← Compilación
├── WIKI_DEPLOYMENT.md            ← Deployment
├── WIKI_TESTING.md               ← Testing
└── WIKI_FLUJO_DESARROLLO.md      ← Flujo de Desarrollo
```

## 🚀 Pasos para Importar a GitHub Wiki

### Opción 1: Crear Wiki desde Web UI (Manual)

1. **Ve a tu repositorio en GitHub**
   ```
   https://github.com/tu-usuario/QW3100_modbus
   ```

2. **Click en "Wiki" (o "Add Wiki" si no existe)**

3. **Crear primera página "Home"**
   - Click "Create the first page"
   - Copia contenido de `WIKI_HOME.md`
   - Pega en el editor
   - Click "Save Page"

4. **Agregar más páginas**
   - Click "+ New Page"
   - Name: `Arquitectura`
   - Copia contenido de `WIKI_ARQUITECTURA.md`
   - Click "Save Page"
   - Repetir para las demás páginas

5. **Los nombres que use en GitHub Wiki deben ser**:
   - `Home` (automático)
   - `Arquitectura`
   - `Setup`
   - `Compilacion`
   - `Deployment`
   - `Testing`
   - `Flujo-de-Desarrollo` (con guiones)

### Opción 2: Usar Git para Wiki (Automático)

Si prefieres workflow más directo:

```bash
# Clonar wiki repository
git clone https://github.com/tu-usuario/QW3100_modbus.wiki.git
cd QW3100_modbus.wiki

# Copiar archivos y renombrar (quitar WIKI_ prefix)
cp ../docs/WIKI_HOME.md Home.md
cp ../docs/WIKI_ARQUITECTURA.md Arquitectura.md
cp ../docs/WIKI_SETUP.md Setup.md
cp ../docs/WIKI_COMPILACION.md Compilacion.md
cp ../docs/WIKI_DEPLOYMENT.md Deployment.md
cp ../docs/WIKI_TESTING.md Testing.md
cp ../docs/WIKI_FLUJO_DESARROLLO.md Flujo-de-Desarrollo.md

# Crear barra lateral (opcional)
cat > _Sidebar.md << 'EOF'
# QW3100 Modbus Sensor

**Main**
- [Home](Home)

**Development**
- [Flujo de Desarrollo](Flujo-de-Desarrollo)
- [Setup](Setup)

**Technical**
- [Arquitectura](Arquitectura)
- [Compilacion](Compilacion)
- [Deployment](Deployment)
- [Testing](Testing)
EOF

# Hacer commit
git add .
git commit -m "docs: Importar documentación wiki del proyecto"
git push origin master
```

## 📝 Estructura Sugerida de Wiki

### Barra Lateral (_Sidebar.md)

```markdown
# QW3100 Modbus Sensor

## Getting Started
- [Home](Home)
- [Setup](Setup)

## Development
- [Arquitectura](Arquitectura)
- [Flujo de Desarrollo](Flujo-de-Desarrollo)

## Build & Deploy
- [Compilacion](Compilacion)
- [Deployment](Deployment)

## Testing & Debugging
- [Testing](Testing)
```

### Pie de Página (_Footer.md) - Opcional

```markdown
---
Last updated: 2026-03-24  
Contributors: [Tu Nombre]  
License: [Tu Licencia]
```

## ✅ Verificación

Después de importar:

- [ ] Todas las 7 páginas creadas
- [ ] Links internos funcionan (`[[link]]` o `[link](link)`)
- [ ] Images/diagramas se ven correctamente
- [ ] Tablas formateadas bien
- [ ] Code blocks con sintaxis resaltada
- [ ] Sidebar (si la creaste) aparece

## 🔗 Links Internos en Wiki

En GitHub Wiki, puedes linkear entre páginas:

```markdown
# Sintaxis Wiki
[[Home]]
[[Arquitectura]]
[[Setup]]

# O markdown standard
[Home](Home)
[Setup](Setup)
```

## 📊 Mantenimiento

### Actualizar Documentación

Cuando hagas cambios:

1. **Edita el archivo local** (`docs/WIKI_*.md`)
2. **Commit en main repo**
   ```bash
   git add docs/WIKI_*.md
   git commit -m "docs: Actualizar setup guide"
   git push
   ```

3. **Sincroniza con Wiki** (manual o scripts)
   ```bash
   cd QW3100_modbus.wiki
   cp ../docs/WIKI_SETUP.md Setup.md
   git add Setup.md
   git commit -m "docs: Actualizar setup guide"
   git push
   ```

### Script Sync Automatizado (Opcional)

Crear `sync-wiki.sh`:

```bash
#!/bin/bash

WIKI_REPO="QW3100_modbus.wiki"

echo "Sincronizando wiki..."
cd "$WIKI_REPO" || exit 1

# Copiar y renombrar
cp ../docs/WIKI_HOME.md Home.md
cp ../docs/WIKI_ARQUITECTURA.md Arquitectura.md
cp ../docs/WIKI_SETUP.md Setup.md
cp ../docs/WIKI_COMPILACION.md Compilacion.md
cp ../docs/WIKI_DEPLOYMENT.md Deployment.md
cp ../docs/WIKI_TESTING.md Testing.md
cp ../docs/WIKI_FLUJO_DESARROLLO.md Flujo-de-Desarrollo.md

# Commit
git add .
git commit -m "docs: Sincronización automática desde main"
git push origin master

echo "✅ Wiki sincronizada!"
```

Uso:
```bash
chmod +x sync-wiki.sh
./sync-wiki.sh
```

## 🎯 Flujo Recomendado

1. ✅ **Primero**: Importar documentación a Wiki (una sola vez)
2. ✅ **Luego**: Compartir link a Wiki con equipo
3. ✅ **Mantener**: Actualizar docs si cambian procesos
4. ✅ **Versionar**: Guardar `docs/WIKI_*.md` en main repo como backup

## 🤝 Reutilizar para Otros Proyectos

Los documentos están estructurados para ser **plantillas reutilizables**:

1. Copia `docs/WIKI_*.md` a nuevo repo
2. Reemplaza placeholders (nombres, puertos, paths, etc.)
3. Adapta secciones específicas
4. Listo para importar a Wiki

## ❓ Preguntas Frecuentes

### ¿Puedo editar directamente en GitHub Wiki?
Sí, pero se recomienda editar primero en `docs/` y luego syncronizar, así mantienes versión en Git.

### ¿Se sincroniza automáticamente?
No, necesitas copiar manualmente o usar script. GitHub no tiene eso nativo.

### ¿Otros colaboradores ven la Wiki?
Sí, si el repo es público. Si es privado, solo colaboradores.

### ¿Puedo exportar Wiki a PDF?
No oficialmente en GitHub, pero hay herramientas de terceros.

