#pragma once 
#include <iostream>
#include <regex>
void reqParse(){
    std::cout << "------------------first line start-----------------\n";
    // std::string str = "GET /xxxxx?xxxxx HTTP/1.1\r\n";
    // std::string str = "GET /xxxxx?xxxxx HTTP/1.1\n";
    std::string str = "GET /Fcw?a=b&c=d HTTP/1.1\r\n";
    std::regex re("(GET|HEAD|POST|PUT|DELETE) (([^?]+)(?:\\?(.*?))?) (HTTP/1\\.[01])(?:\r\n|\n)");
    std::smatch matches;//存储匹配结果
    std::regex_match(str, matches, re);

    for (int i = 0; i < matches.size(); i++){
        std::cout << "matches[" << i << "]: ";
        std::cout << matches[i] << std::endl;
    }
    if (matches[4].length() > 0){
        std::cout << "have param!\n";
    }
    else{
        std::cout << "have not param!\n";
    }
    std::cout << "------------------first line start-----------------\n";

}
void methodMatch(const std::string str){
    std::cout << "------------------method start-----------------\n";
    std::regex re("(GET|HEAD|POST|PUT|DELETE) .*");
    /* () 表⽰捕捉符合括号内格式的数据
    * GET|HEAD|POST... |表⽰或，也就是匹配这⼏个字符串中的任意⼀个
    * .* 中.表⽰匹配除换⾏外的任意单字符, *表⽰匹配前边的字符任意次; 合起来在这⾥就是
   表⽰空格后匹配任意字符
    * 最终合并起来表⽰匹配以GET或者POST或者PUT...⼏个字符串开始，然后后边有个空格的字
   符串, 并在匹配成功后捕捉前边的请求⽅法字符串
    */
    std::smatch matches;
    std::regex_match(str, matches, re);
    std::cout << matches[0] << std::endl;
    std::cout << matches[1] << std::endl;
    std::cout << "------------------method over------------------\n";
}
void pathMatch(const std::string str){
    // std::regex re("(([^?]+)(?:\\?(.*?))?)");
    std::cout << "------------------path start------------------\n";
    std::regex re("([^?]+).*");
    /*
    * 最外层的() 表⽰捕捉提取括号内指定格式的内容
    * ([^?]+) [^xyz] 负值匹配集合，指匹配⾮^之后的字符， ⽐如[^abc] 则plain就匹配到
   plin字符
    * +匹配前⾯的⼦表达式⼀次或多次
    * 合并合并起来就是匹配⾮?字符⼀次或多次
    */
    std::smatch matches;
    std::regex_match(str, matches, re);
    std::cout << matches[0] << std::endl;
    std::cout << matches[1] << std::endl;
    std::cout << "------------------path over------------------\n";
}
void queryMatch(const std::string str){
    std::cout << "------------------query start------------------\n";
    std::regex re("(?:\\?(.*?))? .*");
    /*
    * (?:\\?(.*?))? 最后的?表⽰匹配前边的表达式0次或1次，因为有的请求可能没有查询
   字符串
    * (?:\\?(.*?)) (?:pattern)表⽰匹配pattern但是不获取匹配结果
    * \\?(.*?) \\?表⽰原始的?字符，这⾥表⽰以?字符作为起始
    * .表⽰\n之外任意单字符，
    *表⽰匹配前边的字符0次或多次,
    ?跟在*或+之后表⽰懒惰模式, 也就是说以?开始的字符串就只匹配这⼀次就⾏，
   后边还有以?开始的同格式字符串也不不会匹配
    () 表⽰捕捉获取符合内部格式的数据
    * 合并起来表⽰的就是，匹配以?开始的字符串，但是字符串整体不要，
    * 只捕捉获取?之后的字符串,且只匹配⼀次，就算后边还有以?开始的同格式字符串也不不会匹
   配
    */
    std::smatch matches;
    std::regex_match(str, matches, re);
    std::cout << matches[0] << std::endl;
    std::cout << matches[1] << std::endl;
    std::cout << "------------------query over------------------\n";
}
void versionMathch(const std::string str){
    std::cout << "------------------version start------------------\n";
    std::regex re("(HTTP/1\\.[01])(?:\r\n|\n)");
    /*
    * (HTTP/1\\.[01]) 外层的括号表⽰捕捉字符串
    * HTTP/1 表⽰以HTTP/1开始的字符串
    * \\. 表⽰匹配 . 原始字符
    * [01] 表⽰匹配0字符或者1字符
    * (?:\r\n|\n) 表⽰匹配⼀个\r\n或者\n字符，但是并不捕捉这个内容
    * 合并起来就是匹配以HTTP/1.开始，后边跟了⼀个0或1的字符，且最终以\n或者\r\n作为结
   尾的字符串
    */
    std::smatch matches;
    std::regex_match(str, matches, re);
    std::cout << matches[0] << std::endl;
    std::cout << matches[1] << std::endl;
    std::cout << "------------------version over------------------\n";
}
