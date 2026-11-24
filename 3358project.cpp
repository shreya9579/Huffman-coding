#include <iostream>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <iterator>  // std::istreambuf_iterator
#include <stdexcept> // std::runtime_error

struct Node {
    int symbol; // 0..255 for leaves; -1 for an internal node
    std::uint64_t frequency;
    std::uint64_t order;
    Node* left;
    Node* right;
    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

struct MinFrequency {
    bool operator()(const Node* a, const Node* b) const {
        if (a->frequency != b->frequency) return a->frequency > b->frequency;
        return a->order > b->order;
    }
};

using Frequencies = std::array<std::uint64_t, 256>;

static std::vector<unsigned char> readBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input file: " + path);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

static Frequencies countFrequencies(const std::vector<unsigned char>& data) {
    Frequencies freq{};
    for (unsigned char b : data) ++freq[b];
    return freq;
}

static Node* buildTree(const Frequencies& freq,
                       std::vector<std::unique_ptr<Node>>& storage) {
    std::priority_queue<Node*, std::vector<Node*>, MinFrequency> heap;
    std::uint64_t order = 0;
    for (int symbol = 0; symbol < 256; ++symbol) {
        if (freq[symbol] != 0) {
            storage.push_back(std::make_unique<Node>(Node{symbol, freq[symbol], order++, nullptr, nullptr}));
            heap.push(storage.back().get());
        }
    }
    while (heap.size() > 1) {
        Node* left = heap.top(); heap.pop();
        Node* right = heap.top(); heap.pop();
        storage.push_back(std::make_unique<Node>(Node{-1, left->frequency + right->frequency,
                                                        order++, left, right}));
        heap.push(storage.back().get());
    }
    return heap.empty() ? nullptr : heap.top();
}

static void makeCodes(const Node* node, const std::string& path,
                      std::array<std::string, 256>& codes) {
    if (!node) return;
    if (node->isLeaf()) {
        codes[static_cast<std::size_t>(node->symbol)] = path.empty() ? "0" : path;
        return;
    }
    makeCodes(node->left, path + '0', codes);
    makeCodes(node->right, path + '1', codes);
}

static void writeU64(std::ostream& out, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) out.put(static_cast<char>((value >> (8 * i)) & 0xff));
}

static std::uint64_t readU64(std::istream& in) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        int c = in.get();
        if (c == EOF) throw std::runtime_error("Compressed file has a truncated header.");
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(c)) << (8 * i);
    }
    return value;
}

static void compressFile(const std::string& inputPath, const std::string& outputPath) {
    const auto data = readBytes(inputPath);
    const auto freq = countFrequencies(data);
    std::vector<std::unique_ptr<Node>> storage;
    Node* root = buildTree(freq, storage);
    std::array<std::string, 256> codes{};
    makeCodes(root, "", codes);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create output file: " + outputPath);
    out.write("HUF1", 4);
    writeU64(out, static_cast<std::uint64_t>(data.size()));
    for (std::uint64_t f : freq) writeU64(out, f);

    unsigned char packed = 0;
    int usedBits = 0;
    for (unsigned char symbol : data) {
        for (char bit : codes[symbol]) {
            packed = static_cast<unsigned char>((packed << 1) | (bit == '1'));
            if (++usedBits == 8) { out.put(static_cast<char>(packed)); packed = 0; usedBits = 0; }
        }
    }
    if (usedBits != 0) {
        packed = static_cast<unsigned char>(packed << (8 - usedBits));
        out.put(static_cast<char>(packed));
    }
    if (!out) throw std::runtime_error("Failed while writing compressed data.");
    std::cout << "Compressed " << data.size() << " bytes into " << out.tellp()
              << " bytes (header included).\n";
}

static void decompressFile(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream in(inputPath, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open compressed file: " + inputPath);
    char magic[4];
    if (!in.read(magic, 4) || std::string(magic, 4) != "HUF1")
        throw std::runtime_error("Not a supported HUF1 compressed file.");
    const std::uint64_t originalSize = readU64(in);
    Frequencies freq{};
    for (auto& f : freq) f = readU64(in);
    std::vector<std::unique_ptr<Node>> storage;
    Node* root = buildTree(freq, storage);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot create restored file: " + outputPath);
    if (originalSize == 0) { std::cout << "Restored empty file.\n"; return; }
    if (!root) throw std::runtime_error("Header contains no tree for non-empty data.");
    if (root->isLeaf()) {
        for (std::uint64_t i = 0; i < originalSize; ++i) out.put(static_cast<char>(root->symbol));
    } else {
        Node* current = root;
        std::uint64_t produced = 0;
        char raw;
        while (produced < originalSize && in.get(raw)) {
            unsigned char byte = static_cast<unsigned char>(raw);
            for (int bit = 7; bit >= 0 && produced < originalSize; --bit) {
                current = ((byte >> bit) & 1) ? current->right : current->left;
                if (current->isLeaf()) {
                    out.put(static_cast<char>(current->symbol));
                    ++produced;
                    current = root;
                }
            }
        }
        if (produced != originalSize) throw std::runtime_error("Compressed data ended before expected output size.");
    }
    if (!out) throw std::runtime_error("Failed while writing restored data.");
    std::cout << "Restored " << originalSize << " bytes.\n";
}

static void demo() {
    const std::string sample = "ABACABA";
    Frequencies freq{};
    for (unsigned char c : sample) ++freq[c];
    std::vector<std::unique_ptr<Node>> storage;
    Node* root = buildTree(freq, storage);
    std::array<std::string, 256> codes{};
    makeCodes(root, "", codes);
    std::size_t bitCount = 0;
    std::cout << "Codes for ABACABA:\n";
    for (char c : std::string("ABC")) {
        std::cout << c << " (" << freq[static_cast<unsigned char>(c)] << "): "
                  << codes[static_cast<unsigned char>(c)] << '\n';
    }
    for (unsigned char c : sample) bitCount += codes[c].size();
    std::cout << "Encoded length: " << bitCount << " bits\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--demo") { demo(); return 0; }
        if (argc == 4 && std::string(argv[1]) == "compress") {
            compressFile(argv[2], argv[3]); return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "decompress") {
            decompressFile(argv[2], argv[3]); return 0;
        }
        std::cerr << "Usage:\n  huffman.exe --demo\n  huffman.exe compress <input> <output.huf>\n"
                     "  huffman.exe decompress <input.huf> <restored-output>\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
