#!/usr/bin/php
<?php
/* 
   READ THIS FIRST!!!!!!!!! DO NOT ASK QUESTIONS WITHOUT READING THIS BLURB!!!!!

   this is an anope 1.9.2 flatfile database conversion utility.
   upon usage you will recieve a shiny new atheme database.
   
   this script takes the anope.db file in which it is ran and will generate
   an atheme.db in opensex format.  your anope.db will not be altered.
   
   side effects:
   it will not save MLOCKS on channels
   it does not save user/channel FLAGS - accounts and channels are all 
   granted default atheme flags
   it only understands anope default access level numerics, if you have channels
   that use non-default, you need to adjust the PHP accordingly.
   
   this script is UNSUPPORTED by atheme.org and is provided for convience only
   ask all questions to stitch on IRC
   
*/

/* configuration */
$use_crypted_passwords = true; /* set to false for enc_none using morons */

/* myuser data */
$mu_account = "";
$mu_pass = "";
$mu_email = "";
$mu_register = 0;
$mu_lastlogin = 0;

$mu_flags = -1;
$mu_lang = "default";


/* mychan data */
$mc_channel = "";
$mc_founder = "";
$mc_flags = -1;
$mc_tsregister = 0;
$mc_tslastused = 0;

$hostnicks = array();
$queue = "";
$chanfounders = array();

$atheme = fopen("atheme.db.tmp", 'w');

function queue_line($str)
{
	global $queue;
	$queue = $queue . $str;
}

function commit_queue()
{
	global $queue;
	global $atheme;
	
	fwrite($atheme, $queue);

	$queue = "";
}

function mc_commitable()
{
	global $mc_channel, $mc_founder, $mc_flags, $mc_tsregister, $mc_tslastused;
	global $queue, $chanfounders, $hostnicks;
	if($mc_channel != "" && $mc_founder != "" && $mc_flags != 0 
		&& $mc_tsregister != 0 && $mc_tslastused != 0)
	{
		return true;
	}
	
	return false;
}

function mc_commit()
{
	global $mc_channel, $mc_founder, $mc_flags, $mc_tsregister, $mc_tslastused;
	global $queue, $chanfounders, $hostnicks;
	global $atheme;
	/* MC <name> <registered> <used> <flags> <mlock_on> <mlock_off> <mlock_limit> [mlock_key] */	
	printf("exported mychan for %s\n", $mc_channel);

	if($mc_channel != "")
	{
		/* were going to give default flags */
		fwrite($atheme, sprintf("MC %s %s %s 528 0 0 0\n",
			$mc_channel, $mc_tsregister, $mc_tslastused));
	}
	
	$mc_channel = "";
	$mc_founder = "";
	$mc_flags = 0;
	$mc_tsregister = 0;
	$mc_tslastused = 0;
	
	$chanfounders[$mc_channel] = $mc_founder;
}

function mu_commitable()
{
	global $mu_account, $mu_pass, $mu_email, $mu_register, $mu_lastlogin;
	global $mu_flags, $mu_lang, $use_crypted_passwords;

	if($mu_account != "" && $mu_pass != "" && $mu_email != "" && $mu_flags != -1)
		return true;

	return false;
}

function mu_commit()
{
	/* MU <name> <pass> <email> <registered> <lastlogin> 
	<failnum*> <lastfail*>  <lastfailon*> <flags> <language> */

	global $mu_account, $mu_pass, $mu_email, $mu_register, $mu_lastlogin;
	global $mu_flags, $mu_lang, $use_crypted_passwords;
	
	global $atheme;
	printf("exported myuser for %s\n", $mu_account);

	fwrite($atheme, sprintf("MU %s %s %s %s %s %s %s\n",
		$mu_account, $mu_pass, $mu_email,
		9999, time(), $mu_flags, $mu_lang));
	
	$mu_account = "";
	$mu_pass = "";
	$mu_email = "";
	$mu_register = 0;
	$mu_lastlogin = 0;

	$mu_flags = -1;
	$mu_lang = "default";

}

function me_commit()
{
	global $mu_account, $mu_pass, $mu_email, $mu_register, $mu_lastlogin;
	global $mu_flags, $mu_lang, $use_crypted_passwords;

	queue_line(sprintf("ME %s %s %s %s %s\n",
		$mu_account, $mu_pass, $mu_email, $mu_register, $mu_lastlogin,
		0, 0, 0));
}

function writeline($str)
{
	global $atheme;
	fwrite($atheme, $str);
}

