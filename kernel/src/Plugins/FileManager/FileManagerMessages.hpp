#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
	namespace Plugins
	{
		namespace FileManagerExtent
		{
			enum 
			{
				MaxPathLength = 0x400,
				MaxNameLength = 0xFF,
				MaxEchoLength = 0x100,
				MaxBufferLength = 0x4000,
			};

			typedef enum _Commands
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
				FileManager_DecryptSelf = 0xEA9BF6A9
			} Commands;

			typedef struct MSGPACK  _EmptyPayload
			{
				// This must remain empty, aka sizeof(EmptyPayload) == 0
			} EmptyPayload;
			static_assert(sizeof(EmptyPayload) <= 1);

			typedef struct MSGPACK  _EchoRequest
			{
				uint16_t Length;
				char Message[MaxEchoLength];
			} EchoRequest;

			typedef struct MSGPACK  _OpenRequest
			{
				int32_t Flags;
				int32_t Mode;
				uint16_t PathLength;
				char Path[MaxPathLength];
			} OpenRequest;

			typedef struct MSGPACK  _CloseRequest
			{
				int32_t Handle;
			} CloseRequest;

			typedef struct MSGPACK  _SeekRequest
			{
				int32_t Handle;
				uint64_t Offset;
			} SeekRequest;

			typedef struct MSGPACK  _ReadRequest
			{
				int32_t Handle;
				int32_t Count;
			} ReadRequest;

			typedef struct MSGPACK  _ReadResponse
			{
				int16_t Count;
				uint8_t Buffer[MaxBufferLength];
			} ReadResponse;

			typedef struct MSGPACK  _WriteRequest
			{
				int32_t Handle;
				int32_t Count;
				uint8_t Buffer[MaxBufferLength];
			} WriteRequest;

			typedef struct MSGPACK _WriteResponse
			{
				// Empty, response is returned in header
			} WriteResponse;

			typedef struct MSGPACK  _Dent
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
				char name[MaxNameLength];
			} Dent;

			typedef struct MSGPACK  _GetDentsRequest
			{
				int16_t PathLength;
				char Path[MaxPathLength];
			} GetDentsRequest;

			typedef struct MSGPACK  _GetDentsResponse
			{
				uint64_t DentIndex;
				Dent Info;
			} GetDentsResponse;

			typedef struct MSGPACK _StatRequest
			{
				// opened file handle, -1 if checking by path
				int32_t Handle;

				// path length, if 0, handle is used, if handle is -1 and this is 0, error
				int16_t PathLength;

				// path of the stat request
				char Path[MaxPathLength];
			} StatRequest;

			typedef struct MSGPACK  _StatResponse
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
			} StatResponse;

			typedef struct MSGPACK  _MkdirRequest
			{
				int32_t Mode;
				uint16_t PathLength;
				char Path[MaxPathLength];
			} MkdirRequest;

			typedef struct MSGPACK  _RmDirRequest
			{
				int16_t PathLength;
				char Path[MaxPathLength];
			} RmDirRequest;

			typedef struct MSGPACK  _UnlinkRequest
			{
				uint16_t PathLength;
				char Path[MaxPathLength];
			} UnlinkRequest;

			typedef struct MSGPACK _DecryptSelfRequest
			{
				int16_t PathLength;
				char Path[MaxPathLength];
			} DecryptSelfRequest;

			typedef struct MSGPACK _DecryptSelfResponse
			{
				uint64_t Index;
				uint64_t Offset;
				uint64_t Length;
				uint8_t Data[MaxBufferLength];
			} DecryptSelfResponse;
		}
	}
}