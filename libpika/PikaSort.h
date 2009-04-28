/*
 *  PAraySort.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PARRAY_HEADER
#define PARRAY_HEADER

namespace pika {

#define INSERTION_SORT_CUTOFF 20

template<typename IterT, typename CompT>
inline void
insertion_sort(IterT left, IterT right, CompT compfn)
{
    for (IterT i = left + 1; i < right; ++i)
    {   
        typename std::iterator_traits<IterT>::value_type store = *i;       
        IterT j = i - 1; 
        for (; j >= left; --j)
        {
            if(compfn(store, *j))
            {
                *(j + 1) = *j;
            }
            else
                break;
        }
        *(j+1) = store;
    }
}

template<typename IterT, typename CompT>
inline IterT
median3(IterT left, IterT right, CompT compfn)
{
    IterT center =  left + (right - left) / 2;
    
    if (compfn(*center , *left))
        std::swap(*left, *center);
    if (compfn(*right , *left))
        std::swap(*left, *right);
    if (compfn(*right , *center))
        std::swap(*right, *center);
    return right;
}

template<typename IterT, typename CompT>
inline void
quick_sort(IterT left, IterT right, CompT compfn)
{
    size_t stride = right - left;
    if (stride > INSERTION_SORT_CUTOFF)
    {
        IterT pivot = median3(left, right - 1, compfn);
        IterT i     = left;
        IterT store = left;
        IterT stop  = right - 1;
        
        for (IterT i = left; i < stop; ++i)
        {
            if (compfn(*i, *pivot))
            {
                std::swap(*i, *store);
                ++store;
            }
        }
        std::swap(*store, *stop);
        
        if (left  != store) quick_sort(left, store, compfn);
        if (store != right) quick_sort(store + 1, right, compfn);
    }
    else
    {
        insertion_sort(left, right, compfn);
    }
}

template<typename IterT, typename CompT>
inline void
pika_sort(IterT left, IterT right, CompT compfn)
{
    quick_sort(left, right, compfn);
}

}// pika

#endif
