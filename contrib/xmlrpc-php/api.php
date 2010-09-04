<?php

// Usage: 	Post values of api.php?username=&password=&service=&command=&params=
// Var:		username	Description: Your NickServ username
// Var:		password	Description: Your NickServ password
// Var:		service		Description: The name of the service you wish to communicate with on IRC (ChanServ, NickServ, etc)
// Var:		command		Description: The command to use, such as "identify", or "topic"
// Var:		params		Description: Parameters, just like the way you'd pass them in IRC.

// Example usage: api.php?username=Tony&password=Blair&service=ChanServ&command=topicappend&params=#somechannel some topic information

// By Ricky Burgin (Pseudonym: Orbixx) of Exoware

require_once('./atheme.php');

$params = explode(" ", urldecode($_POST['params']));

// echo print_r(explode(" ", urldecode($_POST['params'])));
if (isset($_POST['params']))
{
	echo atheme("127.0.0.1", 8080, "/xmlrpc", $_SERVER['REMOTE_ADDR'], $_POST['username'], $_POST['password'], $_POST['service'], $_POST['command'], $params);
}
else
{
	echo atheme("127.0.0.1", 8080, "/xmlrpc", $_SERVER['REMOTE_ADDR'], $_POST['username'], $_POST['password'], $_POST['service'], $_POST['command'], NULL);
}

?>
