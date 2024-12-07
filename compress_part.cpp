#include "Haffman_tree.h"
void Haffman_tree::write_file_tree_to_file(
    std::filesystem::path const& file_path) {
  std::ofstream ofile(file_path, std::ios::binary | std::ios::app);
  size_t file_size = pre_order_file_name_.size();
  ofile.write(reinterpret_cast<char const*>(&file_size), sizeof(size_t));
  for (auto const& str : pre_order_file_name_) {
    size_t file_name_size = str.size();
    ofile.write(reinterpret_cast<char const*>(&file_name_size), sizeof(size_t));
    ofile.write(reinterpret_cast<char const*>(str.c_str()),
                str.size() * sizeof(wchar_t));
  }
  if (ofile) {
    std::cout << "succeeded to write file tree to file\n";
  } else {
    throw std::runtime_error("failed to write file_tree to file\n");
  }
}
void Haffman_tree::write_haffman_tree_to_file(
    std::filesystem::path const& file_path) {
  std::ofstream ofile(file_path, std::ios::binary | std::ios::trunc);
  if (!ofile.is_open()) {
    throw std::runtime_error(
        "failed to open the filed in write_tree_to_file\n");
  }
  uint16_t code_num = record_queue_.size();
  //    std::cout <<"\n\n\n\n\n\nwrite:     "<< code_num << std::endl;
  ofile.write(reinterpret_cast<char const*>(&code_num), sizeof(uint16_t));
  while (!record_queue_.empty()) {
    auto temp_record = record_queue_.top();
    record_queue_.pop();
    ofile.write(reinterpret_cast<char const*>(&temp_record.node->data),
                sizeof(unsigned char));
    //       std::cout << temp_record.node->data<<"      ";
    ofile.write(reinterpret_cast<char const*>(&temp_record.frequncy),
                sizeof(uint64_t));
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
      write_single_file_code_to_file(file_path,
                                     encode(read_origin_file(temp_path)));
    }
  }
}
void Haffman_tree::write_single_file_code_to_file(
    std::filesystem::path const& file_path,
    std::pair<uint8_t, std::vector<unsigned char>> const& code_pair) {
  auto code = code_pair.second;
  auto ava_bit = code_pair.first;
  std::ofstream ofile(file_path, std::ios::binary | std::ios::app);
  ofile.write(reinterpret_cast<char const*>(&ava_bit), sizeof(uint8_t));
  size_t code_size = code.size();
  std::cout << std::endl;
  std::cout << "write data size:" << code_size << std::endl;
  ofile.write(reinterpret_cast<char const*>(&code_size), sizeof(size_t));
  ofile.write(reinterpret_cast<char const*>(code.data()),
              code_size * sizeof(unsigned char));
  if (ofile) {
    std::cout << "succeeded to write" << std::endl;
  } else {
    throw std::runtime_error("something wrong happened in writing\n");
  }
  ofile.close();
}
std::vector<unsigned char> Haffman_tree::read_origin_file(
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
  std::vector<unsigned char> buffer;
  buffer.resize(file_size);
  ifile.read(reinterpret_cast<char*>(buffer.data()), file_size);
  if (ifile) {
    std::cout << "succeeded to read the file " << ifile.gcount() << " bites"
              << std::endl;
  } else {
    throw std::runtime_error("failed to read the file\n");
  }
  ifile.close();
  return buffer;
}
void Haffman_tree::build_tree_from_original_file(
    std::filesystem::path const& file_path) {
  pre_order_file_name_.push_back(
      std::filesystem::relative(file_path, root_path_).wstring());
  if (std::filesystem::is_directory(file_path)) {
    for (auto const& directory_entry :
         std::filesystem::directory_iterator{file_path}) {
      build_tree_from_original_file(directory_entry.path());
    }
  } else {
    auto buffer = read_origin_file(file_path);
    statics(buffer);
  }
}
void Haffman_tree::tree_ini() {
  std::priority_queue<Frequency_record, std::vector<Frequency_record>,
                      std::less<Frequency_record>>
      temp_queue;
  std::priority_queue<Frequency_record, std::vector<Frequency_record>,
                      std::less<Frequency_record>>
      new_temp_queue;
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
    auto new_record =
        Frequency_record(temp_record1.frequncy + temp_record2.frequncy);
    new_record.node->left = temp_record1.node;
    new_record.node->right = temp_record2.node;
    temp_queue.push(new_record);
  }
  root_ = temp_queue.top().node;
  temp_queue.pop();
  temp_map_.clear();
}
std::pair<uint8_t, std::vector<unsigned char>> Haffman_tree::encode(
    std::vector<unsigned char> const& str) {
  std::vector<unsigned char> result;
  std::string code;
  uint8_t temp_bit = 0;
  unsigned char buff = 0;
  for (auto ch : str) {
    code = code_map_[ch];
    for (auto bit : code) {
      if (bit == '1') {
        buff |= (1 << temp_bit);
      }
      temp_bit++;
      if (temp_bit >= 8) {
        result.push_back(buff);
        buff = 0;
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
  build_tree_from_original_file(file_path);
  tree_ini();
  init_hash_map();
  auto new_path = file_path;
  new_path.replace_extension(extension_);
  if (std::filesystem::exists(new_path)) {
    std::filesystem::path temp_path = new_path;
    std::wstring stem = new_path.stem().wstring();

    int counter = 1;
    while (std::filesystem::exists(new_path)) {
      // 在文件名后面加上 (counter)
      new_path = root_path_ /
                 (stem + L"(" + std::to_wstring(counter) + L")" + extension_);
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
void Haffman_tree::init_hash_map(std::shared_ptr<Haffman_node> node,
                                 std::string& code) {
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
    code_map_.emplace(node->data, code);
  }
}