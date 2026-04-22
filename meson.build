project('qw3100-modbus', 'c',
  version : '1.0.0',
  default_options : ['c_std=c11', 'warning_level=2'])

# ---------------------------------------------------------------------------
# Detección de target
#   ARM cross-compile : meson setup builddir --cross-file cross/armv7.ini
#   devlinux nativo   : meson setup builddir
# ---------------------------------------------------------------------------
is_arm_cross = meson.is_cross_build()

tp_arm = meson.project_source_root() / 'third_party' / 'arm'

# ---------------------------------------------------------------------------
# Dependencias
# ---------------------------------------------------------------------------
cc = meson.get_compiler('c')

if is_arm_cross
  # Libs precompiladas en third_party/arm/ — linking estático total
  dep_modbus = cc.find_library('modbus',
    dirs : tp_arm / 'modbus' / 'lib',
    static : true)

  dep_curl = cc.find_library('curl',
    dirs : tp_arm / 'curl' / 'lib',
    static : true)

  dep_mosquitto = cc.find_library('mosquitto',
    dirs : tp_arm / 'mosquitto' / 'lib',
    static : true)

  dep_ssl = cc.find_library('ssl',
    dirs : tp_arm / 'openssl' / 'lib',
    static : true)

  dep_crypto = cc.find_library('crypto',
    dirs : tp_arm / 'openssl' / 'lib',
    static : true)

  dep_cares = cc.find_library('cares',
    dirs : tp_arm / 'cares' / 'lib',
    static : true)

  inc_modbus    = include_directories('third_party/arm/modbus/include/modbus')
  inc_curl      = include_directories('third_party/arm/curl/include')
  inc_mosquitto = include_directories('third_party/arm/mosquitto/include')

  all_deps = [dep_modbus, dep_curl, dep_cares, dep_mosquitto, dep_ssl, dep_crypto]
  all_incs = [inc_modbus, inc_curl, inc_mosquitto]

else
  # devlinux: paquetes del sistema (apt install libmodbus-dev libcurl4-openssl-dev libmosquitto-dev)
  dep_modbus    = dependency('libmodbus')
  dep_curl      = dependency('libcurl')
  dep_mosquitto = dependency('libmosquitto', required : false)
  if not dep_mosquitto.found()
    dep_mosquitto = cc.find_library('mosquitto')
  endif
  dep_ssl    = dependency('openssl')

  all_deps = [dep_modbus, dep_curl, dep_mosquitto, dep_ssl]
  all_incs = []

endif

# ---------------------------------------------------------------------------
# Fuentes comunes
# ---------------------------------------------------------------------------
srcs_common = files(
  'src/sensor.c',
  'src/modbus_comm.c',
  'src/config.c',
  'src/persist.c',
  'src/http_sender.c',
  'src/mqtt_sender.c',
  'src/circuit_breaker.c',
  'lib/cJSON.c',
)

# ---------------------------------------------------------------------------
# Binario principal (solo ARM)
# ---------------------------------------------------------------------------
if is_arm_cross
  executable('sensor_trident_modbus_ARM',
    sources : ['src/main.c'] + srcs_common,
    include_directories : all_incs,
    dependencies : all_deps,
    link_args : ['-static'],
    install : false)
endif

# ---------------------------------------------------------------------------
# Tests (devlinux)
# ---------------------------------------------------------------------------
if not is_arm_cross
  srcs_test = files('test/main_test.c') + srcs_common

  test_exe = executable('main_test',
    sources : srcs_test,
    include_directories : all_incs,
    dependencies : all_deps,
    c_args : ['-g', '-O0'])

  test('unit tests', test_exe, timeout : 30)
endif
