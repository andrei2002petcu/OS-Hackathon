#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include "ipc.h"
#include "server.h"

#ifndef OUTPUTFILE_TEMPLATE
#define OUTPUTFILE_TEMPLATE "../checker/output/out-XXXXXX"
#endif

#define PORT 5000
#define IP_ADDR "127.0.0.1"

static int lib_prehooks(struct lib *lib)
{
	return 0;
}

static int lib_load(struct lib *lib)
{
	lib->handle = dlopen(lib->libname, RTLD_LAZY);
	//printf("%s\n",(char *)lib->handle);
	//getchar();
	return 0;
}

static int lib_execute(struct lib *lib)
{
	if(lib->funcname == NULL) {
	 	lib->run = dlsym(lib->handle, "run");
	  	lib->run();
	// 	write(lib->outputfile, "jale", 5);
	}
	else {
		lib->p_run = dlsym(lib->handle, lib->funcname);
		//lib->p_run(lib->filename);
	}
	return 0;
}

static int lib_close(struct lib *lib)
{
	dlclose(lib->handle);
	return 0;
}

static int lib_posthooks(struct lib *lib)
{
	return 0;
}

static int lib_run(struct lib *lib)
{
	int err;

	err = lib_prehooks(lib);
	if (err)
		return err;
	
	err = lib_load(lib);
	if (err)
		return err;

	err = lib_execute(lib);
	if (err)
		return err;

	err = lib_close(lib);
	if (err)
		return err;

	return lib_posthooks(lib);
}

static int parse_command(const char *buf, char *name, char *func, char *params)
{
	return sscanf(buf, "%s %s %s", name, func, params);
}

int main(void)
{
	int ret;
	struct lib lib;

	/* TODO - Implement server connection */
	struct sockaddr_in addr;
	int server_fd, val = 1;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &val, sizeof(val))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

	char buffer[BUFSIZE];
	while(1) {

		int addrlen = sizeof(addr), new_socket;
	if ((new_socket = accept(server_fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
		
		/* TODO - get message from client */
		char template[BUFSIZE];
		strcpy(template, OUTPUTFILE_TEMPLATE);
		int fd = mkstemp(template);
		if (fd < 0) {
			perror("file open failed");
			exit(EXIT_FAILURE);
		}
		send(new_socket, template, strlen(template), 0);
		memset(buffer, 0, BUFSIZE);
		read(new_socket, buffer, BUFSIZE);
		if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0)
			break;
		write(fd, buffer, sizeof(buffer));
		//printf("%s\n", buffer);
		close(fd);
			
		/* TODO - parse message with parse_command and populate lib */
		char name[BUFSIZE], func[BUFSIZE], params[BUFSIZE];
		parse_command(buffer, name, func, params);
		//printf("%s %s %s\n", name, func, params);

		lib.libname = (char *)calloc(strlen(name), sizeof(char));
		lib.funcname = (char *)calloc(strlen(func), sizeof(char));
		lib.filename = (char *)calloc(strlen(params), sizeof(char));
		lib.outputfile = (char *)calloc(strlen(template), sizeof(char));

		strcpy(lib.libname, name);
		strcpy(lib.funcname, func);
		strcpy(lib.filename, params);
		strcpy(lib.outputfile, template);

		printf("%s\n%s\n%s\n%s\n", lib.libname, lib.funcname, lib.filename, lib.outputfile);

		/* TODO - handle request from client */

		ret = lib_run(&lib);
	}

	return 0;
}
