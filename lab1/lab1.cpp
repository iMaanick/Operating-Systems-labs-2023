#include <stdio.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>
#include <pwd.h>
#include <ftw.h>
#include <string.h>
#include <sstream>
#include <glob.h>

std::string config_path;
std::string pid_file_path = "/var/run/lab1_daemon";
std::string folder1;
std::string folder2;
unsigned int interval = 0;

bool is_dir_exist(std::string& path) {
    if (path[0] == '~') {
         passwd *pw;
         uid_t uid;

        uid = geteuid();
        pw = getpwuid(uid);
        if (pw)
            path.replace(0, 1, std::string("/home/") + pw->pw_name); 
        else
            syslog(LOG_WARNING, "WARNING: Couldn't find username by UID %u. There is no guarantee to find folder which path contains `~`.", uid);
        
    }
    struct stat buffer;
    if (path.length() != 0 && stat(path.c_str(), &buffer) == 0 && (S_ISDIR(buffer.st_mode)))
        return true;
    return false;
}

bool is_file_exist(const std::string &name) {
    struct stat buffer;
    if (name.length() != 0 && stat(name.c_str(), &buffer) == 0)
        return true;
    return false;
}

void copy_file(std::string src_path, std::string dst_path) {
    std::ifstream src(src_path.c_str(), std::ios::binary);
    std::ofstream dst(dst_path.c_str(), std::ios::binary);
    dst << src.rdbuf();
}

static int rmFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    if (remove(pathname) < 0) {
        perror("ERROR: remove");
        return -1;
    }
    return 0;
}

void clear_folder(const std::string& path) {
    nftw(path.c_str(), rmFiles, 10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS);
    
    if (mkdir(path.c_str(), 0777) == -1) {
       syslog (LOG_ERR, "Unable to create directory %s.", path.c_str());
    }
}

void copy_bk_files(std::string& src, std::string& dst) {
    if (!is_dir_exist(src)) {
        syslog (LOG_ERR, "Source directory %s does not exist. Copy failed.", src.c_str());
        return;
    }
    glob_t glob_result;
    glob((src + std::string("/*")).c_str(), GLOB_TILDE, NULL, &glob_result);

    for(unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
        std::string path = glob_result.gl_pathv[i];
        if (is_dir_exist(path))
            copy_bk_files(path, dst);
        
        std::string::size_type idx;

        idx = path.rfind('.');

        if (idx != std::string::npos) {
            std::string extension = path.substr(idx + 1);
            if (extension == std::string("bk")) {
                copy_file(src + std::string("/") + std::string(basename(path.c_str())), dst + std::string("/") + std::string(basename(path.c_str())));
            }
        }
    }
}

int read_config_file() {
    if (!is_file_exist(config_path.c_str())) {
        printf("Config file %s does not exist. \n", config_path.c_str());
        return EXIT_FAILURE;
    }
    std::ifstream config_file(config_path.c_str());
    if (config_file.is_open() && !config_file.eof()) {
        interval = 0;
        config_file >> folder1 >> folder2 >> interval;
        config_file.close();
        config_path = realpath(config_path.c_str(), NULL);
    }
    if (folder1.length() == 0 || folder2.length() == 0 || interval == 0) {
        printf("Empty args, please check config file %s. \n", config_path.c_str());
        return EXIT_FAILURE;
    }

    if (!is_dir_exist(folder1)) {
        printf("Directory %s does not exist. \n", folder1.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void kill_last_daemon() {
    std::ifstream pid_file(pid_file_path.c_str());

    if (pid_file.is_open() && !pid_file.eof()) {
        pid_t prev_daemon_pid;
        pid_file >> prev_daemon_pid;
        
        if (prev_daemon_pid > 0) {
            kill(prev_daemon_pid, SIGTERM);
        }
    }

    pid_file.close();
}

void signal_handler(int signum) {
    if (signum == SIGHUP) {
        syslog (LOG_NOTICE, "SIGHUP signal caught.");
        if (read_config_file() != EXIT_SUCCESS) {
            syslog(LOG_ERR, "Can't read config file");
            kill_last_daemon();
        }
    }
    else if (signum == SIGTERM) {
        syslog (LOG_NOTICE, "SIGTERM signal caught.");
        unlink(pid_file_path.c_str());
        exit(0);
    }
}


static void create_daemon() {
    pid_t pid;

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (chdir("/") < 0) {
        syslog (LOG_NOTICE, "Error occured while changing working directory.");
        exit(EXIT_FAILURE);
    }

    syslog (LOG_NOTICE, "New daemon.");

    kill_last_daemon();

    /* set new pid */

    std::ofstream pid_file_out(pid_file_path.c_str());

    if (pid_file_out.is_open()) {
        pid_file_out << getpid();
        pid_file_out.close();
    }

}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Error: exprected 2 arguments, got %d \n", argc);
        return EXIT_FAILURE;
    }

    openlog ("lab1_daemons", LOG_PID, LOG_DAEMON);
    syslog (LOG_NOTICE, "Open syslog");

    config_path = argv[1];

    int last_error = read_config_file();
    if (last_error != EXIT_SUCCESS)
        return last_error;

    fflush(stdout);
    
    create_daemon();

    while (true) {
        if (!is_dir_exist(folder2))
            printf("WARNING: Destination directory %s does not exist. It will be created. \n", folder2.c_str());

        clear_folder(folder2);
        copy_bk_files(folder1, folder2);
        sleep (interval);
    }

    syslog (LOG_NOTICE, "Close syslog");
    closelog();

    return EXIT_SUCCESS;
}