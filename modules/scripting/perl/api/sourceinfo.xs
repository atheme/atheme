MODULE = Atheme			PACKAGE = Atheme::Sourceinfo

void
success(Atheme_Sourceinfo self, char *message)
CODE:
	command_success_nodata(self, "%s", message);

void
fail(Atheme_Sourceinfo self, int faultcode, char *message)
CODE:
	command_fail(self, faultcode, "%s", message);
