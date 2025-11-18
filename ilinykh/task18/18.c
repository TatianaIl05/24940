#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <locale.h>

const char* get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return pw->pw_name;
    }
    return "?"; 
}

const char* get_groupname(gid_t gid) {
    struct group *gr = getgrgid(gid);
    if (gr) {
        return gr->gr_name;
    }
    return "?"; 
}

const char* get_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename) {
        return filename + 1; 
    }
    return path; 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <файл1> [файл2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        const char* filepath = argv[i];
        struct stat file_stat;

        if (lstat(filepath, &file_stat) == -1) { 
            perror("Ошибка при получении информации о файле");
            fprintf(stderr, "%s: %s: %s\n", argv[0], filepath, "Не удалось получить информацию");
            continue;
        }

        char file_type = '?'; 
        if (S_ISDIR(file_stat.st_mode)) {
            file_type = 'd';
        } else if (S_ISREG(file_stat.st_mode)) {
            file_type = '-'; 
        }

        char permissions[10]; 
        permissions[0] = (file_stat.st_mode & S_IRUSR) ? 'r' : '-';
        permissions[1] = (file_stat.st_mode & S_IWUSR) ? 'w' : '-';
        permissions[2] = (file_stat.st_mode & S_IXUSR) ? 'x' : '-';
        permissions[3] = (file_stat.st_mode & S_IRGRP) ? 'r' : '-';
        permissions[4] = (file_stat.st_mode & S_IWGRP) ? 'w' : '-';
        permissions[5] = (file_stat.st_mode & S_IXGRP) ? 'x' : '-';
        permissions[6] = (file_stat.st_mode & S_IROTH) ? 'r' : '-';
        permissions[7] = (file_stat.st_mode & S_IWOTH) ? 'w' : '-';
        permissions[8] = (file_stat.st_mode & S_IXOTH) ? 'x' : '-';
        permissions[9] = '\0';

        int num_links = file_stat.st_nlink;

        const char* owner_name = get_username(file_stat.st_uid);
        const char* group_name = get_groupname(file_stat.st_gid);

        off_t file_size = -1; 
        file_size = file_stat.st_size;
        

        setlocale(LC_TIME, "ru_RU.UTF-8");
        struct tm *tm_info;
        char date_buffer[80];
        tm_info = localtime(&file_stat.st_mtime);
        strftime(date_buffer, sizeof(date_buffer), "%b %e %H:%M", tm_info); 

        const char* filename = get_filename(filepath);

        printf("%c", file_type); 
        printf("%9s ", permissions); 
        printf("%d ", num_links); 
        printf("%s ", owner_name); 
        printf("%s ", group_name); 

        printf("%lld ", (long long)file_size); 
        
        printf("%s ", date_buffer); 
        printf("%s\n", filename); 
    }

    return EXIT_SUCCESS;
}
