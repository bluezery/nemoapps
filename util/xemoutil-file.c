#include "xemoutil-internal.h"
#include "xemoutil.h"
#include "xemoutil-list.h"

/**********************************************************/
/* file */
/**********************************************************/
const char *MAGIC_IMAGES[] =
{
    "PNG image data",
    "JPEG image data",
    "GIF image data",
    "TIFF image data"
};

const char *MAGIC_SVGS[] =
{
    "SVG Scalable Vector Graphics image",
};

const char *MAGIC_VIDEOS[] =
{
    "RIFF (little-endian) data, AVI",
    "ISO Media, MP4",
    "Microsoft ASF",
    "Matroska data",
    "ISO Media, Apple QuickTime movie",
    "WebM",
    "MPEG sequence",
    "Macromedia Flash Video",
    "Macromedia Flash Data, version 6"
};

const char *MAGIC_PDFS[] =
{
    "PDF document",
};

const char *MAGIC_TEXTS[] =
{
    "ASCII text",
};

const char *MAGIC_EXECS[] =
{
    "ELF 64-bit LSB executable",
};

const char *MAGIC_URLS[] =
{
    "Internet shortcut text",
};

char *file_get_magic(const char *path, int flags)
{
    magic_t _magic = magic_open(MAGIC_SYMLINK | flags);
    if (!_magic) {
        ERR("magic open failed: %s", path);
        return NULL;
    }
    if (magic_load(_magic, NULL) == -1) {
        ERR("magic load failed: %s", path);
        return NULL;
    }
    const char *str = magic_file(_magic, path);
    if (!str) {
        ERR("magic file failed: %s", path);
        return NULL;
    }
    char *magic_str = strdup(str);
    magic_close(_magic);
    return magic_str;
}

char *file_get_basename(const char *path)
{
    char *tmp = strdup(path);
    char *name = strdup(basename(tmp));
    free(tmp);
    return name;
}

char *file_get_dirname(const char *path)
{
    char *tmp = strdup(path);
    char *name = strdup(dirname(tmp));
    free(tmp);
    return name;
}

static bool _path_check_magic(const char *magic_str, const char **magics, int size)
{
    RET_IF(!magic_str, false);
    RET_IF(!magics || !magics[0], false);

    int i = 0;
    for (i = 0; i < size ; i++) {
        if (strstr(magic_str, magics[i]))
            return true;
    }
    return false;
}

bool file_is_exist(const char *path)
{
    if (!path) return false;
    struct stat st;
    int r = stat(path, &st);
    if (r) return false;
    return true;
}

bool file_is_null(const char *path)
{
    if (!path) return false;
    struct stat st;
    int r = stat(path, &st);
    if (r) return true;
    if (st.st_size > 0) return false;
    return true;
}

bool file_is_dir(const char *path)
{
    if (!path) return false;
    struct stat st;
    int r = stat(path, &st);
    if (r) {
        LOG("%s:%s", strerror(errno), path);
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool file_is_exec(const char *path)
{
    if (!path) return false;
    struct stat st;
    int r = stat(path, &st);
    if (r) return false;
    if ((st.st_mode & S_IXUSR) == S_IXUSR) return true;
    return false;
}

bool file_is_image(const char *path)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    bool check = _path_check_magic(magic_str, MAGIC_IMAGES,
            sizeof(MAGIC_IMAGES)/sizeof(MAGIC_IMAGES[0]));
    free(magic_str);
    return check;
}

bool file_is_svg(const char *path)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    bool check = _path_check_magic(magic_str, MAGIC_SVGS,
            sizeof(MAGIC_SVGS)/sizeof(MAGIC_SVGS[0]));
    free(magic_str);
    return check;
}

bool file_is_video(const char *path)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    bool check = _path_check_magic(magic_str, MAGIC_VIDEOS,
            sizeof(MAGIC_VIDEOS)/sizeof(MAGIC_VIDEOS[0]));
    free(magic_str);
    return check;
}

