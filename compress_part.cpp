#include "Haffman_tree.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <mutex>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

void Haffman_tree::write_file_tree_to_file(std::filesystem::path const& file_path) {
  std::ofstream ofile(file_path, std::ios::binary | std::ios::app);
  //  auto pos         = ofile.tellp();
  size_t file_size = pre_order_file_name_.size();
  ofile.write(reinterpret_cast<char const*>(&file_size), sizeof(size_t));
  //  pos = ofile.tellp();
  for (auto const& str : pre_order_file_name_) {
    size_t file_name_size = str.size();
    ofile.write(reinterpret_cast<char const*>(&file_name_size), sizeof(size_t));
    ofile.write(reinterpret_cast<char const*>(str.c_str()), str.size() * sizeof(wchar_t));
    //    pos=ofile.tellp();
  }
  if (ofile) {
    std::cout << "succeeded to write file tree to file\n";
  } else {
    throw std::runtime_error("failed to write file_tree to file\n");
  }
}
std::unordered_map<unsigned char, uint64_t> Haffman_tree::asynchronous_statistics(
    std::filesystem::path const& file_path) {
  std::unordered_map<unsigned char, uint64_t> one_map;
  auto buff = read_origin_file(file_path);
  for (auto ch : buff) {
    one_map[ch]++;
  }
  return one_map;
}
void Haffman_tree::write_haffman_tree_to_file(std::filesystem::path const& file_path) {
  std::ofstream ofile(file_path, std::ios::binary | std::ios::trunc);
  if (!ofile.is_open()) {
    throw std::runtime_error("failed to open the filed in write_tree_to_file\n");
  }
  uint16_t code_num = record_queue_.size();
  //    std::cout <<"\n\n\n\n\n\nwrite:     "<< code_num << std::endl;
  ofile.write(reinterpret_cast<char const*>(&code_num), sizeof(uint16_t));
  while (!record_queue_.empty()) {
    auto temp_record = record_queue_.top();
    record_queue_.pop();
    ofile.write(reinterpret_cast<char const*>(&temp_record.node->data), sizeof(unsigned char));
    //       std::cout << temp_record.node->data<<"      ";
    ofile.write(reinterpret_cast<char const*>(&temp_record.frequncy), sizeof(uint64_t));
    //     std::cout << temp_record.frequncy << std::endl;
  }
  if (ofile) {
    std::cout << "succeeded to write haffman code to file\n";
  } else {
    throw std::runtime_error("failed to write haffman code to file\n");
  }
}
void Haffman_tree::write_code_to_file(std::filesystem::path const& file_path) {
  for (auto const& relative_path : pre_order_file_name_) {
    auto temp_path = root_path_ / std::filesystem::path(relative_path);
    if (!std::filesystem::is_directory(temp_path)) {
      process_one_origin_file(temp_path, file_path);
      //write_single_file_code_to_file(file_path, encode(read_origin_file(temp_path)));
      //如何并发处理文件 计算数据大小，分块内容，并发读取（文件分块读取，返回数据内容，数据大小），
      //编码（返回编码，有效编码位，编码大小），等待编码完全结束并发写入（流指针开始位置需要通过编码大小来计算，编码）
    }
  }
}
void Haffman_tree::process_one_origin_file(std::filesystem::path const& file_path,
                                           std::filesystem::path const& new_path) {
  auto buffer_vec = read_and_trunk_origin_file(file_path);
  std::vector<std::pair<uint8_t, std::vector<unsigned char>>> code_pair_vec;
  std::vector<std::future<std::pair<uint8_t, std::vector<unsigned char>>>> future_vec;
  for (size_t i = 1; i < buffer_vec.size(); i++) {
    future_vec.emplace_back(std::async(std::launch::async, Haffman_tree::encode, buffer_vec[i]));
  }
  code_pair_vec.push_back(encode(buffer_vec[0]));
  for (auto& temp_future : future_vec) {
    code_pair_vec.push_back(temp_future.get());
  }
  write_single_file_code_to_file(new_path, code_pair_vec);
}
void Haffman_tree::write_single_file_code_to_file(
    std::filesystem::path const& file_path,
    std::vector<std::pair<uint8_t, std::vector<unsigned char>>> const& code_pair_vec) {
  std::vector<size_t> code_size_vec;
  std::vector<size_t> offset_vec;
  std::vector<std::future<void>> future_vec;
  uint8_t thread_num = code_pair_vec.size();
  code_size_vec.resize(thread_num);
  offset_vec.resize(thread_num);
  for (size_t i = 0; i < thread_num; i++) {
    code_size_vec[i] = code_pair_vec[i].second.size() + sizeof(uint8_t);
  }
  std::partial_sum(code_size_vec.begin(), code_size_vec.end(), offset_vec.begin());
  std::ofstream ofile(file_path, std::ios::binary | std::ios::app);

  ofile.write(reinterpret_cast<char const*>(&thread_num), sizeof(uint8_t));
  std::cout << "thread num: " << (int)thread_num << ' ';
  //线程数+每块数据大小（最多2+每块有效位数+块数据（这样会不会显得数据有些冗余（雾（算了反正写着玩的
  //  pos = ofile.tellp();
  for (size_t i = 0; i < code_size_vec.size(); i++) {
    ofile.write(reinterpret_cast<char const*>(&code_size_vec[i]), sizeof(size_t));
    std::cout << "size: " << code_size_vec[i] << ' ';
    //  pos = ofile.tellp();
    //                   test_ofile << std::to_string(int(code_size_vec[i]) )<< ' ';
  }
  std::cout << std::endl;
  //test_ofile << std::endl;
  auto pos           = ofile.tellp();
  auto global_offset = static_cast<std::size_t>(pos);
  for (auto& offset : offset_vec) {
    offset += global_offset;
  }

  if (!ofile) {
    throw std::runtime_error("something wrong happened while getting offset in compressed file");
  }

  ofile.close();

  for (size_t i = 1; i < code_pair_vec.size(); i++) {
    future_vec.emplace_back(std::async(
        std::launch::async, Haffman_tree::write_trunked_code_to_file, file_path, offset_vec[i - 1], code_pair_vec[i]));
  }
  write_trunked_code_to_file(file_path, global_offset, code_pair_vec[0]);
  //test_ofile << std::endl;
  for (auto& temp_future : future_vec) {
    temp_future.get();
  }
}
void Haffman_tree::write_trunked_code_to_file(std::filesystem::path const& file_path,
                                              size_t offset,
                                              std::pair<uint8_t, std::vector<unsigned char>> const& code_pair) {
  auto code    = code_pair.second;
  auto ava_bit = code_pair.first;
  std::ofstream ofile(file_path, std::ios::binary | std::ios::ate);
  //  std::ofstream test_ofile("C:\\Users\\30408\\Desktop\\英语\\压缩时写文件输出.txt", std::ios::app);
  ofile.seekp(offset, std::ios::beg);
  if ((size_t)ofile.tellp() != offset) {
    throw std::runtime_error("something strange happened\n");
  }

  size_t code_size = code.size();
  ofile.write(reinterpret_cast<char const*>(&ava_bit), sizeof(uint8_t));
  mtx.lock();
  std::cout << "ava_bit:" << (int)ava_bit << ' ';
  std::cout << "code size:" << code_size << std::endl;
  std::cout << "written ava_bit offset: " << size_t(ofile.tellp()) << std::endl;
  mtx.unlock();
  //test_ofile << std::to_string((int)ava_bit) << ' ';

  //  std::cout << "write data size:" << code_size << std::endl;
  //  std::cout << "write begin pos:" << ofile.tellp() << std::endl;
  //ofile.write(reinterpret_cast<char const*>(&code_size), sizeof(size_t));
  ofile.write(reinterpret_cast<char const*>(code.data()), code_size * sizeof(unsigned char));
  //test_ofile <<std::to_string(int(code_size)) << ' ';
  //  std::cout << "write end pos:" << ofile.tellp() << std::endl;
  if (ofile) {
    std::cout << "succeeded to write" << std::endl;
  } else {
    throw std::runtime_error("something wrong happened in writing\n");
  }
  ofile.close();
}
std::vector<unsigned char> Haffman_tree::read_origin_file(std::filesystem::path const& file_path) {
  if (!std::filesystem::exists(file_path)) {
    throw std::runtime_error("empty path!\n");
  }
  if (!std::filesystem::is_regular_file(file_path)) {
    throw std::runtime_error("I have not done this part\n");
  }
  std::ifstream ifile(file_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open the file\n");
  }
  ifile.seekg(0, std::ios::end);
  size_t file_size = ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  std::vector<unsigned char> buffer;
  buffer.resize(file_size);
  ifile.read(reinterpret_cast<char*>(buffer.data()), file_size);
  if (ifile) {
    //std::cout << "succeeded to read the file " << ifile.gcount() << " bites" << std::endl;
  } else {
    throw std::runtime_error("failed to read the file\n");
  }
  ifile.close();
  return buffer;
}

