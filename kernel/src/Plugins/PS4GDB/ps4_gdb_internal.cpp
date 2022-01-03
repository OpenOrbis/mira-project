extern "C"
{
#include <sys/param.h>
#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/stdint.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vm/vm.h>
#include <sys/malloc.h>
#include <sys/sysproto.h>
#include <sys/param.h>
#include <sys/ctype.h>
#include <sys/limits.h>
#include "log.h"
#include <sys/wait.h>
}

#include "gdbstub.hpp"
#include "syscalls.hpp"
#include "ptrace.hpp"
#include <Utils/SysWrappers.hpp>

int gdb_server_socket = -1;
int gdb_server_setup = 0;
extern int GDB_SERVER_PORT;
extern int GDB_SERVER_PID;

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long ps4gdb_strtoul(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	unsigned long acc;
	unsigned char c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (!isascii(c))
			break;
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endptr != NULL)
		*endptr = __DECONST(char *, any ? s - 1 : nptr);
	return (acc);
}

int startsWith(const char *pre, const char *str)
{
    char cp;
    char cs;

    if (!*pre)
        return 1;

    while ((cp = *pre++) && (cs = *str++)){
        if (cp != cs)
            return 0;
    }

    if (!cs)
        return 0;

    return 1;
}

int gdb_strlen(char *str)
{
	const char *s;
	for (s = str; *s; ++s){}
	return (s - str);
}

int gdb_write(void *buff, int len)
{
	return ps4gdb_sys_write(gdb_server_socket,buff,len);
}

void gdb_close_client_socket()
{
    ps4gdb_sys_close(gdb_server_socket);
	return;
}

void gdb_putDebugChar(unsigned char c)
{
	if(gdb_server_socket != -1)
		gdb_write(&c,1);
}

int gdb_read(void *buff, int len)
{
	// If no connection is available, start server
	if(gdb_server_socket == -1)
	{
		gdb_server_socket = gdb_start_server(GDB_SERVER_PORT,GDB_SERVER_PID);
	}

	return ps4gdb_sys_read(gdb_server_socket,buff,len);
}


unsigned char gdb_getDebugChar()
{
	unsigned char c = 0x00;
	
	// If no connection is available, start server
	if(gdb_server_socket == -1)
	{
		gdb_server_socket = gdb_start_server(GDB_SERVER_PORT,GDB_SERVER_PID);
	}
	while(gdb_read(&c,1) != 1){  }
	
	return c;
}

void gdb_strcpy(char *dest, const char *src)
{
    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';
}

void gdb_exceptionHandler(int exception_number, void *exception_address)
{
	
}

void gdb_memset(void *buff, uint8_t value, int len)
{
	uint8_t *tmp = (uint8_t*)buff;

	int i = 0;
	for(i = 0; i < len; i++)
		tmp[i] = value;

	return;
}

extern int errno;
void handle_exception (int gdb_i386vector, unsigned long long *registersOld, int errorCode);
uint8_t *get_process_list(uint64_t *len);

int ps4gdb_start_cmd_server(int cmd_port, int gdb_port)
{
    PS4GDB_kkproc_exit();
    LOG_DBG("Starting server...\n");
    GDB_SERVER_PORT = gdb_port;
    
    int server_socket = ps4gdb_sys_socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
        LOG_DBG("Failed to create server socket. errno: %d\n",errno);
        return server_socket;
    }
    
    int r = ps4gdb_sys_bind(server_socket,cmd_port);
	if(r != 0){
		LOG_ERROR("sys_bind failed. errno: %d\n",errno);
		return -1;
	}
	
	// Preparing to listen
	LOG_DBG("Starting to listen...\n");
	r = ps4gdb_sys_listen(server_socket, 10);
	if(r != 0){
		LOG_ERROR("sys_listen returned %d\n",errno);
		return -1;
	}
	
	for(;;){
		// Accept connections
		LOG_DBG("Accepting connections...\n");
		int client = ps4gdb_sys_accept(server_socket,(struct sockaddr *)NULL,0);
		if(client == -1){
            LOG_ERROR("sys_accept. errno %d\n",errno);
			ps4gdb_sys_shutdown(server_socket,SHUT_RDWR);
			ps4gdb_sys_close(server_socket);			
			return -1;
		} else {
            LOG_DBG("Got connection...\n");
            uint64_t dataLen;
            struct kinfo_proc *kipp = (struct kinfo_proc *)get_process_list(&dataLen);
            ps4gdb_sys_write(client,&dataLen,8);
            LOG_DBG("Written data len\n");
            ps4gdb_sys_write(client,kipp,dataLen);
            LOG_DBG("Written proc info\n");
            ps4gdb_sys_munmap(kipp,dataLen);
            
            // Read PID to attach
            ps4gdb_sys_read(client,&GDB_SERVER_PID,4);
            
            if(GDB_SERVER_PID != -1)
            {
                if(GDB_SERVER_PID == -2){
                    LOG_DBG("Received command -2, destroying process...\n");
                    kkproc_exit(0);
                    return 0;
                }

                LOG_DBG("Received pid %d\n", GDB_SERVER_PID);
                GDB_SERVER_PORT = gdb_port;
                //ps4gdb_sys_write(client,&GDB_SERVER_PORT,4);
                ps4gdb_sys_close(client);
                if(ptrace_attach(GDB_SERVER_PID) != 0){
                    LOG_ERROR("Attaching to pid failed %d\n",errno);
                    return 0;
                }
                int stat = 0;
                ps4gdb_sys_wait4(GDB_SERVER_PID,&stat,WUNTRACED,0);                
                handle_exception(0x01, NULL, 0);
            }
            ps4gdb_sys_close(client);        
        }
	}
	return 0;
}

int gdb_start_server(int port, int PID)
{
    LOG_DBG("gdb_start_server\n");

    // Prepare socket
	int r = ps4gdb_sys_socket(AF_INET,SOCK_STREAM,0);
	int client = 0;

	LOG_DBG("sys_socket: 0x%llx\n",r);

	if(r > 0)
	{
		int s = r;
        r = ps4gdb_sys_bind(s,port);
		
		LOG_DBG("sys_bind: 0x%llx\n",r);
		if(r == 0)
		{
			r = ps4gdb_sys_listen(s, 10);

			if(r == 0)
			{
                gdb_server_setup = 1;
				int r = ps4gdb_sys_accept(s,(struct sockaddr *)NULL,0);

				LOG_DBG("sys_accept: 0x%llx\n",r);

				if(r > 0)
				{
					client = r;
					LOG_DBG("Client socket fd: 0x%llx\n",client);
                    ps4gdb_sys_shutdown(s,SHUT_RDWR);
                    ps4gdb_sys_close(s);
					return client;
				} else {
                    LOG_ERROR("Accept failed, killing proc to prevent kpanic\n");
                    kkill_t(curproc->p_pid, SIGQUIT, curthread);
                    return -1;
                }
			}
		} else {
            LOG_ERROR("Bind failed, killing proc to prevent kpanic\n");
            kkill_t(curproc->p_pid, SIGQUIT, curthread);
        }
	}

	return r;
}

