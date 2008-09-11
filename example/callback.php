<?php

define("PRIVATE_UPDATE_KEY", "loongSSO_UPDATE_CELLPHP_KEY");

$uid = "1221102757657484218";

echo md5(PRIVATE_UPDATE_KEY."|".$uid);

/*
define("PRIVATE_KEY", "loongSSO_YOU54_2008_KEY");	

session_start();
header("Content-Type: application/x-javascript");


if($_GET["action"] == "login")
{
	$parm = urldecode($_GET["loong_info"]);
	$sign = md5(PRIVATE_KEY. "|". $parm. "|". $_GET["loong_time"]);
	
	if($sign == $_GET["loong_sign"])
	{
		$_SESSION["user"] = loongsso_decode($parm);
	}
	else
	{
		echo "alert('no');";
	}
}
else if($_GET["action"] == "logout")
{
	unset($_SESSION["user"]);
	session_unset();
	session_destroy();
}
*/



function loongsso_decode($parm)
{
	$len  = strlen($parm);
	$flag = false;

	$key   = "";
	$value = "";
	$info  = array();

	for($i=0; $i<$len; $i++)
	{
		if($parm{$i} == ':')
		{
			$flag = true;
		}
		else if($parm{$i} == '|')
		{
			$info[$key] = $value;

			$flag  = false;
			unset($key, $value);
			$key   = "";
			$value = "";
		}
		else
		{
			if(!$flag)
			{
				$key .= $parm{$i};
			}
			else
			{
				$value .= $parm{$i};
			}
		}
	}

	return $info;
}

?>