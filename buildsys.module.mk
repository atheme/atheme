# Additional extensions for building single-file modules.
.SUFFIXES: $(PLUGIN_SUFFIX)

plugindir = ${MODDIR}/modules/$(MODULE)
PLUGIN=${SRCS:.c=$(PLUGIN_SUFFIX)}

all: $(PLUGIN)
install: $(PLUGIN)

phase_cmd_cc_plugin = CompileModule

.c$(PLUGIN_SUFFIX):
	$(call echo-cmd,cmd_cc_plugin)
	$(cmd_cc_plugin)
