// nlcodec — BPE learning algorithm implementation

#include "bpe.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <unordered_map>

namespace nlcodec {

static auto compute_char_freqs(const TermFreqs& term_freqs) -> TermFreqs {
    TermFreqs out;
    for (auto& [word, freq] : term_freqs)
        for_each_utf8_char(word, [&](const std::string& ch) {
            out[ch] += freq;
        });
    return out;
}

static auto word_to_idxs(const std::string& word,
                         const std::unordered_map<std::string, int32_t>& idx,
                         int32_t unk) -> std::vector<int32_t> {
    std::vector<int32_t> out;
    for_each_utf8_char(word, [&](const std::string& ch) {
        auto it = idx.find(ch);
        out.push_back(it != idx.end() ? it->second : unk);
    });
    return out;
}

auto learn_codes(const TermFreqs& term_freqs, std::vector<Type> vocab,
                 int32_t n_merges, int min_co_evidence) -> std::vector<Type> {
    std::unordered_map<std::string, int32_t> name_to_idx;
    for (auto& t : vocab) name_to_idx[t.name] = t.idx;

    NodeArena arena(1 << 20);
    std::unordered_map<int32_t, int64_t> uni;
    std::unordered_map<Bigram, int64_t, BigramHash> bi;
    BigramIndex bi_ixs;

    for (auto& [word, freq] : term_freqs) {
        auto seq = word_to_idxs(word, name_to_idx, Reserved::UNK_IDX);
        if (seq.empty()) continue;
        auto nodes = arena.from_seq(seq, freq);
        for (size_t i = 0; i + 1 < seq.size(); i++) {
            Bigram bg{seq[i], seq[i + 1]};
            bi[bg] += freq;
            bi_ixs[bg].insert(nodes[i]);
            uni[seq[i]] += freq;
        }
        uni[seq.back()] += freq;
    }

    MaxHeap heap(bi);
    HeapDirty dirty;
    auto t0 = std::chrono::steady_clock::now();
    auto last_log = t0;

    for (int32_t i = 0; i < n_merges; i++) {
        if (heap.empty()) break;

        auto [pair, freq] = heap.pop();
        for (auto it = dirty.find(pair); it != dirty.end(); it = dirty.find(pair)) {
            int64_t d = it->second; dirty.erase(it);
            int64_t c = freq + d; assert(c >= 0);
            if (c > 0) heap.push(pair, c);
            if (heap.empty()) goto done;
            std::tie(pair, freq) = heap.pop();
        }

        if (freq < min_co_evidence) {
            fprintf(stderr, "Early stop: evidence %ld < %d\n", (long)freq, min_co_evidence);
            break;
        }

        auto ni = static_cast<int32_t>(vocab.size());
        auto [a, b] = pair;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 2) {
            fprintf(stderr, "%.1f%% :: %d || %ld || %s %s\n",
                    100.0 * i / n_merges, ni, (long)freq,
                    vocab[a].name.c_str(), vocab[b].name.c_str());
            last_log = now;
        }

        vocab.emplace_back(vocab[a].name + vocab[b].name, Level::subword, ni, freq, a, b);
        uni[ni] = freq; uni[a] -= freq; uni[b] -= freq;

        std::unordered_map<Bigram, int64_t, BigramHash> deltas;
        auto nodes = std::move(bi_ixs[pair]); bi_ixs.erase(pair);

        for (auto* nd : nodes) {
            auto *x = nd->left, *bn = nd->right;
            if (nd->is_unlinked() || (a == b && bn && (nd->val == ni || bn->val == ni))) {
                uni[a] += nd->freq; uni[b] += nd->freq; uni[ni] -= nd->freq;
                continue;
            }
            assert(bn && nd->val == a && bn->val == b && nd->freq == bn->freq);
            auto* y = bn->right;
            bn->unlink(); nd->val = ni;

            if (x) {
                deltas[{x->val, a}] -= x->freq;
                bi_ixs[{x->val, a}].erase(x);
                if (bi_ixs[{x->val, a}].empty()) bi_ixs.erase({x->val, a});
                deltas[{x->val, ni}] += x->freq;
                bi_ixs[{x->val, ni}].insert(x);
            }
            if (y) {
                deltas[{b, y->val}] -= bn->freq;
                bi_ixs[{b, y->val}].erase(bn);
                if (bi_ixs[{b, y->val}].empty()) bi_ixs.erase({b, y->val});
                deltas[{ni, y->val}] += bn->freq;
                bi_ixs[{ni, y->val}].insert(nd);
            }
        }
        assert(uni[a] >= 0 && uni[b] >= 0);

        for (auto& [p, d] : deltas) {
            if (d > 0) heap.push(p, d);
            else if (d < 0) dirty[p] += d;
        }
    }

done:
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    fprintf(stderr, "BPE merge loop: %zu types in %.1fs\n", vocab.size(), ms / 1000.0);
    return vocab;
}

auto learn_bpe_vocab(const TermFreqs& term_freqs, int64_t line_count,
                     const BPELearnConfig& config) -> std::vector<Type> {
    TermFreqs prepared;
    for (auto& [word, freq] : term_freqs)
        if (freq >= config.min_freq)
            prepared[prepare_word(word)] = freq;

    auto vocab = build_char_vocab(compute_char_freqs(prepared), line_count, config.char_coverage, 1);
    fprintf(stderr, "Initial vocab: %zu types, %zu word types\n", vocab.size(), prepared.size());

    if (config.vocab_size <= static_cast<int32_t>(vocab.size())) return vocab;
    return learn_codes(prepared, std::move(vocab),
                       config.vocab_size - static_cast<int32_t>(vocab.size()), config.min_co_ev);
}

}  // namespace nlcodec
