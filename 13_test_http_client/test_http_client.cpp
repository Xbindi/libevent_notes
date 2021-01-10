#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <iostream>
#include <signal.h>
#include <string.h>

using namespace std;

/*********************************************************************
 * 1. 指定url，然后分析url
 * 2. 获取host 解析port 请求的服务器地址 解析query 	
 * 3. 发送请求，获取连接后，将请求的数据保存到本地
 * ********************************************************************/

void http_client_cb(struct evhttp_request* req, void* ctx)
{
	cout << "-------http_client_cb----------" << endl;
	bufferevent* bev = (bufferevent*)ctx;
	//服务器端响应错误
	if(req == NULL)
	{
		int errcode = EVUTIL_SOCKET_ERROR();
		cout << "socket error " << evutil_socket_error_to_string(errcode) << endl;
		return ;
	}
	//获取path
	const char* path = evhttp_request_get_uri(req);
	cout << "request path is " << path << endl;

	//将请求的服务器数据保存到本地，创建目录以及对应名字的文件
	string filepath = ".";
	filepath += path;
	cout << "filepath is " << filepath << endl;
	//如果url路径中有目录，需要分析出目录，并创建
	FILE* fp = fopen(filepath.c_str(), "wb");
	if(!fp)
	{
		cout << "open file" << filepath << "failed!" << endl; 
	}

	//获取返回的code eg:200 404
	cout << "Response: " << evhttp_request_get_response_code(req); //200
	cout << " " << evhttp_request_get_response_code_line(req) << endl;	//OK
	
	char buf[1024];
	evbuffer* input = evhttp_request_get_input_buffer(req);
	for(;;)
	{
		int len = evbuffer_remove(input, buf, sizeof(buf)-1);
		if(len <= 0) break;
		buf[len] = 0;
		if(!fp)
			continue;
		fwrite(buf, 1, len, fp);//将获取的服务器数据，保存写入本地文件
		//cout << buf << flush;//调试信息：输出获取的文件内容信息
	}

	if(fp)
		fclose(fp);
}

int main()
{
	//忽略管道信号，发送数据给以关闭的
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}

	std:: cout << "http client test!\n";
	event_base* base = event_base_new();
	if(base)
	{
		cout << "event_base_new success!" << endl;
	}

	//生成请求信息GET  分析URL地址
	string http_url = "http://ffmpeg.club/index.html?id=1";
	//string http_url = "http://ffmpeg.club/101.jpg";
	//uri
	evhttp_uri* uri = evhttp_uri_parse(http_url.c_str());
	//http https scheme可能为空，若赋值给string容易导致程序崩溃
	const char* scheme = evhttp_uri_get_scheme(uri);
	if(!scheme)
	{
		cerr << "scheme is null" << endl;
	}

	//解析port	
	int port = evhttp_uri_get_port(uri);
	if(port < 0)
	{
		if(strcmp(scheme, "http") == 0)
			port = 80;
	}
			
	cout << "port is " << port << endl;

	//host ffmpeg.club
	const char* host = evhttp_uri_get_host(uri);
	if(!host)
	{
		cerr << "host is null" << endl;
		return -1;
	}

	cout << "host is " << host << endl;

	//请求的服务器地址	
	const char* path = evhttp_uri_get_path(uri);
	if(!path || strlen(path) == 0)
	{
		path = "/";
	}
	if(path)
		cout << "path is " << path << endl;
	
	//解析query  在url中？后的内容
	const char* query = evhttp_uri_get_query(uri);
	if(query)
	{
		cout << "query is " << query << endl;
	}else{
		cout << "query is NULL" << endl;
	}
	
	//接下来是具体的请求操作
	//bufferevent 链接http服务器   建立连接信息
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, host, port);

	//http_client 请求 回调函数设置 连接建立后执行回调函数
	evhttp_request* req = evhttp_request_new(http_client_cb, bev);
	
	//设置请求的head信息
	evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
	evhttp_add_header(output_headers, "Host", host);

	//发送请求
	evhttp_make_request(evcon, req, EVHTTP_REQ_GET, path);

	if(base)
		event_base_dispatch(base);
	if(base)
		event_base_free(base);

	return 0;
}

