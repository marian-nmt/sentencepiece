// nlcodec — Term frequency counting and vocabulary building.
// I/O and tokenizer classes are provided by tokenizerpp.

#include "nlcodec.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace nlcodec {

// ─── Term Frequency Counting ─────────────────────────────────────────────────

static auto count_words(const char* data, size_t len) -> TermFreqs {
    TermFreqs freqs;
    size_t i = 0;
    while (i < len) {
        while (i < len && (data[i] == ' ' || data[i] == '\t' ||
                           data[i] == '\n' || data[i] == '\r'))
            i++;
        if (i >= len) break;
        size_t start = i;
        while (i < len && data[i] != ' ' && data[i] != '\t' &&
               data[i] != '\n' && data[i] != '\r')
            i++;
        freqs[std::string(data + start, i - start)]++;
    }
    return freqs;
}

static auto count_lines(const char* data, size_t len) -> int64_t {
    int64_t n = 0;
    for (size_t i = 0; i < len; i++)
        if (data[i] == '\n') n++;
    if (len > 0 && data[len - 1] != '\n') n++;
    return n;
}

static auto read_file_buf(const std::string& path) -> std::string {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return {};
    fseek(f, 0, SEEK_END);
    auto size = static_cast<size_t>(ftell(f));
    fseek(f, 0, SEEK_SET);
    std::string buf(size, '\0');
    buf.resize(fread(buf.data(), 1, size, f));
    fclose(f);
    return buf;
}

auto compute_term_freqs(const std::vector<std::string>& lines) -> std::pair<TermFreqs, int64_t> {
    TermFreqs freqs;
    for (auto& line : lines)
        for (auto& [w, c] : count_words(line.data(), line.size()))
            freqs[w] += c;
    return {freqs, static_cast<int64_t>(lines.size())};
}

auto compute_term_freqs_from_file(const std::string& path) -> std::pair<TermFreqs, int64_t> {
    auto buf = read_file_buf(path);
    return {count_words(buf.data(), buf.size()), count_lines(buf.data(), buf.size())};
}

// ─── Vocabulary Building ─────────────────────────────────────────────────────

auto filter_types_coverage(const TermFreqs& types, double coverage) -> std::pair<TermFreqs, int64_t> {
    std::vector<std::pair<std::string, int64_t>> sorted(types.begin(), types.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second > b.second; });

    double total = 0;
    for (auto& [_, f] : sorted) total += f;

    TermFreqs included;
    double cum = 0;
    size_t i = 0;
    for (; i < sorted.size(); i++) {
        cum += sorted[i].second / total;
        included[sorted[i].first] = sorted[i].second;
        if (cum >= coverage) { i++; break; }
    }

    int64_t unk_count = 0;
    for (; i < sorted.size(); i++) unk_count += sorted[i].second;
    return {included, unk_count};
}

auto prepare_word(const std::string& word) -> std::string {
    return word + Reserved::SPACE_TOK;
}

auto build_char_vocab(const TermFreqs& char_freqs, int64_t line_count,
                      double coverage, int min_freq) -> std::vector<Type> {
    auto vocab = Reserved::make_reserved_vocab();
    auto [filtered, unk_count] = filter_types_coverage(char_freqs, coverage);

    std::vector<std::pair<std::string, int64_t>> sorted(filtered.begin(), filtered.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second > b.second; });

    for (auto& [ch, freq] : sorted) {
        if (freq < min_freq) continue;
        bool reserved = false;
        for (auto& r : vocab) if (r.name == ch) { reserved = true; break; }
        if (reserved) continue;
        vocab.emplace_back(ch, Level::char_level, static_cast<int32_t>(vocab.size()), freq);
    }

    vocab[Reserved::UNK_IDX].freq = unk_count;
    vocab[Reserved::BOS_IDX].freq = line_count;
    vocab[Reserved::EOS_IDX].freq = line_count;
    vocab[Reserved::CLS_IDX].freq = line_count;
    return vocab;
}

}  // namespace nlcodec
