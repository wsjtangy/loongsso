<?php

define("loongSSO_KEY", "loongSSO_CELLPHP_KEY");	


session_start();
header("Content-Type: application/x-javascript");
header('P3P: CP="CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT DEM STA PRE COM NAV OTC NOI DSP COR"');


$info = urldecode($_GET["loong_info"]);
$sign = md5(loongSSO_KEY ."|". $info ."|". $_GET["loong_time"]);
if($sign != $_GET["loong_sign"])
{
	echo 'alert("sign no");';
	exit;
}


$_SESSION["user"] = loongsso_decode($info);


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