#pragma once
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

static uint8_t const max_thread_num                   = 12;
static uint64_t const max_allowed_bites_in_one_thread = static_cast<uint64_t>(36) * 1024 * 1024;

class Haffman_tree {
 public:
  static std::mutex mtx;
  struct Haffman_node {
    unsigned char data                  = '\0';
    std::shared_ptr<Haffman_node> left  = nullptr;
    std::shared_ptr<Haffman_node> right = nullptr;
    Haffman_node()                      = default;
    explicit Haffman_node(unsigned char in_data) : data(in_data) {}
  };
  struct Frequency_record {
    uint64_t frequncy = 0;
    std::shared_ptr<Haffman_node> node;
    explicit Frequency_record(uint64_t fre, unsigned char in_data = '\0') : frequncy(fre) {
      node = std::make_shared<Haffman_node>(in_data);
    }

    bool operator>(Frequency_record const& other) const {
      return frequncy > other.frequncy;
    }
    bool operator<(Frequency_record const& other) const {
      return frequncy > other.frequncy;
    }
    bool operator==(Frequency_record const& other) const {
      return frequncy == other.frequncy;
    }
  };

 private:
  std::wstring extension_ = L".ipp";
  static std::shared_ptr<Haffman_node> root;
  void show_code(std::shared_ptr<Haffman_node> node, std::string& code);
  void tree_ini();
  std::vector<std::wstring>
      pre_order_file_name_;  // 该前序遍历的文件名数组在压缩文件时构建，解压缩文件时用来重构文件结构（前序遍历重建文件树
  std::unordered_map<unsigned char, uint64_t> temp_map_;
  std::priority_queue<Frequency_record, std::vector<Frequency_record>, std::less<Frequency_record>> record_queue_;
  std::filesystem::path root_path_;
  std::wstring changed_root_path_after_repeat_;
  size_t root_path_size_ = 0;
  static std::unordered_map<unsigned char, std::string> code_map;

 public:
  void init_hash_map() {
    std::string code;
    init_hash_map(root, code);
  }
  static void write_origin_trunk_to_file(std::filesystem::path const& file_path,
                                         size_t offset,
                                         std::vector<unsigned char> const& message);
  static std::vector<unsigned char> read_single_trunk(std::filesystem::path const& file_path,
                                                      size_t offset,
                                                      size_t file_size);
  void build_tree_from_original_file(
      std::filesystem::path const& file_path,
      std::vector<std::future<std::unordered_map<unsigned char, uint64_t>>>& static_future_count);
  void init_hash_map(std::shared_ptr<Haffman_node> node, std::string& code);
  std::streampos decode_single_file(std::filesystem::path const& read_path,
                                    std::filesystem::path const& write_path,
                                    std::streampos pos);
  void decompress_file(std::filesystem::path const& file_path, std::streampos pos);
  std::streampos read_file_tree_from_file(std::filesystem::path const& file_path, std::streampos pos);
  std::streampos read_haffman_tree_from_file(std::filesystem::path const& file_path);
  void build_tree_from_original_file(std::filesystem::path const& file_path);
  static std::unordered_map<unsigned char, uint64_t> asynchronous_statistics(std::filesystem::path const& file_path);
  static void write_single_file_code_to_file(
      std::filesystem::path const& file_path,
      std::vector<std::pair<uint8_t, std::vector<unsigned char>>> const& code_pair_vec);
  static std::vector<unsigned char> read_origin_file(std::filesystem::path const& file_path);
  static void write_trunked_code_to_file(std::filesystem::path const& file_path,
                                         size_t offset,
                                         std::pair<uint8_t, std::vector<unsigned char>> const& code_pair_vec);
  void write_code_to_file(std::filesystem::path const& file_path);
  void write_haffman_tree_to_file(std::filesystem::path const& file_path);
  void write_file_tree_to_file(std::filesystem::path const& file_path);
  static void process_one_origin_file(std::filesystem::path const& file_path, std::filesystem::path const& new_path);
  static std::vector<std::vector<unsigned char>> read_and_trunk_origin_file(std::filesystem::path const& file_path);
  static std::vector<unsigned char> read_chunked_file(std::filesystem::path const& file_path,
                                                      size_t offset,
                                                      size_t read_size);
  static std::pair<uint8_t, std::vector<unsigned char>> encode(
      std::vector<unsigned char> const& str);  // 编码后的结构，有一个末尾有效位数和编码vec组成
  static std::vector<unsigned char> decode(uint8_t ava_bit,
                                           std::vector<unsigned char> const& code);  // 单个文件压缩过程
  void show_code() {
    std::string str;
    show_code(root, str);
  }
  std::vector<unsigned char> search_code(unsigned char ch) {
    std::vector<unsigned char> code;
    std::vector<unsigned char> temp_str;
    search_code(root, ch, code, temp_str);
    return code;
  }
  void search_code(std::shared_ptr<Haffman_node> node,
                   unsigned char ch,
                   std::vector<unsigned char>& code,
                   std::vector<unsigned char>& temp_str);
  void statics(std::vector<unsigned char> const& str);
  void compress_file(std::filesystem::path const& file_path);
  void decompress_file(std::filesystem::path const& file_path);
  static void show_menu();
};
/*
尝试加入并发模块
压缩文件区：
          统计文件并初始化哈夫曼树
              并发加持：每一个文件都有一个独立的哈希表，在统计结束之后合并 完成
          编码文件并写入压缩文件
              并发加持：单个文件中并发编码文件（设计为，每30个MB并发一次，线程上限为12个，若超过12个，则设计为每个线程读整个文件大小的1/12，余数加在第一个线程上）(问题，编码之后会有遗留的有效位数问题），之后并发写入文件
解压缩文件区：
          读取文件，重构哈夫曼树，还原文件结构
              并发加持：单个文件中并发解编码文件（问题，随意选择unsigned char读入，可能导致编码被中断），之后并发写入文件
*/