std::vector<unsigned char> Haffman_tree::read_chunked_file(std::filesystem::path const& file_path,

                                                           size_t offset,
                                                           size_t read_size) {
  std::ifstream ifile(file_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open the file" + file_path.string());
  }
  ifile.seekg(offset, std::ios::beg);
  std::vector<unsigned char> buffer;
  buffer.resize(read_size);
  ifile.read(reinterpret_cast<char*>(buffer.data()), read_size);
  if (!ifile) {
    throw std::runtime_error("failed to read " + file_path.string());
  }
  ifile.close();
  return buffer;
}
std::vector<std::vector<unsigned char>> Haffman_tree::read_and_trunk_origin_file(
    std::filesystem::path const& file_path) {
  if (!std::filesystem::exists(file_path)) {
    throw std::runtime_error("empty path!\n");
  }
  if (!std::filesystem::is_regular_file(file_path)) {
    throw std::runtime_error("I have not done this part\n");
  }
  std::ifstream ifile(file_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to open the file\n");
  }
  ifile.seekg(0, std::ios::end);
  size_t file_size = ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  if (!ifile) {
    throw std::runtime_error("something wrong happened in counting the size of:" + file_path.string());
  }
  ifile.close();
  std::vector<std::vector<unsigned char>> buffer;
  uint8_t thread_num = 0;
  size_t first_size  = 0;
  std::vector<size_t> chunk_size;
  if (file_size < max_allowed_bites_in_one_thread * max_thread_num) {
    thread_num        = file_size / max_allowed_bites_in_one_thread + 1;
    size_t every_size = file_size / thread_num;
    first_size        = file_size - (thread_num - 1) * every_size;
    chunk_size.resize(thread_num, every_size);
    chunk_size[0] = first_size;
  } else {
    size_t every_size = file_size / max_thread_num;
    first_size        = file_size - every_size * (max_thread_num - 1);
    chunk_size.resize(max_thread_num, every_size);
    chunk_size[0] = first_size;
  }
  std::vector<size_t> offset(chunk_size.size());
  std::vector<std::future<std::vector<unsigned char>>> future_vec;
  std::partial_sum(chunk_size.begin(), chunk_size.end(), offset.begin());
  for (size_t i = 1; i < chunk_size.size(); i++) {
    future_vec.emplace_back(
        std::async(std::launch::async, Haffman_tree::read_chunked_file, file_path, offset[i - 1], chunk_size[i]));
  }
  buffer.push_back(read_chunked_file(file_path, 0, chunk_size[0]));
  for (auto& temp_future : future_vec) {
    buffer.push_back(temp_future.get());
  }
  return buffer;
}
void Haffman_tree::build_tree_from_original_file(
    std::filesystem::path const& file_path,
    std::vector<std::future<std::unordered_map<unsigned char, uint64_t>>>& static_future_count) {
  if (std::filesystem::is_directory(file_path)) {
    pre_order_file_name_.push_back(std::filesystem::relative(file_path, root_path_).wstring() + L'/');
    for (auto const& directory_entry : std::filesystem::directory_iterator{file_path}) {
      build_tree_from_original_file(directory_entry.path(), static_future_count);
    }
  } else {
    pre_order_file_name_.push_back(std::filesystem::relative(file_path, root_path_).wstring());
    static_future_count.emplace_back(std::async(Haffman_tree::asynchronous_statistics, file_path));
  }
}
void Haffman_tree::build_tree_from_original_file(std::filesystem::path const& file_path) {
  std::vector<std::future<std::unordered_map<unsigned char, uint64_t>>> static_future_count;
  build_tree_from_original_file(file_path, static_future_count);
  while (!static_future_count.empty()) {
    for (auto iter = static_future_count.begin(); iter != static_future_count.end();) {
      if (iter->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto one_map = iter->get();
        for (auto const& temp_pair : one_map) {
          temp_map_[temp_pair.first] += temp_pair.second;
        }
        static_future_count.erase(iter);
      } else {
        iter++;
      }
    }
  }
}
void Haffman_tree::tree_ini() {
  std::priority_queue<Frequency_record, std::vector<Frequency_record>, std::less<Frequency_record>> temp_queue;
  std::priority_queue<Frequency_record, std::vector<Frequency_record>, std::less<Frequency_record>> new_temp_queue;
  for (auto const& pair : temp_map_) {
    record_queue_.emplace(pair.second, pair.first);
    new_temp_queue.emplace(pair.second, pair.first);
  }

  while (new_temp_queue.size() > 0) {
    temp_queue.push(new_temp_queue.top());
    new_temp_queue.pop();
  }
  if (temp_queue.size() == 0) {
    return;
  }
  while (temp_queue.size() > 1) {
    auto temp_record1 = temp_queue.top();
    // std::cout << "compress bulid " << temp_record1.frequncy << "   " <<
    // temp_record1.node->data << std::endl;
    temp_queue.pop();
    auto temp_record2 = temp_queue.top();
    //  std::cout << "compress bulid " << temp_record2.frequncy << "   " <<
    //  temp_record2.node->data << std::endl;
    temp_queue.pop();
    auto new_record        = Frequency_record(temp_record1.frequncy + temp_record2.frequncy);
    new_record.node->left  = temp_record1.node;
    new_record.node->right = temp_record2.node;
    temp_queue.push(new_record);
  }
  root = temp_queue.top().node;
  temp_queue.pop();
  temp_map_.clear();
}
std::pair<uint8_t, std::vector<unsigned char>> Haffman_tree::encode(std::vector<unsigned char> const& str) {
  std::vector<unsigned char> result;
  std::string code;
  uint8_t temp_bit   = 0;
  unsigned char buff = 0;
  for (auto ch : str) {
    code = code_map[ch];
    for (auto bit : code) {
      if (bit == '1') {
        buff |= (1 << temp_bit);
      }
      temp_bit++;
      if (temp_bit >= 8) {
        result.push_back(buff);
        buff     = 0;
        temp_bit = 0;
      }
    }
  }
  if (temp_bit != 0) {
    result.push_back(buff);
  }

  return {temp_bit, result};
}

