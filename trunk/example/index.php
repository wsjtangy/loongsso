<?php
session_start();

include_once "page.inc.php";
$pp    = new pp;
$size  = 6;

$rs    = new SQLite3("guestbook.sdb");

//id, uid, username, reg_time, post_time, ip, ip_pos, content
if(is_array($_SESSION["user"]) && $_POST["book"])
{
	$userip  = ip();
	$content = nl2br(stripslashes(Replace($_POST["content"])));
	
	$query   = "INSERT INTO guestbook (uid, username, reg_time, post_time, ip, content) VALUES ('{$_SESSION["user"]["id"]}', '{$_SESSION["user"]["username"]}', '{$_SESSION["user"]["time"]}', '".time()."', '{$userip}', '{$content}')";
	
	$rs->query($query);
	$rs->close();
	
	echo "<script>alert('发表成功');window.location.href = 'index.php';</script>";
	exit();
}

$co   = $rs->fetch_line("SELECT COUNT(*) AS c FROM guestbook");
$page = $pp->show($co["c"], $size);
$list = $rs->fetch_all("SELECT id, uid, username, reg_time, post_time, ip, content FROM guestbook ORDER BY id DESC LIMIT {$pp->limit}");

$rs->close();

function Replace($str)
{
	if(is_null($str)) return $str;

	$word = array("<" => "&lt;", ">" => "&gt;", "'" => "&quot;", '"' => "&quot;");
	return strtr($str, $word);
}

//获取客户端的IP
function ip()
{
	if(getenv('HTTP_CLIENT_IP') && strcasecmp(getenv('HTTP_CLIENT_IP'), 'unknown')) 
	{
		$onlineip = getenv('HTTP_CLIENT_IP');
	}
	elseif(getenv('HTTP_X_FORWARDED_FOR') && strcasecmp(getenv('HTTP_X_FORWARDED_FOR'), 'unknown'))
	{
		$onlineip = getenv('HTTP_X_FORWARDED_FOR');
	}
	elseif(getenv('REMOTE_ADDR') && strcasecmp(getenv('REMOTE_ADDR'), 'unknown')) 
	{
		$onlineip = getenv('REMOTE_ADDR');
	} 
	elseif(isset($_SERVER['REMOTE_ADDR']) && $_SERVER['REMOTE_ADDR'] && strcasecmp($_SERVER['REMOTE_ADDR'], 'unknown')) 
	{
		$onlineip = $_SERVER['REMOTE_ADDR'];
	}
	$onlineip = preg_replace("/^([\d\.]+).*/", "\\1", $onlineip);
	return $onlineip;
}

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml"  xml:lang="zh-CN" lang="zh-CN">
<head>
<title>留言本 首页</title>
<meta http-equiv="Content-Type" content="text/html; charset=gbk" />
<link href="css/global.css" rel="stylesheet" type="text/css" />
  <script language="JavaScript">
  <!--
	
	var eInterval;
	var loong_status;
	var intervalCount;
	var maxIntervalCount;
	
	eInterval        = false;
	loong_status     = -1;
	intervalCount    = 0;
	maxIntervalCount = 100;
	
	var http_response_t = {
		HTTP_RESPONSE_EMAIL_NO            : 0,
		HTTP_RESPONSE_EMAIL_OK            : 1,
		HTTP_RESPONSE_LOGIN_OK            : 2,
		HTTP_RESPONSE_LOGOUT_OK           : 3,
		HTTP_RESPONSE_REGISTER_OK         : 4,
		HTTP_RESPONSE_VALIDATE_NO         : 5,
		HTTP_RESPONSE_VALIDATE_OK         : 6,
		HTTP_RESPONSE_USERNAME_NO         : 7,
		HTTP_RESPONSE_USERNAME_OK         : 8,
		HTTP_RESPONSE_PASSWORD_NO         : 9,
		HTTP_RESPONSE_EMAIL_EXISTS        : 10,
		HTTP_RESPONSE_UNKNOWN_TYPE        : 11,
		HTTP_RESPONSE_UNKNOWN_MODULE      : 12,
		HTTP_RESPONSE_USERNAME_EXISTS     : 13,
		HTTP_RESPONSE_EMAIL_NOT_EXISTS    : 14,
		HTTP_RESPONSE_USERNAME_NOT_EXISTS : 15
	};

	
	function is_password(str)
	{
		var i, c;
		if(str.length < 5 || str.length > 19)
		{
			return false;
		}
		for(i=0; i < str.length; i++)
		{
			c = str.charAt(i);
			if( (c < "0" || c > "9") && (c < "a" || c > "z" ) && (c < "A" || c > "Z" ))
			{
				return false;
			}
		}
		return true;
	}


	function isEmail(strEmail) 
	{
		if (strEmail.search(/^\w+((-\w+)|(\.\w+))*\@[A-Za-z0-9]+((\.|-)[A-Za-z0-9]+)*\.[A-Za-z0-9]+$/) != -1)
		{
			return true;
		}

		return false;
	}

	function is_username(username)
	{
		var patrn = /^[a-zA-Z0-9\u4e00-\u9fa5]{2,20}$/; 


		if(!patrn.exec(username))
		{
			return false;
		}

		return true;
	}

	function loginIntervalProc()
	{
		var frm;

        if (loong_status == -1 && intervalCount < maxIntervalCount)
        {
            intervalCount++;
            return ;
        }
		
		switch (loong_status)
		{
			case http_response_t.HTTP_RESPONSE_LOGOUT_OK :
				alert("退出成功");
				window.location.href = "http://www.xxxabc.cn:8080/index.php";
				break;
			default:
				alert("验证超时");
		}
		

		clearInterval(eInterval);
		intervalCount = 0;
        eInterval     = false;
		login_status  = -1;
	}

	function load_script(url)
	{
		var newScript  = document.createElement("script");
		newScript.type = "text/javascript";
		
		if(url.indexOf("?") == -1)
		{
        	newScript.src  = url + "?rand=" + Math.random();
		}
		else
		{
			newScript.src  = url + "&rand=" + Math.random();
		}
		document.getElementsByTagName("head")[0].appendChild(newScript);
	}


	function disposal()
	{
		var url;
		
		url = "http://www.cellphp.com:7171/?module=logout";
		load_script(url);

	    eInterval = setInterval(function()
        {
            loginIntervalProc();
        }
        , 100);

		return false;
	}
  //-->
  </script>
