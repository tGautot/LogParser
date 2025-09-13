#ifndef CYCLIC_DEQUE_HPP
#define CYCLIC_DEQUE_HPP

#include <cstddef>

#define CYCLIC_INCR(x) x = (x+1)%n
#define CYCLIC_DECR(x) x = (x==0) ? n-1 : x-1

template <typename T>
class cyclic_deque {
private:
  T* _storage;
  size_t n;
  size_t _elem_count;
  size_t _front_id, _back_id;
public:
  cyclic_deque(size_t size) :n(size), _elem_count(0){
    _storage = new T[n];
    _back_id = n/2;
    _front_id = (_back_id + 1) % n;
  }
  cyclic_deque(cyclic_deque&& tomove){
    _storage = tomove._storage;
    tomove._storage = nullptr;
    n = tomove.n;
    _elem_count = tomove._elem_count;
    _front_id = tomove._front_id;
    _back_id = tomove._back_id;
  }
  ~cyclic_deque(){
    if(_storage != nullptr) delete[] _storage;
  }

  bool empty(){ return _elem_count==0; }
  bool full(){ return _elem_count==n; }
  size_t size(){ return _elem_count; }
  size_t max_size(){ return n; }

  size_t getFrontId(){ return _front_id; }
  size_t getBackId(){ return _back_id; }

  void resize(const size_t& s){
    size_t size_before = size();
    _elem_count = (s > n) ? n : s; 
    _back_id = (_back_id + (_elem_count - size_before)) % n;
  }

  T& front(){ return _storage[_front_id]; }
  T& back(){ return _storage[_back_id]; }

  void push_back(const T& e){
    bool full_before = full();
    
    CYCLIC_INCR(_back_id);
    _storage[_back_id] = e;
    _elem_count++;
    if(full_before){
      _elem_count--;
      CYCLIC_INCR(_front_id);
    }
  }

  void push_front(const T& e){
    bool full_before = full();

    CYCLIC_DECR(_front_id);
    _storage[_front_id] = e;
    _elem_count++;
    if(full_before){
      _elem_count--;
      CYCLIC_DECR(_back_id);
    }
  }

  T& pop_back(){
    T e = _storage[_back_id];
    if(!empty()){
      CYCLIC_DECR(_back_id);
      _elem_count--;
    }
    return e;
  }

  T& pop_front(){
    T e = _storage[_front_id];
    if(!empty()){
      CYCLIC_INCR(_front_id);
      _elem_count--;
    }
    return e;
  }

  T& operator[](const size_t& id){
    if(id >= size()){
      printf("Trying to get cyclic deque id %lu but is of size %lu\n", id, size());
      throw 0;
    }
    return _storage[ (_front_id+id)%n ];
  }
};


#endif