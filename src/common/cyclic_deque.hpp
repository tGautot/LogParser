#ifndef CYCLIC_DEQUE_HPP
#define CYCLIC_DEQUE_HPP

#include <algorithm>
#include <cstddef>

#define CYCLIC_INCR(x) x = (x+1)%_size
#define CYCLIC_DECR(x) x = (x==0) ? _size-1 : x-1

template <typename T>
class cyclic_deque {
private:
  T* _storage;
  size_t _size; // Maximum allowed number of elements in the _storage
  size_t _elem_count; // Number of elements populated
  size_t _front_id, _back_id;
public:
  cyclic_deque(size_t size) : _size(size), _elem_count(0){
    _storage = new T[_size];
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
    if(_storage != nullptr) delete[] _storage;
  }

  bool empty(){ return _elem_count==0; }
  bool full(){ return _elem_count==_size; }
  size_t size(){ return _elem_count; }
  size_t max_size(){ return _size; }

  size_t getFrontId(){ return _front_id; }
  size_t getBackId(){ return _back_id; }

  void clear(){
    _elem_count = 0;
    
    _back_id = 0;
    _front_id = 1 % _size;
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
    return _storage[ (_front_id+id)%_size ];
  }
};


#endif