</head>
<body>

<!-- 头部 -->
<div class="top">
	<div class="l">
		<img src="images/logo.gif" alt="" />
	</div>
	<div class="top_ad r">

	</div>
</div>

<!-- 导航 -->
<div class="menu">
	<span class="l">目前共有 <span class="fb fc5"><?=$co["c"];?></span> 条留言！&nbsp;&nbsp;<?=$page;?></span>
	<span class="r">
		<? if(!is_array($_SESSION["user"])): ?>
		<a href="login.html">登录</a>
		<a href="reg.html">注册</a>
		<? else: ?>
		用户名: <?=$_SESSION["user"]["username"];?>&nbsp;&nbsp;&nbsp;注册时间: <?=date("Y-m-d H:i:s", $_SESSION["user"]["time"]);?>&nbsp;&nbsp;&nbsp;<a href="javascript:void(0);" onclick="return disposal();">退出</a>&nbsp;&nbsp;
		<? endif; ?>		
	</span>
</div>
<div class="bottombg"></div>

<!-- 循环留言内容 -->
<? foreach($list as $item): ?>
<div class="book">
	<div class="book_L l">
		<div class="book_L_top">第<span class="fb"><?=$item["id"]?></span>条</div>
		<div class="book_L_tx"><img src="nice.gif" width="82" height="81" /></div>
		<h1><?=$item["username"]?></h1>
		<div class="book_L_p">
			<p>IP: <?=$item["ip"]?></p>
			<p>Reg: <?=date("Y-m-d", $item["reg_time"]);?></p>
			<p><?=date("Y-m-d H:i:s", $item["post_time"]);?></p>
		</div>
	</div>

	<div class="book_R f14px r">
		<?=$item["content"]?>
	</div>
</div>
<div class="bottombg"></div>

<? endforeach; ?>
<!-- 循环留言内容 END -->

<!-- 发表留言 -->
<? if(is_array($_SESSION["user"])): ?>
<div class="publish">
	<h1>快速发表留言</h1>
	<div class="publish_L fb l">内容：</div>

	<form id="form1" name="form1" method="post" action="index.php">

		<textarea name="content" class="publish_L_p l" id="content"></textarea>
		<div class="submit">
		  <label><input class="publish_submit" type="submit" name="book" value="提交" /></label>
		  <label><input class="publish_submit" type="reset" name="Submit" value="重置" /></label>
		</div>

	</form>

</div>
<? else: ?>
<div class="publish">
	<h1>请您<a href="login.html">登录</a>或<a href="reg.html">注册</a> 再进行留言</h1>
</div>
<? endif; ?>
<div class="footer">
	<p>版权所有：loongSSO</p>
	<p>技术支持：七夜(李锦星) lijinxing@gmail.com </p>
</div>



</body>
</html>