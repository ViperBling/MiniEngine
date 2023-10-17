#include "Reflection.hpp"

#include <map>
#include <string>

namespace MiniEngine
{
    namespace Reflection
    {
        const char* kUnknownType = "UnknownType";
        const char* kUnknown      = "Unknown";

        static std::map<std::string, ClassFunctionTuple*>       mClassMap;
        static std::multimap<std::string, FieldFunctionTuple*>  mFieldMap;
        static std::multimap<std::string, MethodFunctionTuple*> mMethodMap;
        static std::map<std::string, ArrayFunctionTuple*>       mArrayMap;

        void TypeMetaRegisterInterface::RegisterToFieldMap(const char* name, FieldFunctionTuple* value)
        {
            mFieldMap.insert(std::make_pair(name, value));
        }

        void TypeMetaRegisterInterface::RegisterToMethodMap(const char *name, MethodFunctionTuple *value)
        {
            mMethodMap.insert(std::make_pair(name, value));
        }

        void TypeMetaRegisterInterface::RegisterToArrayMap(const char* name, ArrayFunctionTuple* value)
        {
            if (mArrayMap.find(name) == mArrayMap.end())
            {
                mArrayMap.insert(std::make_pair(name, value));
            }
            else
            {
                delete value;
            }
        }

        void TypeMetaRegisterInterface::RegisterToClassMap(const char* name, ClassFunctionTuple* value)
        {
            if (mClassMap.find(name) == mClassMap.end())
            {
                mClassMap.insert(std::make_pair(name, value));
            }
            else
            {
                delete value;
            }
        }

        void TypeMetaRegisterInterface::UnregisterAll()
        {
            for (const auto& itr : mFieldMap)
            {
                delete itr.second;
            }
            mFieldMap.clear();
            for (const auto& itr : mClassMap)
            {
                delete itr.second;
            }
            mClassMap.clear();
            for (const auto& itr : mArrayMap)
            {
                delete itr.second;
            }
            mArrayMap.clear();
        }

        TypeMeta::TypeMeta(std::string typeName) : mTypeName(typeName)
        {
            mbIsValid = false;
            mFields.clear();

            auto fileds_iter = mFieldMap.equal_range(typeName);
            while (fileds_iter.first != fileds_iter.second)
            {
                FieldAccessor f_field(fileds_iter.first->second);
                mFields.emplace_back(f_field);
                mbIsValid = true;

                ++fileds_iter.first;
            }
        }

        TypeMeta::TypeMeta() : mTypeName(kUnknownType), mbIsValid(false) { mFields.clear(); }

        TypeMeta TypeMeta::NewMetaFromName(std::string typeName)
        {
            TypeMeta f_type(typeName);
            return f_type;
        }

        bool TypeMeta::NewArrayAccessorFromName(std::string arrayTypeName, ArrayAccessor& accessor)
        {
            auto iter = mArrayMap.find(arrayTypeName);

            if (iter != mArrayMap.end())
            {
                ArrayAccessor newAccessor(iter->second);
                accessor = newAccessor;
                return true;
            }

            return false;
        }

        ReflectionInstance TypeMeta::NewFromNameAndJson(std::string typeName, const Json& jsonContext)
        {
            auto iter = mClassMap.find(typeName);

            if (iter != mClassMap.end())
            {
                return ReflectionInstance(TypeMeta(typeName), (std::get<1>(*iter->second)(jsonContext)));
            }
            return ReflectionInstance();
        }

        Json TypeMeta::WriteByName(std::string type_name, void* instance)
        {
            auto iter = mClassMap.find(type_name);

            if (iter != mClassMap.end())
            {
                return std::get<2>(*iter->second)(instance);
            }
            return Json();
        }

        std::string TypeMeta::GetTypeName() { return mTypeName; }

        int TypeMeta::GetFieldsList(FieldAccessor*& out_list)
        {
            int count = mFields.size();
            out_list  = new FieldAccessor[count];
            for (int i = 0; i < count; ++i)
            {
                out_list[i] = mFields[i];
            }
            return count;
        }

        int TypeMeta::GetMethodsList(MethodAccessor *&out_list)
        {
            int count = mMethods.size();
            out_list  = new MethodAccessor[count];
            for (int i = 0; i < count; ++i)
            {
                out_list[i] = mMethods[i];
            }
            return count;
        }

        int TypeMeta::GetBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance)
        {
            auto iter = mClassMap.find(mTypeName);

            if (iter != mClassMap.end())
            {
                return (std::get<0>(*iter->second))(out_list, instance);
            }

            return 0;
        }

