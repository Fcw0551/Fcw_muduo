#pragma once 
#include <iostream>
#include <typeinfo>
#include <cassert>


// Any类处理不同类型
class Any{
    private:
        class BaseHolder{
        public:
            virtual ~BaseHolder() = default;
            // 定义接口规范
            // 返回类型信息
            virtual const std::type_info &typeInfo() = 0;
            // 拷贝
            virtual BaseHolder *clone() = 0;
        };
        template <typename T>
        class Holder : public BaseHolder{
        public:
            Holder(const T &value)
                : _value(value)
            {
            }
            ~Holder()
            {
            }
            const std::type_info &typeInfo()
            {
                return typeid(T);
            }
            BaseHolder *clone()
            {
                return new Holder(_value);
            }
            T _value;
        };

    private:
        BaseHolder* _holder;
    public:
        Any()
            : _holder(nullptr)
        {}
        template <typename T>
        Any(const T &val) 
        : _holder(new Holder<T>(val)) 
        {}
        Any(const Any &other) 
        : _holder(other._holder ? other._holder->clone():NULL) 
        {}
        ~Any(){
            if (_holder)
                delete _holder;
        }
        const std::type_info &typeInfo() {
             return _holder ? _holder->typeInfo() : typeid(void); 
        }
        Any& swap(Any &other){
            std::swap(_holder, other._holder);
            return *this;
        }

        template <typename T>
        T* get(){
            assert(typeid(T) == _holder->typeInfo());
            return &(dynamic_cast<Holder<T>*>(_holder)->_value);
        }
        template <typename T>
        Any& operator=(const T &val){
            /*为val构建⼀个临时对象出来，然后进⾏交换，这样临时对象销毁的时候，顺带原先
           保存的BaseHolder也会被销毁*/
            Any(val).swap(*this);
            return *this;
        }
        Any &operator=(Any other){
            /*这⾥要注意形参只是⼀个临时对象，进⾏交换后就会释放，所以交换后，原先保存的
           BaseHolder指针也会被销毁*/
            other.swap(*this);
            return *this;
        }
};
