#pragma once

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
};

namespace Utils
{
    template <typename T>
    class RingBuffer
    {
    private:
        struct mtx m_Mutex;
        T* m_Buffer;
        size_t m_Head;
        size_t m_Tail;
        const size_t m_MaxSize;
        bool m_Full;

    public:
        RingBuffer(size_t p_Size) :
            m_Buffer(new T[p_Size]),
            m_Head(0),
            m_Tail(0),
            m_MaxSize(p_Size),
            m_Full(false)
        {
            auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);
            mtx_init(&m_Mutex, "", nullptr, 0);
        }

        ~RingBuffer()
        {
            auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);

            if (m_Buffer)
                delete [] m_Buffer;
            
            m_Head = 0;
            m_Tail = 0;
            m_MaxSize = 0;
            m_Full = false;

            mtx_destroy(&m_Mutex);
        }

        void reset() 
        {
            // TODO: lock
            m_Head = m_Tail;
            m_Full = false;
            // TODO: unlock
        }

        bool empty() const
        {
            return (!m_Full && (m_Head == m_Tail));
        }

        bool full() const
        {
            return m_Full;
        }

        size_t capacity() const
        {
            return m_MaxSize;
        }

        size_t size() const
        {
            size_t s_Size = m_MaxSize;
            if (!m_Full)
            {
                if (m_Head >= m_Tail)
                    s_Size = m_Head - m_Tail;
                else
                    s_Size = m_MaxSize + (m_Head - m_Tail);
            }

            return s_Size;
        }

        void put(T p_Item)
        {
            // TODO: lock
            m_Buffer[m_Head] = p_Item;

            if (m_Full)
                m_Tail = (m_Tail + 1) % m_MaxSize;
            
            m_Head = (m_Head + 1) % m_MaxSize;
            m_Full = m_Head == m_Tail;
            // TODO: unlock
        }

        T get()
        {
            // TODO: lock
            if (empty())
                return T();
            
            auto s_Value = m_Buffer[m_Tail];
            m_Full = false;
            m_Tail = (m_Tail + 1) % m_MaxSize;
            // TODO: unlock

            return s_Value;
        }

        const T* data()
        {
            return m_Buffer;
        }
    };
}