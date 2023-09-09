
// #include <iostream>
// #include <exception>
// #include <cassert>

#include <initializer_list>
#include <stdexcept>
#include <vector>


template <class TYPE>
class ring_buffer
{

public:

    using value_type = TYPE;
    using reference = TYPE & ;
    using const_reference = const TYPE &;
    using size_type = std::size_t;
    using container = std::vector<value_type>;

private:

    container m_array;

    size_type m_head;

    size_type m_tail;

    size_type m_contents_size;

    const size_type m_array_size;

    template <bool isconst> struct ring_iterator;
    using iterator = ring_iterator<false>;
    using const_iterator = ring_iterator<true>;

public:

    ring_buffer (size_type sz = 8) :
        m_array         (sz <= 1 ? 8 : sz),
        m_array_size    (sz <= 1 ? 8 : sz),
        m_head          (1),
        m_tail          (0),
        m_contents_size (0)
    {
        // No code needed
    }

    ring_buffer (std::initializer_list<TYPE> lst) :
        m_array         (lst),
        m_array_size    (lst.size()),
        m_head          (0),
        m_tail          (lst.size() - 1),
        m_contents_size (lst.size())
    {
        // No code, no check on size
    }


    reference operator [] (size_type index)
    {
        // const ring_buffer<TYPE>& constMe = *this;
        return const_cast<reference>(operator [](index));
    }

    const_reference operator [] (size_type index) const
    {
        index += m_head;
        index %= m_array_size;
        return m_array[index];
    }

    reference at (size_type index)
    {
        if (index < m_contents_size)
            return operator [](index);

        throw std::out_of_range("index too large");
    }

    const_reference at (size_type index) const
    {
        if (index < m_contents_size)
            return operator[](index);

        throw std::out_of_range("index too large");
    }


    iterator begin ();
    const_iterator begin () const;

    iterator end ();
    const_iterator end () const;

    // iterator cbegin () const;        TODO
    const_iterator cbegin () const;

    iterator rbegin ();
    const_iterator rbegin () const;

    // iterator cend () const;        TODO
    const_iterator cend () const;

    iterator rend ();
    const_iterator rend () const;

    reference front ()
    {
        return m_array[m_head];
    }

    const_reference front () const
    {
        return m_array[m_head];
    }

    reference back ()
    {
        return m_array[m_tail];
    }

    const_reference back () const
    {
        return m_array[m_tail];
    }

    void clear ()
    {
        m_head = 1;
        m_tail = m_contents_size = 0;
    }

    void push_back (const value_type & item);
    void pop_front ();

    size_type size () const
    {
        return m_contents_size;
    }

    size_type capacity () const
    {
        return m_array_size;
    }

    bool empty () const
    {
        return m_contents_size == 0;
    }

    bool full () const
    {
        return m_contents_size == m_array_size;
    }

    size_type max_size () const
    {
        return size_type(-1) / sizeof(value_type);
    }


private:

    /*
     * Start of nested class ring_iterator
     */

    template <bool isconst = false>
    class ring_iterator
    {
        friend class ring_buffer<TYPE>;

    private:

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = long long;
        using reference = typename std::conditional_t
        <
            isconst, TYPE const &, TYPE &
        >;
        using pointer = typename std::conditional_t
        <
            isconst, TYPE const *, TYPE *
        >;
        using vec_pointer = typename std::conditional_t
        <
            isconst, std::vector<TYPE> const *, std::vector<TYPE> *
        >;

    private:

        vec_pointer buffer_ptr;
        size_type offset;
        size_type index;
        bool reverse;

        bool comparable (const ring_iterator & other)
        {
            return (reverse == other.reverse);
        }

    public:

        ring_iterator () :
            buffer_ptr  (nullptr),
            offset      (0),
            index       (0),
            reverse     (false)
        {
            // No code
        }

        ring_iterator (const ring_buffer<TYPE>::ring_iterator<false> & i) :
            buffer_ptr  (i.buffer_ptr),
            offset      (i.offset),
            index       (i.index),
            reverse     (i.reverse)
        {
            // No code
        }

        reference operator * ()
        {
            if (reverse)
                return (*buffer_ptr)[
                    (buffer_ptr->size() + offset - index) % (buffer_ptr->size())];

            return (*buffer_ptr)[(offset+index)%(buffer_ptr->size())];
        }

