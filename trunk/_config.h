#ifndef __CONFIG_H_
#define __CONFIG_H_


#define TABLE_CHUNK         10                                            //分表的个数
#define WATER_MARK          "loong SSO"                                   //验证码水印的文字
#define SIGN_FONT_PATH      "/home/lijinxing/labs/augie.ttf"              //分表的个数
#define LOONG_INFO_PATH     "/home/lijinxing/labs/cache/loong_info.db"    //存储用户具体信息的缓存库
#define LOONG_MAIL_PATH     "/home/lijinxing/labs/cache/loong_mail.db"    //存储用户key=mail, value=password的mail缓存库
#define LOONG_USER_PATH     "/home/lijinxing/labs/cache/loong_user.db"    //存储用户key=username, value=password的user缓存库
#define LOONG_CONFIG_PATH   "/home/lijinxing/labs/conf/loong_sso.xml"     //loong SSO配置文件的绝对路径
#define LOONG_LOG_PATH      "/home/lijinxing/labs/log/loong_sso.log"      //loong SSO日志文件的绝对路径

#endif

