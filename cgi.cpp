#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<Windows.h>
#include<string.h>
#include<errno.h>
#include<typeinfo>

#define DEFAULT_BUFFLEN 128
#define DEFAULT_PARAMNUM 16

int ParseUrlParam(char* param, char(*key)[DEFAULT_BUFFLEN], char(*value)[DEFAULT_BUFFLEN])
{
	// wd=gttp&rsv=t
	int count = 1;
	int _ = 0, col = 0, flag = 0; // flag: 0->key  1->value
	while (param[_] != '\0')
	{
		if (param[_] == '=' || param[_] == '&')
		{
			// close string
			if (param[_] == '=')
				*(*key + col) = '\0';
			*(*value + col) = '\0';

			// switch
			flag = !flag, col = 0;
			if (param[_] == '&') key++, value++, count++;
			_++;
			continue;
		}

		if (!flag)
			*(*key + col++) = param[_++];
		else
			*(*value + col++) = param[_++];
	}
	*(*value + col) = '\0';

	return count;
}

int main()
{
	// child process
	HANDLE child_read = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE parent_write = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWrite;
	char html[4096];	// return to Web server

	char method[128], query_string[255], content_len[128], content[255];
	char key[DEFAULT_PARAMNUM][DEFAULT_BUFFLEN], value[DEFAULT_PARAMNUM][DEFAULT_BUFFLEN];
	int param_nums = 0;

	sprintf_s(method, "%s", getenv("REQUEST_METHOD"));

	//fprintf(stderr, "--1 %s\n", method);
;
	if (_stricmp(method, "GET") == 0)
	{
		sprintf_s(query_string, "%s", getenv("QUERY_STRING"));
		param_nums = ParseUrlParam(query_string, key, value);

		//fprintf(stderr, "2 %s\n", query_string);
	}
	else if (_stricmp(method, "POST") == 0)
	{
		DWORD dwRead;
		char* c = content;
		sprintf_s(content_len, "%s", getenv("CONTENT_LENGTH"));
		int contlen = atoi(content_len);

		//fprintf(stderr, "3 %d\n", contlen);

		while (contlen--) ReadFile(child_read, c++, 1, &dwRead, NULL);
		*c = '\0';
		//fprintf(stderr, "post content: %s\n", content);
		param_nums = ParseUrlParam(content, key, value);
	}
	else
	{
		fprintf(stderr, "CGI error!!!\n");
		//Sleep(10000);
		return 0;
	}
	
	//fprintf(stderr, "4\n");
	//for (int i = 0; i < param_nums; i++)
	//	fprintf(stderr, "k: %s, v: %s\n", key[i], value[i]);
	//fprintf(stderr, "5\n");

	sprintf_s(html, "<html><body>\r\n");
	WriteFile(parent_write, html, strlen(html), &dwWrite, NULL);
	for (int i = 0; i < param_nums; i++)
	{
		sprintf_s(html, "<h4>Params k-v: %s - %s</h4>\r\n", key[i], value[i]);
		WriteFile(parent_write, html, strlen(html), &dwWrite, NULL);
	}
	sprintf_s(html, "</body></html>\r\n");
	WriteFile(parent_write, html, strlen(html), &dwWrite, NULL);

	sprintf_s(html, "\n");
	WriteFile(parent_write, html, strlen(html), &dwWrite, NULL);

	//fprintf(stderr, "6\n");
	CloseHandle(child_read);
	CloseHandle(parent_write);
	//Sleep(10000);
	return 0;
}