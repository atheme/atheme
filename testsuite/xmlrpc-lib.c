/* XML RPC Library - sample code
 *
 * (C) 2005 Trystan Scott Lee
 * Contact trystan@nomadirc.net
 *
 * Please read COPYING and README for furhter details.
 *
 * Based on the original code from Denora
 * 
 *
 */

#include "xmlrpc.h"

#define TESTBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.getStateName</methodName><params>     <param> <value><i4>41</i4></value> </param> </params> </methodCall>"
#define TIMEBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.gettime</methodName><params>     <param> <value><string>now</string></value> </param> </params> </methodCall>"
#define INTBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.getStateId</methodName><params>     <param> <value><string>South Dakota</string></value> </param> </params> </methodCall>"
#define DOUBLEBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.double</methodName><params>     <param> <value><int>7</int></value> </param> </params> </methodCall>"
#define B64BUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.b64encode</methodName><params>     <param> <value><string>This is a test</string></value> </param> </params> </methodCall>"
#define BOOLEANTBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.boolean</methodName><params>     <param> <value><i4>1</i4></value> </param> </params> </methodCall>"
#define BOOLEANFBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.boolean</methodName><params>     <param> <value><i4>0</i4></value> </param> </params> </methodCall>"
#define DB64BUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.db64encode</methodName><params>     <param> <value><string>VGhpcyBpcyBhIHRlc3Q=</string></value> </param> </params> </methodCall>"
#define ARRAYBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.simplearray</methodName><params>     <param> <value><string>test string</string></value> </param> </params> </methodCall>"
#define ERRORBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.genericerror</methodName><params>     <param> <value><string>showing generic error handler</string></value> </param> </params> </methodCall>"
#define ABOUTBUFFER "<?xml version=\"1.0\"?> <methodCall><methodName>examples.getversion</methodName><params>     <param> <value><string>showing how to get the version</string></value> </param> </params> </methodCall>"

char *getbuffer(char *buffer, int lenght);
int getstatename(int ac, char **av);
int gettime(int ac, char **av);
int getStateId(int ac, char **av);
int getdouble(int ac, char **av);
int getb64encode(int ac, char **av);
int getboolean(int ac, char **av);
int getdb64encode(int ac, char **av);
int getsimplearray(int ac, char **av);
int getgenericerror(int ac, char **av);

