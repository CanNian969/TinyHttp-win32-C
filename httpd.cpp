#include"httpd.h"

int ErrorInfo(const char* str)
{
	perror(str);
	return -1;
}

SOCKET Startup(unsigned short* port)
{

	// 1. init network-com
	WSADATA data; // out - init data, 一般情况忽略
	int ret = WSAStartup(MAKEWORD(1, 1), &data); // version 1.1
	if (ret != 0)
		return ErrorInfo("net init: ");

	// 2. create socket
	SOCKET server_socket = socket(PF_INET, // socket type
		SOCK_STREAM, // data stream
		IPPROTO_TCP);	// protocol
	if (!server_socket)
		return ErrorInfo("socket: ");

	// 3. set socket options : port-reuse
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	if (ret != 0)
		return ErrorInfo("socket options: ");

	// 4. set server-address and bind socket to port
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; // 传输地址的地址系列。 此成员应始终设置为AF_INET
	server_addr.sin_port = htons(*port);	// htons 将u_short从主机字节顺序转换为 TCP/IP 网络字节顺序 (这是大端)
	server_addr.sin_addr.s_addr = INADDR_ANY; // 包含 IPv4 传输地址的 IN_ADDR 结构, (ANY表示任意网络ip)

	ret = bind(server_socket, (const struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret != 0)
		return ErrorInfo("bind: ");

	// 5. if port == 0: auto assign an available port
	if (!*port)
	{
		int namelen = sizeof(server_addr); // length of name-buff
		ret = getsockname(server_socket, (struct sockaddr*)&server_addr, &namelen);
		if (ret != 0)
			return ErrorInfo("assign port: ");
		*port = server_addr.sin_port;
	}

	// 6. creat a listen queue
	ret = listen(server_socket, 5);
	if (ret != 0)
		return ErrorInfo("listen: ");

	return server_socket;
}

int GetLine(int sock, char* buff, int buff_len)
{
	char c = 0;//'\0'
	int idx= 0;
	// \r\n
	while (idx < buff_len - 1 && c != '\n')
	{
		int n = recv(sock, &c, 1, 0);
		if (n)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n && c == '\n')
					recv(sock, &c, 1, 0);
				else // if not '\n',stop manualy to prevent reading the next line
					c = '\n';
			}
			buff[idx++] = c;
		}
		else
			c = '\n';
	}
	buff[idx] = 0;
	return idx;
}

void Unimplemented(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Server: cxn-httpd/0.1.0\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

void BadRequest(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 400 BAD REQUEST\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "<P>Your browser sent a bad request, ");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "such as a POST without a Content-Length.\r\n");
	send(client, buf, sizeof(buf), 0);
}

void NotFound(int client)
{
	char root[128];
	strcpy(root, ROOTDIR);
	Headers(client, "404 NOT FOUND", GetContentType(".html"));
	ResponseFile(client, strcat(root, "/not_found.html"));
}

const char *GetContentType(const char *file_name)
{
	const char* res = "text/html";
	// find '.' index
	char* temp = strrchr((char*)file_name, '.');
	if (!temp) return res;

	temp++;
	if (strcmp(temp, "png") == 0) res = "image/png";
	else if (strcmp(temp, "jpg") == 0) res = "image/jpeg";
	else if (strcmp(temp, "css") == 0) res = "text/css";
	else if (strcmp(temp, "js") == 0) res = "application/x-javascript";
	else if (strcmp(temp, "ico") == 0) res = "image/x-icon";

	//printf("Content type: %s\n", res);
	return res;
}

void Headers(int client, const char *status_code, const char* cnt_type)
{
	char buff[1024];
	time_t ti;
	time(&ti);
	//printf("type: %s\n", cnt_type);

	sprintf(buff, "HTTP/1.1 %s\r\n", status_code);
	send(client, buff, strlen(buff), 0);

	sprintf(buff, SERVER_STRING);
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Content-type: %s\n", cnt_type);
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Date: %s \n", ctime(&ti));
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}

