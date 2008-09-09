<?php

define("loongSSO_KEY", "loongSSO_315TOGO_XX_KEY");	


$info = urldecode($_GET["loong_info"]);
$sign = md5(loongSSO_KEY ."|". $info ."|". $_GET["loong_time"]);
if($sign != $_GET["loong_sign"])
{
	echo 'alert("sign no");';
	exit;
}
echo loongSSO_KEY ."|". $info ."|". $_GET["loong_time"];
  

?>