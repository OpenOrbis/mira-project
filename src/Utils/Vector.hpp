#pragma once
#include <Utils/Types.hpp>
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
}

template <typename T>
class Vector
{
private:
    uint32_t m_Capacity;    // Available space
    uint32_t m_Size;        // Count
    T* m_Array;
    static T m_Default;

    struct mtx m_Mutex;

private:
    /**
     * This must be called while the proc is locked
     */
    void reserve_locked(uint32_t p_Count, bool p_Copy)
    {
        // Calculate the size that would be needed to copied
        uint32_t s_CopyCount = 0;
        if (p_Count < m_Size)
            s_CopyCount = p_Count;
        if (p_Count > m_Size)
            s_CopyCount = m_Size;
        
        // Allocate the new array
        T* s_NewArray = new T[p_Count];
        if (s_NewArray == nullptr)
        {
            //WriteLog(LL_Error, "could not allocate new array of size (%d).", p_Count);
            return;
        }

        // Copy everything from the old list to the new list
        if (p_Copy && m_Array != nullptr)
        {
            //WriteLog(LL_Debug, "copying %d entries", s_CopyCount);
            for (uint32_t i = 0; i < s_CopyCount; ++i)
                s_NewArray[i] = m_Array[i];
        }

        // Delete the previous array
        if (m_Array != nullptr)
        {
            //WriteLog(LL_Debug, "deleting previous array %p", m_Array);
            delete [] m_Array;
            m_Array = nullptr;
        }

        //WriteLog(LL_Debug, "replacing array (%p) with capacity (%d).", s_NewArray, p_Count);
        m_Array = s_NewArray;
        m_Capacity = p_Count;
    }

public:
    Vector() :
        m_Capacity(0),
        m_Size(0),
        m_Array(nullptr)
    {
        auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
        mtx_init(&m_Mutex, "", nullptr, MTX_SPIN);
    }

    ~Vector()
    {
        auto _mtx_unlock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_unlock_spin_flags);
	    auto _mtx_lock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_lock_spin_flags);
        auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);
	    
        _mtx_lock_spin_flags(&m_Mutex, 0);
        if (m_Array != nullptr)
        {
            delete [] m_Array;
            m_Capacity = 0;
            m_Size = 0;
        }
        _mtx_unlock_spin_flags(&m_Mutex, 0);

        // Destroy the mutex afterwards
        mtx_destroy(&m_Mutex);

        //WriteLog(LL_Debug, "deleted vector");
    }

    T& operator[] (uint32_t p_Index)
    {
        if (p_Index >= m_Size)
        {
            // Ghetto assert
            WriteLog(LL_Error, "out of bound index access");
            for (;;)
                __asm__("nop");
        }
        
        return m_Array[p_Index];
    }

    T& at(uint32_t p_Index)
    {
        if (p_Index >= m_Size)
        {
            WriteLog(LL_Error, "index out of bounds (%d) max (%d).", p_Index, m_Size);
            for (;;)
                __asm__("nop");
        }

        return m_Array[p_Index];
    }

    //template<typename T>
    void push_back(const T& obj)
    {
        auto _mtx_unlock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_unlock_spin_flags);
	    auto _mtx_lock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_lock_spin_flags);

        _mtx_lock_spin_flags(&m_Mutex, 0);
        if (m_Size == m_Capacity)
        {
            if (m_Capacity == 0)
            {
                //WriteLog(LL_Debug, "Allocating (1) capacity");
                reserve_locked(1, false); // Allocate 1 entry, don't copy any
            }
            else
            {
                //WriteLog(LL_Debug, "Doubling the capacity from (%d) to (%d).", m_Capacity, (2 * m_Capacity));
                reserve_locked(2 * m_Capacity, true); // Double the capacity
            }
        }

        //WriteLog(LL_Debug, "pushing object at %p", &obj);
        m_Array[m_Size] = obj;
        m_Size++;
        _mtx_unlock_spin_flags(&m_Mutex, 0);
    }

    uint32_t size()
    {
        return m_Size;
    }

    T& front()
    {
        if (m_Size <= 0)
            return T();
        
        if (m_Array == nullptr)
            return T();
        
        return m_Array[0];
    }

    T& back()
    {
        if (m_Size <= 0)
            return T();
        
        if (m_Array == nullptr)
            return T();
        
        uint32_t s_Index = m_Size - 1;
        return m_Array[s_Index];
    }
    
    void clear()
    {
        auto _mtx_unlock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_unlock_spin_flags);
	    auto _mtx_lock_spin_flags = (void(*)(struct mtx* mutex, int flags))kdlsym(_mtx_lock_spin_flags);

        _mtx_lock_spin_flags(&m_Mutex, 0);
        reserve_locked(1, false);
        m_Size = 0;
        _mtx_unlock_spin_flags(&m_Mutex, 0);
    }
};