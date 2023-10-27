#include "daemon.h"

#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include <fstream>
#include <filesystem>
#include <csignal>
#include <unistd.h>
#include <cstring>
#include <pwd.h>
#include <glob.h>
#include <ftw.h>

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGHUP:
		Daemon::get_instance().read_config_file();
		break;
	case SIGTERM:
		Daemon::get_instance().terminate();
		break;
	default:
		break;
	}
}

void Daemon::check_pid()
{
	syslog(LOG_INFO, "Checking pid file");
	std::ifstream pidFile(config_path);
	if (pidFile.is_open())
	{
		int pid = 0;
		if (pidFile >> pid && kill(pid, 0) == 0)
		{
			syslog(LOG_INFO, "Killing another daemon instance");
			kill(pid, SIGTERM);
		}
		pidFile.close();
	}
}

void Daemon::make_fork()
{
	syslog(LOG_INFO, "Forking process");

	int stdinCopy = dup(STDIN_FILENO);
	int stdoutCopy = dup(STDOUT_FILENO);
	int stderrCopy = dup(STDERR_FILENO);

	pid_t pid = fork();
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}
	else if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}
	umask(0);
	if (setsid() < 0)
	{
		exit(EXIT_FAILURE);
	}
	if (chdir("/") < 0)
	{
		exit(EXIT_FAILURE);
	}
	for (long x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
	{
		close(x);
	}

	dup2(stdinCopy, STDERR_FILENO);
	dup2(stdoutCopy, STDOUT_FILENO);
	dup2(stderrCopy, STDERR_FILENO);
}

void Daemon::write_to_PID()
{
	syslog(LOG_INFO, "Writing to PID file");
	std::ofstream pidFile(config_path);
	if (!pidFile.is_open())
	{
		syslog(LOG_ERR, "Error while opening PID file");
		exit(EXIT_FAILURE);
	}
	pidFile << getpid();
	pidFile.close();
}

void Daemon::set_signals()
{
	std::signal(SIGHUP, signal_handler);
	std::signal(SIGTERM, signal_handler);
}


int Daemon::read_config_file()
{
	if (!is_file_exist(config_path.c_str()))
	{
		printf("Config file %s does not exist. \n", config_path.c_str());
		return EXIT_FAILURE;
	}
	std::ifstream config_file(config_path.c_str());
	if (config_file.is_open() && !config_file.eof())
	{
		interval = 0;
		config_file >> folder1 >> folder2 >> interval;
		config_file.close();
		config_path = realpath(config_path.c_str(), NULL);
	}
	if (folder1.length() == 0 || folder2.length() == 0 || interval == 0)
	{
		printf("Empty args, please check config file %s. \n", config_path.c_str());
		return EXIT_FAILURE;
	}

	if (!is_dir_exist(folder1))
	{
		printf("Directory %s does not exist. \n", folder1.c_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

bool Daemon::is_file_exist(const std::string &name)
{
	struct stat buffer;
	return (name.length() != 0 && stat(name.c_str(), &buffer) == 0);
}

bool Daemon::is_dir_exist(std::string &path)
{
	if (path[0] == '~')
	{
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
	return (path.length() != 0 && stat(path.c_str(), &buffer) == 0 && (S_ISDIR(buffer.st_mode)));
}

void Daemon::initialize(const std::string configPath)
{
	openlog("bk_copier_daemon", LOG_NDELAY | LOG_PID, LOG_USER);
	syslog(LOG_INFO, "Initializing daemon");

	config_path = configPath;
	int last_error = read_config_file();
	if (last_error != EXIT_SUCCESS)
		return;
	check_pid();
	make_fork();
	write_to_PID();
	set_signals();
	is_working = true;
	syslog(LOG_INFO, "Daemon initialized");
}

void Daemon::terminate()
{
	is_working = false;
	syslog(LOG_INFO, "Daemon terminated");
	closelog();
}

int rmFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) 
{
    if (remove(pathname) < 0) {
        perror("ERROR: remove");
        return -1;
    }
    return 0;
}

void Daemon::clear_folder(const std::string& path) 
{
    nftw(path.c_str(), rmFiles, 10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS);
    
    if (mkdir(path.c_str(), 0777) == -1) {
       syslog (LOG_ERR, "Unable to create directory %s.", path.c_str());
    }
}

void Daemon::copy_bk_files(std::string& src, std::string& dst) 
{
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

void Daemon::copy_file(std::string src_path, std::string dst_path) 
{
    std::ifstream src(src_path.c_str(), std::ios::binary);
    std::ofstream dst(dst_path.c_str(), std::ios::binary);
    dst << src.rdbuf();
}

void Daemon::run()
{
	while (is_working) {
        if (!is_dir_exist(folder2))
            printf("WARNING: Destination directory %s does not exist. It will be created. \n", folder2.c_str());

        clear_folder(folder2);
        copy_bk_files(folder1, folder2);
        sleep (interval);
    }

    syslog (LOG_NOTICE, "Close syslog");
    closelog();
}