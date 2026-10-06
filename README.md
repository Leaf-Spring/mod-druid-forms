# AzerothCore: Módulo Formas de Druida (mod-druid-forms)

Este módulo para **AzerothCore (WotLK 3.3.5a)** permite a los jugadores de clase Druida personalizar y cambiar la apariencia (`displayId`) de sus transformaciones metamórficas mediante comandos dentro del juego, manteniendo sus selecciones guardadas de forma permanente en la base de datos de personajes.

---

## Características Principales

* **Comandos dedicados:** Selección e inspección de aspectos para Forma de Gato, Oso, Viaje, Árbol de la Vida, Lechúcico Lunar, Forma Acuática y Forma de Vuelo.
* **Persistencia total:** Las elecciones de cada personaje se almacenan en la base de datos de personajes y se reasignan automáticamente al cambiar de zona, mapa o al iniciar sesión.
* **Manejo por facción o unificado:** Opción para mostrar formas exclusivas por facción (Alianza / Horda) o habilitar todas para todos los jugadores.
* **Instalación limpia sin scripts externos:** Importación automática de tablas mediante el gestor de base de datos nativo de AzerothCore (`dbimport`).

---

## Requisitos Prácticos

* **AzerothCore (branch `master`)** compilado y en ejecución.
* Un personaje de clase **Druida** para interactuar con los comandos.

---

## Instalación

1. Clona o descarga este repositorio dentro del directorio de módulos de tu código fuente de AzerothCore:
   cd /ruta/a/tu/azerothcore-wotlk/modules
   git clone https://github.com/tu-usuario/mod-druid-forms.git

2. **Recompila el servidor:** Vuelve a ejecutar `cmake` y compila el proyecto (`make` en Linux o Visual Studio en Windows) para registrar las firmas C++ del módulo.

3. **Carga automática de base de datos:** Al iniciar `worldserver`, el sistema `dbimport` creará automáticamente la tabla `character_druid_forms_selections` en la base de datos de personajes (`acore_characters`) utilizando el esquema ubicado en `data/sql/db-characters/base/`.

4. **Configuración (Opcional):**
   * Copia la plantilla de configuración desde la carpeta del módulo a la carpeta de ejecución de tu servidor:
     cp modules/mod-druid-forms/conf/mod_druid_forms.conf.dist /ruta/a/tu/servidor/etc/modules/mod_druid_forms.conf
   * Edita `mod_druid_forms.conf` para ajustar los IDs de modelos y nombres de las transformaciones según tus preferencias.

---

## Uso en el Juego

### 1. Consultar Opciones Disponibles
Escribe el comando de la forma que deseas consultar sin ningún parámetro adicional para desplegar la lista numerada de modelos configurados:

* `.cat_form`
* `.bear_form`
* `.travel_form`
* `.tree_form`
* `.moonkin_form`
* `.aquatic_form`
* `.flight_form`

### 2. Seleccionar una Apariencia
Añade el número de índice correspondiente a la opción que deseas utilizar:

```text
.cat_form 1
.travel_form 3
```

> **Nota:** Se puede añadir la forma racial predeterminada sin transformaciones personalizadas en el archivo de configuración usando como ID el `0`.

### 3. Recargar Configuración en Caliente
Si editas el archivo `mod_druid_forms.conf` mientras el servidor está encendido, puedes aplicar los cambios inmediatamente ejecutando:

.reload config

---

## Apoyo

Si este módulo te ha sido de utilidad y deseas apoyar mi trabajo, puedes invitarme un café:  
☕ [buymeacoffee.com/foliaprintempo](https://buymeacoffee.com/foliaprintempo)