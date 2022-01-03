#include "kdl.h"

#define LOG_ERROR(fmt, ...) do {  if (_DEBUG) { PS4GDB_kprintf(); kprintf("[%s] ERROR - " fmt, __FUNCTION__, ##__VA_ARGS__);  } } while (0)
#define LOG_DBG(fmt, ...) do {  if (_DEBUG) {PS4GDB_kprintf(); kprintf("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__); }} while (0)
#define LOG_SIMPLE(fmt, ...) do {  if (_DEBUG) {PS4GDB_kprintf(); kprintf("%s" fmt, "", ##__VA_ARGS__); }} while (0)