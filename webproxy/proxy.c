/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"



/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

void *created_new_thread(void* conn_args);  // Threads
void getHTTPRequest(int connfd, int port, struct sockaddr_in *sockaddr);

FILE *logFile;

struct connectionInfo_struct
{
	int fd;
	int port;
	int id;
	struct sockaddr_in clientaddr;
};

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
	int port, listen_fd;
	socklen_t clientlen;	
	struct sockaddr_in clientaddr;
	pthread_t thread_id;
	int id= 0;
    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }

// ignore SIGPIPE
	Signal(SIGPIPE, SIG_IGN); 
	logFile = Fopen("proxy.log", "w");
	logFile = Fopen("proxy.log", "a");
	// Connect to port given
	port = atoi(argv[1] );
	if ((listen_fd = Open_listenfd(port))==-1)
      unix_error("Failed to establish connection to port");
	for(;;) 
	{	
		id = id + 1;
		struct connectionInfo_struct *td = malloc(sizeof(struct connectionInfo_struct));
		clientlen = sizeof(clientaddr);
		td->fd = accept(listen_fd, (SA *)&clientaddr, &clientlen);
		td->clientaddr = clientaddr;
		td->port = port;
		td->id = id;
		printf("Accepted client request!\n");		
		//Create thread
		pthread_create(&thread_id, NULL, created_new_thread, td);
	}
	close(listen_fd);
	fclose(logFile);
    exit(0);
}


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}

void *created_new_thread(void* conn_args)
{
	struct connectionInfo_struct *td = ((struct connectionInfo_struct*)conn_args);
	int fd = td->fd;
	int port = td->port;
	struct sockaddr_in clientaddr = td->clientaddr;
	pthread_detach(pthread_self());
	free(conn_args);
	getHTTPRequest(fd, port, &clientaddr); 
	close(fd);
	return NULL;
}

void getHTTPRequest(int connection_filedesc, int port, struct sockaddr_in *sockaddr)
{
	int n;
	int totalByteCount = 0;
	int host_filedesc =0;
	int carriage_return = 1;
	int host_port;
	char buffer[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char hostname[MAXLINE], pathname[MAXLINE];
	char log_entry[MAXLINE];
	rio_t robust_io, host;
	
	struct sockaddr_in *sock_Address = sockaddr;
	// Read the request
	Rio_readinitb(&robust_io, connection_filedesc);
	n = Rio_readlineb(&robust_io, buffer, MAXLINE);
	sscanf(buffer, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET"))
	{
		printf("Only GET requests accepted.\n");
		return;
	}
	//Parse the uri to retrieve host name and path name
	parse_uri(uri, hostname, pathname, &host_port);
	//Connect to host on host_filedesc
	if((host_filedesc = Open_clientfd(hostname, host_port)) < 0)
	{
		return;
	}
	//Send request to host			
	Rio_writen(host_filedesc, buffer, strlen(buffer));
	Rio_readinitb(&host, host_filedesc);
	for(;carriage_return && ((n = Rio_readlineb(&robust_io, buffer, MAXLINE)) > 0);)
	{
		Rio_writen(host_filedesc, buffer, n);
		carriage_return = !(buffer[0] == '\r');
	}			
	//Read response lines sent by remote host
	for(;(n = Rio_readnb(&host, buffer, MAXLINE) ) != 0;)
	{
		Rio_writen(connection_filedesc, buffer, n);
		totalByteCount = totalByteCount + n;
	}
	format_log_entry(log_entry, sock_Address, uri, totalByteCount);
	fprintf(logFile, "%s %d\n", log_entry, totalByteCount);
	fflush(logFile);
	close(host_filedesc);
}
