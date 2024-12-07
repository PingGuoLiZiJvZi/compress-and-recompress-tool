#pragma once
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
class Haffman_tree {
 public:
  struct Haffman_node {
    unsigned char data = '\0';
    std::shared_ptr<Haffman_node> left = nullptr;
    std::shared_ptr<Haffman_node> right = nullptr;
    Haffman_node() = default;
    explicit Haffman_node(unsigned char in_data) : data(in_data) {}
  };
  struct Frequency_record {
    uint64_t frequncy = 0;
    std::shared_ptr<Haffman_node> node;
    explicit Frequency_record(uint64_t fre, unsigned char in_data = '\0')
        : frequncy(fre) {
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
  std::shared_ptr<Haffman_node> root_;
  void show_code(std::shared_ptr<Haffman_node> node, std::string& code);
  void tree_ini();
  std::vector<std::wstring>
      pre_order_file_name_;  // 该前序遍历的文件名数组在压缩文件时构建，解压缩文件时用来重构文件结构（前序遍历重建文件树
  std::unordered_map<unsigned char, uint64_t> temp_map_;
  std::priority_queue<Frequency_record, std::vector<Frequency_record>,
                      std::less<Frequency_record>>
      record_queue_;
  std::filesystem::path root_path_;
  std::wstring changed_root_path_after_repeat_;
  size_t root_path_size_ = 0;
  std::unordered_map<unsigned char, std::string> code_map_;

 public:
  void init_hash_map() {
    std::string code;
    init_hash_map(root_, code);
  }
  void init_hash_map(std::shared_ptr<Haffman_node> node, std::string& code);
  std::streampos decode_single_file(std::filesystem::path const& read_path,
                                    std::filesystem::path const& write_path,
                                    std::streampos pos);
  void decompress_file(std::filesystem::path const& file_path,
                       std::streampos pos);
  std::streampos read_file_tree_from_file(
      std::filesystem::path const& file_path, std::streampos pos);
  std::streampos read_haffman_tree_from_file(
      std::filesystem::path const& file_path);
  void build_tree_from_original_file(std::filesystem::path const& file_path);

  void write_single_file_code_to_file(
      std::filesystem::path const& file_path,
      std::pair<uint8_t, std::vector<unsigned char>> const& code_pair);
  void write_code_to_file(std::filesystem::path const& file_path);
  void write_haffman_tree_to_file(std::filesystem::path const& file_path);
  void write_file_tree_to_file(std::filesystem::path const& file_path);

  static std::vector<unsigned char> read_origin_file(
      std::filesystem::path const& file_path);

  std::pair<uint8_t, std::vector<unsigned char>> encode(
      std::vector<unsigned char> const&
          str);  // 编码后的结构，有一个末尾有效位数和编码vec组成
  std::vector<unsigned char> decode(
      uint8_t ava_bit,
      std::vector<unsigned char> const& code);  // 单个文件压缩过程
  void show_code() {
    std::string str;
    show_code(root_, str);
  }
  std::vector<unsigned char> search_code(unsigned char ch) {
    std::vector<unsigned char> code;
    std::vector<unsigned char> temp_str;
    search_code(root_, ch, code, temp_str);
    return code;
  }
  void search_code(std::shared_ptr<Haffman_node> node, unsigned char ch,
                   std::vector<unsigned char>& code,
                   std::vector<unsigned char>& temp_str);
  void statics(std::vector<unsigned char> const& str) {
    for (auto ch : str) {
      temp_map_[ch]++;
    }
  }
  void compress_file(std::filesystem::path const& file_path);
  void decompress_file(std::filesystem::path const& file_path);
  static void show_menu();
};
