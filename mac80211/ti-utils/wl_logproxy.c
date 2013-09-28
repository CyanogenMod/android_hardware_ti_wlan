/*
 *
 * Description: wl Firmware Log Service for off-line and real-time log viewing
 * Maintainer: gil.barak@ti.com
 * Last Update: 11/09/2012
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

/* Defines */
#define INVALID_HANDLE -1

#define FILE_NAME "wl_fwlog.log"
#define FILE_NAME_PREV "wl_fwlog_prev.log"

#define BUFFER_SIZE 1024
#define FILE_POLL_INTERVAL 5 // in seconds
#define KEEP_ALIVE_INTERVAL 30 // in seconds
#define MAX_PATH 256
#define MAX_FILE_SIZE 100000000 // Max file size is 100mb
/***********/


/* Globals */
int socket_connected = 0;
int socket_enabled = 1;
/***********/


void error(const char *msg)
{
    printf("FW LOGGER ERROR: %s\r\n", msg);
}


void sigpipe_handler()
{
	error("sigpipe caught!\r\n");
	socket_connected = 0;
}

// ************************************************** //
//
// Function name: create_listen_socket
//
// Description: Listen on a local port for remote
//				client connections
//
//
//
//
// ************************************************** //
int create_listen_socket(int listen_port)
{
	struct sockaddr_in serv_addr, cli_addr;
	int sock_flags = 0;
	int listen_sock = INVALID_HANDLE;

	if(listen_port == 0)
	{
		socket_enabled = 0;
		return INVALID_HANDLE;
	}

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_HANDLE)
	{
		error("opening socket\r\n");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(listen_port);

	// Bind to local port
	if (bind(listen_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		  error("on binding\r\n");
		  exit(1);
	}

	listen(listen_sock,1);

	// Set the callback for socket timeout signal handling
	signal(SIGPIPE,sigpipe_handler);


	// Set the listening socket to non-blocking mode
	sock_flags = fcntl(listen_sock, F_GETFL, 0);
	fcntl(listen_sock, F_SETFL, sock_flags | O_NONBLOCK);

	return listen_sock;
}

// ************************************************** //
//
// Function Name: accept_client
//
// Description:
//				Monitor client activity and allow new
//				connections if the client is disconnected
//
//
// ************************************************** //
int accept_client(int listen_sock)
{
	int client_sock = INVALID_HANDLE;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	struct timeval tv;

	tv.tv_sec = KEEP_ALIVE_INTERVAL;
	tv.tv_usec = 0;

	clilen = sizeof(cli_addr);
	// Non-Blocking accept for remote client connections
	client_sock = accept(listen_sock, (struct sockaddr *) &cli_addr, &clilen);

	if(client_sock != INVALID_HANDLE)
	{
		printf("Client connected!\r\n");
		if(setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)))
		{
			error("failed to set timeout for recv\r\n");
			close(client_sock);
			close(listen_sock);
			exit(EXIT_FAILURE);
		}
		socket_connected = 1;
	}

	return client_sock;
}

// ************************************************** //
//
// Function Name: send_data
//
// Description:
//				Send the binary logs to the remote
//				client that is currently connected
//
//
// ************************************************** //
void send_data(int client_sock, char *buffer, int buf_size)
{
	int sent_bytes = 0;
	// Send the buffer to the remote client
	 if(socket_connected)
	 {
		 sent_bytes = 0;
		 while( buf_size > 0 )
		 {
			 sent_bytes = send(client_sock,(char *)(buffer + sent_bytes),buf_size,0);
			 if (sent_bytes < 0)
			 {
				 printf("Send failed! %d %d\n", buf_size, client_sock);
				 close(client_sock);
				 socket_connected = 0;
				 break;
			 }
			 buf_size -= sent_bytes;
		 }
	 }
}

// ************************************************** //
//
// Function Name: keep_alive
//
// Description:
//				Verify that the remote client is
//				alive by receiving keep alive data
//
//
// ************************************************** //
void keep_alive(int client_sock)
{
	char ka_buff;
	int n = 0;
	// Verify keep alive
	 if(socket_connected)
	 {
		 n = recv(client_sock, &ka_buff, 1, 0);
		 if (n < 0)
		 {
			 error("Client disconnected");
			 close(client_sock);
			 socket_connected = 0;
		 }
	 }
}

// ************************************************** //
//
// Function Name: open_fwlog
//
// Description:
//				Wait until the fwlog file is created
//				and open it for read access
//
// Return:
//			file handle to fwlog
//
// ************************************************** //
int open_fwlog(int fp_fwlog, char *file_path)
{
	 if(fp_fwlog == INVALID_HANDLE)
	 {
		 fp_fwlog = open(file_path, O_RDONLY);
	 }

	 return fp_fwlog;
}

// ************************************************** //
//
// Function Name: read_file
//
// Description:
//				read the binary logs from the file
//				into the buffer
//
// Return:
//			file handle
//
// ************************************************** //
int read_file(int fp_file, char *buffer)
{
	int n = 0;
	bzero(buffer,BUFFER_SIZE);

	// Attempt to read logs from the fwlog
	n = read(fp_file,buffer,BUFFER_SIZE);
	if (n < 0)
	{
		error("reading from file\r\n");
		return INVALID_HANDLE;
	}
	return n;
}