int ResponseFile(int client, char *file_name)
{
	FILE* resource = NULL;
	// .html or else
	resource = fopen(file_name, strstr(file_name, ".html") ? "r" : "rb");
	if (!resource)
		return -1;

	char buff[1024];
	int ret = 0, count = 0;
	while(1)
	{
		ret = fread(buff, 1, sizeof(buff), resource);
		if (ret <= 0)
			break;
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("sended file size: [%d]byte\n", count);
	
	fclose(resource);
	return 0;
}

void ResponseMsg(int client, char *file_name)
{
	char buff[DEFAULT_BUFFLEN];
	int line_size = 0, bufflen = DEFAULT_BUFFLEN;

	// clear the buffer
	do {
		line_size = GetLine(client, buff, bufflen);
		//PRINT(buff);
	} while (line_size > 0 && strcmp(buff, "\n") != 0);


	// send Headers
	Headers(client, "200 OK", GetContentType(file_name));
	// send resource
	if(ResponseFile(client, file_name) != 0)
		NotFound(client);

	return;
}

void ExcuteCGI(int client, const char* path, const char* method, const char* query_string)
{
	char buff[DEFAULT_BUFFLEN], meth_env[255], query_env[255], length_env[255];
	char c;
	int line_size = 1, content_length = 0;
	HANDLE parent_read, parent_write;
	HANDLE child_read, child_write;

	buff[0] = 'a', buff[1] = 0;
	if (_stricmp(method, "GET") == 0)
		while (line_size > 0 && strcmp(buff, "\n") != 0)
			line_size = GetLine(client, buff, DEFAULT_BUFFLEN);
	else if (_stricmp(method, "POST") == 0)
	{
		do {
			line_size = GetLine(client, buff, DEFAULT_BUFFLEN);
			buff[15] = '\0';	// buff[15] == 'space' buff[16] == length
			if (_stricmp(buff, "Content-Length:") == 0)
				content_length = atoi(&buff[16]);
		} while (line_size > 0 && strcmp(buff, "\n") != 0);

		if (content_length == -1)
		{
			BadRequest(client);
			return;
		}
	}
	else // other method
	{

	}

	// Create Pipe
	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	if (CreatePipe(&child_read, &parent_write, &sa, 0) == 0)
	{
		CannotExcute(client);
		return;
	}
	if (CreatePipe(&parent_read, &child_write, &sa, 0) == 0)
	{
		CannotExcute(client);
		return;
	}

	STARTUPINFO sinfo;
	PROCESS_INFORMATION pi;
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);
	sinfo.dwFlags = STARTF_USESTDHANDLES;
	sinfo.hStdInput = child_read;
	sinfo.hStdOutput = child_write;
	sinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	sinfo.wShowWindow = SW_SHOW;
	sinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	sprintf(meth_env, "REQUEST_METHOD=%s", method);
	int tr = _putenv(meth_env);
	if (_stricmp(method, "GET") == 0)
	{
		sprintf(query_env, "QUERY_STRING=%s", query_string);
		tr = _putenv(query_env);
	}
	else  // POST
	{
		sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
		tr = _putenv(length_env);
	}

	// child process --- CGI
	TCHAR comand_line[] = TEXT(CGI_PATH);
	//TCHAR comand_line[] = TEXT("ping www.baidu.com");
	if (!CreateProcess(NULL, comand_line, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &pi))
	{
		CannotExcute(client);
		return;
	}

	// parent process
	Headers(client, "200 OK", "text/html");

	DWORD dwWrite, total_dwread = 10;
	if(_stricmp(method, "POST") == 0)
		for (int i = 0; i < content_length; i++)
		{
			recv(client, &c, 1, 0);
			WriteFile(parent_write, &c, 1, &dwWrite, NULL);
		}

	// synchronous read: wait until the datas enter
	ReadFile(parent_read, &c, 1, &dwWrite, NULL);
	do{
		send(client, &c, 1, 0);
		if (total_dwread <= 1)
			break;
	} while (PeekNamedPipe(parent_read, &c, 1, &dwWrite, &total_dwread, NULL) && ReadFile(parent_read, &c, 1, &dwWrite, NULL));

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, 10000);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(parent_read);
	CloseHandle(parent_write);
	CloseHandle(child_read);
	CloseHandle(child_write);
	return;
}