function ts_convert()
{	
	printf("Converting some registration timestamps for MU's\n");

	$tdata = file_get_contents("atheme.db.tmp");
	$data = explode("\n", $tdata);
	$data2 = explode("\n", $tdata);
	unset($tdata);

	$atheme = fopen("atheme.db", 'w');

        for($i = 0; $i < count($data); $i++)
        {
                $line = $data[$i];
                $type = substr($line, 0, 2);
                $nudata = explode(" ", $line);
	
		if($type == "MN")
		{
			/* MN is all we care about */

			$account = $nudata[1];
			$ts = $nudata[3];
			for($j = 0; $j < count($data2); $j++)
			{
				$line2 = $data2[$j];
				$type2 = substr($line2, 0, 2);
				$nudata2 = explode(" ", $line2);

				if($type2 == "MU" && $nudata2[1] == $account)
				{
					printf("Changing TS for account %s\n", $account);
					$nudata2[4] = $ts;
					$data2[$j] = implode(" ", $nudata2);
					break; /* this should only occur once */
				}
			}

		}
	}

	for($k = 0; $k < count($data2); $k++)
	{
		fwrite($atheme, $data2[$k] . "\n");
	}

	fclose($atheme);
}

function atheme_convert()
{
	/* atheme MU flags */
	$mu_hold =       	0x00000001;
	$mu_neverop =    	0x00000002;
	$mu_noop =       	0x00000004;
	$mu_waitauth =   	0x00000008;
	$mu_hidemail =   	0x00000010;
	$mu_oldalias =   	0x00000020;
	$mu_nomemo =     	0x00000040;
	$mu_emailmemos =	0x00000080;
	$mu_cryptpass = 	0x00000100;
	$mu_old_sasl =   	0x00000200;
	$mu_noburstlogin =  0x00000400;
	$mu_enforce =   	0x00000800;
	$mu_usepriv =  		0x00001000;
	$mu_private =  		0x00002000;
	$mu_quietchg =  	0x00004000;

	/* numeric -> channel access flags */
	$a1 = "+A";
	$a3 = "+VA";
	$a4 = "+vHiA";
	$a5 = "+vhoOirtA";
	$a10 = "+vhoOairRftA";
	$a10000 = "+vhoOaqsirRftA";

	global $mu_account, $mu_pass, $mu_email, $mu_register, $mu_lastlogin;
	global $mu_flags, $mu_lang, $use_crypted_passwords;
	global $mc_channel, $mc_founder, $mc_flags, $mc_tsregister, $mc_tslastused;

	global $queue, $chanfounders, $hostnicks;

	global $atheme;
	
	printf("Opening old anope database\n");

	printf("Beginning conversion process\n");
	printf("atheme opensex compatable db - DBV 7\n");

	fwrite($atheme, "DBV 7\n");
	fwrite($atheme, "CF +vVoOtsriRfhHAb\n");

	$tdata = file_get_contents("anope.db");
	$data = explode("\n", $tdata);

	unset($tdata);

	/* are we in channels/nicks ?*/
	$lastid = "";
	$ismd = false;
	$is_mu = false;
	$is_cu = false;
	$is_na = false;
	$lastnick = "";
	
	print("Lines in DB: " . count($data) . "\n");
	for($i = 0; $i < count($data); $i++)
	{
		$line = $data[$i];
		$type = substr($line, 0, 2);
		$nudata = explode(" ", $line);

		/* track our last type, usually when we get outside the metadata we can commit */
		if($is_mu == true && $type != "MD")
		{
			mu_commit();
		}
		
		if($is_cu == true && $lastid != $type && $type != "MD")
		{
			mc_commit();
		}
		
		$lastid = $type;

		/* this is an ANOPE account */		
		if($type == 'NC')
		{
	     	commit_queue();
			$ismd = false;
			$is_mu = true;
			$is_cu = false;
			$is_na = false;

			$passvars = explode(":", $nudata[2]);
			$pass = $passvars[1];
			$mu_account = $nudata[1];
			 
			$mu_pass = '$rawsha1';
			
			/*$nc_pass = base64_decode($pass);
			for ($ii = 0; $ii <= 19; $ii++)
 				sprintf($mu_pass + 9 + 2 * ii, "%02x", 255 & (int)$nc_pass[$ii]));
 			*/
			$mu_pass = '$rawsha1$' . bin2hex(base64_decode($pass));
			$nu_lang = $nudata[2];
			
			$lastid = 'NC';
		}

		/* this is a grouped nickname */
		if($type == 'NA')
		{
			commit_queue();
			$ismd = false;
			$is_mu = false;
			$is_cu = false;
			$is_na = true;
			$tmplined = substr($line, 2);
			$tmplined = 'MN' . $tmplined;
			$lastnick = $nudata[1];
			printf("Writing MN %s \n", $tmplined);
			writeline($tmplined . "\n");		
			$lastid = 'NA';
		}
		
		/* this is an anope channel */
		if($type == 'CH')
		{
			if($nudata[1] != "")
			{
				commit_queue();
				$ismd = false;
				$is_mu = false;
				$is_cu = true;
				$is_na = false;
			
				$mc_channel = $nudata[1];
				$mc_tsregister = $nudata[2];
				/* anope doesn't store a last used date on channels
				   but it will get auto-updated anyway, set to anything but 0 */
				$mc_tslastused = 9001;
				$lastid = 'CH';			
			}
		}
		
		/* this is ANOPE meta data */
		if($type == 'MD')
		{
			$ismd = true;
			$lastid = 'MD';
			if($is_mu == true && $is_cu == false)
			{
				/* nick metadata */

				if($nudata[1] == "EMAIL")
					$mu_email = $nudata[2];

				if($nudata[1] == "NI")
				{
					/* check on STATUS --stitch */
					/* this is a memoserv message */
					/* ME <name> <sender> <sent> <status> <text> */
					$meline = explode(":", $line);
					queue_line(sprintf("ME %s %s %s %s %s\n",
					$mu_account, $nudata[4], $nudata[3], 
			     				0, $meline[1]));		
				}
				
				if($nudata[1] == "ACCESS")
				{
					queue_line(sprintf("AC %s %s\n", $mu_account, $nudata[2]));
				}
				
				if($nudata[1] == "FLAGS")
				{
					$mu_flags = 0;
					for($j = 2; $j < count($nudata) - 2; $j++)
					{
				    	$mu_flags |= $mu_cryptpass;
					    	
						if($nudata[$j] == "HIDE_EMAIL")
							$mu_flags |= $mu_hidemail;
							
						if($nudata[$j] == "PRIVATE")
							$mu_flags |= $mu_private;
							
						if($nudata[$j] == "SECURE")
							$mu_flags |= $mu_enforce;
							
					}
					
					$mu_flags = 272;
				}
			}

			if($is_na == true)
			{
				if($nudata[1] == "VHOST")
				{
					/* atheme has one vhost per account, anope has one vhost per nick,
						 fuck this */
					if($hostnicks[$lastnick] != "used")
					{
						writeline(sprintf("MDU %s private:usercloak %s\n",
							$lastnick, $nudata[4]));
						$hostnicks[$lastnick] = "used";
					}
				}

				if($nudata[1] == "LAST_USERMASK")
				{
					//writeline(sprintf("MDU %s private:host:actual %s\n", $lastnick, $nudata[2]));
					writeline(sprintf("MDU %s private:host:vhost %s\n", $lastnick, $nudata[2]));
				}
			}
			
			if($is_cu == true)
			{
				/* channel metadata */
				if($nudata[1] == "FOUNDER")
				{
					queue_line(sprintf("CA %s %s +vhoOaqsirRftAF %s\n", $mc_channel, $nudata[2], $mc_tsregister));
					$mc_founder = $nudata[2];
				}
									
				if($nudata[1] == "TOPIC")
				{
					$tdat = trim(strstr($line, ":"), ":");
					queue_line(sprintf("MDC %s private:topic:setter %s\n", $mc_channel, $nudata[2]));
					queue_line(sprintf("MDC %s private:topic:text %s\n", $mc_channel, $tdat));
					queue_line(sprintf("MDC %s private:topic:ts %s\n", $mc_channel, $nudata[3]));
				}
				

				if($nudata[1] == "ACCESS")
				{
					$ln = "";
					$nick = $nudata[2];
					$level = $nudata[3];
					$ts = $nudata[4];
					
					if($ts == "")
						$ts = 0;
						
					if($level < 0)
						$ln = sprintf("CA %s %s +b %s\n", $mc_channel, $nick, ($nudata[4] != "" ? $nudata[4] : 0));
					else
					{
						if($level == 1)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a1, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 3)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a3, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 4)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a4, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 5)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a5, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 10)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a10, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 9999)
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $a10000, ($nudata[4] != "" ? $nudata[4] : 0));
						if($level == 10000 || $level == 10001)
						{
							$ln = sprintf("CA %s %s %s %s\n", $mc_channel, $nick, $flag, ($nudata[4] != "" ? $nudata[4] : 0));
						}
					}
					
					queue_line($ln);
				}
			}
		}
	}
}

atheme_convert();

fclose($atheme);
ts_convert();
unlink('atheme.db.tmp');
printf("Conversion complete. Enjoy atheme :)\n");

?>

