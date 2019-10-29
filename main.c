#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

/* AUTHOR Matej Kovar
 *
 * Program, which calculates size of each directory and file in PATH, which is given in input.
 * Directories and files are written in specific format -> tree.
 * There are several OPTIONS how to write size of directories and files.
 * This is how arguments should look -> "./dt [OPTIONS] PATH", where PATH is path to directory (or file)
 * OPTIONS :
 *      "-a" : calculates size with actual size of file instead of real size allocated on disk (allocated blocks * 512)
 *      "-s" : sorts files or directories in directory based on size, not alphabetical
 *      "-p" : writes percentage usage of disk instead of size
 *      "-d NUMBER" : writes only directories and files in maximum depth of NUMBER
 *
 * You can combine these options (where it makes sense) -> for example "-p -s -d 3 PATH"
 */

struct options {
    int a, s, d, p, number;
};

struct File {
    char name[256];
    double size;
    size_t len;
    size_t max;
    int isdir;
    int isError;
    struct File *files;
};

int check_arg(const char *argv, struct options *opt){
    if (argv[0] != '-') {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }
    switch (argv[1]) {
        case 'a':
            if (opt->a == 1){
                fprintf(stderr, "Invalid arguments\n");
                return -1;
            }
            opt->a = 1;
            break;
        case 's':
            if (opt->s == 1){
                fprintf(stderr, "Invalid arguments\n");
                return -1;
            }
            opt->s = 1;
            break;
        case 'p':
            if (opt->p == 1){
                fprintf(stderr, "Invalid arguments\n");
                return -1;
            }
            opt->p = 1;
            break;
        case 'd':
            if (opt->d == 1){
                fprintf(stderr, "Invalid arguments\n");
                return -1;
            }
            opt->d = 1;
            break;
        default:
            fprintf(stderr, "Invalid arguments\n");
            return -1;
    }
    return 0;
}

void freeAll(struct File *file)
{
    for (size_t i = 0; i < file->len; i++) {
        if (file->files[i].files != NULL) {
            freeAll(&(file->files[i]));
        }
    }
    free(file->files);
}

static int initialize_opt(struct options *opt) {
    opt->s = 0; opt->a = 0; opt->d = 0; opt->number = -1; opt->p = 0;
    return 0;
}

static int initialize(struct File *file) {
    file->files = NULL;
    file->len = 0;
    file->max = 8;
    strcpy(file->name, "\0");
    file->size = 0;
    file->isdir = 0;
    file->isError = 0;
    return 0;
}

int nameCmp(const void *elem1, const void *elem2)
{
    struct File file1 = *((struct File *) elem1);
    struct File file2 = *((struct File *) elem2);
    size_t min = 0;

    if (strlen(file1.name) <= strlen(file2.name)) {
        min = strlen(file1.name);
    } else {
        min = strlen(file2.name);
    }

    for (size_t i = 0; i <= min; i++) {
        if (tolower(file1.name[i]) < tolower(file2.name[i])) {
            return -1;
        }
        if (tolower(file1.name[i]) > tolower(file2.name[i])) {
            return 1;
        }
    }

    for (size_t i = 0; i <= min; i++) {
        if (file1.name[i] < file2.name[i]) {
            return -1;
        }
        if (file1.name[i] > file2.name[i]) {
            return 1;
        }
    }
    return 0;
}

int sizeCmp(const void *elem1, const void *elem2)
{
    struct File file1 = *((struct File *) elem1);
    struct File file2 = *((struct File *) elem2);

    if (file1.size > file2.size)
        return -1;
    if (file1.size < file2.size)
        return 1;
    return nameCmp(elem1, elem2);
}

size_t fullLength(struct File *file, size_t depth)
{
    size_t full = depth, new = 0;
    for (size_t i = 0; i < file->len; i++) {
        if (file->files[i].len != 0) {
            new = fullLength(&(file->files[i]), depth + 1);
            if (new > full) {
                full = new;
            }
        }
    }
    return full;
}

