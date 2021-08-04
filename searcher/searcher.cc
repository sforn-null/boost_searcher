#include "searcher.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>
#include <string>
#include <algorithm>
#include <jsoncpp/json/json.h>

#include "../common/util.hpp"

namespace searcher {

//////////////////////////////////////////////////////////////////////////////////////////////////////
// 以下代码为 Index 模块的代码
//////////////////////////////////////////////////////////////////////////////////////////////////////

const char* const DICT_PATH = "../jieba_dict/jieba.dict.utf8";
const char* const HMM_PATH = "../jieba_dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "../jieba_dict/user.dict.utf8";
const char* const IDF_PATH = "../jieba_dict/idf.utf8";
const char* const STOP_WORD_PATH = "../jieba_dict/stop_words.utf8";

Index::Index() : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {
}

const DocInfo* Index::GetDocInfo(int64_t doc_id) {
    if (doc_id < 0 || doc_id >= forward_index.size()) {
        return nullptr;
    }
    return &forward_index[doc_id];
}

const InvertedList* Index::GetInvertedList(const string& key) {
    auto it = inverted_index.find(key);
    if (it == inverted_index.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Index::Build(const string& input_path) {
    // 1. 按行读取输入文件内容(上个环节预处理模块生成的 raw_input 文件)
    //    每一行又分成三个部分, 使用 \3 来切分, 分别是标题, url, 正文
    std::cerr << "开始构建索引" << std::endl;
    std::ifstream file(input_path.c_str());
    if (!file.is_open()) {
        std::cout << "raw_input 文件打开失败" << std::endl;
        return false;
    }
    string line;
    while (std::getline(file, line)) {
        // 2. 解析成 正排结构的DocInfo 对象, 并构造为正排索引
        DocInfo* doc_info = BuildForward(line);
        if (doc_info == nullptr) {
            std::cout << "构建正排失败!" << std::endl;
            continue;
        }
        // 3.构造成倒排索引.
        BuildInverted(*doc_info);

        //此时需要知道进度是多少，但是如果每一个都进行打印
        //每个数据都进行I/O操作的话过于影响效率
        if (doc_info->doc_id % 100 == 0) {
            std::cerr << doc_info->doc_id << std::endl;
        }
    }
    std::cerr << "结束构建索引" << std::endl;
    file.close();
    return true;
}

// 按照 \3 对 line 进行切分
// C++ 标准库中没有字符串切分的操作.
// 因此使用boost库中split
DocInfo* Index::BuildForward(const string& line) {
    // 1. 先把 line 按照 \3 切分成 3 个部分
    vector<string> tokens;
    common::Util::Split(line, "\3", &tokens);
    if (tokens.size() != 3) {
        return nullptr;
    }
    // 2. 把切分结果填充到 DocInfo 对象中
    DocInfo doc_info;
    doc_info.doc_id = forward_index.size();
    doc_info.title = tokens[0];
    doc_info.url = tokens[1];
    doc_info.content = tokens[2];
    forward_index.push_back(std::move(doc_info));
    return &forward_index.back();
}

// 倒排是一个 hash 表.
// key 是词 (针对文档的分词结果)
// value 是倒排拉链 (包含若干个 Weight 对象)
void Index::BuildInverted(const DocInfo& doc_info) {
    // 创建专门用于统计词频的结构
    struct WordCnt {
        int title_cnt;
        int content_cnt;
        WordCnt() : title_cnt(0), content_cnt(0) {
        }
    };
    unordered_map<string, WordCnt> word_cnt_map;
    // 针对标题进行分词
    vector<string> title_token;
    CutWord(doc_info.title, &title_token);
    // 遍历分词结果, 统计每个词出现的次数
    for (string word : title_token) {
        //将只是大小写不同的词当作同一个词来处理
        //因此都转化成小写
        boost::to_lower(word);
        ++word_cnt_map[word].title_cnt;
    }
    // 针对正文进行分词
    vector<string> content_token;
    CutWord(doc_info.content, &content_token);
    // 遍历分词结果, 统计每个词出现的次数
    for (string word : content_token) {
        boost::to_lower(word);
        ++word_cnt_map[word].content_cnt;
    }
    // 填充Weight对象
    for (const auto& word_pair : word_cnt_map) {
        // 构造 Weight 对象
        Weight weight;
        weight.doc_id = doc_info.doc_id;
        // 权重 = 标题出现次数 * 10 + 正文出现次数
        weight.weight = 10 * word_pair.second.title_cnt + word_pair.second.content_cnt;
        weight.word = word_pair.first;

        // 把 weight 对象插入到倒排索引中
        InvertedList& inverted_list = inverted_index[word_pair.first];
        inverted_list.push_back(std::move(weight));
    }
}

void Index::CutWord(const string& input, vector<string>* output) {
    jieba.CutForSearch(input, *output);
}

// 以下代码为 Searcher 模块的代码

bool Searcher::Init(const string& input_path) {
    return index->Build(input_path);
}

// 搜索查询词, 得到搜索结果.
bool Searcher::Search(const string& query, string* output) {
    // 针对查询词进行分词
    vector<string> tokens;
    index->CutWord(query, &tokens);
    // 根据分词结果, 查倒排, 把相关的文档都获取到
    vector<Weight> all_token_result;
    for (string word : tokens) {
        boost::to_lower(word);
        auto* inverted_list = index->GetInvertedList(word);
        if (inverted_list == nullptr) {
            continue;
        }
        //  将搜索到的结果都合并到一起进行统一的排序
        all_token_result.insert(all_token_result.end(), inverted_list->begin(), inverted_list->end());
    }
    std::sort(all_token_result.begin(), all_token_result.end(), [](const Weight& w1, const Weight& w2) {
        return w1.weight > w2.weight;
    });

    // 把得到的这些倒排拉链中的 文档 id 获取到, 然后去查正排.
    Json::Value results; 
    for (const auto& weight : all_token_result) {
        // 根据 weight 中的 doc_id 查正排
        const DocInfo* doc_info = index->GetDocInfo(weight.doc_id);
        // 把这个 doc_info 对象再进一步的包装成一个 JSON 对象
        Json::Value result;
        result["title"] = doc_info->title;
        result["url"] = doc_info->url;
        result["desc"] = GenerateDesc(doc_info->content, weight.word);
        results.append(result);
    }
    // 把得到的 results 这个 JSON 对象序列化成字符串. 写入 output 中
    Json::FastWriter writer;
    *output = writer.write(results);
    return true;
}

string Searcher::GenerateDesc(const string& content, const string& word) {
    // 1. 先找到 word 在正文中出现的位置.
    size_t first_pos = content.find(word);
    size_t desc_beg = 0;
    if (first_pos == string::npos) {
        // 如果找不到, 就直接从头开始作为起始位置.
        if (content.size() < 160) {
            return content;
        }
        string desc = content.substr(0, 160);
        desc[desc.size() - 1] = '.';
        desc[desc.size() - 2] = '.';
        desc[desc.size() - 3] = '.';
        return desc;
    }
    // 2. 找到了 first_pos 位置, 以这个位置为基准, 往前找一些字节.
    desc_beg = first_pos < 80 ? 0 : first_pos - 80;
    if (desc_beg + 160 >= content.size()) {
        // desc_beg 后面的内容不够 160 了, 直接到末尾结束即可
        return content.substr(desc_beg);
    } else {
        string desc = content.substr(desc_beg, 160);
        desc[desc.size() - 1] = '.';
        desc[desc.size() - 2] = '.';
        desc[desc.size() - 3] = '.';
        return desc;
    }
}

} // namespace searcher
