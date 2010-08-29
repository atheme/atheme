<?php

/* This script is in the public domamin.  Please give it out freely to help the spread of atheme, and assist others in your position. */
/*
convert.php is a work in progress, but it has successfully converted production
databases.  It is to be considered a release candidate.  It reads an ircservices
XML database export from STDIN and prints a atheme database to STDOUT.

NOTE: The use of a custom password encryption module (included) is required!
      It (ircs_crypto_trans.c) replaces the current ircservices.c in modules/crypto/.

The password encryption module includes code to allow a transition to the
encryption module of your choice.  Uncommenting the following line will
cause the module to execute the listed program every time a password is hashed.
The program recieves two lines on stdin, newline separated.  They are
the unencrypted password followed by the currently recorded password.  Your
program may encrypt the unencrypted password however it chooses and store it
with the pre-encrypted version.  After all nicks have been identified to
(after the expiry time for nicks), use this map to replace the passwords stored
in the database.  The password location in the database is shown below.

NOTE: There is no harm in leaving some few ircservices passwords in the
      database after switching to POSIX (others not tested), if you so choose.
      Those nicks will simply not be able to log in, and need their password
      reset, such as by the use of SENDPASS.

Program Activation Line:
// #define PW_TRANSITION_LOG "./pwtransition.sh"

Password Location:
MU nick CRYPTED_PW ...

NOTE: if channels for some reason cannot be converted, errors.log will be populated with
information about what channels & why.

Bugs & Patches to Jason@WinSE.ath.cx
*/

$nicks = array();
$chans = array();


$raw_lines = array();
while (!feof(STDIN))
	$raw_lines[] = html_entity_decode(fgets(STDIN));


/* Read in and parse IRCservices
 */
$category = '';
foreach($raw_lines as $raw_line)
{
	$line = trim($raw_line);

	if ($line == '<nickgroupinfo>')
		$category = 'nickgroupinfo';
	if ($line == '<nickinfo>')
		$category = 'nickinfo';
	if ($line == '<channelinfo>')
		$category = 'channelinfo';
	if ($line == '</'. $category .'>')
		$category = '';

	switch ($category)
	{
		case 'nickgroupinfo': read_nickgroupinfo($line); break;
		case 'nickinfo': read_nickinfo($line); break;
		case 'channelinfo': read_channelinfo($line); break;
	}
}

//print_r($nicks);
//print_r($chans);

/* Print out atheme
 */
$MUout = 0;
$MCout = 0;
$CAout = 0;

printf("DBV 6\n");
printf("CF +vVoOtsriRfhHAb\n");

// Nicks
foreach ($nicks as $id => $data)
{
	$first_reg = 0;
	$last_login = 0;
	$nick_vhost = '';
	$nick_rhost = '';
	$nick_noexpire = false;
	foreach ($data['nicks'] as $nick)
	{
		$first_reg = ($first_reg == 0 ? $nick['time_registered'] : min($first_reg,$nick['time_registered']));
		if ($last_login < $nick['last_seen'])
		{
			$last_login = $nick['last_seen'];
			$nick_rhost = $nick['realhost'];
			$nick_vhost = $nick['fakehost'];
		}
		$nick_noexpire = $nick_noexpire || $nick['noexpire'];
	}
	$password = substr(bin2hex(de_ircs($data['pass'],false)),0,16);
	if (!$password)
		$password = "nopass";
	printf("MU %s %s %s %u %u 0 0 0 %u\n",
		$data['mainnick'],
		$password,						// My god, what were they smoking?!
									// Atheme's IRCservices compatibility module had to be edited.
									// IRCservices database dump gives custom-encoded md5()+remainder_of_buffer
		$data['email'] ? $data['email'] : 'nomail',
		$first_reg,
		$last_login,
		(1280 | ( $nick_noexpire ? 0x1 : 0 ) | ( $data['hide-email'] ? 0x10 : 0 )) & ~0x100
	      );
	$MUout++;

	printf("MD U %s private:host:vhost %s\n",$data['mainnick'],$nick_vhost);
	printf("MD U %s private:host:actual %s\n",$data['mainnick'],$nick_rhost);
	if ($data['enforce'])
		printf("MD U %s private:doenforce 1\n",$data['mainnick']);

	foreach ($data['nicks'] as $nick)
		printf("MN %s %s %u %u\n",
			$data['mainnick'],
			$nick['nick'],
			$nick['time_registered'],
			$nick['last_seen']
		      );

	if (isset($data['access_list']))
		foreach ($data['access_list'] as $entry)
			printf("AC %s %s\n", $data['mainnick'], $entry);

	if (isset($data['memos']))
		foreach ($data['memos'] as $memo)
			printf("ME %s %s %u %u %s\n",
				$data['mainnick'],
				$memo['from'],
				$memo['time'],
				$memo['read'],
				$memo['text']
			      );

	$sAjoin = "";
	if (isset($data['ajoin']))
		foreach ($data['ajoin'] as $sChan)
			$sAjoin .= $sChan . ",";

	if (!empty($sAjoin))
	{
		printf("MD U %s private:autojoin %s\n", $data['mainnick'], $sAjoin);
	}
}

