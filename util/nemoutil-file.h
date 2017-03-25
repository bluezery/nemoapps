#ifndef __UTIL_FILE_H__
#define __UTIL_FILE_H__

#include <nemoutil-list.h>

#ifdef __cplusplus
extern "C" {
#endif

char *file_get_magic(const char *path, int flags);
bool file_is_exist(const char *path);
bool file_is_null(const char *path);
bool file_is_dir(const char *path);
bool file_is_exec(const char *path);
bool file_is_image(const char *path);
bool file_is_svg(const char *path);
bool file_is_video(const char *path);
bool file_is_text(const char *path);
bool file_is_pdf(const char *path);
bool file_get_image_wh(const char *path, int *w, int *h);
const char *file_get_homedir();
char *file_get_updir(const char *path);
bool file_mkdir(const char *dir, int mode);
bool file_mkdir_recursive(const char *path, int mode);
void file_exec(const char *filename, ...);
char *file_load(const char *path, size_t *sz);
char **file_read_line(const char *path, int *line_len);
char *file_get_ext(const char *path);
char *file_get_basename(const char *path);
char *file_get_dirname(const char *path);

typedef struct _FileInfo FileInfo;
struct _FileInfo {
    bool is_dir;
    char *path;
    char *name;
    char *ext;
    char *magic_str;
};

void fileinfo_destroy(FileInfo *fileinfo);
FileInfo *fileinfo_create(bool is_dir, const char *path, const char *name);
List *fileinfo_readdir(const char *path);
List *fileinfo_readdir_sorted(const char *path);
bool fileinfo_is_image(FileInfo *fileinfo);
bool fileinfo_is_svg(FileInfo *fileinfo);
bool fileinfo_is_video(FileInfo *fileinfo);
bool fileinfo_is_text(FileInfo *fileinfo);
bool fileinfo_is_pdf(FileInfo *fileinfo);
bool fileinfo_is_dir(FileInfo *fileinfo);
bool fileinfo_is_exec(FileInfo *fileinfo);

#ifdef __cplusplus
}
#endif

#endif
