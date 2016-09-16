#include "decode.hpp"

#include <iostream>

using namespace std;

string getMetadata(const string& key, const vector<AVDictionary*>& dictionaries)
{
    for (auto &dict : dictionaries)
    {
        AVDictionaryEntry* entry = av_dict_get(dict, key.c_str(), nullptr, 0);
        if (entry != nullptr)
        {
            return entry->value;
        }
    }
    return {};
}
