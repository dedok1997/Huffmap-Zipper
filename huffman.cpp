#include "huffman.hpp"

#include <fstream>
#include <map>
#include <vector>
#include <queue>
#include <iostream>
#include <list>

namespace {

    const std::uint8_t pows[]{1, 2, 4, 8, 16, 32, 64, 128};


    struct Node {
        std::uint8_t c{};
        std::int32_t freq{};
        Node *l = nullptr;
        Node *r = nullptr;

        bool leaf() {
            return l == nullptr && r == nullptr;
        }

        explicit Node() = default;

        bool operator()(const Node *lhs, const Node *rhs) {
            return lhs->freq > rhs->freq;
        }

        virtual ~Node() {
            delete l; // NOLINT: Safe operation
            delete r; // NOLINT: Safe operation
        }
    };

    void read_codes_from_file(std::ifstream &ifstream, std::uint32_t *out,
                              std::uint32_t *bitsLength,
                              std::map<std::uint8_t, std::vector<std::uint8_t>> &result) {
        std::uint32_t codes_size = 0;
        std::uint32_t count = 0;
        std::uint8_t buffer;
        if (!ifstream.read(reinterpret_cast<char *>(&codes_size), sizeof(codes_size))) {
            return;
        }
        ifstream.read(reinterpret_cast<char *>(bitsLength), sizeof(*bitsLength));

        while (ifstream.read(reinterpret_cast<char *>(&buffer), 1)) {
            std::vector<std::uint8_t> codes;

            auto head = buffer;
            std::uint8_t codeSize;
            ifstream.read(reinterpret_cast<char *>(&codeSize), 1);
            ifstream.read(reinterpret_cast<char *>(&buffer), 1);
            count += 3;

            for (std::uint8_t i = 0; i < codeSize; ++i) {

                if ((buffer & pows[i % 8]) != 0) {
                    codes.push_back('1');
                } else {
                    codes.push_back('0');
                }
                if (i % 8 == 7 && i != codeSize - 1) {
                    ifstream.read(reinterpret_cast<char *>(&buffer), 1);
                    ++count;

                }
            }
            result[head] = codes;

            if (codes_size == count) {
                break;
            }
        }
        (*out) = count + 2 * sizeof(codes_size);
    }

    std::uint32_t write_codes_to_file(std::ofstream &ofstream, std::map<std::uint8_t, std::vector<std::uint8_t>> &codes,
                                      std::uint32_t bitLength) {
        std::uint32_t codesSize = 0;
        for (auto &it : codes) {
            codesSize += 2;
            codesSize += it.second.size() / 8;
            if (it.second.size() % 8 != 0) {
                codesSize += 1;
            }
        }
        ofstream.write(reinterpret_cast<char *>(&codesSize), sizeof(codesSize));
        ofstream.write(reinterpret_cast<char *>(&bitLength), sizeof(bitLength));

        for (auto &it : codes) {
            char c1 = static_cast<char>(it.first);
            ofstream.write(&c1, 1);
            char size = static_cast<char>(it.second.size());
            ofstream.write(&size, 1);
            std::uint8_t buffer = 0;

            size_t i = 0;
            while (i < it.second.size()) {
                if (it.second[i] == '1') {
                    buffer |= pows[i % 8];
                }
                if (((i % 8) == 7) || (i == (it.second.size() - 1))) {
                    auto code = static_cast<char>(buffer);
                    ofstream.write(&code, 1);
                    buffer = 0;
                }
                ++i;
            }

        }
        return codesSize + 2 * sizeof(codesSize);
    }

    std::uint32_t
    write_to_file(std::ifstream &ifstream, std::ofstream &ofstream,
                  std::map<std::uint8_t, std::vector<std::uint8_t>> &codes) {
        std::uint32_t result = 0;
        std::uint8_t symbol = 0;
        std::uint8_t index = 0;
        std::uint8_t write = 0;

        while (ifstream.read(reinterpret_cast<char *>(&symbol), 1)) {
            for (auto c: codes[symbol]) {
                if (c == '1') {
                    write |= pows[index];
                }
                ++index;
                if (index == 8) {
                    char code = static_cast<char>(write);
                    ofstream.write(&code, 1);
                    ++result;
                    index = 0;
                    write = 0;
                }
            }
        }
        if (index != 0) {
            char code = static_cast<char>(write);
            ofstream.write(&code, 1);
            ++result;
        }
        return result;
    }


    void build_freq(Node *root, std::map<std::uint8_t, std::vector<std::uint8_t>> &paths,
                    std::list<std::pair<std::uint8_t, std::vector<std::uint8_t>>> &sorted_paths,
                    std::list<std::uint8_t> &path, std::uint32_t *length) {
        if (root->leaf()) {
            auto c = root->c;
            if (path.empty()) {
                path = {'1'};
            }
            std::vector<std::uint8_t> resulted_path(path.begin(), path.end());
            paths[c] = resulted_path;
            sorted_paths.emplace_back(std::make_pair(c, resulted_path));
            (*length) += root->freq * resulted_path.size();
        } else {
            path.push_back('0');
            build_freq(root->l, paths, sorted_paths, path, length);
            path.pop_back();
            path.push_back('1');
            build_freq(root->r, paths, sorted_paths, path, length);
            path.pop_back();
        }
    }