        reference operator [] (size_type index)
        {
            ring_iterator iter = *this;
            iter.index += index;
            return *iter;
        }

        pointer operator->()
        {
            return &(operator *());
        }

        ring_iterator & operator ++ ()
        {
            ++index;
            return *this;
        }

        ring_iterator operator ++ (int)
        {
            ring_iterator iter = *this;
            ++index;
            return iter;
        }

        ring_iterator & operator -- ()
        {
            --index;
            return *this;
        }

        ring_iterator operator -- (int)
        {
            ring_iterator iter = *this;
            --index;
            return iter;
        }

        friend ring_iterator operator + (ring_iterator lhs, int rhs)
        {
            lhs.index += rhs;
            return lhs;
        }

        friend ring_iterator operator + (int lhs, ring_iterator rhs)
        {
            rhs.index += lhs;
            return rhs;
        }

        ring_iterator & operator += (int n)
        {
            index += n;
            return *this;
        }

        friend ring_iterator operator - (ring_iterator lhs, int rhs)
        {
            lhs.index -= rhs;
            return lhs;
        }

        friend difference_type operator -
        (
            const ring_iterator & lhs, const ring_iterator & rhs
        )
        {
            lhs.index -= rhs;
            return lhs.index - rhs.index;
        }

        ring_iterator & operator -= (int n)
        {
            index -= n;
            return *this;
        }

        bool operator == (const ring_iterator & other)
        {
            if (comparable(other))
                return (index + offset == other.index + other.offset);

            return false;
        }

        bool operator != (const ring_iterator & other)
        {
            if (comparable(other))
                return ! this->operator ==(other);

            return true;
        }

        bool operator < (const ring_iterator & other)
        {
            if (comparable(other))
                return (index + offset < other.index + other.offset);

            return false;
        }

        bool operator <= (const ring_iterator & other)
        {
            if (comparable(other))
                return (index + offset <= other.index + other.offset);

            return false;
        }

        bool operator > (const ring_iterator & other)
        {
            if (comparable(other))
                return !this->operator<=(other);

            return false;
        }

        bool operator >= (const ring_iterator & other)
        {
            if (comparable(other))
                return !this->operator<(other);

            return false;
        }

    };      // nested class ring_iterator

};          // class ring_buffer<TYPE>

/*
 * Inline functions.
 */

template<class TYPE>
void
ring_buffer<TYPE>::push_back (const value_type & item)
{
    /*
     * Increment tail
     */

    ++m_tail;
    ++m_contents_size;
    if (m_tail == m_array_size)
        m_tail = 0;

    if (m_contents_size > m_array_size)
    {
        /*
         * Increment head
         */

        if (m_contents_size == 0)
            return;

        ++m_head;
        --m_contents_size;
        if (m_head == m_array_size)
            m_head = 0;
    }
    m_array[m_tail] = item;
}

template<class TYPE>
void
ring_buffer<TYPE>::pop_front ()
{
    /*
     * Increment head
     */

    if (m_contents_size == 0)
        return;

    ++m_head;
    --m_contents_size;
    if (m_head == m_array_size)
        m_head = 0;
}


template<class TYPE>
typename ring_buffer<TYPE>::iterator ring_buffer<TYPE>::begin()
{
    iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = 0;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::begin() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = 0;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::cbegin() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = 0;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::iterator ring_buffer<TYPE>::rbegin()
{
    iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_tail;
    iter.index = 0;
    iter.reverse = true;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::rbegin() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_tail;
    iter.index = 0;
    iter.reverse = true;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::iterator ring_buffer<TYPE>::end()
{
    iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = m_contents_size;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::end() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = m_contents_size;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::cend() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_head;
    iter.index = m_contents_size;
    iter.reverse = false;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::iterator ring_buffer<TYPE>::rend()
{
    iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_tail;
    iter.index = m_contents_size;
    iter.reverse = true;
    return iter;
}

template<class TYPE>
typename ring_buffer<TYPE>::const_iterator ring_buffer<TYPE>::rend() const
{
    const_iterator iter;
    iter.buffer_ptr = &m_array;
    iter.offset = m_tail;
    iter.index = m_contents_size;
    iter.reverse = true;
    return iter;
}


