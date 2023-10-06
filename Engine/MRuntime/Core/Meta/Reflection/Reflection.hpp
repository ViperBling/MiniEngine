#pragma once

#include <functional>
#include <string>

#include "Core/Meta/Json.hpp"

namespace MiniEngine
{
#define META(...)
#define CLASS(className, ...) class className
#define STRUCT(structName, ...) struct structName

#define REFLECTION_BODY(className) \
    friend class Reflection::TypeFieldReflectionOparator::Type##className##Operator; \
    // friend class Serializer;

#define REFLECTION_TYPE(className) \
    namespace Reflection \
    { \
        namespace TypeFieldReflectionOparator \
        { \
            class Type##className##Operator; \
        } \
    };

#define REGISTER_FIELD_TO_MAP(name, value) TypeMetaRegisterInterface::RegisterToFieldMap(name, value);
#define REGISTER_BASE_CLASS_TO_MAP(name, value) TypeMetaRegisterInterface::RegisterToClassMap(name, value);
#define REGISTER_ARRAY_TO_MAP(name, value) TypeMetaRegisterInterface::RegisterToArrayMap(name, value);
#define UNREGISTER_ALL TypeMetaRegisterInterface::UnregisterAll();

#define ME_REFLECTION_NEW(name, ...) Reflection::ReflectionPtr(#name, new name(__VA_ARGS__));
#define ME_REFLECTION_DELETE(value) \
    if (value) \
    { \
        delete value.operator->(); \
        value.GetPtrReference() = nullptr; \
    }
#define ME_REFLECTION_DEEP_COPY(type, dst_ptr, src_ptr) \
    *static_cast<type*>(dst_ptr) = *static_cast<type*>(src_ptr.GetPtr());

    namespace Reflection
    {
        class TypeMeta;
        class FieldAccessor;
        class ArrayAccessor;
        class ReflectionInstance;
    } // namespace Reflection

    using SetFunction        = std::function<void(void*, void*)>;
    using GetFuncion         = std::function<void*(void*)>;
    using GetNameFuncion     = std::function<const char*()>;
    using GetBoolFunction    = std::function<bool()>;
    using SetArrayFunction   = std::function<void(int, void*, void*)>;
    using GetArrayFunction   = std::function<void*(int, void*)>;
    using GetSizeFunction    = std::function<int(void*)>;

    using ConstructorWithJson                        = std::function<void*(const Json&)>;
    using WriteJsonByName                            = std::function<Json(void*)>;
    using GetBaseClassReflectionInstanceListFunction = std::function<int(Reflection::ReflectionInstance*&, void*)>;

    using ClassFunctionTuple = std::tuple<GetBaseClassReflectionInstanceListFunction, ConstructorWithJson, WriteJsonByName>;
    using FieldFunctionTuple = std::tuple<SetFunction, GetFuncion, GetNameFuncion, GetNameFuncion, GetNameFuncion, GetBoolFunction>;
    using ArrayFunctionTuple = std::tuple<SetArrayFunction, GetArrayFunction, GetSizeFunction, GetNameFuncion, GetNameFuncion>;
    
    namespace Reflection
    {
        class TypeMetaRegisterInterface
        {
        public:
            static void RegisterToClassMap(const char* name, ClassFunctionTuple* value);
            static void RegisterToFieldMap(const char* name, FieldFunctionTuple* value);
            static void RegisterToArrayMap(const char* name, ArrayFunctionTuple* value);

            static void UnregisterAll();
        };

        class TypeMeta
        {
            friend class FieldAccessor;
            friend class ArrayAccessor;
            friend class TypeMetaRegisterinterface;

        public:
            TypeMeta();

            // static void Register();
            static TypeMeta           NewMetaFromName(std::string typeName);
            static bool               NewArrayAccessorFromName(std::string arrayTypeName, ArrayAccessor& accessor);
            static ReflectionInstance NewFromNameAndJson(std::string typeName, const Json& jsonContext);
            static Json               WriteByName(std::string typeName, void* instance);

            std::string GetTypeName();
            int GetFieldsList(FieldAccessor*& out_list);
            int GetBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance);
            FieldAccessor GetFieldByName(const char* name);
            bool IsValid() { return mbIsValid; }

            TypeMeta& operator=(const TypeMeta& dest);

        private:
            explicit TypeMeta(std::string typeName);

        private:
            std::vector<FieldAccessor, std::allocator<FieldAccessor>> mFields;
            std::string mTypeName;
            bool mbIsValid;
        };

        class FieldAccessor
        {
            friend class TypeMeta;

        public:
            FieldAccessor();
            void* Get(void* instance) { return static_cast<void*>((std::get<1>(*mFunctions))(instance)); }
            void  Set(void* instance, void* value) { (std::get<0>(*mFunctions))(instance, value); }

            TypeMeta GetOwnerTypeMeta();

            /**
             * param: TypeMeta out_type
             *        a reference of TypeMeta
             *
             * return: bool value
             *        true: it's a reflection type
             *        false: it's not a reflection type
             */
            bool        GetTypeMeta(TypeMeta& field_type);
            const char* GetFieldName() const { return mFieldName; }
            const char* GetFieldTypeName() { return mFieldTypeName; }
            bool        IsArrayType() { return (std::get<5>(*mFunctions))(); }

            FieldAccessor& operator=(const FieldAccessor& dest);

