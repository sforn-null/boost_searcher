#include "searcher.h"

//通过这个函数来进行搜索模块的测试
//让用户通过控制台输入一个查询词
//调用searcher类的search函数来进行搜索
int main(){
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if(!ret){
        std::cout <<"Searcher 初始化失败" << std::endl;
        return 1;
    }

    while(true){
        std::cout << "Searcher >" <<std::flush;
        string query;
        std::cin >> query;
        if(!std::cin.good()){
            //读到EOF
          std::cout <<"goodbye" << std::endl;
          break;
        }
        string results;
        searcher.Search(query, &results);
        std::cout << results << std::endl;
    }

    return 0;
}
