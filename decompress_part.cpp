#include "Haffman_tree.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <iostream>
#include <mutex>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <vector>

void Haffman_tree::write_origin_trunk_to_file(std::filesystem::path const& file_path,
                                              size_t offset,
                                              std::vector<unsigned char> const& message) {
  std::ofstream ofile(file_path, std::ios::binary | std::ios::app);
  ofile.seekp(offset, std::ios::beg);
  if (!ofile.is_open()) {
    throw std::runtime_error("failed to open target file" + file_path.string() + "\n");
  }
  ofile.write(reinterpret_cast<char const*>(message.data()), message.size() * sizeof(unsigned char));
  std::cout << "a trunk written\n";
  if (!ofile) {
    throw std::runtime_error("failed to write target file" + file_path.string() + "\n");
  }
  ofile.close();
}

std::vector<unsigned char> Haffman_tree::read_single_trunk(
    std::filesystem::path const& file_path,
    size_t offset,
    size_t file_size) {  //file size是经过计算后减去bit位的file——size
  std::ifstream ifile(file_path, std::ios::binary | std::ios::in);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open the file trunk\n");
  }
  ifile.seekg(offset, std::ios::beg);
  auto pos        = ifile.tellg();
  uint8_t ava_bit = 0;
  ifile.read(reinterpret_cast<char*>(&ava_bit), sizeof(uint8_t));
  if (!ifile) {
    throw std::runtime_error("failed after read the bit");
  }
  std::vector<unsigned char> code;
  code.resize(file_size);
  ifile.read(reinterpret_cast<char*>(code.data()), file_size * sizeof(unsigned char));
  mtx.lock();
  std::cout << "ava_bit:" << (int)ava_bit << ' ';
  std::cout << "code size:" << file_size << std::endl;
  std::cout << "read ava_bit_pos:" << (size_t)ifile.tellg() << std::endl;
  std::cout << "begin read pos:" << (size_t)pos << std::endl;
  mtx.unlock();

  if (!ifile) {
    throw std::runtime_error(" failed to read from compressed file in decompressing " + file_path.string() + "\n");
  }
  ifile.close();
  code = decode(ava_bit, code);
  return code;
}
std::streampos Haffman_tree::decode_single_file(std::filesystem::path const& read_path,
                                                std::filesystem::path const& write_path,
                                                std::streampos pos) {
  std::ifstream ifile(read_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to read from compressed file\n");
  }
  ifile.seekg(pos);
  uint8_t thread_num = 0;
  std::vector<size_t> code_size;
  std::vector<std::vector<unsigned char>> result;
  std::vector<std::future<std::vector<unsigned char>>> future_vec;
  std::vector<size_t> offset;
  ifile.read(reinterpret_cast<char*>(&thread_num), sizeof(uint8_t));
  std::cout << "thread_num:" << (int)thread_num << ' ';
  if (thread_num > max_thread_num) {
    throw std::runtime_error("read too much threads");
  }
  code_size.resize(thread_num);
  offset.resize(thread_num);
  for (size_t i = 0; i < thread_num; i++) {
    ifile.read(reinterpret_cast<char*>(&code_size[i]), sizeof(size_t));
  }
  std::cout << std::endl;
  std::partial_sum(code_size.begin(), code_size.end(), offset.begin());
  for (auto& num : offset) {
    num += ((size_t)pos + sizeof(uint8_t) + thread_num * sizeof(size_t));
  }
  for (auto& num : code_size) {
    num -= sizeof(uint8_t);
  }
  for (int i = 1; i < thread_num; i++) {
    future_vec.emplace_back(
        std::async(std::launch::async, Haffman_tree::read_single_trunk, read_path, offset[i - 1], code_size[i]));
  }
  result.emplace_back(
      read_single_trunk(read_path, (size_t)pos + sizeof(uint8_t) + thread_num * sizeof(size_t), code_size[0]));
  for (auto& res : future_vec) {
    result.emplace_back(res.get());
  }
  if (!ifile) {
    throw std::runtime_error("failed to read file" + write_path.string());
  }
  ifile.seekg(offset[offset.size() - 1], std::ios::beg);
  pos = ifile.tellg();
  ifile.close();
  for (int i = 0; i < thread_num; i++) {
    code_size[i] = result[i].size();
  }
  std::partial_sum(code_size.begin(), code_size.end(), offset.begin());
  std::vector<std::future<void>> void_future_vec;
  for (int i = 1; i < thread_num; i++) {
    void_future_vec.emplace_back(
        std::async(std::launch::async, Haffman_tree::write_origin_trunk_to_file, write_path, offset[i - 1], result[i]));
  }
  write_origin_trunk_to_file(write_path, 0, result[0]);
  for (auto& future : void_future_vec) {
    future.get();
  }
  return pos;
}
void Haffman_tree::decompress_file(std::filesystem::path const& file_path, std::streampos pos) {
  std::wstring root_file;
  if (pre_order_file_name_[0][pre_order_file_name_[0].size() - 1] == L'/') {
    root_file = pre_order_file_name_[0].substr(0, pre_order_file_name_[0].size() - 1);
  } else {
    root_file = pre_order_file_name_[0];
  }
  root_path_size_                 = root_file.size();
  root_path_                      = file_path.parent_path();
  auto new_path                   = file_path.parent_path() / root_file;
  auto extension                  = new_path.extension();
  root_path_size_                 = root_path_size_ - extension.string().size();
  changed_root_path_after_repeat_ = root_file.substr(0, root_path_size_);
  if (std::filesystem::exists(new_path)) {
    std::filesystem::path temp_path = new_path;
    std::wstring stem               = new_path.stem().wstring();

    int counter = 1;
    while (std::filesystem::exists(new_path)) {
      // 在文件名后面加上 (counter)
      new_path = root_path_ / (stem + L"(" + std::to_wstring(counter) + L")" + extension.wstring());
      counter++;
    }
    changed_root_path_after_repeat_ += (L"(" + std::to_wstring(--counter) + L")");
    root_path_size_                 += extension.wstring().size();
  }
  changed_root_path_after_repeat_ += extension.wstring();
  for (size_t i = 0; i < pre_order_file_name_.size(); i++) {
    auto temp_relative_path = changed_root_path_after_repeat_ + pre_order_file_name_[i].substr(root_path_size_);

    if (temp_relative_path[temp_relative_path.size() - 1] == L'/') {
      temp_relative_path = temp_relative_path.substr(0, temp_relative_path.size() - 1);
      auto temp_path     = root_path_ / temp_relative_path;
      std::filesystem::create_directory(temp_path);
    } else {
      auto temp_path = root_path_ / temp_relative_path;
      pos            = decode_single_file(file_path, temp_path, pos);
    }
  }
}
std::streampos Haffman_tree::read_file_tree_from_file(std::filesystem::path const& file_path, std::streampos pos) {
  std::ifstream ifile(file_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open file_tree_from_file");
  }
  ifile.seekg(pos);
  size_t file_num = 0;
  ifile.read(reinterpret_cast<char*>(&file_num), sizeof(size_t));
  std::wstring file_name;
  size_t file_name_size = 0;
  for (size_t i = 0; i < file_num; i++) {
    ifile.read(reinterpret_cast<char*>(&file_name_size), sizeof(size_t));
    file_name.resize(file_name_size);
    ifile.read(reinterpret_cast<char*>(file_name.data()), file_name_size * sizeof(wchar_t));
    pre_order_file_name_.push_back(file_name);
  }
  if (!ifile) {
    throw std::runtime_error("failed to read file_name from file\n");
  }
  std::cout << "succeeded to read file_name from file\n";
  pos = ifile.tellg();
  ifile.close();
  return pos;
}
std::streampos Haffman_tree::read_haffman_tree_from_file(std::filesystem::path const& file_path) {
  std::ifstream ifile(file_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open haffman_tree_from_file");
  }

  uint16_t code_size = 0;
  unsigned char buff = 0;
  uint64_t freq      = 0;
  ifile.read(reinterpret_cast<char*>(&code_size), sizeof(uint16_t));
  if (code_size != 0) {
    for (size_t i = 0; i < code_size; i++) {
      ifile.read(reinterpret_cast<char*>(&buff), sizeof(unsigned char));
      ifile.read(reinterpret_cast<char*>(&freq), sizeof(uint64_t));
      record_queue_.emplace(freq, buff);
    }
    if (!ifile) {
      throw std::runtime_error("failed to read haffman_tree_from_file\n");
    }
    std::cout << "succeeded to read haffman tree\n";
    while (record_queue_.size() > 1) {
      auto temp_record1 = record_queue_.top();
      record_queue_.pop();
      auto temp_record2 = record_queue_.top();
      record_queue_.pop();
      auto new_record        = Frequency_record(temp_record1.frequncy + temp_record2.frequncy);
      new_record.node->left  = temp_record1.node;
      new_record.node->right = temp_record2.node;
      record_queue_.push(new_record);
    }
    root = record_queue_.top().node;
    record_queue_.pop();
  }
  std::cout << "succeeded to rebuild haffman tree\n";
  auto pos = ifile.tellg();
  ifile.close();
  return pos;
}
std::vector<unsigned char> Haffman_tree::decode(uint8_t ava_bit, std::vector<unsigned char> const& code) {
  std::shared_ptr<Haffman_node> node = root;
  std::vector<unsigned char> result;

  for (size_t i = 0; i < code.size(); i++) {
    for (uint8_t temp_bit = 0; temp_bit < 8; temp_bit++) {
      if ((code[i] & (1 << temp_bit)) == 0) {
        node = node->left;
      } else {
        node = node->right;
      }
      if (node->left == nullptr && node->right == nullptr) {
        result.push_back(node->data);
        node = root;
        if (i == code.size() - 1 && ava_bit != 0 && temp_bit + 1 >= ava_bit) {
          break;
        }
      }
    }
  }
  return result;
}
void Haffman_tree::decompress_file(std::filesystem::path const& file_path) {
  if (file_path.extension() != ".ipp") {
    throw std::runtime_error("only .ipp file could be decompressed\n");
  }
  code_map.clear();
  root     = nullptr;
  auto pos = read_haffman_tree_from_file(file_path);
  pos      = read_file_tree_from_file(file_path, pos);
  decompress_file(file_path, pos);
} /*
C:\Users\30408\Desktop\英语\2023年06月大学英语4级真题与解析(全三套).ipp
C:\Users\30408\Desktop\英语\2023年06月大学英语4级真题与解析(全三套)
C:\Users\30408\Desktop\测试文件夹
C:\Users\30408\Desktop\测试文件夹.ipp
*/