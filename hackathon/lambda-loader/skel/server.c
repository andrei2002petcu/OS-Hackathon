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
int arg_nr;

static int lib_prehooks(struct lib *lib)
{
	return 0;
}

static int lib_load(struct lib *lib)
{
	lib->handle = dlopen(lib->libname, RTLD_LAZY);
	if(lib->handle == NULL) {
		fprintf(stderr, "%s\n", dlerror());
		if (arg_nr == 1)
			printf("Error: %s could not be executed.\n", lib->libname);
		else
			printf("Error: %s %s %s could not be executed.\n", lib->libname, lib->funcname, lib->filename);
		exit(EXIT_FAILURE);
	}
	return 0;
}

static int lib_execute(struct lib *lib)
{
	if(arg_nr == 1) {
	 	lib->run = dlsym(lib->handle, "run");
		if(lib->run == NULL) {
			printf("Error: %s could not be executed.\n", lib->libname);
			exit(EXIT_FAILURE);
		}
	  	lib->run();
	}
	else {
		lib->p_run = dlsym(lib->handle, lib->funcname);
		if(lib->p_run == NULL) {
		 	fprintf(stderr, "%s\n", dlerror());
			printf("Error: %s %s %s could not be executed.\n", lib->libname, lib->funcname, lib->filename);
			exit(EXIT_FAILURE);
		}
	 	lib->p_run(lib->filename);
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
		close(fd);
		
		send(new_socket, template, strlen(template), 0);
		memset(buffer, 0, BUFSIZE);
		read(new_socket, buffer, BUFSIZE);
		
		if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0)
			break;
		
		/* TODO - parse message with parse_command and populate lib */
		
		lib.libname = (char *)calloc(BUFSIZE, sizeof(char));
		lib.funcname = (char *)calloc(BUFSIZE, sizeof(char));
		lib.filename = (char *)calloc(BUFSIZE, sizeof(char));
		lib.outputfile = (char *)calloc(strlen(template), sizeof(char));
		arg_nr = parse_command(buffer, lib.libname, lib.funcname, lib.filename);

		FILE *fp;
		fp = freopen(template, "w+", stdout);
		ret = lib_run(&lib);

		free(lib.libname);
		free(lib.funcname);
		free(lib.filename);
		free(lib.outputfile);
		fclose(fp);	
	}
	return 0;
}
