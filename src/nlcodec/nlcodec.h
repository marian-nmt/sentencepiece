#pragma once
// BPE trainer based on nlcodec by Thamme Gowda (https://github.com/isi-nlp/nlcodec)
// Paper: "Many-to-English Machine Translation Tools, Data, and Pretrained Models"
//        Gowda et al., ACL 2021. https://arxiv.org/abs/2104.00290v2
//
// Core types, UTF-8 iteration, term frequency counting, and vocabulary building.
// C++17 compatible (no coroutines).

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nlcodec {

// ─── Types ───────────────────────────────────────────────────────────────────

struct Level {
    static constexpr int reserved = -1;
    static constexpr int char_level = 0; // also byte-level
    static constexpr int subword = 1;
    static constexpr int word = 2;
    static constexpr int phrase = 3;
};

struct Type {
    std::string name;
    int level;
    int32_t idx;
    int64_t freq;
    int32_t kid_left = -1;
    int32_t kid_right = -1;

    Type() = default;
    Type(std::string name, int level, int32_t idx, int64_t freq,
         int32_t kid_left = -1, int32_t kid_right = -1)
        : name(std::move(name)), level(level), idx(idx), freq(freq),
          kid_left(kid_left), kid_right(kid_right) {}

    auto has_kids() const -> bool { return kid_left >= 0 && kid_right >= 0; }
};

struct Reserved {
    static constexpr int PAD_IDX = 0, UNK_IDX = 1, BOS_IDX = 2;
    static constexpr int EOS_IDX = 3, CLS_IDX = 4, SPACE_IDX = 5;
    static constexpr const char* SPACE_TOK = "\xe2\x96\x81";  // ▁ (U+2581)

    static auto make_reserved_vocab() -> std::vector<Type> {
        return {
            {"<pad>",  Level::reserved, 0, -1}, {"<unk>", Level::reserved, 1, -1},
            {"<s>",    Level::reserved, 2, -1}, {"</s>",  Level::reserved, 3, -1},
            {"<cls>",  Level::reserved, 4, -1}, {SPACE_TOK, Level::reserved, 5, -1},
        };
    }
};

using TermFreqs = std::unordered_map<std::string, int64_t>;

// ─── UTF-8 ───────────────────────────────────────────────────────────────────

// Call fn(substr) for each UTF-8 code point in s.
template <typename Fn>
inline void for_each_utf8_char(const std::string& s, Fn&& fn) {
    size_t i = 0;
    while (i < s.size()) {
        size_t len = 1;
        auto c = static_cast<unsigned char>(s[i]);
        if (c >= 0xF0) len = 4;
        else if (c >= 0xE0) len = 3;
        else if (c >= 0xC0) len = 2;
        if (i + len > s.size()) len = s.size() - i;
        fn(s.substr(i, len));
        i += len;
    }
}

// ─── API ─────────────────────────────────────────────────────────────────────

auto compute_term_freqs(const std::vector<std::string>& lines) -> std::pair<TermFreqs, int64_t>;
auto compute_term_freqs_from_file(const std::string& path) -> std::pair<TermFreqs, int64_t>;

auto filter_types_coverage(const TermFreqs& types, double coverage) -> std::pair<TermFreqs, int64_t>;
auto build_char_vocab(const TermFreqs& char_freqs, int64_t line_count,
                      double coverage, int min_freq) -> std::vector<Type>;
auto prepare_word(const std::string& word) -> std::string;

}  // namespace nlcodec
