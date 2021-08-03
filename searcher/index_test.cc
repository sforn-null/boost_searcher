#include "searcher.h"

int main(){
    //先创建一个Index实例
    searcher::Index index;
    bool ret = index.Build("../data/tmp/raw_input");
    if(!ret){
        std::cout << "索引失败" << std::endl;
        return 1;
    }
    //如果索引构建成功，就调用索引中的相关函数，（查正排+查倒排）
    auto* inverted_list = index.GetInvertedList("filesystem");
    for(const auto& weight : *inverted_list){
        std::cout << "doc_id:" << weight.doc_id << "weight:" << weight.weight << std::endl;
        auto* doc_info = index.GetDocInfo(weight.doc_id);
        std::cout << "title" << doc_info->title << std::endl;
        std::cout << "url" << doc_info->url << std::endl;
        std::cout << "content" << doc_info->content << std::endl;
        std::cout << "***********************************************************************";
    }
    return 0;
}
