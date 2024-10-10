#include "utill.hpp"
#include "logs/mylog.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>

#define LINE_END "\r\n"
#define SEP ": "
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define PAGE_404 "404.html"

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

// 依照请求资源的后缀，构建HTTP响应的报头中Content-Type的类型（Content-Type: text/html）
static std::string Suffix2Desc(const std::string &suffix)
{
    // static 只会初始化一次
    static std::unordered_map<std::string, std::string> suffix2desc = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".jpg", "application/x-jpg"},
        {".xml", "application/xml"},
    };

    auto iter = suffix2desc.find(suffix);
    if (iter != suffix2desc.end())
    {
        return iter->second;
    }
    return "text/html";
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

    // 解析请求行 = 方法 + uri + HTTP/版本（它们之间是采取空格分割）（⭐⭐⭐⭐⭐transform函数）
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

    // 判断HTTP请求是否含有请求正文，为接收请求正文作准备
    bool IsNeedRecvHttpRequestBody()
    {
        auto &method = http_request.method;
        if (method == "POST") // GET 和 POST 两种方法，只有 POST 方法含有请求正文
        {
            auto &header_kv = http_request.header_kv;
            auto iter = header_kv.find("Content-Length"); // 若有请求正文，则在请求报头中会提示Content-Length的大小
            if (iter != header_kv.end())
            {
                INFO("%s", "Post Method, Content-Length: " + iter->second); // 显示post方法对应的正文长度
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
            INFO("%s", body); // 提示接收到的正文内容
        }
        return stop;
    }

    // 构建HTTP响应
    void BuildHttpResponseHelper() {}

    // 非CGI机制返回信息
    int ProcessNonCgi()
    {
        http_response.fd = open(http_request.path.c_str(), O_RDONLY);
        if (http_response.fd >= 0)
        {
            INFO("%s", http_request.path + " open success!");
            return OK;
        }
        return NOT_FOUND;
    }

    // CGI机制返回信息
    int ProcessCgi()
    {
    }

    // 构建成功的HTTP响应报头
    void BuildOkResponse()
    {
        // 构建HTTP的响应报头（Content-Type）
        std::string line = "Content-Type: ";
        line += Suffix2Desc(http_request.suffix);
        line += LINE_END;
        http_response.response_header.push_back(line);

        // 构建HTTP的响应报头（Content-Length）
        line = "Content-Length: ";
        if (http_request.cgi) // CGI机制
        {
            line += std::to_string(http_response.response_body.size());
        }
        else // 非CGI机制，在构建HTTP响应函数中，完善了HTTP请求路径
        {
            line += std::to_string(http_request.size); // Get
        }
        line += LINE_END;
        http_response.response_header.push_back(line);
    }

    // 构建出错的HTTP响应报头
    void HandlerError(std::string page)
    {
        std::cout << "debug: " << page << std::endl;
        http_request.cgi = false;
        // 要给用户返回对应的404页面
        http_response.fd = open(page.c_str(), O_RDONLY);
        if (http_response.fd > 0)
        {
            struct stat st;
            stat(page.c_str(), &st);
            http_request.size = st.st_size;

            std::string line = "Content-Type: text/html";
            line += LINE_END;
            http_response.response_header.push_back(line);

            line = "Content-Length: ";
            line += std::to_string(st.st_size);
            line += LINE_END;
            http_response.response_header.push_back(line);
        }
    }

    // 辅助函数构建HTTP响应（状态行（HTTP版本 + 状态码 + 状态码描述） + 响应报头 + 空行 + 响应正文）
    void BuildHttpResponseHelper()
    {
        // 构建状态行
        auto &code = http_response.status_code;        // 获取状态行中的状态码
        auto &status_line = http_response.status_line; // 引用HTTP响应对象的状态行
        status_line += HTTP_VERSION;                   // 首先，添加HTTP_VERSION（HTTP/1.0）
        status_line += " ";                            // 状态行不同信息之间的分隔符是空格
        status_line += std::to_string(code);           // 其次，添加状态码
        status_line += " ";                            // 状态行不同信息之间的分隔符是空格
        status_line += Code2Desc(code);                // 最后，添加状态码描述
        status_line += LINE_END;                       // 状态行构建结束

        // 构建响应正文，可能包括响应报头
        std::string path = WEB_ROOT;
        path += "/";
        switch (code)
        {
        case OK:
            BuildOkResponse();
            break;
        case NOT_FOUND:
            path += PAGE_404;
            HandlerError(path);
            break;
        case BAD_REQUEST:
            path += PAGE_404;
            HandlerError(path);
            break;
        case SERVER_ERROR:
            path += PAGE_404;
            HandlerError(path);
            break;
        default:
            break;
        }
    }

public:
    EndPoint(int _sock) : sock(_sock), stop(false)
    {
    }

    bool IsStop()
    {
        return stop;
    }

    // 接收HTTP请求信息
    void RecvHttpRequest()
    {
        // 短路求值（接收请求行和请求报头，如果没有这两个，则无需进行解析）
        if ((!RecvHttpRequestLine()) && (!RecvHttpRequestHeader()))
        {
            ParseHttpRequestLine();   // 解析请求行
            ParseHttpRequestHeader(); // 解析请求报头
            RecvHttpRequestBody();    // 接收请求正文（仅POST方法）
        }
    }

    // 依旧接收到的HTTP请求信息构建HTTP响应
    void BuildHttpResponse()
    {
        auto &code = http_response.status_code;
        std::string _path;
        struct stat st;
        std::size_t found = 0;
        auto &code = http_response.status_code;

        // 强制要求接收到的请求的方法必须是GET和POST
        if (http_request.method != "GET" && http_request.method != "POST")
        {
            // 不是上述方法为非法请求
            std::cout << "method: " << http_request.method << std::endl;
            WARN("%s", "method is not right");
            code = BAD_REQUEST;
            goto END;
        }

        // 以下是请求方法为GET的处理流程
        if (http_request.method == "GET")
        {
            // GET方法可以带参数也可以不带，若带则是通过URI传参（URI = /a/b?1+1）
            size_t pos = http_request.uri.find('?'); // 判断是否带参数，（路径?交互信息）
            if (pos != std::string::npos)
            {
                Utill::CutString(http_request.uri, http_request.path, http_request.query_string, "?");
                http_request.cgi = true; // 带有参数则为CGI机制
            }
            else
            {
                http_request.path = http_request.uri; // 没有找到?则说明URI就是一段路径
            }
        }
        // 以下是请求方法为POST的处理流程
        else if (http_request.method == "POST")
        {
            http_request.cgi = true;
            http_request.path = http_request.uri;
        }
        // 可拓展其他请求方法
        else
        {
            // Do Nothing===为了拓展其他方法
        }

        // 重新构建HTTP请求的资源路径，从WEB根目录下开始
        _path = http_request.path;    // 先保存从请求行中获取到的路径
        http_request.path = WEB_ROOT; // 重新设定HTTP请求的资源路径的开始
        http_request.path += _path;   // 加上从请求行中获取到的路径

        // 请求的路径对应资源是一个目录
        if (http_request.path[http_request.path.size() - 1] == '/')
        {
            http_request.path += HOME_PAGE; // 如果路径最后一个字符是/，则请求的是一个目录，返回该目录下的主页
        }

        // 请求的路径对应资源是一个可执行程序，先判断该程序是否存在当前目录下（⭐⭐⭐⭐⭐stat函数）
        if (stat(http_request.path.c_str(), &st) == 0)
        {
            // 说明该程序是存在的，判断请求的资源是否为一个目录
            if (S_ISDIR(st.st_mode))
            {
                // 说明请求的资源是一个目录，不被允许的,需要做一下相关处理
                // 虽然是一个目录，但是绝对不会以/结尾！上面处理过了
                http_request.path += "/";
                http_request.path += HOME_PAGE;
                stat(http_request.path.c_str(), &st);
            }

            // 都有可执行程序权限检测标志位
            if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            {
                http_request.cgi = true; // 特殊处理
            }

            // 获取文件的大小
            http_request.size = st.st_size;
        }
        // 请求的路径对应资源是不存在的
        else
        {
            WARN("%s", http_request.path + " Not Found");
            code = NOT_FOUND;
            goto END;
        }

        // 查找请求资源的后缀名
        found = http_request.path.rfind(".");
        if (found == std::string::npos)
        {
            http_request.suffix = ".html"; // 默认后缀是 .html
        }
        else
        {
            http_request.suffix = http_request.path.substr(found); // 截取后缀
        }

        // 判断是否是CGI机制
        if (http_request.cgi) // 是，执行目标程序
        {
            code = ProcessCgi(); // 拿到结果:http_response.response_body;
        }
        else // 不是，构建HTTP响应，返回静态网页
        {
            // 1. 目标网页一定是存在的
            // 2. 返回并不是单单返回网页，而是要构建HTTP响应
            code = ProcessNonCgi(); // 简单的网页返回，返回静态网页，只需要打开即可
        }

    END:
        BuildHttpResponseHelper();
    }

    ~EndPoint()
    {
        close(sock);
    }
};