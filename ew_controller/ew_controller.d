#
# Archivo de configuracion para ew_controller (Panel de control grafico de startstop)
#
# Este modulo NO consume anillos de datos: usa un anillo dedicado de control
# (CONTROL_RING) por donde intercambia mensajes TYPE_REQSTATUS / TYPE_STATUS /
# TYPE_STOP / TYPE_RESTART / TYPE_RECONFIG con el manager de Earthworm (startstop).

MyModuleId      MOD_CONTROL       # ID del modulo (debe estar registrado en earthworm.d)
Ring            CONTROL_RING      # Anillo de control dedicado (key 1020, size 512 kb)
HeartBeatInt    30                # Intervalo de latidos (segundos)
PollInt         5                 # Intervalo de peticion de estado a startstop (segundos)
LogFile         1                 # 1 = Escribir log a disco, 0 = Solo consola

# LogDir (OPCIONAL): directorio donde estan los archivos de log que muestra el
# visor. Se resuelve con prioridad: variable de ambiente EW_LOG > este valor > "logs".
# Normalmente no hace falta definirlo: si el sistema se arranca con ew_unix.sh,
# EW_LOG ya apunta al directorio correcto (p. ej. logs/ de la instalacion).
#LogDir         "logs"