<h1>XML-RPC Explorer</h1>
<?php
/* Copyright (c) 2005 Alex Lambert <alambert at quickfire dot org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * An XML-RPC testing script.
 *
 * $Id: xmlrpc_explorer.php 5207 2006-05-03 15:02:49Z jilles $
 */

/* This script requires XML-RPC for PHP 1.1
 * You can get it at http://phpxmlrpc.sf.net
 */
include('xmlrpc.inc');

/* Set this appropriately. This needs to match
 * the information in your xmlrpc{} block.
 *
 * For Atheme, you can leave PATH set to '/'.
 */
define('HOST', 'some.xmlrpc.server');
define('PORT', 8080);
define('PATH', '/');

/* Here, you can set up the methods and their arguments.
 * 
 * keys: method names;
 * value: array of strings (friendly names of parameters, in order)
 *
 * This is a simple testing script: we only support strings.
 */
$methods = array(
	'test.method' => array('string'),
	'atheme.login' => array('account', 'password'),
	'atheme.logout' => array('authcookie', 'account'),
	'atheme.account.register' => array('account', 'password', 'email'),
	'atheme.account.verify' => array('authcookie', 'account', 'operation', 'key'),
	'atheme.account.set_password' => array('authcookie', 'account', 'password'),
	'atheme.account.metadata.set' => array('authcookie', 'account', 'key', 'value'),
	'atheme.account.metadata.get' => array('account', 'key'),
	'atheme.account.metadata.delete' => array('authcookie', 'account', 'key', 'value'),
	'atheme.account.vhost' => array('authcookie', 'account', 'vhost'),
	'atheme.channel.register' => array('authcookie', 'account', 'channel'),
	'atheme.channel.metadata.set' => array('authcookie', 'account', 'channel', 'key', 'value'),
	'atheme.channel.metadata.get' => array('channel', 'key'),
	'atheme.channel.metadata.delete' => array('authcookie', 'account', 'channel', 'key', 'value'),
	'atheme.channel.topic.set' => array('authcookie', 'account', 'channel', 'topic'),
	'atheme.channel.topic.append' => array('authcookie', 'account', 'channel', 'topic'),
	'atheme.channel.access.get' => array('authcookie', 'account', 'channel'),
);

switch ($_REQUEST['state'])
{
	case 1:
		$method = $_REQUEST['method'];
		if (array_key_exists($method, $methods))
		{
			printf('<h2>Enter parameters for %s</h2>', htmlentities($method));
			printf('<form method="get">');
			printf('<input type="hidden" name="state" value="2">');
			printf('<input type="hidden" name="method" value="%s">', htmlentities($method));
			foreach ($methods[$method] as $name)
				printf('%s: <input type="text" name="in_%s"><p>', htmlentities($name), htmlentities($name));
			printf('<input type="submit">');
			printf('</form>');
		} else {
			printf('Unknown method: %s', htmlentities($method));
		}
		break;
	case 2:
		printf('<h2>Results</h2>');
		$method = $_REQUEST['method'];
		if (array_key_exists($method, $methods))
		{
			$message = new xmlrpcmsg($method);
			foreach ($methods[$method] as $name)
				$message->addParam(new xmlrpcval($_REQUEST['in_' . $name], $xmlrpcString));
			$client = new xmlrpc_client(PATH, HOST, PORT);
			$response = $client->send($message);
			if (!$response->faultCode())
			{
				printf('<h3>Success</h3>');
				printf('<pre>%s</pre>', htmlentities($response->serialize()));
			} else {
				printf('<h3>Fault</h3>');
				printf('Fault code: %d<br>', htmlentities($response->faultCode()));
				printf('Fault string: %s', htmlentities($response->faultString()));
			}
		} else {
			printf('Unknown method: %s', htmlentities($method));
		}
		break;
	default:
		printf('<h2>Choose a method</h2>');
		printf('<ul>');
		foreach ($methods as $name => $service)
			printf('<li><a href="?state=1&amp;method=%s">%s</a></li>', $name, htmlentities($name));
		printf('</ul>');
		break;
}

?>
<hr>
$Id: xmlrpc_explorer.php 5207 2006-05-03 15:02:49Z jilles $
