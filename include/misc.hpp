#pragma once

#include <list>
#include <functional>
#include <memory>
#include <iterator>

namespace misc
{
    template< typename T >
    void updateSortedList(std::list<T>& dst, const std::list<T>& src)
    {
        auto insertIter = dst.begin();
        for (auto srcIter = src.begin(); srcIter != src.end(); ++srcIter)
        {
            if (insertIter == dst.end() || insertIter == std::prev(dst.end()))
            {
                dst.insert(dst.end(), srcIter, src.end());
                break;
            }

            if (*insertIter == *srcIter)
            {
                auto copyBegin = std::next(srcIter);
                auto copyEnd = copyBegin;
                while (*copyEnd != *(std::next(insertIter)) && copyEnd != src.end())
                {
                    ++copyEnd;
                }

                if (copyBegin != copyEnd)
                {
                    dst.insert(insertIter, copyBegin, copyEnd);
                    std::advance(insertIter, std::distance(copyBegin, copyEnd));
                }
            }
        }
    }

    template< typename T >
        std::function<int(std::shared_ptr<T>, std::shared_ptr<T>)> getDataCmp()
        {
            return [](std::shared_ptr<T> fst, std::shared_ptr<T> snd)
            {
                if (fst->name == snd->name)
                {
                    return 0;
                }
                else if (fst->name < snd->name)
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            };
        }
}
