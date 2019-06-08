#pragma once
#include <Utils/Types.hpp>

#ifndef MAX_PATH 
#define MAX_PATH 0x400
#endif

enum FileManagerCommands
{
	FileManager_Open = 0x58AFA0D4,
	FileManager_Close = 0x43F82FDB,
	FileManager_Read = 0x64886217,
	FileManager_Write = 0x2D92D440,
	FileManager_GetDents = 0x7433E67A,
	FileManager_Stat = 0xDC67DC51,
	FileManager_MkDir = 0x66F97F1E,
	FileManager_RmDir = 0xA1222091,
	FileManager_Unlink = 0x569F464B,
	FileManager_Echo = 0xEBDB1342,
};

struct MSGPACK fileexplorer_echoRequest_t
{
	// Length of the message
	uint16_t length;

	// Message data, length determined by the length field
	char message[];
};

struct MSGPACK fileexplorer_openRequest_t
{
	int32_t flags;
	int32_t mode;
	uint16_t pathLength;
	char path[];
};

struct MSGPACK fileexplorer_closeRequest_t
{
	int32_t handle;
};

struct MSGPACK fileexplorer_seekRequest_t
{
	int32_t handle;
	uint64_t offset;
};

struct MSGPACK fileexplorer_readRequest_t
{
	int32_t handle;
	int32_t count;
};

struct MSGPACK fileexplorer_readResponse_t
{
	int32_t count;
	uint8_t data[];
};

struct MSGPACK fileexplorer_writeRequest_t
{
	int32_t handle;
	int32_t count;
	uint8_t data[];
};

struct MSGPACK fileexplorer_getdentsRequest_t
{
	uint16_t length;
	char path[];
};

struct MSGPACK fileexplorer_dent_t
{
	// File id
	uint32_t fileno;

	// Record length
	uint16_t reclen;

	// Dent type
	uint8_t type;

	// Name length
	uint8_t namlen;

	// Name of the file, length provided by namlen
	char name[];
};

// The protocol here is on request, send back at least one of these
struct MSGPACK fileexplorer_getdentsResponse_t
{
	// The total file count, if 0 there are no dents to process
	uint64_t totalDentCount;
};

struct MSGPACK fileexplorer_statRequest_t
{
	// opened file handle, -1 if checking by path
	int32_t handle;

	// path length, if 0, handle is used, if handle is -1 and this is 0, error
	uint16_t pathLength;

	// path of the stat request
	char path[];
};

struct MSGPACK fileexplorer_stat_t
{
	uint32_t   st_dev;		/* inode's device */
	uint32_t	  st_ino;		/* inode's number */
	uint16_t	  st_mode;		/* inode protection mode */
	uint16_t	  st_nlink;		/* number of hard links */
	uint32_t	  st_uid;		/* user ID of the file's owner */
	uint32_t	  st_gid;		/* group ID of the file's group */
	uint32_t   st_rdev;		/* device type */
	struct	timespec st_atim;	/* time of last access */
	struct	timespec st_mtim;	/* time of last data modification */
	struct	timespec st_ctim;	/* time of last file status change */
	int64_t	  st_size;		/* file size, in bytes */
	int64_t st_blocks;		/* blocks allocated for file */
	uint32_t st_blksize;		/* optimal blocksize for I/O */
	uint32_t  st_flags;		/* user defined flags for file */
	uint32_t st_gen;		/* file generation number */
	int32_t st_lspare;
	struct timespec st_birthtim;	/* time of file creation */
};

struct MSGPACK fileexplorer_mkdirRequest_t
{
	int32_t mode;
	uint16_t pathLength;
	char path[];
};

struct MSGPACK fileexplorer_rmdirRequest_t
{
	uint16_t pathLength;
	char path[];
};

struct MSGPACK fileexplorer_unlinkRequest_t
{
	uint16_t pathLength;
	char path[];
};