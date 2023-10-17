#include "MetaTest.hpp"
#include "Generated/Serializer/all_serializer.h"

#include "MRuntime/Core/Base/Marco.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace MiniEngine
{
    void metaExample()
    {
        Test1 test1_in;
        test1_in.m_int  = 12;
        test1_in.m_char = 'g';
        int i           = 1;
        test1_in.m_int_vector.emplace_back(&i);

        Test1 test1_out;
        // test on array
        Test2 test2_in;
        test2_in.m_test_base_array.emplace_back("Test1", &test1_in);
        Test1 Test2_temp;
        test2_in.m_test_base_array.emplace_back("Test1", &Test2_temp);

        // serializer & deserializer

        // write Test1_in (object) to Test1_json_in (json)
        auto test1_json_in = Serializer::Write(test1_in);

        std::string test1_context = test1_json_in.dump();

        // read Test1_context (json) to Test1_out (object)
        std::string err;

        auto&& Test1_json = Json::parse(test1_context, err);
        Serializer::Read(Test1_json, test1_out);
        LOG_INFO(test1_context);

        auto        Test2_json_in = Serializer::Write(test2_in);
        std::string test2_context = Test2_json_in.dump();

        std::fstream out_put("out.txt", std::ios::out);
        out_put << test2_context;
        out_put.flush();
        out_put.close();

        Test2  test2_out;
        auto&& test2_json = Json::parse(test2_context, err);
        Serializer::Read(test2_json, test2_out);
        LOG_INFO(test2_context.c_str());

        // reflection
        auto                       meta = TypeMetaDef(Test2, &test2_out);
        Reflection::FieldAccessor* fields;
        int                        fields_count = meta.mMeta.GetFieldsList(fields);
        for (int i = 0; i < fields_count; ++i)
        {
            auto filed_accesser = fields[i];
            std::cout << filed_accesser.GetFieldTypeName() << " " << filed_accesser.GetFieldName() << " "
                      << (char*)filed_accesser.Get(meta.mInstance) << std::endl;
            if (filed_accesser.IsArrayType())
            {
                Reflection::ArrayAccessor array_accesser;
                if (Reflection::TypeMeta::NewArrayAccessorFromName(filed_accesser.GetFieldTypeName(), array_accesser))
                {
                    void* field_instance = filed_accesser.Get(meta.mInstance);
                    int   count          = array_accesser.GetSize(field_instance);
                    auto  typeMetaItem   = Reflection::TypeMeta::NewMetaFromName(array_accesser.GetElementTypeName());
                    for (int index = 0; index < count; ++index)
                    {
                        std::cout << ":L:" << index << ":R:" << (int*)array_accesser.Get(index, field_instance)
                                  << std::endl;
                        ;
                    }
                }
            }
        }
    }
}