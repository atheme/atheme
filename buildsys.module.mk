# Additional extensions for building single-file modules.
.SUFFIXES: $(PLUGIN_SUFFIX)

plugindir = ${MODDIR}/modules/$(MODULE)
PLUGIN=${SRCS:.c=$(PLUGIN_SUFFIX)}

all: $(PLUGIN)
install: $(PLUGIN)

phase_cmd_cc_module = CompileModule
quiet_cmd_cc_module = $@
      cmd_cc_module = ${CC} ${DEPFLAGS} ${CFLAGS} ${PLUGIN_CFLAGS} ${CPPFLAGS} ${PLUGIN_LDFLAGS} ${LDFLAGS} -o $@ $< ${LIBS}

.c$(PLUGIN_SUFFIX):
	$(call echo-cmd,cmd_cc_module)
	$(cmd_cc_module)
