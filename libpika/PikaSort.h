/*
 *  PAraySort.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PARRAY_HEADER
#define PARRAY_HEADER

namespace pika {

#define INSERTION_SORT_CUTOFF 20

template<typename _Iter, typename _Comp>
inline void
insertion_sort(_Iter left, _Iter right, _Comp compfn)
{
    for (_Iter i = left + 1; i < right; ++i)
    {   
        typename std::iterator_traits<_Iter>::value_type store = *i;       
        _Iter j = i - 1; 
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

template<typename _Iter, typename _Comp>
inline _Iter
median3(_Iter left, _Iter right, _Comp compfn)
{
    _Iter center =  left + (right - left) / 2;
    
    if (compfn(*center , *left))
        std::swap(*left, *center);
    if (compfn(*right , *left))
        std::swap(*left, *right);
    if (compfn(*right , *center))
        std::swap(*right, *center);
    return right;
}

template<typename _Iter, typename _Comp>
inline void
quick_sort(_Iter left, _Iter right, _Comp compfn)
{
    size_t stride = right - left;
    if (stride > INSERTION_SORT_CUTOFF)
    {
        _Iter pivot = median3(left, right - 1, compfn);
        _Iter i     = left;
        _Iter store = left;
        _Iter stop  = right - 1;
        
        for (_Iter i = left; i < stop; ++i)
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

template<typename _Iter, typename _Comp>
inline void
pika_sort(_Iter left, _Iter right, _Comp compfn)
{
    quick_sort(left, right, compfn);
}

}// pika

#endif