void Haffman_tree::compress_file(std::filesystem::path const& file_path) {
  if (!file_path.has_parent_path()) {
    throw std::runtime_error("failed to compress a root file\n");
  };
  root_path_ = file_path.parent_path();
  code_map.clear();
  root = nullptr;
  build_tree_from_original_file(file_path);
  tree_ini();
  init_hash_map();
  auto new_path = file_path;
  new_path.replace_extension(extension_);
  if (std::filesystem::exists(new_path)) {
    std::filesystem::path temp_path = new_path;
    std::wstring stem               = new_path.stem().wstring();

    int counter = 1;
    while (std::filesystem::exists(new_path)) {
      // 在文件名后面加上 (counter)
      new_path = root_path_ / (stem + L"(" + std::to_wstring(counter) + L")" + extension_);
      counter++;
    }
  }
  write_haffman_tree_to_file(new_path);
  write_file_tree_to_file(new_path);
  write_code_to_file(new_path);
}
void Haffman_tree::search_code(std::shared_ptr<Haffman_node> node,
                               unsigned char ch,
                               std::vector<unsigned char>& code,
                               std::vector<unsigned char>& temp_str) {
  if (node == nullptr) {
    return;
  }
  if (node->left == nullptr && node->right == nullptr && node->data == ch) {
    code = temp_str;
    return;
  }
  if (node->left != nullptr) {
    temp_str.push_back('0');
    search_code(node->left, ch, code, temp_str);
    temp_str.pop_back();
  }
  if (node->right != nullptr) {
    temp_str.push_back('1');
    search_code(node->right, ch, code, temp_str);
    temp_str.pop_back();
  }
}
void Haffman_tree::init_hash_map(std::shared_ptr<Haffman_node> node, std::string& code) {
  if (node == nullptr) {
    return;
  }
  if (node->left != nullptr) {
    code.push_back('0');
    init_hash_map(node->left, code);
    code.pop_back();
  }
  if (node->right != nullptr) {
    code.push_back('1');
    init_hash_map(node->right, code);
    code.pop_back();
  }
  if (node->left == nullptr && node->right == nullptr) {
    code_map.emplace(node->data, code);
  }
}
//D:\星露谷物语（Stardew Valley）v1.68 免安装中文版\Stardew.Valley.v1.6.8-GOG.part2.rar