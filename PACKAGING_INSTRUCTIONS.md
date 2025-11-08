# Instrucciones para Empaquetar Veyon

## Cambios Realizados

He modificado el sistema de empaquetado para que **siempre use los binarios más recientes** del directorio `build/` en lugar del directorio `dist/`.

### Archivos Modificados:
1. **package-veyon-macos-v3.sh** - Ahora busca primero en `build/`, luego en `dist/` como fallback
2. **fix-worker-post-package.sh** - Ahora busca primero `build/worker/veyon-worker`, luego `dist/bin/veyon-worker`

## Comandos para Ejecutar Manualmente

### Paso 1: Limpieza y Preparación
```bash
cd /Users/pablo/GitHub/veyon
pkill -9 veyon-server veyon-worker
rm -rf veyon-macos-package
```

### Paso 2: Ejecutar Empaquetado
```bash
./package-veyon-macos-v3.sh
```

**⏱️ DURACIÓN ESPERADA**: ~15 minutos

Durante el empaquetado verás mensajes como:
- `✓ Usando veyon-server de BUILD/server (compilación reciente)`
- `✓ Usando veyon-configurator de BUILD/configurator (compilación reciente)`
- `✓ Usando veyon-master de BUILD/master (compilación reciente)`

Si ves mensajes con `⚠` significa que está usando binarios antiguos de DIST.

### Paso 3: Arreglar veyon-worker
```bash
./fix-worker-post-package.sh
```

Deberías ver:
```
✓ Using veyon-worker from BUILD directory (most recent)
```

### Paso 4: Probar el Servidor
```bash
chmod +x quick-test-server.sh
./quick-test-server.sh
```

## Qué Verificar en los Logs

Después del Paso 4, busca estas líneas en los logs para confirmar que el código nuevo está activo:

### 1. Logs de PUMP (main.cpp nuevo):
```
[PUMP] macOS: Started main dispatch queue pump timer
[PUMP] Main queue pump active (iteration 100)
[PUMP] Main queue pump active (iteration 200)
```

### 2. Logs de ScreenCapturer (código nuevo):
```
[DEBUG] ScreenCapturer: startCaptureWithError called (isMainThread=...)
[DEBUG] ScreenCapturer: In capture block (isMainThread=...)
[DEBUG] ScreenCapturer: Adding stream output with main queue...
[DEBUG] ScreenCapturer: startCapture succeeded! Waiting for frames...
```

### 3. Logs de Frames (si funciona):
```
[DEBUG] didOutputSampleBuffer called! Frame received
```

## Si Algo Sale Mal

### Problema: No aparecen logs [PUMP]
**Causa**: El binario empaquetado no incluye los cambios de main.cpp
**Solución**:
```bash
# Recompilar server
cd build
rm -rf server/CMakeFiles/veyon-server.dir
make -j4
cd ..
# Repetir empaquetado
```

### Problema: Logs antiguos (dispatching startCaptureWithError)
**Causa**: macvnc no se recompiló
**Solución**:
```bash
cd build
rm -rf 3rdparty/macvnc
make -j4
cd ..
# Repetir empaquetado
```

## Monitorear Progreso en Tiempo Real

En otra terminal, puedes monitorear el progreso del empaquetado:
```bash
# Ver progreso general
tail -f /tmp/veyon-packaging.log

# O simplemente ver cuándo termina
while ps aux | grep -q "[p]ackage-veyon-macos-v3.sh"; do
    sleep 5
    echo -n "."
done
echo ""
echo "¡Empaquetado completado!"
```

## Estructura de Directorios

```
build/
  ├── server/veyon-server.app      ← Usado ahora (NUEVO)
  ├── configurator/veyon-configurator.app ← Usado ahora
  ├── master/veyon-master.app      ← Usado ahora
  └── worker/veyon-worker          ← Usado ahora

dist/
  ├── Applications/Veyon/
  │   ├── veyon-server.app         ← Ignorado si build/ existe
  │   ├── veyon-configurator.app
  │   └── veyon-master.app
  └── bin/veyon-worker              ← Ignorado si build/worker/ existe

veyon-macos-package/              ← Output final
  ├── veyon-server.app
  ├── veyon-configurator.app
  └── veyon-master.app
```

## Próximos Pasos Después del Empaquetado

Una vez completado el empaquetado y las pruebas:

1. Avísame cuando terminen los pasos
2. Compartiré los logs relevantes que encuentres
3. Verificaremos si los callbacks de ScreenCaptureKit finalmente funcionan
4. Si los frames llegan, deberías ver la pantalla real en veyon-master en lugar del rectángulo blanco/gris

---

**Nota**: Estos scripts ahora siempre usarán los binarios más recientes de `build/`, lo que te servirá para futuros cambios sin necesidad de modificar los scripts de nuevo.
