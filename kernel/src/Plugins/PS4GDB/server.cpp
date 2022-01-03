extern "C"
{
    #include <sys/param.h>
    #include <sys/types.h>
    #include <sys/stdint.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/mman.h>
    #include <sys/user.h>
    #include <sys/wait.h>
    #include <sys/errno.h>
    #include "log.h"
}

#include "syscalls.hpp"
#include "ptrace.hpp"

int errno;
int GDB_SERVER_PORT;
int GDB_SERVER_PID;
int GDB_SERVER_SETUP = 0;

void handle_exception (int gdb_i386vector, unsigned long long *registersOld, int errorCode);

uint8_t *get_process_list(uint64_t *len){

    *len = 0;
    int mib[3];

	mib[0] = 1;
	mib[1] = 14;
	mib[2] = 8;
    
    if(ps4gdb_sys_sysctl(mib,3,NULL,len,NULL,0) < 0){
        LOG_ERROR("sysctl returned %d\n",errno);
        return NULL;
    }
    
    uint8_t *proc_list = (uint8_t *)ps4gdb_sys_mmap(NULL,*len,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANON,-1,0);
    if(ps4gdb_sys_sysctl(mib,3,proc_list,len,NULL,0) < 0){
        LOG_ERROR("sysctl returned %d\n",errno);
        ps4gdb_sys_munmap(proc_list,*len);
        return NULL;
    }
    
    return proc_list;
}


int gdb_start_rpc_server(int cmd_port, int gdb_port){
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
            LOG_DBG("Socket: %d...\n", client);
            uint64_t dataLen = 0;
            struct kinfo_proc *kipp = (struct kinfo_proc *)get_process_list(&dataLen);
            LOG_DBG("kproc_info data len: %d\n");
            if(ps4gdb_sys_write(client,&dataLen,8) != -1)
            {
                LOG_DBG("Written data len\n");
                if(ps4gdb_sys_write(client,kipp,dataLen) != -1)
                {
                    LOG_DBG("Written proc info\n");
                    ps4gdb_sys_munmap(kipp,dataLen);
                    
                    // Read PID to attach
                    ps4gdb_sys_read(client,&GDB_SERVER_PID,4);
                    
                    if(GDB_SERVER_PID != -1)
                    {
                        if(GDB_SERVER_PID == -2){
                            LOG_DBG("Received command -2, destroying thread...\n");
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
                    } else {
                        LOG_DBG("Sending kproc_info only\n");
                    }
                } else {
                    LOG_ERROR("Could not write kproc_info\n");
                }
            } else {
                LOG_ERROR("Could not write dataLen\n");
            }
            ps4gdb_sys_close(client);        
        }
	}
	return 0;
}
