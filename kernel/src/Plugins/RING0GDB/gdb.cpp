/************************************************************
                 psfbsdk by m0rph3us1987
************************************************************/

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
}

#include "kdl.h"
#include "log.h"
#include "syscalls.hpp"
#include <Utils/SysWrappers.hpp>

extern "C"
{
	void cpu_sidt(void*);
	void cpu_enable_wp();
	void cpu_disable_wp();
	int gdb_start_serverRing0(int port);
	extern struct thread *RING0GDBMainThread;
}


static int gdb_server_socket = -1;
int gdb_start_server(int port);

int get_gdb_server_socket(){
	return gdb_server_socket;
}

void set_gdb_server_socket(int val){
	gdb_server_socket = val;
}

int gdb_writeRing0(void *buff, int len)
{
	return kwrite_t(gdb_server_socket, buff, len, RING0GDBMainThread);
}

void gdb_close_client_socketRing0()
{
	kclose_t(gdb_server_socket, RING0GDBMainThread);
	gdb_server_socket = -1;
	return;
}

void gdb_putDebugCharRing0(unsigned char c)
{
	if(gdb_server_socket != -1)
		gdb_writeRing0(&c,1);
}

int gdb_readRing0(void *buff, int len)
{
	return kread_t(gdb_server_socket, buff, len, RING0GDBMainThread);
}


unsigned char gdb_getDebugCharRing0()
{
	unsigned char c = 0x00;
	
	// If no connection is available, start server
	if(gdb_server_socket == -1)
	{
		gdb_server_socket = gdb_start_serverRing0(9946);
	}

	while(gdb_readRing0(&c,1) != 1){}
	
	return c;
}

extern "C"
{

void gdb_exceptionHandlerRing0(int exception_number, void *exception_address)
{
	LOG_DBG("overwriting exception handler for int %d\n", exception_number);

	uint64_t funcAddrIU64 = (uint64_t)exception_address;	
	uint8_t *funcAddress = (uint8_t*)&funcAddrIU64;

	// Read idrt
	uint8_t idtr[10];
	cpu_sidt((void*)&idtr);

	// Find idt begin
	uint64_t *base = (uint64_t*)&idtr[2];	

	LOG_DBG("found idt at 0x%llx\n",*base);

	// Find correct entry
	uint8_t *entry = (uint8_t*)*base;
	entry += (0x10 * exception_number);

	// Getting original pointer
	uint64_t OriginalEntry = 0x00;
	uint8_t *origBytes = (uint8_t*)&OriginalEntry;
	origBytes[0] = entry[0];
	origBytes[1] = entry[1];
	origBytes[2] = entry[6];
	origBytes[3] = entry[7];
	origBytes[4] = entry[8];
	origBytes[5] = entry[9];
	origBytes[6] = entry[10];
	origBytes[7] = entry[11];

	LOG_DBG("Entry %d at 0x%llx pointing to 0x%llx\n", exception_number, entry, OriginalEntry);

	// Replace function pointer
	cpu_disable_wp();
	asm("cli");
	entry[0] = funcAddress[0];
	entry[1] = funcAddress[1];
	entry[6] = funcAddress[2];
	entry[7] = funcAddress[3];
	entry[8] = funcAddress[4];
	entry[9] = funcAddress[5];
	entry[10] = funcAddress[6];
	entry[11] = funcAddress[7];
	asm("sti");
	cpu_enable_wp();

	LOG_DBG("done!\n");
	return;	
}
}


extern "C"
{
int gdb_start_serverRing0(int port)
{
    LOG_DBG("gdb_start_server\n");

    // Prepare socket
	int r = ksocket_t(AF_INET, SOCK_STREAM, 0, RING0GDBMainThread);
	int client = 0;

	LOG_DBG("sys_socket: 0x%llx\n",r);

	if(r > 0)
	{
		int s = r;
        r = ps4gdb_sys_bindt(s,port,RING0GDBMainThread);
		
		LOG_DBG("sys_bind: 0x%llx\n",r);
		if(r == 0)
		{
			r = klisten_t(s, 10, RING0GDBMainThread);

			if(r == 0)
			{
                int r = kaccept_t(s, (struct sockaddr *) 0, 0, RING0GDBMainThread);
				LOG_DBG("Waiting for gdb connection on port %d\n", port);
                LOG_DBG("sys_accept: 0x%llx\n",r);

				if(r > 0)
				{
					client = r;
					LOG_DBG("Client socket fd: 0x%llx\n",client);
                    kshutdown_t(s,SHUT_RDWR, RING0GDBMainThread);
                    kclose_t(s,RING0GDBMainThread);
					return client;
				} else {
                    LOG_ERROR("Accept failed, killing proc to prevent kpanic\n");
                    return -1;
                }
			}
		} else {
            LOG_ERROR("Bind failed, killing proc to prevent kpanic\n");
        }
	}

	return r;
}
}

