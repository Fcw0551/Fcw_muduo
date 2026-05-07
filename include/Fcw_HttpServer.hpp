#pragma once 
#include "Fcw_Log.hpp"
#include "Fcw_TcpServer.hpp"
#include "Fcw_Util.hpp"
#include "Fcw_Http.hpp"
#include <functional>
#include <regex>
#include <assert.h>

// 默认超时时间
#define DEFAULT_TIMEOUT 60

//httpserver，应用层
class HttpServer
{
private:
    using Handler = std::function<void(const HttpRequest&, HttpResponse*)>;
    using Handlers = std::vector<std::pair<std::regex, Handler>>;//存储正则表达式和处理方法
    Handlers _get_route;
    Handlers _post_route;
    Handlers _put_route;
    Handlers _delete_route;
    std::string _basedir; // 静态资源根⽬录
    TcpServer _server;
public:
    HttpServer(int port, int timeout = DEFAULT_TIMEOUT) 
    : _server(port)
    {
        _server.enableInactiveRelease(timeout);//超时断开
        _server.setConnectedCallback(std::bind(&HttpServer::onConnected,this, std::placeholders::_1));//连接成功之后的回调函数，设置上下文
        _server.setMessageCallback(std::bind(&HttpServer::onMessage, this,std::placeholders::_1, std::placeholders::_2));//设置业务回调
    }

private:
    //错误处理回调，根据req和rsp，组织错误信息，返回给客户端
    void errorHandler(const HttpRequest &req, HttpResponse *rsp){
        // 1. 组织⼀个错误展⽰⻚⾯
        std::string body;
        body += "<html>";
        body += "<head>";
        body += "<meta http-equiv='Content-Type' content = 'text/html;charset=utf-8' > ";
        body += "</head>";
        body += "<body>";
        body += "<h1>";
        body += std::to_string(rsp->_statu);//状态码
        body += " ";
        body += Util::statuDesc(rsp->_statu);//状态描述
        body += "</h1>";
        body += "</body>";
        body += "</html>";
        // 2. 将⻚⾯数据，当作响应正⽂，放⼊rsp中
        rsp->setContent(body, "text/html");
    }

    // 将HttpResponse中的要素按照http协议格式进⾏组织，发送
    void writeReponse(const PtrConnection &conn, const HttpRequest &req,HttpResponse &rsp){
        // 1. 先完善头部字段
        if (req.close() == true){
            rsp.setHeader("Connection", "close");
        }
        else{
            rsp.setHeader("Connection", "keep-alive");
        }
        if (rsp._body.empty() == false && rsp.hasHeader("Content-Length") == false){
            rsp.setHeader("Content-Length",std::to_string(rsp._body.size()));
        }
        if (rsp._body.empty() == false && rsp.hasHeader("Content-Type") ==false){
            rsp.setHeader("Content-Type", "application/octet-stream");
        }
        //是否重定向
        if (rsp._redirect_flag == true){
            rsp.setHeader("Location", rsp._redirect_url);
        }

        // //处理utf-8
        // std::string content_type = rsp.getHeader("Content-Type"); // 获取当前的 Content-Type
        // std::cout<<content_type<<std::endl;
        // if (!content_type.empty()){
        //     // 定义「需要设置 UTF-8 编码的文本类 MIME 类型」
        //     bool is_text_type =
        //         // 纯文本类型
        //         (content_type.find("text/") == 0) ||
        //         // 常用结构化文本类型
        //         (content_type == "application/json") ||
        //         (content_type == "application/javascript") ||
        //         (content_type == "application/xml");
        //     std::cout<<"is_text: "<<is_text_type<<std::endl;
        //     // 文本类：追加 ; charset=utf-8（避免重复添加）
        //     if (is_text_type && content_type.find("charset=") == std::string::npos){
        //         DBG_LOG("正在设置");
        //         content_type += "; charset=utf-8";
        //         std::cout<<"设置完的content_type: "<<content_type<<std::endl;
        //         rsp.setHeader("Content-Type", content_type); // 更新响应头
        //     }
        // }

        // 2. 将rsp中的要素，按照http协议格式进⾏组织
        std::stringstream rsp_str;
        rsp_str << req._version << " " << std::to_string(rsp._statu) << " "<< Util::statuDesc(rsp._statu) << "\r\n";
        for (auto &head : rsp._headers){
            rsp_str << head.first << ": " << head.second << "\r\n";
        }
        rsp_str << "\r\n";
        rsp_str << rsp._body;
        //std::cout<<rsp_str.str()<<std::endl;
        // 3. 发送数据
        conn->send(rsp_str.str().c_str(), rsp_str.str().size());
    }

    //request请求处理拼接实际目录，实际请求的文件
    bool isFileHandler(const HttpRequest &req){
        // 1. 必须设置了静态资源根⽬录
        if (_basedir.empty()){
            return false;
        }
        // 2. 请求⽅法，必须是GET / HEAD请求⽅法
        if (req._method != "GET" && req._method != "HEAD"){
            return false;
        }
        // 3. 请求的资源路径必须是⼀个合法路径
        if (Util::validPath(req._path) == false){
            return false;
        }
        // 4. 请求的资源必须存在,且是⼀个普通⽂件
        //拼接实际目录，web根目录+请求的资源路径
        // 有⼀种请求⽐较特殊 -- ⽬录：/, /image/， 这种情况给后边默认追加⼀个index.html
            // index.html /image/a.png
            // 不要忘了前缀的相对根⽬录,也就是将请求路径转换为实际存在的路径  image / a.png->./ wwwroot / image / a.png 
        std::string req_path = _basedir + req._path; // 为了避免直接修改请求的资源路径，因此定义⼀个临时对象
        if (req._path.back() == '/'){
            req_path += "index.html";
        }
        if (Util::isRegular(req_path) == false){
            return false;
        }
        return true;
    }
    // 静态资源的请求处理 --- 将静态资源⽂件的数据读取出来，放到rsp的_body中, 并设置mime
    void fileHandler(const HttpRequest &req, HttpResponse *rsp){
        std::string req_path = _basedir + req._path;
        if (req._path.back() == '/'){
            req_path += "index.html";
        }
        bool ret = Util::readFile(req_path, &rsp->_body);
        if (ret == false){
            return;
        }
        std::string mime = Util::extMime(req_path);
        rsp->setHeader("Content-Type", mime);
        return;
    }


