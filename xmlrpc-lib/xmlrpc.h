#include "atheme.h"

#define XMLRPC_STOP 1
#define XMLRPC_CONT 0

#define XMLRPC_ERR_OK			0
#define XMLRPC_ERR_MEMORY		1
#define XMLRPC_ERR_PARAMS		2
#define XMLRPC_ERR_EXISTS		3
#define XMLRPC_ERR_NOEXIST		4
#define XMLRPC_ERR_NOUSER		5
#define XMLRPC_ERR_NOLOAD      6
#define XMLRPC_ERR_NOUNLOAD    7
#define XMLRPC_ERR_SYNTAX		8
#define XMLRPC_ERR_NODELETE	9
#define XMLRPC_ERR_UNKNOWN		10
#define XMLRPC_ERR_FILE_IO     11
#define XMLRPC_ERR_NOSERVICE   12
#define XMLRPC_ERR_NO_MOD_NAME 13

/*
 * Header that defines much of the xml/xmlrpc library
 */

#define XMLRPC_BUFSIZE         1024
#define XMLLIB_VERSION		 "1.0.0"
#define XMLLIB_AUTHOR		 "Trystan Scott Lee <trystan@nomadirc.net>"

#define XMLRPCCMD XMLRPCCMD_cmdTable
#define CMD_HASH(x)      (((x)[0]&31)<<5 | ((x)[1]&31))	/* Will gen a hash from a string :) */
#define MAX_CMD_HASH 1024

#define XMLRPC_ON "on"
#define XMLRPC_OFF "off"

#define XMLRPC_I4 "i4"
#define XMLRPC_INT "integer"

#define XMLRPC_HTTP_HEADER 1
#define XMLRPC_ENCODE 2
#define XMLRPC_INTTAG 3

typedef struct XMLRPCCmd_ XMLRPCCmd;
typedef struct XMLRPCCmdHash_ XMLRPCCmdHash;

extern XMLRPCCmdHash *XMLRPCCMD[MAX_CMD_HASH];

struct XMLRPCCmd_ {
    int (*func)(int ac, char **av);
	char *name;
    int core;
    char *mod_name;
    XMLRPCCmd *next;
};

struct XMLRPCCmdHash_ {
        char *name;
        XMLRPCCmd *xml;
        XMLRPCCmdHash *next;
};

typedef struct xmlrpc_settings {
	char *(*setbuffer)(char *buffer, int len);
	char *encode;
    int httpheader;
    char *inttagstart;
	char *inttagend;
} XMLRPCSet;

int xmlrpc_getlast_error(void);
void xmlrpc_process(char *buffer);
int xmlrpc_register_method(const char *name, int (*func) (int ac, char **av));
int xmlrpc_unregister_method(const char *method);

char *xmlrpc_array(int argc, ...);
char *xmlrpc_double(char *buf, double value);
char *xmlrpc_base64(char *buf, char *value);
char *xmlrpc_boolean(char *buf, int value);
char *xmlrpc_string(char *buf, char *value);
char *xmlrpc_integer(char *buf, int value);
char *xmlrpc_decode64(char *value);
char *xmlrpc_time2date(char *buf, time_t t);

int xmlrpc_set_options(int type, const char *value);
void xmlrpc_set_buffer(char *(*func)(char *buffer, int len));
void xmlrpc_generic_error(int code, const char *string);
void xmlrpc_send(int argc, ...);
int xmlrpc_about(int ac, char **av);
char *xmlrpc_char_encode(char *outbuffer, char *s1);
