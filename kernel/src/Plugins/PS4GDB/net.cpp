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
	#include <sys/mman.h>
	#include <sys/unistd.h>
	#include <vm/vm.h>
	#include <sys/malloc.h>
	#include <sys/errno.h>
	#include <sys/sx.h>
	#include "log.h"
}

#include <Utils/Kdlsym.hpp>
#include "kdl.h"
#include "syscalls.hpp"


int ReadBytes(int socket, int x, void* buffer)
{
	int bytesRead = 0;
    int result;
	uint8_t *buf = (uint8_t*)buffer;
    while (bytesRead < x)
    {
        result = ps4gdb_sys_read(socket, buf, x - bytesRead);
		
		if (result < 0 ){
			LOG_ERROR("ps4ninja_read returned %d\n",result);
			return -1;
		}else if(result != 0){
			LOG_DBG("Received %d bytes\n",result);
			bytesRead += result;
			buf += result;
		}		
    }
    return 0;
}
