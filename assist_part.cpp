#include "Haffman_tree.h"

void Haffman_tree::show_code(std::shared_ptr<Haffman_node> node,
                             std::string& code) {
  if (node == nullptr) {
    return;
  }
  if (node->left != nullptr) {
    code.push_back('0');
    show_code(node->left, code);
    code.pop_back();
  }
  if (node->right != nullptr) {
    code.push_back('1');
    show_code(node->right, code);
    code.pop_back();
  }
  if (node->left == nullptr && node->right == nullptr) {
    std::cout << node->data << ": " << code << std::endl;
  }
}
void Haffman_tree::show_menu() {
  std::cout << "1:compress a file\n";
  std::cout << "2:decompress a file\n";
  std::cout << "3:exit\n";
}