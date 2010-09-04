<?php

require_once('./xmlrpc.inc');

function atheme($hostname, $port, $path, $sourceip, $username, $password, $service, $command, $params)
{
	$message = new xmlrpcmsg("atheme.login");
	$message->addParam(new xmlrpcval($username, "string"));
	$message->addParam(new xmlrpcval($password, "string"));
	$client = new xmlrpc_client($path, $hostname, $port);
	$response = $client->send($message);

	$session = NULL;
	if (!$response->faultCode())
	{
		$session = explode("<string>", $response->serialize());
		$session = explode("</string", $session[1]);
		$session = $session[0];
	}
	else
	{
		return "Authorisation failed";
	}

	$message = new xmlrpcmsg("atheme.command");
	$message->addParam(new xmlrpcval($session, "string"));
	$message->addParam(new xmlrpcval($username, "string"));
	$message->addParam(new xmlrpcval($sourceip, "string"));
	$message->addParam(new xmlrpcval($service, "string"));
	$message->addParam(new xmlrpcval($command, "string"));
	if ($params != NULL)
	{
		if (sizeof($params) < 2)
		{
			foreach($params as $param)
			{
				$message->addParam(new xmlrpcval($param, "string"));
			}
		}
		else
		{
			$firstparam = $params[0];
			$secondparam = "";
			for ($i = 1; $i < sizeof($params); $i++)
			{
				$secondparam .= $params[$i] . " ";
			}
			$message->addParam(new xmlrpcval($firstparam, "string"));
			$message->addParam(new xmlrpcval($secondparam, "string"));
		}
		$response = $client->send($message);
	}

	if (!$response->faultCode())
	{
		return $response->serialize();
	}
	else
	{
		return "Command failed: " . $response->faultString();
	}

}

?>
