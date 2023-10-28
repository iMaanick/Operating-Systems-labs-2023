#include <string>
class Daemon
{
  private:
    std::string pid_file_path = "/var/run/lab1_daemon";
    std::string config_path;
    std::string folder1;
    std::string folder2;
    unsigned int interval = 0;
    bool is_working = false;
    void check_pid();
    void make_fork();
    void write_to_PID();
    void set_signals();

    bool is_file_exist(const std::string &name);
    int read_config_file();
    bool is_dir_exist(std::string& path);
    void copy_bk_files(std::string& src, std::string& dst);
    void clear_folder(const std::string& path);
    friend int rmFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb);
    void copy_file(std::string src_path, std::string dst_path);
    friend void signal_handler(int signal);
    Daemon() = default;
    Daemon(Daemon const&) = delete;
    void operator=(Daemon const&) = delete;
  public:
    static Daemon& get_instance() {
        static Daemon instance;
        return instance;
    };
    void initialize(const std::string configPath);
    void terminate();
    void run();
};