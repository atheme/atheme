#!/usr/bin/env python
# Simple example of using Atheme's XMLRPC server with python

import xmlrpclib

atheme = xmlrpclib.ServerProxy('http://127.0.0.1:8080/xmlrpc')

print atheme.atheme.command('', '', '::1', 'ALIS', 'LIST', '*')
