#pragma once
#include <Utils/Types.hpp>
#include <Utils/Kernel.hpp>

// WARNING: Spans should never OWN the buffer
template<typename T>
class Span
{
protected:
    // Data pointer
    uint8_t* m_Data;

    // Data Length
    uint32_t m_DataLength;

    // Current offset within the span
    uint32_t m_Offset;

public:
    Span() :
        m_Data(nullptr),
        m_DataLength(0),
        m_Offset(0)
    {
        // DO Nothing
    }

    Span(uint8_t* p_Source, uint32_t p_SourceLength) :
        m_Data(p_Source),
        m_DataLength(p_SourceLength),
        m_Offset(0)
    {
    }

    ~Span()
    {        
        // Zero out everything for this span
        m_Data = nullptr;
        m_DataLength = 0;
        m_Offset = 0;
    }

    // Copy constructor
    Span(const Span& p_Other) :
        m_Data(p_Other.m_Data),
        m_DataLength(p_Other.m_DataLength),
        m_Offset(p_Other.m_Offset)
    {

    }

    // Copy assignment
    Span& operator=(const Span& p_Other)
    {
        if (&p_Other == this)
            return *this;

        m_Data = p_Other.m_Data;
        m_DataLength = p_Other.m_DataLength;
        m_Offset = p_Other.m_Offset;

        return *this;
    }

    // Move constructor
    Span(Span&& p_Other) :
        m_Data(p_Other.m_Data),
        m_DataLength(p_Other.m_DataLength),
        m_Offset(p_Other.m_Offset)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_DataLength = 0;
        p_Other.m_Offset = 0;
    }

    // Move assignment
    Span& operator=(Span&& p_Other)
    {
        if (&p_Other == this)
            return *this;
        
        m_Data = p_Other.m_Data;
        m_DataLength = p_Other.m_DataLength;
        m_Offset = p_Other.m_Offset;

        p_Other.m_Data = nullptr;
        p_Other.m_DataLength = 0;
        p_Other.m_Offset = 0;

        return *this;
    }

    T& operator*() const 
    { 
        if (sizeof(T) > m_DataLength)
            return T();
        
        return *reinterpret_cast<T*>(m_Data); 
    }

    bool isNull() const { return m_Data == nullptr || m_DataLength == 0; }

    T* operator->() const { return m_Data; }

    Span subspan(uint32_t p_Length)
    {
        if (p_Length > getRemainingBytes())
            return Span();

        return Span(m_Data + m_Offset, p_Length, false);
    }

    Span subspan(uint32_t p_Offset, uint32_t p_Length)
    {
        if (p_Offset + p_Length >= m_DataLength)
            return Span();
        
        return Span(m_Data + p_Offset, p_Length, false);
    }

    // Get a structure, returns nullptr if invalid or out of bounds of span
    template<typename X>
    X* get_struct()
    {
        auto s_ObjectSize = sizeof(X);
        if (getRemainingBytes() < s_ObjectSize)
            return nullptr;
        
        auto s_ObjectPtr = reinterpret_cast<X*>(m_Data + m_Offset);
        m_Offset += s_ObjectSize;
        return s_ObjectPtr;
    }

    // Gets a structure at a specific offset
    template<typename X>
    X* get_struct(uint32_t p_Offset)
    {
        if (p_Offset >= m_DataLength)
            return nullptr;
        
        if (p_Offset + sizeof(X) >= m_DataLength)
            return nullptr;
        
        return reinterpret_cast<X*>(m_Data + p_Offset);
    }

    template<typename X>
    bool set_struct(uint32_t p_Offset, X* p_Object)
    {
        if (p_Offset >= m_DataLength)
            return false;
        
        if (p_Offset + sizeof(X) >= m_DataLength)
            return false;
        
        if (p_Object == nullptr)
            return false;
        
        memcpy(m_Data + p_Offset, p_Object, sizeof(*p_Object));
        return true;
    }

    bool set_buffer(uint32_t p_Offset, uint8_t* p_Source, uint32_t p_Length)
    {
        if (p_Offset >= m_DataLength)
            return false;
        
        if (p_Offset + p_Length > m_DataLength)
            return false;
        
        if (p_Source == nullptr)
            return false;
        
        memcpy(m_Data + p_Offset, p_Source, p_Length);
        return true;
    }

    // Set the current offset
    void setOffset(uint32_t p_Offset)
    {
        if (p_Offset >= m_DataLength)
            return;
        
        m_Offset = p_Offset;
    }

    // Get the current offset
    uint32_t getOffset() { return m_Offset; }

    // Get the remaining bytes left
    uint32_t getRemainingBytes() { return m_DataLength - m_Offset; }
    
    // Get the length of the entier span
    uint32_t getLength() { return m_DataLength; }

    // Not operator
    bool operator !()
    {
        if (m_Data == nullptr || m_DataLength == 0)
            return true;
        
        return false;
    }

    // Creates a copy of a buffer, and holds it as a span
    static Span make_span_from(uint8_t* p_Source, uint32_t p_SourceLength)
    {       
        auto s_Span = Span(p_SourceLength);
        if (!s_Span)
            return Span();
        
        if (!s_Span.set_buffer(0, p_Source, p_SourceLength))
            return Span();

        return s_Span;
    }

    T* data() { return m_Data; }
    uint8_t* dataAtOffset() 
    {
        if (m_Data != nullptr) 
            return &m_Data[m_Offset]; 
        else 
            return nullptr; 
    }
    uint32_t size() { return m_DataLength; }

    void zero() { if (m_Data != nullptr && m_DataLength != 0) memset(m_Data, 0, m_DataLength); }
};