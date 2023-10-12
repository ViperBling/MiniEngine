#pragma once

#include <unordered_map>

namespace MiniEngine
{
    static const size_t sInvalidGuid = 0;

    template<typename T>
    class GuidAllocator
    {
    public:
        static bool IsValidGuid(size_t guid) { return guid != sInvalidGuid; }

        size_t AllocateGuid(const T& t)
        {
            auto find_it = mElementsGuidMap.find(t);
            if (find_it != mElementsGuidMap.end())
            {
                return find_it->second;
            }

            for (size_t i = 0; i < mGuidElementsMap.size() + 1; i++)
            {
                size_t guid = i + 1;
                if (mGuidElementsMap.find(guid) == mGuidElementsMap.end())
                {
                    mGuidElementsMap.insert(std::make_pair(guid, t));
                    mElementsGuidMap.insert(std::make_pair(t, guid));
                    return guid;
                }
            }

            return sInvalidGuid;
        }

        bool GetGuidRelatedElement(size_t guid, T& t)
        {
            auto find_it = mGuidElementsMap.find(guid);
            if (find_it != mGuidElementsMap.end())
            {
                t = find_it->second;
                return true;
            }
            return false;
        }

        bool GetElementGuid(const T& t, size_t& guid)
        {
            auto find_it = mElementsGuidMap.find(t);
            if (find_it != mElementsGuidMap.end())
            {
                guid = find_it->second;
                return true;
            }
            return false;
        }

        bool HasElement(const T& t) { return mElementsGuidMap.find(t) != mElementsGuidMap.end(); }

        void FreeGuid(size_t guid)
        {
            auto find_it = mGuidElementsMap.find(guid);
            if (find_it != mGuidElementsMap.end())
            {
                const auto& ele = find_it->second;
                mElementsGuidMap.erase(ele);
                mGuidElementsMap.erase(guid);
            }
        }

        void FreeElement(const T& t)
        {
            auto find_it = mElementsGuidMap.find(t);
            if (find_it != mElementsGuidMap.end())
            {
                const auto& guid = find_it->second;
                mElementsGuidMap.erase(t);
                mGuidElementsMap.erase(guid);
            }
        }

        std::vector<size_t> GetAllocatedGuids() const
        {
            std::vector<size_t> allocated_guids;
            for (const auto& ele : mGuidElementsMap)
            {
                allocated_guids.push_back(ele.first);
            }
            return allocated_guids;
        }

        void Clear()
        {
            mElementsGuidMap.clear();
            mGuidElementsMap.clear();
        }

    private:
        std::unordered_map<T, size_t> mElementsGuidMap;
        std::unordered_map<size_t, T> mGuidElementsMap;
    };
} // namespace MiniEngine
