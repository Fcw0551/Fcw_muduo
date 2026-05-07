#pragma once 
#include <iostream>
#include <string>
#include <unordered_map>
#include <regex>
#include "Fcw_Util.hpp"
#include<algorithm>
#include"Fcw_Buffer.hpp"



//解析http的请求
class HttpRequest
{
public:
    std::string _method;                                   // 请求⽅法
    std::string _path;                                     // 资源路径
    std::string _version;                                  // 协议版本
    std::string _body;                                     // 请求正⽂
    std::smatch _matches;                                  // 资源路径的正则表达式提取数据
    std::unordered_map<std::string, std::string> _headers; // 头部字段
    std::unordered_map<std::string, std::string> _params;  // 查询参数
public:
    HttpRequest();
    void reSet();
    // 插⼊头部字段
    void setHeader(const std::string &key, const std::string &val);
    // 判断是否存在指定头部字段
    bool hasHeader(const std::string &key) const;
    // 获取指定头部字段的值
    std::string getHeader(const std::string &key) const;
    // 插⼊查询字符串 
    void setParam(const std::string &key, const std::string &val) ;
    // 判断是否有某个指定的查询字符串
    bool hasParam(const std::string &key) const;
    // 获取指定的查询字符串
    std::string GetParam(const std::string &key) const;
    // 获取正⽂⻓度
    size_t contentLength() const;
    // 判断是否是短链接
    bool close() const;
};




// http的响应
class HttpResponse
{
public:
    int _statu;                                             //状态码
    bool _redirect_flag;                                    //重定向标志
    std::string _redirect_url;                              //重定向的url 
    std::string _body;                                      //响应的正⽂
    std::unordered_map<std::string, std::string> _headers;  // 头部字段

public:
    HttpResponse();
    HttpResponse(int statu);
    //清空防止干扰
    void reSet();
    // 插⼊头部字段
    void setHeader(const std::string &key, const std::string &val);
    // 判断是否存在指定头部字段
    bool hasHeader(const std::string &key);
    // 获取指定头部字段的值
    std::string getHeader(const std::string &key);

    // 设置响应的正⽂body部分
    void setContent(const std::string &body, const std::string &type ="text/html");

    // 设置重定向
    void setRedirect(const std::string &url, int statu = 302);
    // 判断是否是短链接
    bool close();
};







//解析http请求
typedef enum{   
    //严格执行状态转换
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatu;
#define MAX_LINE 8192
class HttpContext
{
private:
    int _resp_statu;            // 响应状态码
    HttpRecvStatu _recv_statu;  // 当前接收及解析的阶段状态
    HttpRequest _request;       // 已经解析得到的请求信息
private:

    
    //解析请求行
    bool parseHttpLine(const std::string &line);

    //接收http的请求行
    bool recvHttpLine(Buffer *buf);

    bool recvHttpBody(Buffer *buf);
    //接收http的head
    bool recvHttpHead(Buffer *buf);
    bool parseHttpHead(std::string &line);

public:
    HttpContext();
    void reSet();
    int respStatu();
    HttpRecvStatu recvStatu();

    HttpRequest &request();
    
    // 接收并解析HTTP请求
    void recvHttpRequest(Buffer *buf);
};