const char *get_sufix(int x) {
    const char *a[6][5];
    *a[0] = " B  "; *a[1] = " KiB"; *a[2] = " MiB"; *a[3] = " GiB"; *a[4] = " TiB"; *a[5] = " PiB";
    switch (x) {
        case 20:
            return *a[1];
        case 30:
            return *a[2];
        case 40:
            return *a[3];
        case 50:
            return *a[4];
        case 60:
            return *a[5];
        default:
            return *a[0];
    }
}

double which_power(struct File *file) {
    if (file->size < 1024) {
        return 10;
    }
    if (file->size < pow(2, 20)) {
        return 20;
    }
    if (file->size < pow(2, 30)) {
        return 30;
    }
    if (file->size < pow(2, 40)) {
        return 40;
    }
    if (file->size < pow(2, 50)) {
        return 50;
    }
    return 60;
}

void print_size(const double mainSize, struct File *file_son, struct options *opt) {
    double power = which_power(file_son);
    if (opt->p == 1) {
        double size = 100 * (file_son->size / mainSize);
        size = trunc(size * 10) / 10.0;
        printf("%5.1f", size);
    } else {
        double size = file_son->size / (pow(2, power - 10));
        size = trunc(size * 10) / 10.0;
        printf("%6.1f", size);
    }
}

void print_sufix(struct File *file, struct options *opt) {
    if (opt->p == 1) {
        printf("%% ");
    } else {
        int power = (int) which_power(file);
        const char *sufix = get_sufix(power);
        printf("%s ", sufix);
    }
}

void print_tree_prefix(char *prefix, const size_t depth, size_t *last_files, int isLast) {
    if (isLast) {
        last_files[depth] = 1;
        strcpy(prefix + (depth - 1) * 4, "\\-- ");
    } else {
        last_files[depth] = 0;
        strcpy(prefix + (depth - 1) * 4, "|-- ");
    }
    printf("%s", prefix);
}

void print_error(struct File *file) {
    if (file->isError != 0) {
        printf("? ");
    } else {
        printf("%*c", 2, ' ');
    }
}

void print_files(struct File *file_son, struct File *file_father, const double mainSize, struct options *opt, const size_t depth, const int error, size_t *last_files) {
    size_t i = 0;
    qsort(file_father->files, file_father->len, sizeof(struct File), nameCmp);
    if (opt->s == 1) {
        qsort(file_father->files, file_father->len, sizeof(struct File), sizeCmp);
    }
    char prefix[depth * 4 + 1];
    strcpy(prefix, "");
    for (size_t j = 1; j < depth; j++) {
        if (last_files[j]) {
            strcat(prefix, "    ");
        } else {
            strcat(prefix, "|   ");
        }
    }

    while (i < file_father->len) {
        if (error != 0) {
            print_error(file_son);
        }
        print_size(mainSize, file_son, opt);
        print_sufix(file_son, opt);
        if (i + 1 == file_father->len) {
            print_tree_prefix(prefix, depth, last_files, 1);
        } else {
            print_tree_prefix(prefix, depth, last_files, 0);
        }
        printf("%s \n", file_son->name);
        if (file_son->isdir) {
            if (depth < (size_t)opt->number){
                print_files(file_son->files, file_son, mainSize, opt, depth + 1, error, last_files);
            }
        }

        file_son = file_father->files + i + 1;
        i++;
    }
}

// File is written in specific format -> {ERROR}{SIZE}{SUFFIX} {TREE_PREFIX} {FILE_NAME}
void print_file(struct File *file, struct options *opt, const int error, size_t *last_files) {
    if (error != 0) {
        print_error(file);
    }
    print_size(file->size, file, opt);
    print_sufix(file, opt);
    printf("%s \n", file->name);
    if (file->len > 0 && opt->number != 0) {
        print_files(file->files, file, file->size, opt, 1, error, last_files);
    }
}

void append_slash(char *buffer)
{
    if (buffer[strlen(buffer) - 1] != '/') {
        strcat(buffer, "/");
    }
}

