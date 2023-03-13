#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>
#include<time.h>
//#define WIN32_LEAN_AND_MEAN	// repeated definition between windows.h and winsock2.h
#include<sys/types.h>
#include<sys/stat.h>
#include<WinSock2.h>
#include<Windows.h>
#pragma comment(lib, "WS2_32.lib")
#include<errno.h>

#define PRINT(str) printf("[%s-%d] "#str"= %s\n", __func__, __LINE__, str);

#define STDIN   0
#define STDOUT  1
#define STDERR  2
#define DEFAULT_BUFFLEN 1024
#define ROOTDIR "template"
#define CGI_PATH "C:\\Users\\Administrator\\source\\repos\\ChttpServer\\Debug\\CGI.exe"
#define SERVER_STRING "Server: CXN-httpd/0.1.0\r\n"

/// <summary>
/// init server and return a socket
/// </summary>
/// <param name="port">if 0: assign an available port</param>
/// <returns>socket or -1</returns>
SOCKET Startup(unsigned short* port);

/// <summary>
/// process http request
/// </summary>
/// <param name="arg">client socket</param>
/// <returns></returns>
DWORD WINAPI AcceptRequest(LPVOID arg);

/// <summary>
/// get a line text from sock to buff
/// </summary>
/// <param name="sock">socket</param>
/// <param name="buff">buff to store line txt</param>
/// <param name="buff_len">buff-length</param>
/// <returns></returns>
int GetLine(int sock, char* buff, int buff_len);

void Headers(int client, const char* status_code, const char* cnt_type);

int ResponseFile(int client, char* file_name);

const char* GetContentType(const char* file_name);

void ResponseMsg(int client, char* file_name);

void NotFound(int client);

void BadRequest(int client);

void Unimplemented(int client);

int ErrorInfo(const char* str);

void ExcuteCGI(int client, const char* path, const char* method, const char* query_string);

void CannotExcute(int client);