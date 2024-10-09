#include "utill.hpp"
#include "logs/mylog.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

#define LINE_END "\r\n"
#define SEP ": "
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define SERVER_ERROR 500

// 状态码描述
static std::string Code2Desc(int code)
{
    std::string desc;
    switch (code)
    {
    case 200:
        desc = "OK";
        break;
    case 404:
        desc = "Not Found";
        break;
    default:
        break;
    }
    return desc;
}

// HTTP请求信息
class HttpRequest
{
public:
    // 接收到的请求信息
    std::string request_line;                // 请求行 = 请求方法（post/get） + uri + 协议版本
    std::vector<std::string> request_header; // 请求报头 = key: value
    std::string blank;                       // 空行
    std::string request_body;                // 请求正文

    // 解析请求行的结果：请求行 = 方法 + uri + 协议版本
    std::string method;  // 方法
    std::string uri;     // uri = path?args
    std::string version; // 协议版本

    // 解析请求报头的结果：
    std::unordered_map<std::string, std::string> header_kv; // 将每行数据存入map中
    int content_length;                                     // 记录请求正文的大小
    std::string path;                                       // 记录请求资源的路径
    std::string suffix;                                     // 记录请求资源的后缀
    std::string query_string;                               // 记录请求

    bool cgi; // 是否是CGI机制（是否需要http或相关程序作数据处理）
    int size; // 正文的大小

public:
    HttpRequest() : content_length(0), cgi(false) {}
    ~HttpRequest() {}
};

// HTTP响应信息
class HttpResponse
{
public:
    // 返回的响应信息
    std::string status_line;                  // 状态行 = HTTP/version + 状态码 + 状态码描述
    std::vector<std::string> response_header; // 响应报头
    std::string blank;                        // 空行
    std::string response_body;                // 响应正文

    int status_code; // 状态码
    int fd;          //

public:
    HttpResponse() : blank(LINE_END), status_code(OK), fd(-1) {}
    ~HttpResponse() {}
};

// 读取请求，分析请求，构建响应
class EndPoint
{
private:
    int sock;                   // 文件标识符
    HttpRequest http_request;   // HTTP请求
    HttpResponse http_response; // HTTP响应
    bool stop;                  // 停止

private:
    // 接收请求行
    bool RecvHttpRequestLine()
    {
        // 引用请求行
        auto &line = http_request.request_line;
        // 按行读取操作，成果则将结果存入 line（http_request.request_line）中
        if (Utill::ReadLine(sock, line) > 0)
        {
            line.resize(line.size() - 1);
            INFO("%s", http_request.request_line);
        }
        else
        {
            stop = true; //--------------------
        }
        std::cout << "RecvHttpRequestLine: " << stop << std::endl; //---------------------
        return stop;
    }

    // 接收请求报头
    bool RecvHttpRequestHeader()
    {
        std::string line;
        while (true)
        {
            line.clear();
            if (Utill::ReadLine(sock, line) <= 0)
            {
                stop = true; //---------------------?
                break;
            }
            if (line == "\n")
            {
                http_request.blank = line;
                break;
            }
            line.resize(line.size() - 1);
            // 读取的结果输入到请求报头中
            http_request.request_header.push_back(line);
            INFO("%s", line);
        }
        std::cout << "stop debug: " << stop << std::endl; //------------
        return stop;                                      //---------------
    }

    // 解析请求行 = 方法 + uri + HTTP/版本（采取空格分割）
    void ParseHttpRequestLine()
    {
        // line引用字符串http_request.request_line
        auto &line = http_request.request_line;

        // 使用请求行字符串构造stringstream对象ss
        std::stringstream ss(line);
        // 将其输出到 method uri version
        ss >> http_request.method >> http_request.uri >> http_request.version;

        auto &method = http_request.method; /// 对请求方法全部转换成大写（⭐⭐⭐⭐⭐transform函数）
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    }

    // 解析请求报头，数据存入 unordered_map
    void ParseHttpRequestHeader()
    {
        std::string key;
        std::string value;
        // 由于请求报头是 key: value 格式，将其存入 unordered_map 里面
        for (auto &iter : http_request.request_header)
        {
            if (Utill::CutString(iter, key, value, SEP)) // 分隔符是 “: ”
            {
                http_request.header_kv.insert({key, value});
            }
        }
    }

    // 判断HTTP请求是否含有请求正文，为解析请求正文作准备
    bool IsNeedRecvHttpRequestBody()
    {
        auto &method = http_request.method;
        if (method == "POST") // GET 和 POST 两种方法，只有 POST 方法含有请求正文
        {
            auto &header_kv = http_request.header_kv;
            auto iter = header_kv.find("Content-Length"); // 有请求正文则在请求报头中会提示Content-Length的大小
            if (iter != header_kv.end())
            {
                INFO("%s", "Post Method, Content-Length: " + iter->second);
                http_request.content_length = atoi(iter->second.c_str());
                return true;
            }
        }
        return false;
    }

    // 接收请求正文，将正文存入 http_request.request_body
    bool RecvHttpRequestBody()
    {
        // 只有 POST 方法才需要进行接收
        if (IsNeedRecvHttpRequestBody())
        {
            int content_length = http_request.content_length;
            auto &body = http_request.request_body;

            char ch = 0;
            while (content_length)
            {
                ssize_t s = recv(sock, &ch, 1, 0);
                if (s > 0)
                {
                    body.push_back(ch);
                    content_length--;
                }
                else
                {
                    stop = true;
                    break;
                }
            }
            INFO("%s", body);
        }
        return stop;
    }

public:
};