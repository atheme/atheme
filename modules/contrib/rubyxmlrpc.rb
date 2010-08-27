#!/usr/bin/env ruby
# Simple example of using Atheme's XMLRPC server with ruby
# $Id: rubyxmlrpc.rb 8403 2007-06-03 21:34:06Z jilles $

require "xmlrpc/client"

login = "someaccount"
password = "somepassword"
sourceip = "::1"

server = XMLRPC::Client.new("localhost", "/xmlrpc", 8080)
result = server.call("atheme.login", login, password, sourceip)
authcookie = result
puts "authcookie is #{authcookie}\n"

begin
	result = server.call("atheme.command", authcookie, login, sourceip, 'ChanServ', 'FLAGS', '#irc')
	puts result
rescue XMLRPC::FaultException => e
	puts "Error: (" + e.faultCode.to_s + ") " + e.faultString
end
server.call2("atheme.logout", authcookie, login)
