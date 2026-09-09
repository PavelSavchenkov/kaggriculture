#pragma once
#include <memory>
#include <vector>
#include <cstdlib>
namespace compositions_crop_parent {
// Read-only after construction. Only the original constructor appends entries;
// appending after the object has been copied fails instead of mutating peers.
template<class T> class SharedVector {
    std::shared_ptr<std::vector<T>> values_;
public:
    SharedVector():values_(std::make_shared<std::vector<T>>()){}
    SharedVector(std::vector<T> values):values_(std::make_shared<std::vector<T>>(std::move(values))){}
    bool empty()const{return values_->empty();}
    size_t size()const{return values_->size();}
    const T& operator[](size_t i)const{return (*values_)[i];}
    auto begin()const{return values_->cbegin();}
    auto end()const{return values_->cend();}
    void push_back(const T& value){if(!values_.unique())std::abort();values_->push_back(value);}
};
}
