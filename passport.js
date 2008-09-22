
var passport = {
	
	obj      : document.getElementById("loginMsg"),
	appid    : 0,
	eInterval: false,
	callback : false,
	intervalCount: 0,
	login_status: false,
	cvsid: "$Id: passport.js,v 1.0 2008-3-19 13:17 七夜 Exp $",
	maxIntervalCount: 100
	, Login : function() 
	{
		
		var html = "通行证正在登陆中......";
		var frm  = document.forms["loginform"];
		var url  = "http://www.oos.com/passport.php?action=Login&user=" + frm.elements["user"].value + "&pass=" + frm.elements["pass"].value;
		
		this.addCookie("tmp_name", frm.elements["user"].value, 336);

		var newScript  = document.createElement("script");
		newScript.type = "text/javascript";
        newScript.src  = url + "&rand=" + Math.random();

		this.obj.innerHTML = html;
        this.obj.appendChild(newScript);
		
	    this.eInterval = setInterval(function()
        {
            passport.loginIntervalProc();
        }
        , 100);
        return false;
	}
	, Logout : function() 
	{
		
		var html = '<form method="post" onsubmit="return passport.Login();" name="loginform">用户名: <input type="text" name="user" value="'+this.getCookie("tmp_name")+'" /><BR />密  码: <input name="pass" idd ="pass" type="password" /><BR /><input type="Submit" value="登录" /></form><a href="http://www.oos.com/reg.html?appid='+ this.appid +'&fromuser=myweb&forward='+encodeURIComponent(window.top.location.href)+'">注册通行证</a>';		
		this.obj.innerHTML = html;
	}
	, loginIntervalProc : function() 
	{
		
		var msg;
        if (this.login_status < 1 && this.intervalCount < this.maxIntervalCount)
        {
            this.intervalCount++;
            return ;
        }
		
		//回调函数 回调数据
		if(typeof(this.callback) == "function")
		{
			this.callback(this.login_status);
		}
		else
		{
			alert("passport callback function is error!!!");
		}

		clearInterval(this.eInterval);
		this.intervalCount = 0;
        this.eInterval     = false;
		this.login_status  = false;
	}
	, addCookie: function(name, value, expireHours)
    {

        var cookieString = name + "=" + escape(value) + ";";
        if (expireHours > 0)
        {
            var date = new Date();
			var ms   = expireHours * 3600 * 1000;
            
			date.setTime(date.getTime() + ms);
			cookieString += "; expires=" + date.toGMTString();
        }
        document.cookie = cookieString;
    }
    , getCookie: function(name)
    {
        var cookies = document.cookie.split('; ');

        for (var i = 0; i < cookies.length; i++)
        {
			var temp = cookies[i].split("=");
			if(temp[0] == name) return unescape(temp[1]);
        }
        return "";
    }
    , deleteCookie: function(name)
    {
		var date = new Date();
		date.setTime(date.getTime() - 10000);
		document.cookie = name + "=a; expires=" + date.toGMTString();
    }
	, load_script: function(url)
	{
		var newScript  = document.createElement("script");
		newScript.type = "text/javascript";
        newScript.src  = url + "&rand=" + Math.random();

		document.getElementsByTagName("head")[0].appendChild(newScript);
	}
}