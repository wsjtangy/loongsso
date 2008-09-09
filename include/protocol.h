#ifndef	_PROTOCOL_H
#define	_PROTOCOL_H

//0 解析失败 1 解析成功  解析http协议头  protocol.c
int parse_http_header(char *header, size_t header_len, loong_conn *conn);

#endif

