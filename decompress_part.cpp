#include "Haffman_tree.h"
std::streampos Haffman_tree::decode_single_file(std::filesystem::path const& read_path,
                                                std::filesystem::path const& write_path,
                                                std::streampos pos) {
  std::ifstream ifile(read_path, std::ios::binary);
  if (!ifile.is_open()) {
    throw std::runtime_error("failed to read from compressed file\n");
  }
  ifile.seekg(pos);
  uint8_t ava_bit  = 0;
  size_t code_size = 0;
  std::vector<unsigned char> code;
  // auto temp_pos = ifile.tellg();
  ifile.read(reinterpret_cast<char*>(&ava_bit), sizeof(uint8_t));
  // temp_pos = ifile.tellg();
  ifile.read(reinterpret_cast<char*>(&code_size), sizeof(size_t));
  // temp_pos = ifile.tellg();
  //   ifile.seekg(std::streamoff(pos) + sizeof(uint8_t) + sizeof(size_t),
  //   std::ios::beg);
  code.resize(code_size);
  std::cout << "read data size:" << code_size << "\n";
  // ifile.seekg(0, std::ios::end);
  // std::cout << ifile.tellg() - pos << std::endl;
  ifile.read(reinterpret_cast<char*>(code.data()), code_size * sizeof(unsigned char));

  if (!ifile) {
    throw std::runtime_error(" failed to read from compressed file in decompressing " + write_path.string() + "\n");
  }
  std::cout << "succeeded to read from compressed file" << write_path.string() << std::endl;
  pos          = ifile.tellg();
  auto message = decode(ava_bit, code);
  ifile.close();

  std::ofstream ofile(write_path);
  if (!ofile.is_open()) {
    throw std::runtime_error("failed to open target file" + write_path.string() + "\n");
  }
  ofile.write(reinterpret_cast<char const*>(message.data()), message.size() * sizeof(unsigned char));
  if (!ofile) {
    throw std::runtime_error("failed to write target file" + write_path.string() + "\n");
  }
  ofile.close();
  return pos;
}
void Haffman_tree::decompress_file(std::filesystem::path const& file_path, std::streampos pos) {
  auto root_file                  = pre_order_file_name_[0];
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
    auto temp_path          = root_path_ / temp_relative_path;
    if (!temp_path.has_extension()) {
      std::filesystem::create_directory(temp_path);
    } else {
      pos = decode_single_file(file_path, temp_path, pos);
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
    root= record_queue_.top().node;
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
}