// Chans
foreach ($chans as $name => $data)
{
	if (empty($name) || empty($nicks[$data['founder']]['mainnick']) ||
			empty($data['time_registered']) || empty($data['time_used']) || 
			empty($data['mlock_on']) || empty($data['mlock_off']) ||
			empty($data['mlock_limit']))
	{
		$sErrors[] = "*** WARNING: potentially skipping entry for $name";

		$bFatal = false;

		if (empty($name))
		{
			$sErrors[] = "Name is empty."; // should never happen
			$bFatal = true;
		}
		if (empty($nicks[$data['founder']]['mainnick']))
		{
			$sErrors[] = "Main nick is empty"; // forbidden channels can get this
			$bFatal = true;
		}
		if (empty($data['time_registered']))
		{
			$sErrors[] = "Time registered is empty";
			$bFatal = true;
		}
		if (empty($data['time_used']))
		{
			$sErrors[] = "Time used is empty";
			$bFatal = true;
		}
		if (empty($data['mlock_on']))
		{
			$sErrors[] = "MLOCK on is empty";
			$data['mlock_on'] = 0;
		}
		if (empty($data['mlock_off']))
		{
			$sErrors[] = "MLOCK off is empty";
			$data['mlock_off'] = 0;
		}
		if (empty($data['mlock_limit']))
		{
			$sErrors[] = "MLOCK limit is empty";
			$data['mlock_limit'] = 0;
		}

		if ($bFatal)
		{
			$sErrors[] = "Cannot write this channel, ignoring";
			continue;
		}
	}

	printf("MC %s 0 %s %u %u 64 %u %u %u\n",
		$name,
		$nicks[$data['founder']]['mainnick'],
		$data['time_registered'],
		$data['time_used'],
		$data['mlock_on'],
		$data['mlock_off'],
		$data['mlock_limit']
	      );
	$MCout++;

	if (isset($data['topic']))
	{
		printf("MD C %s private:topic:setter %s\n", $name, ($data['topic_by'] == '<unknown>' ? 'IRC' : $data['topic_by']) );
		printf("MD C %s private:topic:text %s\n", $name, $data['topic']);
		printf("MD C %s private:topic:ts %u\n", $name, $data['topic_time']);
	}

	foreach ($data['access'] as $id => $flags)
	{
		if ($id{0} == '+')
			$id = $nicks[substr($id,1)]['mainnick'];
		else
			$id = substr($id,1);

		printf("CA %s %s %s %u\n",
			$name,
			$id,
			$flags,
			0 // Timestamp.  Hopefully, 0 is good enough.  Not sure how exactly atheme's compatibility code works.
		      );
		$CAout++;
	}

	if (isset($data['akicks']))
		foreach ($data['akicks'] as $akick)
		{
			printf("CA %s %s +b %u\n",
				$name,
				$akick['mask'],
				$akick['set']
			      );
			$CAout++;

			printf("MD A %s:%s reason %s\n",
				$name,
				$akick['mask'],
				$akick['reason']
			      );
		}
}

printf("DE %d %d %d 0", $MUout, $MCout, $CAout );



file_put_contents("errors.log", implode("\n", $sErrors));




/* Helper functions
 */
