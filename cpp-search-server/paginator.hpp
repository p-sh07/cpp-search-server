//
//  paginator.hpp
//  cpp-search-server
//
//  Created by Pavel Sh on 15.12.2023.
//

#ifndef paginator_hpp
#define paginator_hpp

#include <iostream>
#include <vector>

using std::ostream;
using std::vector;


template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
    : begin_(begin), end_(end)
    {}
    auto begin() const {
        return begin_;
    }
    auto end() const {
        return end_;
    }
    //size
private:
    Iterator begin_, end_;
};

template <typename Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator>& page) {
    for(const auto item : page) {
        out << item;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, int page_size) {
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
    vector<IteratorRange<Iterator>> pages_;
};

#endif /* paginator_hpp */

/* example output:
 { document_id = 2, relevance = 0.402359, rating = 2 }{ document_id = 4, relevance = 0.229073, rating = 2 }
 Page break
 { document_id = 5, relevance = 0.229073, rating = 1 }
 Page break
 */

/*author solution:
 
 template <typename Iterator>
 class IteratorRange {
 public:
     IteratorRange(Iterator begin, Iterator end)
         : first_(begin)
         , last_(end)
         , size_(distance(first_, last_)) {
     }

     Iterator begin() const {
         return first_;
     }

     Iterator end() const {
         return last_;
     }

     size_t size() const {
         return size_;
     }

 private:
     Iterator first_, last_;
     size_t size_;
 };

 template <typename Iterator>
 ostream& operator<<(ostream& out, const IteratorRange<Iterator>& range) {
     for (Iterator it = range.begin(); it != range.end(); ++it) {
         out << *it;
     }
     return out;
 }

 template <typename Iterator>
 class Paginator {
 public:
     Paginator(Iterator begin, Iterator end, size_t page_size) {
         for (size_t left = distance(begin, end); left > 0;) {
             const size_t current_page_size = min(page_size, left);
             const Iterator current_page_end = next(begin, current_page_size);
             pages_.push_back({begin, current_page_end});

             left -= current_page_size;
             begin = current_page_end;
         }
     }

     auto begin() const {
         return pages_.begin();
     }

     auto end() const {
         return pages_.end();
     }

     size_t size() const {
         return pages_.size();
     }

 private:
     vector<IteratorRange<Iterator>> pages_;
 };

 template <typename Container>
 auto Paginate(const Container& c, size_t page_size) {
     return Paginator(begin(c), end(c), page_size);
 }

 
 */
