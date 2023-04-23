// Plugin to fix 32bit Largefile Stat Calls Caused by libfreetype and basically anything else from the 32bit days.
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <PIUTools_SDK.h>

typedef int(*fxstat_t)(int ver, int fildes, struct stat *stat_buf);
typedef int(*xstat_t)(int ver, const char *path, struct stat *stat_buf);
typedef int(*lxstat_t)(int ver, const char *path, struct stat *stat_buf);

fxstat_t next_fxstat;
xstat_t next_xstat;
lxstat_t next_lxstat;

static int fix_fxstat(int ver, int fildes, struct stat *stat_buf){
    int res = next_fxstat(ver,fildes,stat_buf);
    if(res == -1 && ver == 3 && errno == EOVERFLOW){      
        struct stat64 st;
        res = fstat64(fildes, &st);
        if (res != -1) {
            stat_buf->st_dev = st.st_dev;
            stat_buf->st_ino = st.st_ino;
            stat_buf->st_mode = st.st_mode;
            stat_buf->st_nlink = st.st_nlink;
            stat_buf->st_uid = st.st_uid;
            stat_buf->st_gid = st.st_gid;
            stat_buf->st_rdev = st.st_rdev;
            stat_buf->st_size = st.st_size;
            stat_buf->st_blksize = st.st_blksize;
            stat_buf->st_blocks = st.st_blocks;
            stat_buf->st_atime = st.st_atime;
            stat_buf->st_mtime = st.st_mtime;
            stat_buf->st_ctime = st.st_ctime;
        }         
    }
    return res;
}

static int fix_xstat(int ver, const char *path, struct stat *stat_buf){
    int res = next_xstat(ver, path, stat_buf);
    if(res == -1 && ver == 3 && errno == EOVERFLOW){
        struct stat64 st;
        res = stat64(path, &st);
        if (res != -1) {
            stat_buf->st_dev = st.st_dev;
            stat_buf->st_ino = st.st_ino;
            stat_buf->st_mode = st.st_mode;
            stat_buf->st_nlink = st.st_nlink;
            stat_buf->st_uid = st.st_uid;
            stat_buf->st_gid = st.st_gid;
            stat_buf->st_rdev = st.st_rdev;
            stat_buf->st_size = st.st_size;
            stat_buf->st_blksize = st.st_blksize;
            stat_buf->st_blocks = st.st_blocks;
            stat_buf->st_atime = st.st_atime;
            stat_buf->st_mtime = st.st_mtime;
            stat_buf->st_ctime = st.st_ctime;
        }         
    }
    return res;
}

static int fix_lxstat(int ver, const char *path, struct stat *stat_buf){
    int res = next_lxstat(ver, path, stat_buf);
    if(res == -1 && ver == 3 && errno == EOVERFLOW){
        struct stat64 st;
        res = lstat64(path, &st);
        if (res != -1) {
            stat_buf->st_dev = st.st_dev;
            stat_buf->st_ino = st.st_ino;
            stat_buf->st_mode = st.st_mode;
            stat_buf->st_nlink = st.st_nlink;
            stat_buf->st_uid = st.st_uid;
            stat_buf->st_gid = st.st_gid;
            stat_buf->st_rdev = st.st_rdev;
            stat_buf->st_size = st.st_size;
            stat_buf->st_blksize = st.st_blksize;
            stat_buf->st_blocks = st.st_blocks;
            stat_buf->st_atime = st.st_atime;
            stat_buf->st_mtime = st.st_mtime;
            stat_buf->st_ctime = st.st_ctime;
        }         
    }
    return res;
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","__fxstat", fix_fxstat,&next_fxstat, 1),
HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "__xstat", fix_xstat, &next_xstat, 1),
HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "__lxstat", fix_lxstat, &next_lxstat, 1),
{}
};

const PHookEntry plugin_init(void){
DBG_printf("[%s] 64bit stat() fix Enabled.",__FILE__);
return entries;
}
