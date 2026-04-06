### Nuevos requisitos 

fecha: 06 de abril de 2026

### Servicio de API REST

- [ ]  El binario debe soportar TLS/SSL para garantizar comunicación segura con APIs de plataformas modernas.

---

### Persistencia de Datos

- [ ]  Los datos enviados y los datos pendientes por enviar deben persistirse en directorios separados y definidos (backup en formato JSON).
- [ ]  El límite de retención de datos persistidos (ya sea por tiempo o por cantidad de registros) debe alinearse con la implementación actual del proyecto, documentando claramente el criterio utilizado y esto sea definido en el archivo de configuracion.
- [ ]  El archivo de configuración debe exponer los paths de los directorios de persistencia, tanto para datos enviados como para datos pendientes.

---

### Interfaz de Servicios Web

- [ ]  El binario debe ser capaz de enviar datos tanto por **MQTT** como por **API REST**.
- [ ]  El archivo de configuración debe definir cuál interfaz de servicios web actúa como **principal** y cuál como **respaldo** (*failover*).
- [ ]  La integración de MQTT debe coexistir con la implementación actual de API REST, sin reemplazarla ni romper su funcionamiento.