// Function which reads directory/files
double read_directory(char *path, struct File *file, int type_of_size) { // 1 == size, 0 == blocks
    DIR *dir = NULL;
    int isError = 0;
    double nextSize = 0;

    char dir_path[257 + strlen(path)];
    strcpy(dir_path, path);
    append_slash(dir_path);

    file->files = malloc(file->max * sizeof(struct File));
    if (file->files == NULL) {
        fprintf(stderr, "Allocation failed\n");
        return -1;
    }
    if ((dir = opendir(path))) {    // connect to directory
        struct dirent *dirEntry = NULL;
        while ((dirEntry = readdir(dir)) != NULL) {     // obtain next item
            if (strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {
                struct stat st;

                strcpy(dir_path + strlen(path) + 1, dirEntry->d_name);
                if (stat(dir_path, &st) != 0) {
                    perror(dir_path);
                    isError = 1;
                    continue;
                }

                if (file->len == file->max) {
                    file->max *= 2;
                    file->files = realloc(file->files, file->max * sizeof(struct File));
                    if(file->files == NULL){
                        fprintf(stderr, "Failed to reallocate\n");
                        return -1;
                    }
                }
                struct File f;
                initialize(&f);

                if (S_ISREG(st.st_mode)) {
                    strcpy(f.name, dirEntry->d_name);
                    f.isdir = 0;
                    f.isError = isError;
                    if (type_of_size == 1) {
                        f.size = st.st_size;
                    } else {
                        f.size = st.st_blocks * 512;
                    }
                    file->size += f.size;
                    file->files[file->len] = f;
                    file->len++;
                }

                if (S_ISDIR(st.st_mode)) {
                    strcpy(f.name, dirEntry->d_name);
                    f.isdir = 1;
                    f.isError = isError;
                    if (type_of_size == 1) {
                        f.size = st.st_size;
                    } else {
                        f.size = st.st_blocks * 512;
                    }
                    nextSize = read_directory(dir_path, &f, type_of_size);
                    if (nextSize == -1) {
                        return -1;
                    }
                    file->size += nextSize;
                    if (f.isError == 1) {
                        file->isError = 1;
                    }
                    file->files[file->len] = f;
                    file->len++;
                }
            }
        }
        closedir(dir);  // finish work with directory
    } else {
        perror(path);
        file->isError = 1;
    }
    return file->size;
}

int strToInt(char *number)
{
    size_t lenOfString = strlen(number);
    int finalNum = 0;
    for (size_t i = 0; i < lenOfString; i++) {
        if (number[i] < '0' || number[i] > '9') {
            return -1;
        }
        finalNum *= 10;
        finalNum += number[i] - '0';
    }
    return finalNum;
}

int main(int argc, char *argv[])
{
    struct options opt;
    initialize_opt(&opt);
    if (argc == 0) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }
    for (int i = 1; i < argc - 1; i++) {
        if (check_arg(argv[i], &opt) != 0) {
            return -1;
        }
        if (opt.d == 1) {
            opt.number = strToInt(argv[i + 1]);
            if (opt.number == -1) {
                fprintf(stderr, "Invalid number argument\n");
                return -1;
            }
            i++;
        }
    }

    if(opt.d == 1) {
        if (opt.number < 0) {
            fprintf(stderr, "Invalid number\n");
            return -1;
        }
    } else {
        opt.number = INT_MAX;
    }
    struct File file1;
    struct stat st_file;

    if (stat(argv[argc - 1], &st_file) != 0) {
        perror(argv[argc - 1]);
        return -1;
    }
    initialize(&file1);
    strcpy(file1.name, argv[argc - 1]);

    size_t full = 0;
    full = fullLength(&file1, 1);
    size_t last_files[full + 1];
    for (size_t i = 0; i <= full; i++) {
        last_files[i] = 0;
    }

    if (S_ISDIR(st_file.st_mode)) {
        file1.isdir = 1;
        if (opt.a == 1) {
            file1.size = st_file.st_size;
        } else {
            file1.size = st_file.st_blocks * 512;
        }
        if (read_directory(argv[argc - 1], &file1, opt.a) == -1) {
            freeAll(&file1);
            return -1;
        }
        print_file(&file1, &opt, file1.isError, last_files);
    }
    if (S_ISREG(st_file.st_mode)) {
        file1.isdir = 0;
        if (opt.a == 1) {
            file1.size = st_file.st_size;
        } else {
            file1.size = st_file.st_blocks * 512;
        }
        print_file(&file1, &opt, file1.isError, last_files);
    }
    freeAll(&file1);
    return 0;
}
