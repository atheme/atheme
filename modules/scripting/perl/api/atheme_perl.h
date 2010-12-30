#ifndef ATHEME_PERL_H
#define ATHEME_PERL_H

struct perl_command_ {
	command_t command;
	SV * handler;
	SV * help_func;
};

typedef struct perl_command_ perl_command_t;

void perl_command_handler(sourceinfo_t *si, const int parc, char **parv);
void perl_command_help_func(sourceinfo_t *si, const char *subcmd);

#endif