bool file_is_text(const char *path)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    bool check = _path_check_magic(magic_str, MAGIC_TEXTS,
            sizeof(MAGIC_TEXTS)/sizeof(MAGIC_TEXTS[0]));
    free(magic_str);
    return check;
}

bool file_is_pdf(const char *path)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    bool check = _path_check_magic(magic_str, MAGIC_PDFS,
            sizeof(MAGIC_PDFS)/sizeof(MAGIC_PDFS[0]));
    free(magic_str);
    return check;
}

bool file_get_image_wh(const char *path, int *w, int *h)
{
    if (!path) return false;
    char *magic_str = file_get_magic(path, 0);
    RET_IF(!magic_str, false);

    if (!_path_check_magic(magic_str, MAGIC_IMAGES,
            sizeof(MAGIC_IMAGES)/sizeof(MAGIC_IMAGES[0])))  {
        ERR("None image file path: %s", path);
        free(magic_str);
        return false;
    }

    int ret;
    regex_t reg;
    regmatch_t match[3];

    // JPEG
    regcomp(&reg, ",[ ]([0-9]+)x([0-9]+)", REG_EXTENDED);
    ret = regexec(&reg, magic_str, 3, match, 0);

    if (ret != 0) {
        // PNG/GIF
        regcomp(&reg, ",[ ]([0-9]+)[ ]x[ ]([0-9]+)", REG_EXTENDED);
        ret = regexec(&reg, magic_str, 3, match, 0);
    }
    if (ret == 0) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%.*s",
                match[1].rm_eo - match[1].rm_so,
                magic_str + match[1].rm_so);
        if (w) *w = atoi(buf);
        snprintf(buf, PATH_MAX, "%.*s",
                match[2].rm_eo - match[2].rm_so,
                magic_str + match[2].rm_so);
        if (h) *h = atoi(buf);
    } else {
        if (ret == REG_NOMATCH) {
            ERR("No matched widthxheight: %s", path);
        } else {
            char buf[PATH_MAX];
            regerror(ret, &reg, buf, PATH_MAX - 1);
            ERR("regexec failed: %s\n", buf);
        }
        free(magic_str);
        return false;
    }
    free(magic_str);
    return true;
}

const char *file_get_homedir()
{
    const char *homedir = getenv("HOME");
    if (!homedir)
        homedir = getpwuid(getuid())->pw_dir;
    return homedir;
}

char *file_get_updir(const char *path)
{
    RET_IF(!path, NULL);

    char *temp = strdup(path);
    char *p = temp + strlen(temp) - 1;

    if (p == temp) return temp;

    if (*p == '/') {
        *p='\0';
        p--;
    }
    while (p > temp) {
        if (*p == '/') {
            *p = '\0';
            break;
        }
        p--;
    }

    if (p == temp) {
        free(temp);
        return strdup("/");
    }

    return temp;
}

bool file_mkdir(const char *path, int mode)
{
    RET_IF(!path, false);

    if (file_is_dir(path)) {
        LOG("already exists: %s", path);
        return true;
    }

    if (file_is_exist(path)) {
        ERR("already exist but not directory: %s", path);
        return false;
    }

    struct stat st;
    int r = stat(path, &st);
    if (r == 0) {
        ERR("Should not be here??: %s", path);
        return false;
    }
    if (errno != ENOENT) {
        LOG("%s: %s", strerror(errno), path);
        return false;
    }

    int ret = mkdir(path, 0777);
    if (ret) {
        ERR("mkdir failed (%s): %s", path, strerror(errno));
        return false;
    }

    if (mode != -1) {
        ret = chmod(path, mode);
        if (ret) {
            ERR("chmod failed: %s: %s(%d)", strerror(errno), path, mode);
        }
    }
    return true;
}

