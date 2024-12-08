#include "Haffman_tree.h"   // 或者其他合适的默认值

#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
std::shared_ptr<Haffman_tree::Haffman_node> Haffman_tree::root = nullptr;
std::unordered_map<unsigned char, std::string> Haffman_tree::code_map;
int main() {
  while (true) {
    try {
      Haffman_tree tree;
      tree.show_menu();
      char select = 0;
      std::string input;
      std::filesystem::path file_path;
      std::cout << "please input your select:\n";
      std::cin >> select;
      std::cin.get();
      switch (select) {
        case '1':
          std::cout << "Enter a file path: ";
          std::getline(std::cin, input);
          file_path = input;
          tree.compress_file(file_path);
          break;
        case '2':
          std::cout << "Enter a file path: ";
          std::getline(std::cin, input);
          file_path = input;
          tree.decompress_file(file_path);
          break;
        case '3':
          return 0;
          break;

        default:
          break;
      }
    } catch (std::runtime_error& error) {
      std::cerr << error.what() << std::endl;
      continue;
    }
  }
}