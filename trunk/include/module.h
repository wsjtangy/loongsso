#ifndef	_MODULE_H
#define	_MODULE_H

//loongSSO 登陆 module.c
int loong_sso_login(loong_conn *conn);

//loongSSO 退出 module.c
int loong_sso_logout(loong_conn *conn);

//生成验证码 module.c
int loong_sso_validate(loong_conn *conn);

//loongSSO 注册 module.c
int loong_sso_register(loong_conn *conn);

//loongSSO 验证
int loong_sso_check(loong_conn *conn);

//删除用户信息
int loong_sso_delete(loong_conn *conn);

//用户更新 用户信息
int loong_sso_update(loong_conn *conn);

#endif

