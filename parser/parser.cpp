//这个代码用于实现预处理模块
//核心功能就是读取并分析boost文档的.html内容
//解析出每个文档的标题，url，正文（去除html标签）
//最终把结果输出成一个 行文本 文件

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "../common/util.hpp"

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;

// 定义一些相关的变量和结构体

// 这个变量表示从哪个目录中读取boost文档的html
string g_input_path ="../data/input";

// 这个变量表示预处理模块输出结果放到哪里
string g_output_path ="../data/tmp/raw_input";

//创建一个重要的结构体，表示一个文档（一个html）
struct DocInfo{
    string title;//文档的标题
    string url;//文档的url
    string content;//文档的正文
};

bool EnumFile(const string &input_path, vector<string> *file_list){
    //枚举目录需要使用boost来完成
    //需要包含相关的头文件
    //把boost::filesystem这个命名空间定义一个别名
    namespace fs = boost::filesystem;
    fs::path root_path(input_path);
    if(!fs::exists(root_path)){
      std::cout << "当前的目录不存在" << std::endl;
    }
    //递归遍历的时候使用到一个核心的类
    //递归目录迭代器，迭代器使用循环实现的时候可以自动完成递归
    //把迭代器默认构造函数生成的迭代器作为一个“哨兵”
    fs::recursive_directory_iterator end_iter;
    for(fs::recursive_directory_iterator begin_iter(root_path); begin_iter != end_iter; ++begin_iter){
        //需要判定当前的路径对应的是不是一个普通文件
        //如果是目录直接跳过
        if(!fs::is_regular_file(*begin_iter))
        {
            continue;
        }
        //当前路径对应的文件是不是一个html文件，如果不是也跳过
        if(begin_iter->path().extension() != ".html"){
          continue;
        }

        //把得到的路径加入到最终结果的vector中
        file_list->push_back(begin_iter->path().string());
        //因为vector里面放的是string类型
        //所以begin_iter->path也要转化成string模式
    }


    return true;
}

//找到html中的title标签
bool ParseTitle(const string& html, string *title){
    size_t beg = html.find("<title>");
    if(beg == string::npos){
        std::cout << "标题未找到" << std::endl;
        return false;
    }
    size_t end = html.find("</title>");
    if(end == string::npos){
        std::cout <<"标签未找到" <<std::endl;
        return false;
    }
    beg += string("<title>").size();
    if(beg >= end){
        std::cout <<"标题位置不合法" <<std::endl;
        return false;
    }
    *title = html.substr(beg, end-beg);
    return true;

}

//根据本地路径获取到在线文档的路径

bool ParseUrl(const string& file_path, string* url){
    string url_head = "https://www.boost.org/doc/libs/1_53_0/doc";
    string url_tail = file_path.substr(g_input_path.size());
    *url = url_head + url_tail;
    return true;
}

//获取正文
bool ParseContent(const string& html, string* content){
    //这里引入一个bool变量，当进入标签之后，此值为false
    //判断这个变量就能知道是在标签内还是在标签外了
    bool is_content = true;
    for(auto c : html){
        if(is_content){
            //当前是正文
            if(c == '<'){
                is_content = false;
            }
            else{
                //当前是普通字符，就把结果写入到content中
                if(c == '\n'){
                    c = ' ';    
                }
                content->push_back(c);
            }
        }
        else{
            //当前是在标签内
            if(c == '>')
            {
                //标签结束
                is_content = true;
            }
            //标签里的其他内容都直接忽略掉
        }
    }
    return true;
}

bool ParseFile(const string& file_path, DocInfo* doc_info){
    //1.先读取文件的内容
    // Read这样的函数是一个比较底层比较通用的函数，各个模块可能都会用到
    // 那么直接把这个函数放到common目录即可
    string html;
    bool ret = common::Util::Read(file_path, &html);
    if(!ret){
        std::cout << "解析文件失败！读取文件失败！" << std::endl;
        return false;
    }
    //2.根据文件内容解析出标题（html中有一个title标签）
    ret = ParseTitle(html, &doc_info->title);
    if(!ret){
      std::cout << "解析标题失败！" << std::endl;
      return false;
    }
    //3.根据文件的路径，构造出对应的在线文档的url
    ret = ParseUrl(file_path, &doc_info->url);
    if(!ret){
        std::cout << "解析url失败" << std::endl;
        return false;
    }
    //4.根据文件的内容，进行去标签，作为doc_info中的content字段的内容
    ret = ParseContent(html, &doc_info->content);
    if(!ret){
        std::cout <<"解析正文失败" << std::endl;
        return false;
    }
    return true;
}

//ofstream类是没有拷贝构造函数的（不能拷贝的）
//按照参数传的时候，只能引用或者指针
//此处还不能是const引用，否则无法执行里面的写文件操作
//每个doc_info中就是一行
void WriteOutput(const DocInfo& doc_info, std::ofstream& ofstream){
    ofstream << doc_info.title << "\3" << doc_info.url << "\3" << doc_info.content << std::endl;
}

//预处理过程的核心流程
//1.把input目录中所有的html路径都枚举出来
//2.根据枚举出来的路径依次读取每个文件内容，并进行解析
//3.把解析结果写入到最终的输出文件中
int main()
{
    //1.进行枚举路径
    vector<string> file_list;
    bool ret = EnumFile(g_input_path, &file_list);
    if(!ret){
        std::cout << "枚举路径失败!" <<std::endl;
        return 1;
    }
    //2,遍历枚举出来的路径，针对每个文件，单独进行处理
    std::ofstream output_file(g_output_path.c_str());
    if(!output_file.is_open()){
        std::cout << "打开output文件失败" << std::endl;
        return 1;
    }
    for(const auto& file_path : file_list){
        std::cout << file_path << std::endl;
        //先创建一个DocInfo对象
        DocInfo doc_info;
        //通过一个函数来负责这里的解析工作
        ret = ParseFile(file_path, &doc_info);
        if(!ret){
            std::cout <<"解析该文件失败: " << file_path << std::endl;
            continue;
        }
        //3.把解析出来的结果写入到最终的输出文件中
        WriteOutput(doc_info, output_file);
    }
    output_file.close();
    return 0;
}