        private:
            explicit FieldAccessor(FieldFunctionTuple* function);
        private:
            FieldFunctionTuple* mFunctions;
            const char* mFieldName;
            const char* mFieldTypeName;
        };

        /**
         *  Function reflection is not implemented, so use this as an std::vector accessor
         */
        class ArrayAccessor
        {
            friend class TypeMeta;

        public:
            ArrayAccessor() : mFunction(nullptr), mArrayTypeName("UnKnownType"), mElementTypeName("UnKnownType") {}
            const char* GetArrayTypeName() { return mArrayTypeName; }
            const char* GetElementTypeName() { return mElementTypeName; }

            void Set(int index, void* instance, void* element_value)
            {
                // todo: should check validation
                size_t count = GetSize(instance);
                // todo: should check validation(index < count)
                std::get<0> (*mFunction)(index, instance, element_value);
            }
            void* Get(int index, void* instance)
            {
                // todo: should check validation
                size_t count = GetSize(instance);
                // todo: should check validation(index < count)
                return std::get<1>(*mFunction)(index, instance);
            }
            int GetSize(void* instance)
            {
                return std::get<2>(*mFunction)(instance);
            }

            ArrayAccessor& operator=(const ArrayAccessor& dest);

        private:
            explicit ArrayAccessor(ArrayFunctionTuple* array_func);

        private:
            ArrayFunctionTuple* mFunction;
            const char*         mArrayTypeName;
            const char*         mElementTypeName;
        };

        class ReflectionInstance
        {
        public:
            ReflectionInstance(TypeMeta meta, void* instance) : mMeta(meta), mInstance(instance) {}
            ReflectionInstance() : mMeta(), mInstance(nullptr) {}

            ReflectionInstance& operator=(ReflectionInstance& dest);
            ReflectionInstance& operator=(ReflectionInstance&& dest);

        public:
            TypeMeta mMeta;
            void*    mInstance;
        };

        template<typename T>
        class ReflectionPtr
        {
            template<typename U>
            friend class ReflectionPtr;

        public:
            ReflectionPtr(std::string typeName, T* instance) : mTypeName(typeName), mInstance(instance) {}
            ReflectionPtr() : mTypeName(), mInstance(nullptr) {}
            ReflectionPtr(const ReflectionPtr& dest) : mTypeName(dest.mTypeName), mInstance(dest.mInstance) {}

            template<typename U /*, typename = typename std::enable_if<std::is_safely_castable<T*, U*>::value>::type */>
            ReflectionPtr<T>& operator=(const ReflectionPtr<U>& dest)
            {
                if (this == static_cast<void*>(&dest))
                {
                    return *this;
                }
                mTypeName = dest.mTypeName;
                mInstance  = static_cast<T*>(dest.mInstance);
                return *this;
            }

            template<typename U /*, typename = typename std::enable_if<std::is_safely_castable<T*, U*>::value>::type*/>
            ReflectionPtr<T>& operator=(ReflectionPtr<U>&& dest)
            {
                if (this == static_cast<void*>(&dest))
                {
                    return *this;
                }
                mTypeName = dest.mTypeName;
                mInstance  = static_cast<T*>(dest.mInstance);
                return *this;
            }

            ReflectionPtr<T>& operator=(const ReflectionPtr<T>& dest)
            {
                if (this == &dest)
                {
                    return *this;
                }
                mTypeName = dest.mTypeName;
                mInstance  = dest.mInstance;
                return *this;
            }

            ReflectionPtr<T>& operator=(ReflectionPtr<T>&& dest)
            {
                if (this == &dest)
                {
                    return *this;
                }
                mTypeName = dest.mTypeName;
                mInstance  = dest.mInstance;
                return *this;
            }

            bool operator==(const T* ptr) const { return (mInstance == ptr); }
            bool operator!=(const T* ptr) const { return (mInstance != ptr); }
            bool operator==(const ReflectionPtr<T>& rhs_ptr) const { return (mInstance == rhs_ptr.mInstance); }
            bool operator!=(const ReflectionPtr<T>& rhs_ptr) const { return (mInstance != rhs_ptr.mInstance); }

            std::string GetTypeName() const { return mTypeName; }
            void SetTypeName(std::string name) { mTypeName = name; }

            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            explicit operator T1*()
            {
                return static_cast<T1*>(mInstance);
            }
            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            operator ReflectionPtr<T1>()
            {
                return ReflectionPtr<T1>(mTypeName, (T1*)(mInstance));
            }
            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            explicit operator const T1*() const
            {
                return static_cast<T1*>(mInstance);
            }
            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            operator const ReflectionPtr<T1>() const
            {
                return ReflectionPtr<T1>(mTypeName, (T1*)(mInstance));
            }
            T* operator->() { return mInstance; }
            T* operator->() const { return mInstance; }
            T& operator*() { return *(mInstance); }

            T* GetPtr() { return mInstance; }
            T* GetPtr() const { return mInstance; }
            const T& operator*() const { return *(static_cast<const T*>(mInstance)); }

            T*& GetPtrReference() { return mInstance; }

            operator bool() const { return (mInstance != nullptr); }

        private:
            std::string mTypeName {""};
            typedef T   mType;
            T*          mInstance {nullptr};
        };

    } // namespace Reflection
    
} // namespace MiniEngine