function read_nickgroupinfo($line)
{
	global $nicks;
	static $id = 0;
	static $section = '';
	static $memo_num = 0;

	if ($line == '<nickgroupinfo>')
		$id = 0;

	if ($line == '<memo>')
		$section = 'memo';
	if (substr($line,0,7) == '<nicks ')
		$section = 'nicks';
	if (substr($line,0,8) == '<access ')
		$section = 'access';
	if (substr($line, 0, 7) == '<ajoin ')
		$section = 'ajoin';
	if ($line == '</'. $section .'>')
		$section = '';

	if ($section == '' && preg_match('~^<([a-z_]+)(?: [^>]+)?>(.*)</\1>$~', $line, &$parts) > 0)
// XXX old ircservices.	if ($section == '' && preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'id')
			$id = $parts[2];

		if ($parts[1] == 'pass')
			$nicks[$id]['pass'] = $parts[2];

		if ($parts[1] == 'email')
			$nicks[$id]['email'] = $parts[2];

		if ($parts[1] == 'flags')
		{
			$nicks[$id]['hide-email'] = $parts[2] & 0x80;
			$nicks[$id]['enforce'] = $parts[2] & 0x1;
		}
	}
	elseif ($section == 'memo' && preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'number')
			$memo_num = $parts[2];

		if ($parts[1] == 'flags')
			$nicks[$id]['memos'][$memo_num]['read'] = ( $parts[2] == 0 ? true : false );

		if ($parts[1] == 'sender')
			$nicks[$id]['memos'][$memo_num]['from'] = $parts[2];

		if ($parts[1] == 'time')
			$nicks[$id]['memos'][$memo_num]['time'] = $parts[2];

		if ($parts[1] == 'text')
			$nicks[$id]['memos'][$memo_num]['text'] = de_ircs($parts[2],true);
	}
	elseif ($section == 'nicks' && preg_match('~^<([a-z_-]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'array-element' && !isset($nicks[$id]['mainnick']))
			$nicks[$id]['mainnick'] = $parts[2];
	}
	elseif ($section == 'ajoin' && preg_match('~^<([a-z_-]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'array-element')
			$nicks[$id]['ajoin'][] = $parts[2];
	}
	elseif ($section == 'access' && preg_match('~^<([a-z_-]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'array-element')
			$nicks[$id]['access_list'][] = $parts[2];
	}
}

function read_nickinfo($line)
{
	global $nicks;
	static $data = array();

	if ($line == '<nickinfo>')
		$data = array();

	if (preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'nick')
			$data['nick'] = $parts[2];

		if ($parts[1] == 'last_usermask')
			$data['fakehost'] = $parts[2];

		if ($parts[1] == 'last_realmask')
			$data['realhost'] = $parts[2];

		if ($parts[1] == 'time_registered')
			$data['time_registered'] = $parts[2];

		if ($parts[1] == 'last_seen')
			$data['last_seen'] = $parts[2];

		if ($parts[1] == 'status')
			$data['noexpire'] = ( $parts[2] == 4 ? true : false );

		if ($parts[1] == 'nickgroup')
		{
			// Seriously...
			if ($parts[2] == 0)
				return;

			// Now that we have an id to associate it with, commit.
			$nickid = count($nicks[$parts[2]][nicks]);
			foreach($data as $type => $info)
				$nicks[$parts[2]][nicks][$nickid][$type] = $info;
		}
	}
}

function read_channelinfo($line)
{
	global $chans;
	static $chan = '';
	static $levels = array();
	static $akick = array();
	static $acc_who = 0;
	static $section = '';
	static $mode_map = array(
				'i'=>0x00000001, 'l'=>0x00000004, 'm'=>0x00000008, 'n'=>0x00000010, 'p'=>0x00000040,
				's'=>0x00000080, 't'=>0x00000100, 'c'=>0x00001000, 'M'=>0x00002000, 'R'=>0x00004000,
				'O'=>0x00008000, 'A'=>0x00010000, 'Q'=>0x00020000, 'S'=>0x00040000, 'K'=>0x00080000,
				'V'=>0x00100000, 'C'=>0x00200000, 'u'=>0x00400000, 'z'=>0x00800000, 'N'=>0x01000000,
				'G'=>0x04000000
				);
		// Above mode_map is for UnrealIRCd.


	if ($line == '<channelinfo>')
	{
		$levels = array('b'=>-100,
				'A'=>0,
				'v'=>30, 'V'=>30,
				'h'=>40, 'H'=>40, 'r'=>40, 't'=>40,
				'o'=>50, 'O'=>50, 'i'=>50,
				'f'=>100,'R'=>100,
				's'=>1000
				);
	}

	if ($line == '<levels>')
		$section = 'levels';
	if ($line == '<chanaccess>')
		$section = 'chanaccess';
	if ($line == '<akick>')
		$section = 'akick';
	if ($line == '</'. $section .'>')
		$section = '';

	if ($section == '' && preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'name')
			$chan = $parts[2];

		if ($parts[1] == 'founder')
		{
			$chans[$chan]['founder'] = $parts[2];
			$chans[$chan]['access']['+'.$parts[2]] = '+voOtsriRfhA';
		}

		if ($parts[1] == 'time_registered')
			$chans[$chan]['time_registered'] = $parts[2];

		if ($parts[1] == 'last_used')
			$chans[$chan]['time_used'] = $parts[2];

		if ($parts[1] == 'last_topic')
			$chans[$chan]['topic'] =  de_ircs($parts[2],true); // Some control codes can segfault atheme

		if ($parts[1] == 'last_topic_setter')
			$chans[$chan]['topic_by'] = de_ircs($parts[2],true);

		if ($parts[1] == 'last_topic_time')
			$chans[$chan]['topic_time'] = $parts[2];

		if ($parts[1] == 'mlock_on')
		{
			$chans[$chan]['mlock_on'] = 0;
			for ($i = 0; $i < strlen($parts[2]); $i++)
			{
				if ($parts[2]{$i} == ' ')
					break;
				if (array_key_exists($parts[2]{$i}, $mode_map))
					$chans[$chan]['mlock_on'] = $chans[$chan]['mlock_on'] | $mode_map[$parts[2]{$i}];
			}
		}

		if ($parts[1] == 'mlock_off')
		{
			$chans[$chan]['mlock_off'] = 0;
			for ($i = 0; $i < strlen($parts[2]); $i++)
			{
				if ($parts[2]{$i} == ' ')
					break;
				if (array_key_exists($parts[2]{$i}, $mode_map))
					$chans[$chan]['mlock_off'] = $chans[$chan]['mlock_off'] | $mode_map[$parts[2]{$i}];
			}
		}

		if ($parts[1] == 'mlock_limit')
			$chans[$chan]['mlock_limit'] = $parts[2];
	}
	elseif ($section == 'levels' && preg_match('~.*<([a-zA-Z_]+)>(.+)</\1>.*~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'CA_INVITE')		$levels['i'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_SET')		$levels['s'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_AUTOOP')		$levels['O'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_AUTOHALFOP')	$levels['H'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_AUTOVOICE')	$levels['V'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_AUTOPROTECT')		$levels['a'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_OPDEOP')		$levels['o'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_HALFOP')	{	$levels['h'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
							$levels['t'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
						}
		if ($parts[1] == 'CA_VOICE')		$levels['v'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_ACCESS_LIST')	$levels['A'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_ACCESS_CHANGE')	$levels['f'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_CLEAR')		$levels['R'] = ($parts[2] == -1000 ? 1000 : $parts[2]);
		if ($parts[1] == 'CA_NOJOIN')		$levels['b'] = $parts[2];
	}
	elseif ($line == '</levels>')
	{
		$acc_anon = '+';
		foreach($levels as $flag => $level)
		{
			if ($flag == 'b' && 0 <= $level)
				$acc_anon .= $flag;
			elseif ($flag != 'b' && 0 >= $level)
				$acc_anon .= $flag;
		}
		if ($acc_anon != '+')
			$chans[$chan]['access']['-*!*@*'] = $acc_anon;
	}
	elseif ($section == 'chanaccess' && preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'nickgroup')
			$acc_who = '+' . $parts[2];
		if ($parts[1] == 'level' && $acc_who != 0)
		{
			$chans[$chan]['access'][$acc_who] = '+';
			foreach($levels as $flag => $level)
			{
				if ($flag == 'b' && $parts[2] <= $level)
					$chans[$chan]['access'][$acc_who] .= $flag;
				elseif ($flag != 'b' && $parts[2] >= $level)
					$chans[$chan]['access'][$acc_who] .= $flag;
			}
		}
	}
	elseif ($section == 'akick' && preg_match('~^<([a-z_]+)>(.*)</\1>$~',$line,&$parts) > 0)
	{
		if ($parts[1] == 'mask')
			$akick['mask'] = $parts[2];
		if ($parts[1] == 'reason')
			$akick['reason'] = $parts[2];
		if ($parts[1] == 'who')
			$akick['by'] = $parts[2];
		if ($parts[1] == 'set')
		{
			$akick['set'] = $parts[2];
			if (isset($akick['mask'])) // Sigh.  Empty akicks!  Woo!
			$chans[$chan]['akicks'][] = $akick;
			$akick = array();
		}
	}
}

function de_ircs($in,$safe)
{
	$in_escape = false;
	$buf = '';
	$out = '';
	for ($i = 0; $i < strlen($in); $i++)
	{
		if ($in{$i} == '&')
		{
			$buf = '';
			$in_escape = true;
		}
		elseif ($in_escape)
		{
			if ($in{$i} == ';')
			{
				$in_escape = false;
				switch ($buf{0})
				{
					case 'l': $out .= '<'; break;
					case 'g': $out .= '>'; break;
					case 'a': $out .= '&'; break;
					case '#':
						if ($safe)
							switch (substr($buf,1))
							{
								case 3:		// color
								case 2:		// bold
								case 31:	// underline
								case 22:	// reverse
									$out .= chr(substr($buf,1));
							}
						else
							$out .= chr(substr($buf,1));
						break;
				}
				$in_escape = false;
			}
			else
				$buf .= $in{$i};
		}
		else
			$out .= $in{$i};
	}
	return $out;
}

?>
