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

// 这个变量表示从哪个目录中读取boost文档的html
string g_input_path ="../data/input";

// 这个变量表示预处理模块输出结果放到哪里
string g_output_path ="../data/tmp/raw_input";

struct DocInfo{
    string title;//文档的标题
    string url;//文档的url
    string content;//文档的正文
};

bool EnumFile(const string &input_path, vector<string> *file_list){
    namespace fs = boost::filesystem;
    fs::path root_path(input_path);
    if(!fs::exists(root_path)){
      std::cout << "当前的目录不存在" << std::endl;
    }
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
    //bool变量，当进入标签之后，此值为false
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
    string html;
    bool ret = common::Util::Read(file_path, &html);
    if(!ret){
        std::cout << "解析文件失败！读取文件失败！" << std::endl;
        return false;
    }
    //根据文件内容解析出标题（html中有一个title标签）
    ret = ParseTitle(html, &doc_info->title);
    if(!ret){
      std::cout << "解析标题失败！" << std::endl;
      return false;
    }
    //根据文件的路径，构造出对应的在线文档的url
    ret = ParseUrl(file_path, &doc_info->url);
    if(!ret){
        std::cout << "解析url失败" << std::endl;
        return false;
    }
    //根据文件的内容，进行去标签，作为doc_info中的content字段的内容
    ret = ParseContent(html, &doc_info->content);
    if(!ret){
        std::cout <<"解析正文失败" << std::endl;
        return false;
    }
    return true;
}


void WriteOutput(const DocInfo& doc_info, std::ofstream& ofstream){
    ofstream << doc_info.title << "\3" << doc_info.url << "\3" << doc_info.content << std::endl;
}

//把input目录中所有的html路径都枚举出来
//根据枚举出来的路径依次读取每个文件内容，并进行解析
//把解析结果写入到最终的输出文件中
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
        DocInfo doc_info;
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