void backup_file(int fp_bkp, char *buffer, int buf_size)
{
	int wr_bytes = 0;

	do
	{
		wr_bytes = write(fp_bkp, buffer, buf_size);
		if(wr_bytes < 0)
		{
			error("writing to backup file!");
			exit(EXIT_FAILURE);
		}
	}
	while (wr_bytes < buf_size);
}

int create_file(char *filename)
{
	int fp_bkplog;
#ifdef ANDROID
	fp_bkplog = open(filename, O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
#else
	fp_bkplog = open(filename, O_RDWR|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
#endif
	if (fp_bkplog == INVALID_HANDLE)
	{
		error("Could not open output log file");
		exit(EXIT_FAILURE);
	}

	return fp_bkplog;
}
/*
void core_dump_thread( void *ptr)
{
	char buffer[BUFFER_SIZE + 1] = {0}; // +1 for safety
	int fp_fwdump = INVALID_HANDLE;
	int fp_fwdumpbkp = INVALID_HANDLE;
	int buf_size = 0;

	while(1)
	{
		if(fp_fwdump == INVALID_HANDLE)
		{
			fp_fwdump = open(fp_fwdump, O_RDONLY);
			continue;
		}

		if(fp_fwdumpbkp == INVALID_HANDLE)
		{
			fp_fwdumpbkp = open(filename, O_RDWR|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
			continue;
		}

		// Read the core_dump sysfs file - This is a blocking function
		buf_size = read_file(fp_fwdump, buffer);

		while( (buf_size = read_file(fp_fwdump, buffer)) != 0)
		{
			backup_file(fp_fwdumpbkp, buffer, buf_size);
		}

		close(fp_fwdumpbkp);
		close(fp_fwdump);
	}
	pthread_exit(0);
}
*/

// ************************************************** //
//
// Function Name: main
//
// Description:
//
//
//
//
// ************************************************** //
int main(int argc, char *argv[])
{
     int listen_sock, client_sock = INVALID_HANDLE;
     char buffer[BUFFER_SIZE + 1] = {0}; // +1 for safety
     int n = 0, buf_size = 0;
     int fp_fwlog = INVALID_HANDLE;
     int fp_bkplog = INVALID_HANDLE;
     char filename[MAX_PATH + 1] = {0}; // +1 for safety
     char filename_prev[MAX_PATH + 1] = {0}; // +1 for safety
     uint max_file_size = MAX_FILE_SIZE;
     struct stat st;
     pthread_t thread;

     if (argc < 3)
     {
         fprintf(stderr,"Usage: ./logProxy [listen port] [log file] [backup directory]\n");
         exit(EXIT_FAILURE);
     }

     // Max file size is an optional value
     if(argc > 4)
     {
    	 max_file_size = atol(argv[4]);
     }

     listen_sock = create_listen_socket(atoi(argv[1]));

     //
     // If we arrive here the listen socket was created successfully, otherwise we would have exited the process
     //

     // Create the backup file names
     strncpy(filename,argv[3], MAX_PATH - sizeof(FILE_NAME));
     strncpy(filename_prev,argv[3], MAX_PATH - sizeof(FILE_NAME_PREV));
     strcat(filename, FILE_NAME);
     strcat(filename_prev, FILE_NAME_PREV);

     fp_bkplog = create_file(filename);

     // Create the core_dump thread
     //pthread_create(&thread, NULL, (void *) &core_dump_thread, NULL);

     // Main Loop
     while(1)
     {
    	 // Check on the client connection status
    	 if ( 	(socket_enabled == 1) &&
    			(client_sock == INVALID_HANDLE || socket_connected == 0) )
    	 {
    		 client_sock = accept_client(listen_sock);
    	 }

    	 if(fp_fwlog != INVALID_HANDLE)
    	 {
			 // Read the fwlog sysfs file - This is a blocking function
			 buf_size = read_file(fp_fwlog, buffer);

			 if(buf_size == INVALID_HANDLE)
			 {
				 error("driver unloaded: fwlog has been removed");
				 fp_fwlog = INVALID_HANDLE;
				 continue;
			 }

			 // Create a new file if the log file exceeds the max size
			 stat(filename, &st);
			 if(st.st_size >= max_file_size)
			 {
				 close(fp_bkplog);
				 rename(filename, filename_prev);
				 fp_bkplog = create_file(filename);
			 }

			 // keep alive
			 keep_alive(client_sock);

			 if(buf_size > 0)
			 {
				 // write the log data into the backup file
				 backup_file(fp_bkplog, buffer, buf_size);

				 // Try to send the logs to the remote client
				 send_data(client_sock, buffer, buf_size);
			 }


    	 }
    	 else
    	 {
    		 // To prevent high io/cpu usage sleep between attempts
    		 sleep(FILE_POLL_INTERVAL);
    		 // Try to open the fwlog file
    		 fp_fwlog = open_fwlog(fp_fwlog, argv[2]);
    	 }

     }

     return 0;
}