        FieldAccessor TypeMeta::GetFieldByName(const char* name)
        {
            const auto it = std::find_if(mFields.begin(), mFields.end(), [&](const auto& i) {
                return std::strcmp(i.GetFieldName(), name) == 0;
            });
            if (it != mFields.end())
                return *it;
            return FieldAccessor(nullptr);
        }

        MethodAccessor TypeMeta::GetMethodByName(const char *name)
        {
            const auto it = std::find_if(mMethods.begin(), mMethods.end(), [&](const auto& i) {
                return std::strcmp(i.GetMethodName(), name) == 0;
            });
            if (it != mMethods.end())
                return *it;
            return MethodAccessor(nullptr);
        }

        TypeMeta& TypeMeta::operator=(const TypeMeta& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mFields.clear();
            mFields = dest.mFields;

            mTypeName = dest.mTypeName;
            mbIsValid  = dest.mbIsValid;

            return *this;
        }
        FieldAccessor::FieldAccessor()
        {
            mFieldTypeName = kUnknownType;
            mFieldName      = kUnknown;
            mFunctions       = nullptr;
        }

        FieldAccessor::FieldAccessor(FieldFunctionTuple* functions) : mFunctions(functions)
        {
            mFieldTypeName = kUnknownType;
            mFieldName      = kUnknown;
            if (mFunctions == nullptr)
            {
                return;
            }

            mFieldTypeName = (std::get<4>(*mFunctions))();
            mFieldName      = (std::get<3>(*mFunctions))();
        }

        TypeMeta FieldAccessor::GetOwnerTypeMeta()
        {
            // todo: should check validation
            TypeMeta f_type((std::get<2>(*mFunctions))());
            return f_type;
        }

        bool FieldAccessor::GetTypeMeta(TypeMeta& field_type)
        {
            TypeMeta f_type(mFieldTypeName);
            field_type = f_type;
            return f_type.mbIsValid;
        }

        FieldAccessor& FieldAccessor::operator=(const FieldAccessor& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mFunctions       = dest.mFunctions;
            mFieldName      = dest.mFieldName;
            mFieldTypeName = dest.mFieldTypeName;
            return *this;
        }

        MethodAccessor::MethodAccessor()
        {
            mMethodName = kUnknown;
            mFunctions = nullptr;
        }

        void MethodAccessor::Invoke(void *instance)
        {
            (std::get<1>(*mFunctions))(instance);
        }

        const char *MethodAccessor::GetMethodName() const
        {
            return (std::get<0>(*mFunctions))();
        }

        MethodAccessor &MethodAccessor::operator=(const MethodAccessor &dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mFunctions  = dest.mFunctions;
            mMethodName = dest.mMethodName;
            return *this;
        }
        
        MethodAccessor::MethodAccessor(MethodFunctionTuple *functions) : mFunctions(functions)
        {
            mMethodName = kUnknown;
            if (mFunctions == nullptr) return;
            mMethodName = (std::get<0>(*mFunctions))();
        }

        ArrayAccessor::ArrayAccessor(ArrayFunctionTuple* array_func) : mFunction(array_func)
        {
            mArrayTypeName   = kUnknownType;
            mElementTypeName = kUnknownType;
            if (mFunction == nullptr)
            {
                return;
            }

            mArrayTypeName   = std::get<3>(*mFunction)();
            mElementTypeName = std::get<4>(*mFunction)();
        }

        void ArrayAccessor::Set(int index, void *instance, void *element_value)
        {
            // todo: should check validation
            size_t count = GetSize(instance);
            // todo: should check validation(index < count)
            std::get<0> (*mFunction)(index, instance, element_value);
        }

        void *ArrayAccessor::Get(int index, void *instance)
        {
            // todo: should check validation
            size_t count = GetSize(instance);
            // todo: should check validation(index < count)
            return std::get<1>(*mFunction)(index, instance);
        }

        int ArrayAccessor::GetSize(void *instance)
        {
            return std::get<2>(*mFunction)(instance);
        }

        ArrayAccessor &ArrayAccessor::operator=(const ArrayAccessor &dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mFunction        = dest.mFunction;
            mArrayTypeName   = dest.mArrayTypeName;
            mElementTypeName = dest.mElementTypeName;
            return *this;
        }

        ReflectionInstance& ReflectionInstance::operator=(ReflectionInstance& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mInstance = dest.mInstance;
            mMeta     = dest.mMeta;

            return *this;
        }

        ReflectionInstance& ReflectionInstance::operator=(ReflectionInstance&& dest)
        {
            if (this == &dest)
            {
                return *this;
            }
            mInstance = dest.mInstance;
            mMeta     = dest.mMeta;

            return *this;
        }
    }
} // namespace MiniEngine