    Node *numbers_to_tree(std::map<std::uint8_t, std::vector<std::uint8_t>> &codes) {
        auto root = new Node();
        for (auto const &pair : codes) {
            auto current = root;
            for (auto const &c: pair.second) {
                if (c == '0') {
                    if (current->l == nullptr) {
                        current->l = new Node();
                    }
                    current = current->l;
                } else {
                    if (current->r == nullptr) {
                        current->r = new Node();
                    }
                    current = current->r;
                }
            }
            current->c = pair.first;
        }

        return root;
    }


    void print_node(Node *node,
                    std::vector<std::uint8_t> &output) {
        if (node) {
            if (!node->leaf()) {
                print_node(node->l, output);
                print_node(node->r, output);
            } else {
                output.push_back(node->c);
            }
        }
    }

    void print(std::map<std::uint8_t, std::vector<std::uint8_t>> &codes) {
        auto node = numbers_to_tree(codes);
        std::vector<std::uint8_t> output;
        print_node(node, output);
        for (size_t i = 0; i < output.size(); ++i) {
            auto c = output[i];
            auto vector = codes[c];
            for (auto byte: vector) {
                std::cout << byte;
            }
            if (i < output.size() - 1) {
                std::cout << ' ' << static_cast<std::uint32_t>(c) << std::endl;
            } else {
                std::cout << ' ' << static_cast<std::uint32_t>(c);
            }
        }
        delete node;
    }
}

void compress(std::string const &from, std::string const &to, bool v) {
    std::ifstream f(from, std::ios_base::in | std::ios_base::binary);
    std::map<std::uint8_t, std::uint32_t> freq;
    std::priority_queue<Node *, std::vector<Node *>, Node> freqQueue;
    // std::map<std::uint32_t, Node *> tree;
    std::map<std::uint8_t, std::vector<std::uint8_t>> codes;
    std::uint32_t sourceSize = 0;
    std::uint8_t symbol = 0;
    while (f.read(reinterpret_cast<char *>(&symbol), 1)) {
        ++sourceSize;
        if (freq.find(symbol) == freq.end()) {
            ++freq[symbol];
        } else {
            ++freq[symbol];
        }
    }
    f.close();
    std::uint32_t bitLengthSize = 0;

    if (freq.empty()) {
        std::ofstream ofstream(to, std::ios_base::out | std::ios_base::trunc);
        ofstream.close();
        std::cout << 0 << std::endl;
        std::cout << 0 << std::endl;
        std::cout << 0;
        return;
    }

    for (auto &it : freq) {
        auto node = new Node();
        node->freq = it.second;
        node->c = it.first;
        freqQueue.push(node);
    }

    while (freqQueue.size() > 1) {
        auto first = freqQueue.top();
        freqQueue.pop();
        auto second = freqQueue.top();
        freqQueue.pop();
        auto newNode = new Node();
        newNode->freq = first->freq + second->freq;
        newNode->l = first;
        newNode->r = second;
        freqQueue.push(newNode);
    }
    auto root = freqQueue.top();

    std::list<std::uint8_t> path;
    std::list<std::pair<std::uint8_t, std::vector<std::uint8_t>>> sorted_paths;
    build_freq(root, codes, sorted_paths, path, &bitLengthSize);

    std::ofstream ofstream(to, std::ios_base::out | std::ios_base::binary);
    auto codesSize = write_codes_to_file(ofstream, codes, bitLengthSize);

    std::ifstream ifstream(from, std::ios_base::in | std::ios_base::binary);
    auto compressedSize = write_to_file(ifstream, ofstream, codes);

    size_t max = 0;
    for (const auto &c: codes) {

        if (c.second.size() > max) {
            max = c.second.size();
        }
    }
    ofstream.close();
    delete root;

    std::cout << sourceSize << std::endl;
    std::cout << compressedSize << std::endl;
    std::cout << codesSize << std::endl;
    if (v) {
        print(codes);
    }
}

void decompress(std::string const &from, std::string const &to, bool v) {
    std::ifstream ifstream(from, std::ios_base::in | std::ios_base::binary);
    std::ofstream ofstream(to, std::ios_base::out | std::ios_base::binary);
    std::uint32_t codesSize = 0;
    std::uint32_t bitLength = 0;
    std::uint32_t bitLengthCount = 0;
    std::map<std::uint8_t, std::vector<std::uint8_t>> codes;
    read_codes_from_file(ifstream, &codesSize, &bitLength, codes);

    auto tree = numbers_to_tree(codes);

    std::uint8_t number_8 = 0;
    auto current = tree;
    std::uint32_t compressed = 0;
    std::uint32_t resulted = 0;


    while (ifstream.read(reinterpret_cast<char *>(&number_8), 1)) {
        ++compressed;
        for (unsigned char i : pows) {
            ++bitLengthCount;
            if ((i & number_8) == i) {
                if (current->r == nullptr)
                    break;
                current = current->r;
            } else {
                if (current->l == nullptr)
                    break;
                current = current->l;
            }

            if (current->leaf()) {
                char code = static_cast<char>(current->c);
                ofstream.write(&code, 1);
                ++resulted;
                current = tree;
                if (bitLengthCount == bitLength) {
                    break;
                }
            }
        }
    }

    delete tree;
    std::cout << compressed << std::endl;
    std::cout << resulted << std::endl;
    std::cout << codesSize << std::endl;
    if (v) {
        print(codes);
    }
}

