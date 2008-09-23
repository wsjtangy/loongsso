#include <stdio.h>
#include <tchdb.h>
#include <string.h>
#include <tcutil.h>  
#include <stdlib.h>  
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>


struct parameter
{
	int    mode;
	size_t port;
	size_t chunk;
	char   host[100];
	char   table[50];
	char   fields[128];
	char   username[50];
	char   password[50];
	char   database[20];
};

//gcc -o loongsso_convert_data loongsso_convert_data.c -I/home/lijx/local/tokyocabinet/include/ -L/home/lijx/local/tokyocabinet/lib -ltokyocabinet

int main( int argc, char *argv[])
{
	char *buf;
	int	 c, in, i;
	char *data[20];
	char *key, *value;
	char *outer_ptr = NULL;
	char *inner_ptr = NULL;
	struct parameter p;
	
	TCMAP *parm;

	memset(&p, 0, sizeof(p));
	
	p.mode  = 0;
	p.port  = 3306;
	p.chunk = 10;

    while (1) 
	{
        int option_index = 0;
        static struct option long_options[] = {
            { "host",     required_argument, 0, 'o' },
			{ "port",     required_argument, 0, 'r' },
			{ "help",     no_argument,       0, 'h' },
			{ "mode",     no_argument,       0, 'm' },
			{ "chunk",    required_argument, 0, 'c' },
			{ "table",    required_argument, 0, 't' },
			{ "fields",   required_argument, 0, 'f' },
            { "version",  no_argument,       0, 'v' },
            { "username", required_argument, 0, 'u' },
			{ "password", required_argument, 0, 'p' },
			{ "database", required_argument, 0, 'd' },
            { 0, 0, 0, 0 }
        };

        c = getopt_long( argc, argv, "o:p:c:t:f:u:r:d:hv", long_options, &option_index );
        if (c == -1) break;

        switch (c) 
		{
            case 'h':
                printf(
                        "Usage: loong SSO data convert [OPTIONS]\n"
                        "       -h, --help             display this message then exit.\n"
                        "       -v, --version          display version information then exit.\n"
                        "\n"
                        "       -t, --traditional      output a greeting message with traditional format.\n"
                        "       -n, --next-generation  output a greeting message with next-generation format.\n"
                        "\n"
                        "Report bugs to <[email]flw@cpan.org[/email]>\n"
                      );
				return 0;
                break;

            case 'v':
                printf( "hello - flw's hello world. 0.8 version\n" );
				return 0;
                break;

            case 'd':
                memcpy(p.database, optarg, sizeof(p.database));
                break;
            case 'p':
                memcpy(p.password, optarg, sizeof(p.password));
                break;
		    case 'u':
                memcpy(p.username, optarg, sizeof(p.username));
                break;
		    case 'f':
                memcpy(p.fields, optarg, sizeof(p.fields));
                break;
            case 't':
                memcpy(p.table, optarg, sizeof(p.table));
                break;
			case 'o':
                memcpy(p.host, optarg, sizeof(p.host));
                break;
			case 'c':
				p.chunk = atoi(optarg);
                break;
			case 'r':
				p.port  = atoi(optarg);
                break;
			case 'm':
				p.mode  = 1;
                break;
            default:
                break;
        }
    }

    if ( optind < argc )
	{
        printf("Too many arguments\r\nTry `%s --help' for more information.\r\n", argv[0]);
        exit( EXIT_FAILURE );
    }
	
	in  = 0;
	buf = p.fields;
	while((data[in]=strtok_r(buf, "|", &outer_ptr))!=NULL)
	{
		buf = data[in];
		while((data[in]=strtok_r(buf,":",&inner_ptr))!=NULL)
		{
			in++;
            buf = NULL;
		}
		
		buf = NULL;
	}
	
	conn = mysql_init (NULL);
    if (conn == NULL)
    {
        printf ("mysql_init() failed (probably out of memory)\r\n");
        return 0;
    }
		//设置MySQL自动重连
	mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);


	//CLIENT_MULTI_STATEMENTS
	if (mysql_real_connect (conn, p.host, p.username, p.password, p.database, p.port, NULL, CLIENT_MULTI_STATEMENTS) == NULL)
    {
        printf ("mysql_real_connect() failed\r\n");
        return 0;
    }

	parm = tcmapnew2(10);

	for (i=0; i<in; i++)
	{	
		if(i%2)
		{
			value = data[i];
			tcmapput2(parm, key, value);
		}
		else
		{
			key   = data[i];
		}
	}

	printf("host = %s\tdatabase = %s\tusername = %s\r\n", p.host, p.database, p.username);

	tcmapdel(parm);
	mysql_close(conn);
	return 0;
}