    // 功能性请求的分类处理
    void dispatcher(HttpRequest &req, HttpResponse *rsp, Handlers &handlers)
    {
        // 在对应请求⽅法的路由表中，查找是否含有对应资源请求的处理函数，有则调⽤，没有则返回404
        // 思想：路由表存储的时键值对 -- 正则表达式 & 处理函数
        // 使⽤正则表达式，对请求的资源路径进⾏正则匹配，匹配成功就使⽤对应函数进⾏处理
        // /numbers/(\d+) /numbers/12345
            for (auto &handler : handlers){
            const std::regex &re = handler.first;//正则
            const Handler &functor = handler.second;//处理函数
            //提取然后填充到req中
            bool ret = std::regex_match(req._path, req._matches, re);
            if (ret == false){
                continue;
            }
            return functor(req, rsp); // 传⼊请求信息，和空的rsp，执⾏处理函数
        }
        rsp->_statu = 404;
    }
    void route(HttpRequest &req, HttpResponse *rsp){
        // 1. 对请求进⾏分辨，是⼀个静态资源请求，还是⼀个功能性请求
        //  静态资源请求，则进⾏静态资源的处理
        //  功能性请求，则需要通过⼏个请求路由表来确定是否有处理函数
        //  既不是静态资源请求，也没有设置对应的功能性请求处理函数，就返回405
        if (isFileHandler(req) == true){
            // 是⼀个静态资源请求, 则进⾏静态资源请求的处理
            return fileHandler(req, rsp);
        }
        if (req._method == "GET" || req._method == "HEAD"){
            return dispatcher(req, rsp, _get_route);
        }
        else if (req._method == "POST"){
            return dispatcher(req, rsp, _post_route);
        }
        else if (req._method == "PUT"){
            return dispatcher(req, rsp, _put_route);
        }
        else if (req._method == "DELETE"){
            return dispatcher(req, rsp, _delete_route);
        }
        rsp->_statu = 405; // Method Not Allowed
        return;
    }


    // 设置上下⽂：为新连接初始化http协议
    void onConnected(const PtrConnection &conn){
        conn->setContext(HttpContext());
    }


    // 缓冲区数据解析+处理
    void onMessage(const PtrConnection &conn, Buffer *buffer){
        while (buffer->readSize() > 0){
            // 1. 获取上下⽂
            HttpContext *context = conn->getContext()->get<HttpContext>();
            // 2. 通过上下⽂对缓冲区数据进⾏解析，得到HttpRequest对象
            //  1. 如果缓冲区的数据解析出错，就直接回复出错响应
            //  2. 如果解析正常，且请求已经获取完毕，才开始去进⾏处理
            //解析request，然后填充
            buffer->printBufferText();
            context->recvHttpRequest(buffer);
            DBG_LOG("解析完毕。。。");
            HttpRequest& req = context->request();
            HttpResponse rsp(context->respStatu());
            if (context->respStatu() >= 400){
                // 进⾏错误响应，关闭连接
                errorHandler(req, &rsp);      // 填充⼀个错误显⽰⻚⾯数据到rsp中
                writeReponse(conn, req, rsp); // 组织响应发送给客⼾端
                context->reSet();
                buffer->moveReadOffset(buffer->readSize()); // 出错了就把缓冲区数据清空
                conn->shutdown(); // 关闭连接
                return;
            }
            if (context->recvStatu() != RECV_HTTP_OVER){
                // 当前请求还没有接收完整,则退出，等新数据到来再重新继续处理
                return;
            }
            // 3. 请求路由 + 业务处理
            route(req, &rsp);
            // 4. 对HttpResponse进⾏组织发送
            writeReponse(conn, req, rsp);
            // 5. 重置上下⽂
            context->reSet();
            // 6. 根据⻓短连接判断是否关闭连接或者继续处理
            if (rsp.close() == true)
                conn->shutdown(); // 短链接则直接关闭
        }
        return;
    }

public:
   
   //设置根目录
    void setBaseDir(const std::string &path){
        assert(Util::isDirectory(path) == true);
        _basedir = path;
    }
    /*设置/添加，请求（请求的正则表达）与处理函数的映射关系*/
    void Get(const std::string &pattern, const Handler &handler){
        _get_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Post(const std::string &pattern, const Handler &handler){
        _post_route.push_back(std::make_pair(std::regex(pattern),handler));
    }
    void Put(const std::string &pattern, const Handler &handler){
        _put_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Delete(const std::string &pattern, const Handler &handler){
        _delete_route.push_back(std::make_pair(std::regex(pattern),handler));
    }
    void setThreadCount(int count){
        _server.setThreadCount(count);
    }
    void listen(){
        _server.start();
    }
};