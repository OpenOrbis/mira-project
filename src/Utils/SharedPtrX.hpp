#pragma once
#include <Utils/Types.hpp>

template<typename T>
class shared_ptr
{
private:
    int64_t* m_Count;
    T* m_Object;

public:
    shared_ptr() : m_Count(new int64_t(1)), m_Object(new T()) { };

    shared_ptr(T* p_Object) : m_Count(new int64_t(1)), m_Object(p_Object) { }

    ~shared_ptr()
    {
        --(*m_Count);
        if (*m_Count == 0)
            delete m_Object;
    }

    shared_ptr(const shared_ptr& copy) : m_Count(copy.m_Count), m_Object(copy.m_Object)
    {
        ++(*m_Count);
    }

    shared_ptr& operator = (const shared_ptr& rhs)
    {
        rhs.swap(*this);
        return *this;
    }

    shared_ptr& operator=(T* newData)
    {
        shared_ptr tmp(newData);
        tmp.swap(*this);
        return *this;
    }

private:
    void destroy();
    bool isValid() const;
    int64_t getCount() const;

    T* operator ->();
    T& operator *();

    void swap(shared_ptr& other) noexcept
    {
        auto s_Count = m_Count;
        auto s_Object = m_Object;

        m_Count = other.m_Count;
        m_Object = other.m_Object;

        other.m_Count = s_Count;
        other.m_Object = s_Object;
    }
};