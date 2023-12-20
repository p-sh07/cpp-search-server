#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
    : range_begin_(begin), range_end_(end)
    {}
    auto begin() const {
        return range_begin_;
    }
    auto end() const {
        return range_end_;
    }
    //size
private:
    Iterator range_begin_, range_end_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& page) {
    for(const auto item : page) {
        out << item;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for( ; begin != end; std::advance(begin, page_size) ) {
            if(std::distance(begin, end) < page_size) {
                pages_.push_back({begin, end});
                break;
            }
            pages_.push_back({begin, std::next(begin, page_size)});
        }
    }
    //using It_type = vector<IteratorRange<Iterator>>::iterator;
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    //size
private:
    std::vector<IteratorRange<Iterator>> pages_;
};