bool file_mkdir_recursive(const char *path, int mode)
{
    RET_IF(!path, false);

    if (file_is_dir(path)) {
        //LOG("already exists: %s", path);
        return true;
    }
    if (file_is_exist(path)) {
        ERR("already exist but not directory: %s", path);
        return false;
    }

    struct stat st;
    int r = stat(path, &st);
    if (r == 0) {
        ERR("Should not be here??: %s", path);
        return false;
    }
    if (errno != ENOENT) {
        LOG("%s: %s", strerror(errno), path);
        return false;
    }

    char *tmp = strdup(path);
    char *parent = dirname(tmp);
    if (parent && strcmp(parent, ".") && strcmp(parent, "/")) {
        bool ret = file_mkdir_recursive(parent, mode | 0300);
        if (!ret) {
            free(tmp);
            return false;
        }
    }
    free(tmp);

    int ret = mkdir(path, 0777);
    if (ret) {
        ERR("mkdir failed (%s): %s", path, strerror(errno));
        return false;
    }

    if (mode != -1) {
        ret = chmod(path, mode);
        if (ret) {
            ERR("chmod failed (%s) (%d): %s", path, mode, strerror(errno));
            return false;
        }
    }
    return true;
}

void file_exec(const char *filename, ...)
{
    RET_IF(!filename);

    int num = 1;
    char **params = malloc(sizeof(char *) * 1);
    params[0] = strdup(filename);

    va_list ap;
    va_start(ap, filename);
    const char *param;
    while ((param = va_arg(ap, const char *))) {
        params = realloc(params, sizeof(char *) * (num + 1));
        params[num] = strdup(param);
        num++;
    }
    va_end(ap);

    int i = 0;
    params = realloc(params, sizeof(char *) * (num + 1));
    params[num] = NULL;

    pid_t pid = fork();
    if (pid > 0) {
        int ret = execvp(filename, params);
        if (ret < 0) {
            ERR("execvp(%s) failed: %s", filename, strerror(errno));
            exit(-1);
        }
    } else if (pid < 0) {
        ERR("fork failed: %s", strerror(errno));
    }

    for (i = 0 ; i < num ; i++) {
        free(params[i]);
    }
    free(params);
}

char *file_load(const char *path, size_t *sz)
{
    char *buf;

    FILE *fp;
    long len;
    size_t size;
    fp = fopen(path, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);

    buf = calloc(sizeof(char), len + 1);
    size = fread(buf, 1, len, fp);
    if (size != len) {
        ERR("fread error\n");
        free(buf);
        return NULL;
    }

    fclose(fp);

    if (sz) *sz = size;
    return buf;
}

char **file_read_line(const char *path, int *line_len)
{
    FILE *fp;
    char **line = NULL;
    int idx = 0;
    char *buffer = NULL;
    size_t buffer_len;

    RET_IF(!path || !line_len, NULL);

    fp = fopen(path, "r");
    if (!fp) {
        ERR("%s", strerror(errno));
        return NULL;
    }

    buffer_len = 4096; // adequate size for normal file case.
    buffer = (char *)malloc(sizeof(char) * buffer_len);

    while (getline(&buffer, &buffer_len, fp) >= 0) {
        line = (char **)realloc(line, sizeof(char *) * (idx + 1));
        line[idx] = strdup(buffer);
        idx++;
    }
    *line_len = idx;

    free(buffer);
    fclose(fp);
    return line;
}

char *file_get_ext(const char *path){
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path || (dot + 1) == '\0' || (dot + 1) == NULL )
        return NULL;

    return strdup(dot+1);
}

void fileinfo_destroy(FileInfo *fileinfo)
{
    free(fileinfo->path);
    free(fileinfo->name);
    if (fileinfo->ext) free(fileinfo->ext);
    if (fileinfo->magic_str) free(fileinfo->magic_str);
    free(fileinfo);
}