void CannotExcute(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
	send(client, buf, strlen(buf), 0);
}

DWORD WINAPI AcceptRequest(LPVOID arg) // LPVOID == void*
{
	char buff[DEFAULT_BUFFLEN];
	int bufflen = DEFAULT_BUFFLEN;
	int client = (SOCKET)arg;

	int line_size = GetLine(client, buff, bufflen);
	PRINT(buff);

	// GET /user/index.html HTTP / 1.1\n
	// extracte method
	char method[255], url[255], *query_string = NULL;
	int i = 0, j = 0, cgi = 0;
	while (!isspace(buff[j]) && i < sizeof(method)-1 && j < bufflen)
		method[i++] = buff[j++];
	method[i] = 0; // '\0'

	// extracte route
	i = 0;
	while (isspace(buff[j]) && j < bufflen) j++;	// skip space
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < bufflen)
		url[i++] = buff[j++];
	url[i] = 0;

	// get or post ?
	if (_stricmp(method, "get") != 0 && _stricmp(method, "post") != 0)
	{
		// error promot page
		Unimplemented(client);
		PRINT(method);
		printf("no method\n");
		return 0;
	}
	else if (_stricmp(method, "get") == 0)
	{
		// extrate query_string  www.baidu.com/s?wd=gttp&rsv=t\0
		query_string = url;
		while (*query_string != '?' && *query_string != 0) query_string++;
		if (*query_string == '?')
		{
			cgi = 1;
			*query_string = 0;
			query_string++;
		}
		// fprintf(stderr,"---------------- url: %s, query_string: %s\n", url, query_string);
	}
	else if (_stricmp(method, "post") == 0)
	{
		cgi = 1;
	}


	// router
	// / == root
	char path[512];
	const char* root = ROOTDIR;
	sprintf(path, "%s%s", root, url);
	// default: root/index.html
	if (path[strlen(path) - 1] == '/')
		strcat(path, "index.html");

	// check whether the resource exists
	struct stat states;
	if (stat(path, &states) != 0)
	{
		// clear the socket-buffer
		do {
			line_size = GetLine(client, buff, bufflen);
		} while (line_size > 0 && strcmp(buff, "\n") != 0);

		NotFound(client);
	}
	else
	{
		// add .html if path is dir
		if ((states.st_mode & S_IFMT) == S_IFDIR)
			strcat(path, "/index.html");

		if (cgi)
			ExcuteCGI(client, path, method, query_string);
		else
			ResponseMsg(client, path);
	}

	if (closesocket(client) == SOCKET_ERROR)
		wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());

	return 0;
}


int main()
{
	unsigned short port = 9000;
	SOCKET server_socket = Startup(&port);
	if (server_socket == -1)
	{
		printf_s("socket start failed\n");
		return 0;
	}
	printf("httpd server has started, listening port: %d\n", port);
	
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	while (1)
	{
		SOCKET client_socket = -1;
		client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_socket == INVALID_SOCKET)
			return ErrorInfo("accept: ");

		DWORD threadId = 0;
		if (!CreateThread(0, 0, AcceptRequest, (LPVOID)client_socket, 0, &threadId))
		{
			ErrorInfo("creat thread: ");
		}
	}

	if (closesocket(server_socket) == SOCKET_ERROR)
		wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());

	WSACleanup();
	system("pause");
	return 0;
}
