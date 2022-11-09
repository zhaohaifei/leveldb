// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/slice.h"
#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a
// particular Table.  It generates a single string which is stored as
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
// FilterBlockBuilder可以生成一个filter block。一个filter block其实就是一个字符串。
// filter：对一部分keys做计算后的输出。例如，在bloomfilter中，一个filter就是一个bit数组。
// 一个filter不一定对应一个data block，可能对应多个data block，或者对应一个data block的一部分。
// filter block中的offset数组，指的是相对于这个filter block的偏移量。
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);

  FilterBlockBuilder(const FilterBlockBuilder&) = delete;
  FilterBlockBuilder& operator=(const FilterBlockBuilder&) = delete;

  // 理论上，调用一次StartBlock，就会产生一个filter。
  // block_offset是在整个sstable中的偏移量。
  // 意义：从block_offset开始计算一个filter。 
  void StartBlock(uint64_t block_offset);
  void AddKey(const Slice& key);
  Slice Finish(); // 

 private:
  void GenerateFilter(); // 产生的是一个filter，不是filter block。

  const FilterPolicy* policy_;
  std::string keys_;             // Flattened key contents
                                 // 一个filter对应的所有keys，都会append到keys_。
  std::vector<size_t> start_;    // Starting index in keys_ of each key
  std::string result_;           // Filter data computed so far
  std::vector<Slice> tmp_keys_;  // policy_->CreateFilter() argument
                                 // 将keys_中的key，解析出来，会放入tmp_keys_。
  std::vector<uint32_t> filter_offsets_;
};

class FilterBlockReader {
 public:
  // REQUIRES: "contents" and *policy must stay live while *this is live.
  // contents就是整个filter block。
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);
  // block_offset是在整个sstable中的偏移量。
  // 为什么指定block_offset？一个filter不一定对应一个block。
  // 假设一个block对应两个filter，如果只是给定block_offset，
  // 只会找到第一个filter，若恰好不包括key，而是下一个filter才包括key，则就出错了。
  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  const FilterPolicy* policy_;
  const char* data_;    // Pointer to filter data (at block-start)
  const char* offset_;  // Pointer to beginning of offset array (at block-end)
  size_t num_;          // Number of entries in offset array
  size_t base_lg_;      // Encoding parameter (see kFilterBaseLg in .cc file)
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
