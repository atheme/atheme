MODULE = Atheme			PACKAGE = Atheme::Log

void
debug(SV * package, const char * message)
CODE:
	slog(LG_DEBUG, "%s", message);

void
verbose(SV * package, const char * message)
CODE:
	slog(LG_DEBUG, "%s", message);

void
info(SV * package, const char * message)
CODE:
	slog(LG_DEBUG, "%s", message);

void
error(SV * package, const char * message)
CODE:
	slog(LG_DEBUG, "%s", message);

void
command(SV * package, Atheme_Sourceinfo si, int level, const char * message)
CODE:
	logcommand(si, level, "%s", message);