FileInfo *fileinfo_create(bool is_dir, const char *path, const char *name)
{
    RET_IF(!path | !name, NULL);
    FileInfo *fileinfo = calloc(sizeof(FileInfo), 1);
    fileinfo->is_dir = is_dir;
    fileinfo->path = strdup(path);
    fileinfo->name = strdup(name);
    if (!is_dir) fileinfo->ext = file_get_ext(path);
    fileinfo->magic_str = file_get_magic(fileinfo->path, 0);

    return fileinfo;
}

List *fileinfo_readdir(const char *path)
{
    RET_IF(!path, NULL);

    DIR *dir = opendir(path);
    if (!dir) {
        ERR("opendir (%s) failed: %s", path, strerror(errno));
        return NULL;
    }

    char *rpath = realpath(path, NULL);
    List *files = NULL;
    FileInfo *fileinfo;

    struct dirent *dr;
    while ((dr = readdir(dir))) {
        if (!dr->d_name) continue;
        if ((dr->d_name)[0] == '.') continue;

        char _path[PATH_MAX];
        snprintf(_path, PATH_MAX, "%s/%s", rpath, dr->d_name);

        bool is_dir = false;
        if (dr->d_type == DT_DIR) {
            is_dir = true;
        }
        fileinfo = fileinfo_create(is_dir, _path, dr->d_name);
        files = list_append(files, fileinfo);
    }
    free(rpath);
    closedir(dir);

    return files;
}

List *fileinfo_readdir_sorted(const char *path)
{
    RET_IF(!path, NULL);

    DIR *dir = opendir(path);
    if (!dir) {
        ERR("opendir (%s) failed: %s", path, strerror(errno));
        return NULL;
    }

    char *rpath = realpath(path, NULL);
    List *files = NULL;

    struct dirent *dr;
    while ((dr = readdir(dir))) {
        if (!dr->d_name) continue;
        if ((dr->d_name)[0] == '.') continue;

        char child_path[PATH_MAX];
        snprintf(child_path, PATH_MAX, "%s/%s", rpath, dr->d_name);

        bool is_dir = false;
        if (dr->d_type == DT_DIR) {
            is_dir = true;
        }
        FileInfo *fileinfo = fileinfo_create(is_dir, child_path, dr->d_name);

        if (!files) {
            files = list_append(files, fileinfo);
        }

        List *l;
        FileInfo *_fileinfo;
        LIST_FOR_EACH(files, l, _fileinfo) {
            if (strcmp(_fileinfo->path, fileinfo->path) > 0) {
                files = list_insert_before(l, fileinfo);
                break;
            } else if (LIST_LAST(files) == l) {
                files = list_insert_after(l, fileinfo);
                break;
            }
        }
    }
    free(rpath);
    closedir(dir);

    return files;
}

bool fileinfo_is_image(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_IMAGES,
            sizeof(MAGIC_IMAGES)/sizeof(MAGIC_IMAGES[0]));
}

bool fileinfo_is_svg(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_SVGS,
            sizeof(MAGIC_SVGS)/sizeof(MAGIC_SVGS[0]));
}

bool fileinfo_is_video(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_VIDEOS,
            sizeof(MAGIC_VIDEOS)/sizeof(MAGIC_VIDEOS[0]));
}

bool fileinfo_is_text(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_TEXTS,
            sizeof(MAGIC_TEXTS)/sizeof(MAGIC_TEXTS[0]));
}

bool fileinfo_is_pdf(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_PDFS,
            sizeof(MAGIC_PDFS)/sizeof(MAGIC_PDFS[0]));
}

bool fileinfo_is_dir(FileInfo *fileinfo)
{
    return fileinfo->is_dir;
}

bool fileinfo_is_exec(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_EXECS,
            sizeof(MAGIC_EXECS)/sizeof(MAGIC_EXECS[0]));
}

bool fileinfo_is_url(FileInfo *fileinfo)
{
    return _path_check_magic(fileinfo->magic_str, MAGIC_URLS,
            sizeof(MAGIC_URLS)/sizeof(MAGIC_URLS[0]));
}

