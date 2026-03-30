#pragma once
// BPE learning algorithm based on nlcodec by Thamme Gowda
// https://github.com/isi-nlp/nlcodec | https://arxiv.org/abs/2104.00290v2
//
// Algorithm overview:
//   Standard BPE iteratively merges the most frequent adjacent pair of tokens.
//   A naive implementation rescans the entire corpus each iteration → O(N) per merge.
//   This implementation achieves O(log N) per merge using three key data structures:
//
//   1. MaxHeap — priority queue of (bigram → frequency). Finding the best pair
//      to merge is O(log N) via heap pop, instead of O(N) linear scan.
//
//   2. Doubly-linked list (LnNode) — each word's token sequence is stored as a
//      linked list. Merging two adjacent tokens into one is O(1): delete the
//      right node, update the left node's value, and relink neighbors.
//      No array shifting or copying.
//
//   3. Lazy deletion via "dirty" heap — after a merge, neighbor pair frequencies
//      change. Instead of finding and updating entries inside the heap (O(N)),
//      decrements are accumulated in a separate hash map (HeapDirty). When a
//      stale entry is popped, its correction is applied and it's re-inserted
//      if still positive. New pairs from merges are pushed directly. This avoids
//      O(N) heap maintenance while keeping the heap approximately correct.
//
//   Combined: each of the ~vocab_size merges costs O(K log N) where K is the
//   number of positions where the merged pair occurs — identical to the work
//   that must be done regardless. The total is O(M log N) where M is the corpus
//   token count, vs O(M × vocab_size) for naive rescanning.

#include "nlcodec.h"

#include <cstdint>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nlcodec {

// ─── BPE Data Structures ────────────────────────────────────────────────────

struct LnNode {
    int32_t val;
    LnNode* left = nullptr;
    LnNode* right = nullptr;
    int64_t freq = 1;

    LnNode() = default;
    LnNode(int32_t val, int64_t freq) : val(val), freq(freq) {}

    void unlink() {
        if (left) left->right = right;
        if (right) right->left = left;
        left = right = nullptr;
    }
    auto is_unlinked() const -> bool { return !left && !right; }
};

class NodeArena {
public:
    explicit NodeArena(size_t cap = 1 << 20) : blocks_(1, std::vector<LnNode>(cap)) {}

    auto alloc(int32_t val, int64_t freq) -> LnNode* {
        if (pos_ >= blocks_[blk_].size()) {
            blocks_.push_back(std::vector<LnNode>(blocks_[blk_].size() * 2));
            blk_++;
            pos_ = 0;
        }
        auto* n = &blocks_[blk_][pos_++];
        *n = LnNode{val, freq};
        return n;
    }

    auto from_seq(const std::vector<int32_t>& seq, int64_t freq) -> std::vector<LnNode*> {
        std::vector<LnNode*> nodes;
        nodes.reserve(seq.size());
        for (auto tok : seq) nodes.push_back(alloc(tok, freq));
        for (size_t i = 0; i < nodes.size(); i++) {
            if (i > 0) nodes[i]->left = nodes[i - 1];
            if (i + 1 < nodes.size()) nodes[i]->right = nodes[i + 1];
        }
        return nodes;
    }

private:
    std::vector<std::vector<LnNode>> blocks_;
    size_t blk_ = 0, pos_ = 0;
};

using Bigram = std::pair<int32_t, int32_t>;

struct BigramHash {
    auto operator()(const Bigram& b) const -> size_t {
        return std::hash<size_t>{}((static_cast<size_t>(b.first) << 32) |
                                   static_cast<uint32_t>(b.second));
    }
};

class MaxHeap {
public:
    MaxHeap() = default;
    explicit MaxHeap(const std::unordered_map<Bigram, int64_t, BigramHash>& items) {
        for (auto& [bg, f] : items) heap_.push({f, bg});
    }
    void push(const Bigram& bg, int64_t freq) { heap_.push({freq, bg}); }
    auto pop() -> std::pair<Bigram, int64_t> {
        auto [f, bg] = heap_.top(); heap_.pop(); return {bg, f};
    }
    auto empty() const -> bool { return heap_.empty(); }

private:
    std::priority_queue<std::pair<int64_t, Bigram>> heap_;
};

using BigramIndex = std::unordered_map<Bigram, std::unordered_set<LnNode*>, BigramHash>;
using HeapDirty = std::unordered_map<Bigram, int64_t, BigramHash>;

// ─── BPE API ─────────────────────────────────────────────────────────────────

struct BPELearnConfig {
    int32_t vocab_size = 32000;
    int min_freq = 1;
    int min_co_ev = 1;
    double char_coverage = 0.9995;
};

auto learn_bpe_vocab(const TermFreqs& term_freqs, int64_t line_count,
                     const BPELearnConfig& config) -> std::vector<Type>;

auto learn_codes(const TermFreqs& term_freqs, std::vector<Type> vocab,
                 int32_t n_merges, int min_co_evidence) -> std::vector<Type>;

}  // namespace nlcodec
