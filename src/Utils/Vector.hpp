#pragma once
#include <Utils/Types.hpp>
#include <Utils/Logger.hpp>

template <typename T>
class Vector
{
private:
    uint32_t m_Capacity;    // Available space
    uint32_t m_Size;        // Count
    T* m_Array;

public:
    Vector() :
        m_Capacity(0),
        m_Size(0),
        m_Array(nullptr)
    {

    }

    T& operator[] (uint32_t index)
    {
        if (index >= m_Size)
            return T();
        
        return m_Array[index];
    }

    //template<typename T>
    void push_back(const T& obj)
    {
        if (m_Size == m_Capacity)
        {
            if (m_Capacity == 0)
            {
                WriteLog(LL_Debug, "Allocating (1) capacity");
                reserve(1, false); // Allocate 1 entry, don't copy any
            }
            else
            {
                WriteLog(LL_Debug, "Doubling the capacity from (%d) to (%d).", m_Capacity, (2 * m_Capacity));
                reserve(2 * m_Capacity, true); // Double the capacity
            }
        }

        WriteLog(LL_Debug, "pushing object at %p", &obj);
        m_Array[m_Size] = obj;
        m_Size++;
    }

    //template<typename T>
    void reserve(uint32_t p_Count, bool p_Copy)
    {
        // Calculate the size that would be needed to copied
        uint32_t s_CopyCount = 0;
        if (p_Count < m_Size)
            s_CopyCount = p_Count;
        if (p_Count > m_Size)
            s_CopyCount = m_Size;
        
        WriteLog(LL_Debug, "copyCount: %d", s_CopyCount);
        // Allocate the new array
        T* s_NewArray = new T[p_Count];
        if (s_NewArray == nullptr)
        {
            WriteLog(LL_Error, "could not allocate new array of size %d.", p_Count);
            return;
        }

        // Copy everything from the old list to the new list
        if (p_Copy && m_Array != nullptr)
        {
            WriteLog(LL_Error, "copying %d entries", s_CopyCount);
            for (uint32_t i = 0; i < s_CopyCount; ++i)
                s_NewArray[i] = m_Array[i];
        }

        // Delete the previous array
        if (m_Array != nullptr)
        {
            WriteLog(LL_Debug, "deleting previous array %p", m_Array);
            delete [] m_Array;
            m_Array = nullptr;
        }

        WriteLog(LL_Debug, "replacing array (%p) with capacity (%d).", s_NewArray, p_Count);
        m_Array = s_NewArray;
        m_Capacity = p_Count;
    }

    uint32_t size()
    {
        return m_Size;
    }

    T pop_back()
    {
        if (m_Size <= 0)
            return T();
        
        if (m_Array == nullptr)
            return T();
        
        return m_Array[--m_Size];
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
};