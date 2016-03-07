#pragma once

#include <set>
#include <initializer_list>

template< typename T >
class Options
{
    template< typename Y >
	friend Options<Y> operator| (const Options<Y>& fst, const Options<Y>& snd);

    template< typename Y >
	friend Options<Y> operator& (const Options<Y>& fst, const Options<Y>& snd);

    template< typename Y >
	friend Options<Y> operator| (const Options<Y>& fst, const Y& snd);

    template< typename Y >
	friend Options<Y> operator& (const Options<Y>& fst, const Y& snd);

    std::set<T> options;

    public:
    Options<T>(std::set<T> options) :
	options(options) {};

    Options<T>(std::initializer_list<T> options) :
	options(options) {};

    Options<T>(const Options<T>& copy) :
	options(copy.options) {};

    operator bool() const
    {
	if (options.empty())
	{
	    return false;
	}
	else
	{
	    return true;
	}
    }
};


template< typename T >
Options<T> operator| (const Options<T>& fst, const Options<T>& snd)
{
    std::set<T> newOptions(fst.options);
    newOptions.insert(snd.options.begin(), snd.options.end());

    return Options<T>(newOptions);
}

template< typename T >
Options<T> operator& (const Options<T>& fst, const Options<T>& snd)
{
    std::set<T> newOptions;
    for (auto e : fst)
    {
        if (snd.options.count(e) != 0)
	{
	    newOptions.insert(e);
	}
    }

    return Options<T>(newOptions);
}


template< typename T >
Options<T> operator| (const Options<T>& fst, const T& snd)
{
    std::set<T> newOptions(fst.options);
    newOptions.insert(snd);

    return Options<T>(newOptions);
}

template< typename T >
Options<T> operator& (const Options<T>& fst, const T& snd)
{
    if (fst.options.count(snd) == 0)
    {
        return Options<T>{};
    }
    else
    {
        return Options<T>{snd};
    }
}
