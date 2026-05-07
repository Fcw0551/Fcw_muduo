#pragma once 
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include "Fcw_Log.hpp"
extern std::unordered_map<int, std::string> _statu_msg ;
class Util
{
public: 
    // 字符串分割函数,将src字符串按照sep字符进⾏分割，得到的各个字串放到arry中，最终返回字串的数量
    static size_t split(const std::string &src, const std::string &sep,std::vector<std::string> *arry);
    // 读取⽂件的所有内容，将读取的内容放到⼀个Buffer中
    static bool readFile(const std::string &filename, std::string *buf);
    // 向⽂件写⼊数据
    static bool writeFile(const std::string &filename, const std::string&buf);
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产⽣歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀% C++ -> C%2B%2B
    // 不编码的特殊字符： RFC3986⽂档规定 . - _ ~ 字⺟，数字属于绝对不编码字符
    // RFC3986⽂档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
    static std::string urlEncode(const std::string url, bool convert_space_to_plus);
    static char HEXTOI(char c);
    static std::string urlDecode(const std::string url, bool convert_plus_to_space);
    // 响应状态码的描述信息获取
    static std::string statuDesc(int statu);


    // 根据⽂件后缀名获取⽂件mime，设置文件类型
    static std::string extMime(const std::string &filename);
    // 判断⼀个⽂件是否是⼀个⽬录
    static bool isDirectory(const std::string &filename);
    static bool isRegular(const std::string &filename);
    // http请求的资源路径有效性判断
    //  /index.html --- 前边的/叫做相对根⽬录 映射的是某个服务器上的⼦⽬录
    //  想表达的意思就是，客⼾端只能请求相对根⽬录中的资源，其他地⽅的资源都不予理会
    //  /../login, 这个路径中的..会让路径的查找跑到相对根⽬录之外，这是不合理的，不安全的
    static bool validPath(const std::string &path);
};