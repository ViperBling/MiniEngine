#pragma once

#include <functional>
#include <string>

#include "MRuntime/Core/Meta/Json.hpp"

namespace MiniEngine
{
#if defined(__REFLECTION_PARSER__)
#define META(...) __attribute__((annotate(#__VA_ARGS__)))
#define CLASS(class_name, ...) class __attribute__((annotate(#__VA_ARGS__))) class_name
#define STRUCT(struct_name, ...) struct __attribute__((annotate(#__VA_ARGS__))) struct_name
//#define CLASS(class_name,...) class __attribute__((annotate(#__VA_ARGS__))) class_name:public Reflection::object
#else
#define META(...)
#define CLASS(class_name, ...) class class_name
#define STRUCT(struct_name, ...) struct struct_name
//#define CLASS(class_name,...) class class_name:public Reflection::object
#endif // __REFLECTION_PARSER__

#define REFLECTION_BODY(class_name) \
    friend class Reflection::TypeFieldReflectionOperator::Type##class_name##Operator; \
    friend class Serializer;
    // public: virtual std::string getTypeName() override {return #class_name;}

#define REFLECTION_TYPE(class_name) \
    namespace Reflection \
    { \
        namespace TypeFieldReflectionOperator \
        { \
            class Type##class_name##Operator; \
        } \
    };

#define REGISTER_FIELD_TO_MAP(name, value) TypeMetaRegisterInterface::RegisterToFieldMap(name, value);
#define REGISTER_METHOD_TO_MAP(name, value) TypeMetaRegisterinterface::RegisterToMethodMap(name, value);
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

#define TypeMetaDef(class_name, ptr) \
    MiniEngine::Reflection::ReflectionInstance(MiniEngine::Reflection::TypeMeta::NewMetaFromName(#class_name), \
                                            (class_name*)ptr)

#define TypeMetaDefPtr(class_name, ptr) \
    new MiniEngine::Reflection::ReflectionInstance(MiniEngine::Reflection::TypeMeta::NewMetaFromName(#class_name), \
                                                (class_name*)ptr)

    template<typename T, typename U, typename = void>
    struct is_safely_castable : std::false_type
    {};

    template<typename T, typename U>
    struct is_safely_castable<T, U, std::void_t<decltype(static_cast<U>(std::declval<T>()))>> : std::true_type
    {};

    namespace Reflection
    {
        class TypeMeta;
        class FieldAccessor;
        class MethodAccessor;
        class ArrayAccessor;
        class ReflectionInstance;
    } // namespace Reflection

    using SetFunction        = std::function<void(void*, void*)>;
    using GetFunction        = std::function<void*(void*)>;
    using GetNameFunction    = std::function<const char*()>;
    using GetBoolFunction    = std::function<bool()>;
    using SetArrayFunction   = std::function<void(int, void*, void*)>;
    using GetArrayFunction   = std::function<void*(int, void*)>;
    using GetSizeFunction    = std::function<int(void*)>;
    using InvokeFunction     = std::function<void(void*)>;

    using ConstructorWithJson                        = std::function<void*(const Json&)>;
    using WriteJsonByName                            = std::function<Json(void*)>;
    using GetBaseClassReflectionInstanceListFunction = std::function<int(Reflection::ReflectionInstance*&, void*)>;

    using ClassFunctionTuple  = std::tuple<GetBaseClassReflectionInstanceListFunction, ConstructorWithJson, WriteJsonByName>;
    using MethodFunctionTuple = std::tuple<GetNameFunction, InvokeFunction>;
    using FieldFunctionTuple  = std::tuple<SetFunction, GetFunction, GetNameFunction, GetNameFunction, GetNameFunction, GetBoolFunction>;
    using ArrayFunctionTuple  = std::tuple<SetArrayFunction, GetArrayFunction, GetSizeFunction, GetNameFunction, GetNameFunction>;
    
    namespace Reflection
    {
        class TypeMetaRegisterInterface
        {
        public:
            static void RegisterToClassMap(const char* name, ClassFunctionTuple* value);
            static void RegisterToFieldMap(const char* name, FieldFunctionTuple* value);
            static void RegisterToMethodMap(const char* name, MethodFunctionTuple* value);
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
            int GetMethodsList(MethodAccessor*& out_list);
            int GetBaseClassReflectionInstanceList(ReflectionInstance*& out_list, void* instance);
            FieldAccessor GetFieldByName(const char* name);
            MethodAccessor GetMethodByName(const char* name);
            bool IsValid() { return mbIsValid; }

            TypeMeta& operator=(const TypeMeta& dest);

        private:
            explicit TypeMeta(std::string typeName);

        private:
            std::vector<FieldAccessor, std::allocator<FieldAccessor>> mFields;
            std::vector<MethodAccessor, std::allocator<MethodAccessor>> mMethods;
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

        class MethodAccessor
        {
            friend class TypeMeta;
        
        public:
            MethodAccessor();
            void Invoke(void* instance);

            const char* GetMethodName() const;

            MethodAccessor& operator=(const MethodAccessor& dest);

        private:
            MethodAccessor(MethodFunctionTuple* functions);

        private:
            MethodFunctionTuple* mFunctions;
            const char*          mMethodName;
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

            void Set(int index, void* instance, void* element_value);
            void* Get(int index, void* instance);
            int GetSize(void* instance);

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

            template<typename T1>
            explicit operator T1*()
            {
                return static_cast<T1*>(mInstance);
            }

            template<typename T1>
            operator ReflectionPtr<T1>()
            {
                return ReflectionPtr<T1>(mTypeName, (T1*)(mInstance));
            }

            template<typename T1>
            explicit operator const T1*() const
            {
                return static_cast<T1*>(mInstance);
            }

            template<typename T1>
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
