#ifndef __HANDLE_H
#define __HANDLE_H

template <typename T>
class handle_ptr {
   T* _ptr;
public:
   template <typename DT> friend class handle_ptr;
   typedef T element_type;
   handle_ptr() noexcept : _ptr(nullptr) {}
   handle_ptr(T* ptr) noexcept : _ptr(ptr) {}
   handle_ptr(const handle_ptr<T>& ptr) noexcept : _ptr(ptr._ptr)  {}
   handle_ptr(std::nullptr_t ptr)  noexcept : _ptr(ptr) {}
   handle_ptr(handle_ptr<T>&& ptr) noexcept : _ptr(std::move(ptr._ptr)) {}
   template <typename DT> handle_ptr(DT* ptr) noexcept : _ptr(ptr) {}
   template <typename DT> handle_ptr(const handle_ptr<DT>& ptr) noexcept : _ptr(ptr.get()) {}   
   template <typename DT> handle_ptr(handle_ptr<DT>&& ptr) noexcept : _ptr(std::move(ptr._ptr)) {}
   handle_ptr& operator=(const handle_ptr<T>& ptr) { _ptr = ptr._ptr;return *this;}
   handle_ptr& operator=(handle_ptr<T>&& ptr)      { _ptr = std::move(ptr._ptr);return *this;}
   handle_ptr& operator=(T* ptr)                   { _ptr = ptr;return *this;}
   element_type* get() const noexcept { return _ptr;}
   element_type* operator->() const noexcept { return _ptr;}
   element_type& operator*() const noexcept  { return *_ptr;}
   operator bool() const noexcept { return _ptr != nullptr;}
   void dealloc() { delete _ptr;_ptr = nullptr;}
   void free()    { delete _ptr;_ptr = nullptr;}
};

template <class T,class... Args>
inline handle_ptr<T> make_handle(Args&&... formals)
{
   return handle_ptr<T>(new T(std::forward<Args>(formals)...));
}

#endif
