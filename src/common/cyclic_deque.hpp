#ifndef CYCLIC_DEQUE_HPP
#define CYCLIC_DEQUE_HPP

#include <cstddef>
#include <new>
#include <utility>

#define CYCLIC_INCR(x) x = (x+1)%_size
#define CYCLIC_DECR(x) x = (x==0) ? _size-1 : x-1

template <typename T>
class cyclic_deque {
private:
  alignas(T) char* _storage;
  size_t _size; // Maximum allowed number of elements in the _storage
  size_t _elem_count; // Number of elements populated
  size_t _front_id, _back_id;
  T* item(int id){
    // I am not sure I understand what this std::launder does exactly,
    return std::launder(reinterpret_cast<T*>(_storage + id * sizeof(T)));
  }
public:
  cyclic_deque(size_t size) : _size(size), _elem_count(0){
    _storage = new char[_size * sizeof(T)];
    _back_id = 0;
    _front_id = 1 % _size;
  }
  cyclic_deque(cyclic_deque&& tomove){
    _storage = tomove._storage;
    tomove._storage = nullptr;
    _size = tomove._size;
    _elem_count = tomove._elem_count;
    _front_id = tomove._front_id;
    _back_id = tomove._back_id;
  }
  ~cyclic_deque(){
    clear();
    if(_storage != nullptr) delete[] _storage;
  }

  bool empty(){ return _elem_count==0; }
  bool full(){ return _elem_count==_size; }
  size_t size(){ return _elem_count; }
  size_t capacity(){ return _size; }

  size_t getFrontId(){ return _front_id; }
  size_t getBackId(){ return _back_id; }

  void clear(){
    for(size_t i = 0; i < size(); i++){
      item((_front_id + i)%_size)->~T();
    }
    _elem_count = 0;
    
    _back_id = 0;
    _front_id = 1 % _size;
  }

  T& front(){ return *item(_front_id); }
  T& back(){ return *item(_back_id); }

  template<class U>
  void push_back(U&& e){
    if(full()){
      // We will use memspace in which currently is a live object, need to destroy it first
      item(_front_id)->~T();
      CYCLIC_INCR(_front_id);
      _elem_count--;
    }
    CYCLIC_INCR(_back_id);
    ::new(item(_back_id)) T(std::forward<U>(e));
    _elem_count++;
  }

  template<class... Args>
  T& emplace_back(Args&&... args){
    if(full()){
      // We will use memspace in which currently is a live object, need to destroy it first
      item(_front_id)->~T();
      CYCLIC_INCR(_front_id);
      _elem_count--;
    }
    CYCLIC_INCR(_back_id);
    T* p = ::new(item(_back_id)) T(std::forward<Args>(args)...);
    _elem_count++;
    return *p;
  }

  // Just move back/front pointer
  void push_back(){
    bool full_before = full();
    
    CYCLIC_INCR(_back_id);
    _elem_count++;
    if(full_before){
      _elem_count--;
      CYCLIC_INCR(_front_id);
    }
  }

  template<class U>
  void push_front(U&& e){
    if(full()){
      item(_back_id)->~T();
      CYCLIC_INCR(_back_id);
      _elem_count--;
    }

    CYCLIC_DECR(_front_id);
    ::new(item(_front_id)) T(std::forward<U>(e));
    _elem_count++;
  }

  template<class... Args>
  T& emplace_front(Args&&... args){
    if(full()){
      item(_back_id)->~T();
      CYCLIC_INCR(_back_id);
      _elem_count--;
    }

    CYCLIC_DECR(_front_id);
    T* p = ::new(item(_front_id)) T(std::forward<Args>(args)...);
    _elem_count++;
    return *p;
  }

  // Just move front/back pointer
  void push_front(){
    bool full_before = full();

    CYCLIC_DECR(_front_id);
    _elem_count++;
    if(full_before){
      _elem_count--;
      CYCLIC_DECR(_back_id);
    }
  }

  void pop_back(){
    if(!empty()){
      item(_back_id)->~T();
      CYCLIC_DECR(_back_id);
      _elem_count--;
    }
  }

  void pop_front(){
    if(!empty()){
      item(_front_id)->~T();
      CYCLIC_INCR(_front_id);
      _elem_count--;
    }
  }

  T& operator[](const size_t& id){
    if(id >= size()){
      printf("Trying to get cyclic deque id %lu but is of size %lu\n", id, size());
      throw 0;
    }
    return *item((_front_id + id) % _size);
  }
};


#endif