int main(int ac, char **av) 
{
  /* Set the output function */
  xmlrpc_set_buffer(getbuffer);

  /* Register the Methods */
  xmlrpc_register_method("examples.getStateName", getstatename);
  xmlrpc_register_method("examples.gettime", gettime);  
  xmlrpc_register_method("examples.getStateId", getStateId);
  xmlrpc_register_method("examples.double", getdouble);
  xmlrpc_register_method("examples.b64encode", getb64encode);
  xmlrpc_register_method("examples.db64encode", getdb64encode);
  xmlrpc_register_method("examples.boolean", getboolean);
  xmlrpc_register_method("examples.simplearray", getsimplearray);
  xmlrpc_register_method("examples.genericerror", getgenericerror);
  xmlrpc_register_method("examples.getversion", xmlrpc_about);

  printf("Incoming Buffer\r\n");
  printf(TESTBUFFER);
  printf("\n\r");
  /* handle the buffer */
  xmlrpc_process((char *) TESTBUFFER);
  printf("End Example 1\n\r\n\r\n\r");

  printf("Second Buffer\r\n");
  printf(TIMEBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) TIMEBUFFER);
  printf("End Example 2\n\r\n\r\n\r");

  printf("Thrid Buffer\r\n");
  printf(INTBUFFER);
  printf("\n\r");
  
  /* Using set options - this are one time options */
  /* so if you want to use them again you need to reset */
  xmlrpc_set_options(XMLRPC_HTTP_HEADER, XMLRPC_ON);
  xmlrpc_set_options(XMLRPC_ENCODE, "windows-1252");
  xmlrpc_set_options(XMLRPC_INTTAG, XMLRPC_I4);
  xmlrpc_process((char *) INTBUFFER);
  printf("End Example 3\n\r\n\r\n\r");

  printf("Fourth Buffer\r\n");
  printf(INTBUFFER);
  printf("\n\r");
  xmlrpc_set_options(XMLRPC_INTTAG, XMLRPC_INT);
  xmlrpc_process((char *) INTBUFFER);
  printf("End Example 4\n\r\n\r\n\r");

  printf("Fifth Buffer\r\n");
  printf(DOUBLEBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) DOUBLEBUFFER);
  printf("End Example 5\n\r\n\r\n\r");

  printf("Sixth Buffer\r\n");
  printf(B64BUFFER);
  printf("\n\r");
  xmlrpc_process((char *) B64BUFFER);
  printf("End Example 6\n\r\n\r\n\r");

  printf("Seventh Buffer\r\n");
  printf(BOOLEANTBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) BOOLEANTBUFFER);
  printf("End Example 7\n\r\n\r\n\r");

  printf("Eighth Buffer\r\n");
  printf(BOOLEANFBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) BOOLEANFBUFFER);
  printf("End Example 8\n\r\n\r\n\r");

  printf("ninth Buffer\r\n");
  printf(DB64BUFFER);
  printf("\n\r");
  xmlrpc_process((char *) DB64BUFFER);
  printf("End Example 9\n\r\n\r\n\r");

  printf("tenth Buffer\r\n");
  printf(ARRAYBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) ARRAYBUFFER);
  printf("End Example 10\n\r\n\r\n\r");

  printf("eventh Buffer\r\n");
  printf(ERRORBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) ERRORBUFFER);
  printf("End Example 11\n\r\n\r\n\r");

  printf("twevlth Buffer\r\n");
  printf(ABOUTBUFFER);
  printf("\n\r");
  xmlrpc_process((char *) ABOUTBUFFER);
  printf("End Example 12\n\r\n\r\n\r");

  /* unregister the method */
  xmlrpc_unregister_method("examples.gettime");
  xmlrpc_unregister_method("examples.getStateName");
  xmlrpc_unregister_method("examples.getStateId");
  xmlrpc_unregister_method("examples.double");
  xmlrpc_unregister_method("examples.b64encode");
  xmlrpc_unregister_method("examples.db64encode");
  xmlrpc_unregister_method("examples.boolean");
  xmlrpc_unregister_method("examples.simplearray");
  xmlrpc_unregister_method("examples.genericerror");
  xmlrpc_unregister_method("examples.getversion");
  return 1;
}

int getb64encode(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];

  xmlrpc_base64(buf, av[0]);
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

int getgenericerror(int ac, char **av)
{
  xmlrpc_generic_error(42, "Answer to all questions");
  return XMLRPC_CONT;
}

int getsimplearray(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];
  char buf2[XMLRPC_BUFSIZE];
  char *arraydata;

  xmlrpc_base64(buf, av[0]);
  xmlrpc_integer(buf2, strlen(av[0]));
  arraydata = xmlrpc_array(2, buf, buf2);
  
  xmlrpc_send(1, arraydata);
  free(arraydata);
  return XMLRPC_CONT;
}

int getdb64encode(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];
  char *b64;

  b64 = xmlrpc_decode64(av[0]);
  xmlrpc_string(buf, b64);
  xmlrpc_send(1, buf);
  free(b64);
  return XMLRPC_CONT;
}

int getboolean(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];

  xmlrpc_boolean(buf, atoi(av[0]));
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

int getstatename(int ac, char **av)
{
  int state_id;
  char buf[XMLRPC_BUFSIZE];

  state_id = atoi(av[0]);
  if (state_id == 41) {
    xmlrpc_string(buf, (char *) "South Dakota");
  }
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

int gettime(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];

  xmlrpc_time2date(buf, time(NULL));
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

int getStateId(int ac, char **av)
{
  char buf[XMLRPC_BUFSIZE];

  if (!stricmp(av[0], "South Dakota")) {
    xmlrpc_integer(buf, 41);
  }
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

int getdouble(int ac, char **av)
{
  double value;
  char buf[XMLRPC_BUFSIZE];

  value = 13.324;

  xmlrpc_double(buf, value);
  xmlrpc_send(1, buf);
  return XMLRPC_CONT;
}

char *getbuffer(char *buffer, int lenght)
{
  printf("%s\n\r", buffer);
  return buffer;
}

