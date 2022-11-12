#include "stream_reassembler.hh"

#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

long StreamReassembler::merge_block(block_node &elm1, const block_node &elm2) {
    block_node x, y;
    if (elm1.begin > elm2.begin) {
        x = elm2;
        y = elm1;
    } else {
        x = elm1;
        y = elm2;
    }
    if (x.begin + x.length < y.begin) {
        // throw runtime_error("StreamReassembler: couldn't merge blocks\n");
        return -1;  // no intersection, couldn't merge
    } else if (x.begin + x.length >= y.begin + y.length) {
        elm1 = x;
        return y.length;
    } else {
        elm1.begin = x.begin;
        elm1.data = x.data + y.data.substr(x.begin + x.length - y.begin);
        elm1.length = elm1.data.length();
        return x.begin + x.length - y.begin;
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // _output.end_input
    if (index >= _head_index + _capacity) {
        cerr << "7777" << endl;

        return;
    }
    long merged_bytes = 0;
    block_node elm;
    std::set<StreamReassembler::block_node>::iterator iter;
    vector<std::set<StreamReassembler::block_node>::iterator> to_be_delete;
    // size_t to_be_delete_size = 0;
    // 重复字符串，可以直接忽略
    if (index + data.length() <= _head_index) {
                cerr << "6666" << endl;
                cerr << "_head_index:" << _head_index << endl;
                cerr << "index + data.length():" << index + data.length() << endl;
                cerr << "6666" << endl;
        // 收到eof表示已经收到结束字符，只需让剩下的unassemb
        if (eof) {
            _eof_flag = true;
        }
        if (_eof_flag && empty()) {
            // std::cerr<<"end of input 1" << std::endl;
            _output.end_input();
        }
        return;
    } else if (index <= _head_index) {  // 和起始点的重叠了，可以写入stream中，可以向后继续合并
        cerr << "5555" << endl;

        size_t offset = _head_index - index;  // substring的新起点
        elm.data.assign(data.begin() + offset, data.end());
        elm.begin = _head_index;
        elm.length = elm.data.length();
        // 如果此时_block 为空，则直接写入strem就行了
        _unassembled_byte += elm.length;
        if (!_blocks.size() == 0) {
            // merge next
            iter = _blocks.begin();
            while (iter != _blocks.end() && (merged_bytes = merge_block(elm, *iter)) >= 0) {
                // cerr << "2222" << endl;
                _unassembled_byte -= merged_bytes;
                to_be_delete.push_back(iter);
                iter++;
            }
            // 清理掉set中被合并的节点
            for (size_t i = 0; i < to_be_delete.size(); i++) {
                // to_be_delete_size += iter->length;
                _blocks.erase(to_be_delete[i]);
            }
            // _unassembled_byte -= to_be_delete_size;
        }

        // 把新合并的节点刷进stream
        size_t write_bytes = _output.write(elm.data);
        _head_index += write_bytes;
        _unassembled_byte -= write_bytes;
        // 收到eof表示已经收到结束字符，只需让剩下的unassemb
        if (eof) {
            _eof_flag = true;
        }
        if (_eof_flag && empty()) {
            //  std::cerr<<"end of input 2" << std::endl;
            _output.end_input();
        }

    } else {  // 在起始字符串的后面一截，两者没有交集，没法合入到stream中，可以和前后的合并
        cerr << "8888" << endl;
        // 如果碰到eof，加上标记，
        if (eof) {
            _eof_flag = true;
        }
        // 如果长度为零直接不用插入了
        if (data.size() == 0)
        {
            return;
        }
        
        cerr << "3333" << endl;
        elm.begin = index;
        elm.length = data.length();
        elm.data = data;
        _unassembled_byte += elm.length;
        if (!_blocks.size() == 0) {
            iter = _blocks.lower_bound(elm);
            // merge before
            if (iter != _blocks.begin()) {
                iter--;
                if ((merged_bytes = merge_block(elm, *iter)) >= 0) {
                    _unassembled_byte -= merged_bytes;
                    to_be_delete.push_back(iter);
                }
                iter++;
            }

            cerr << "44444" << endl;
            // merge back
            while (iter != _blocks.end() && (merged_bytes = merge_block(elm, *iter)) >= 0) {
                cerr << "111" << endl;
                _unassembled_byte -= merged_bytes;
                to_be_delete.push_back(iter);
                iter++;
            }

            // 清理掉set中被合并的节点
            for (size_t i = 0; i < to_be_delete.size(); i++) {
                // to_be_delete_size += iter->length;
                _blocks.erase(to_be_delete[i]);
            }
        }
        _blocks.insert(elm);
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_byte; }

bool StreamReassembler::empty() const { return _unassembled_byte == 0; }

size_t StreamReassembler::head_index() const{
      return _head_index;
    }