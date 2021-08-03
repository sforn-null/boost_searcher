#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream> //读文件要用的头文件
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>//boost中切分字符串要用的头文件


using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::unordered_map;

namespace common{

  class Util{
  public:
      //负责从指定的路径中，读取文件的整体内容，读到output这个string里
      static bool Read(const string &input_path, string *output){
          std::ifstream file(input_path.c_str());
          if(!file.is_open()){
              return false;
         }
          //读取整个文件内容，思路很简单，只要按行读取就行了，把读到的每行的结果都追加到output中即可
          //getline 功能就是读取文件中的一行
          //如果读取成功，就把内容放到了line中，并返回true
          //如果失败（读到末尾），返回false
          string line;
          while (std::getline(file, line)){
              *output += (line + "\n");
          }
          file.close();
          return true;
      }

      //基于boost中的字符串切分，封装一下
      //delimiter表示分隔符，按照啥字符来切分
      //token_compress_off
      static void Split(const string& input, const string& delimiter, vector<string>* output){
          boost::split(*output, input, boost::is_any_of(delimiter), boost::token_compress_off);
      }
  };
}

