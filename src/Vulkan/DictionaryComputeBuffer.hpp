#ifndef __DICTIONARY_COMPUTE_BUFFER_H__
#define __DICTIONARY_COMPUTE_BUFFER_H__

#include "ComputeBuffer.hpp"
#include "../dictionary.hpp"

class DictionaryComputeBuffer: private ComputeBuffer {
private:
    Dictionary internal;

public:
    using ComputeBuffer::ComputeBuffer;

    void bindDictionary(Dictionary& d) {
        ensureNotCommited();

        internal = d;
        std::vector<uint8_t> data = rawSerializeDictionary(internal);

        // Sync serialized data with the gpu
        bufferSize = data.size();
        createBuffer(data.data());
    }

    const Dictionary& getData(){
        // Sync serialized data with the gpu
		std::vector<uint8_t> data(bufferSize);
        ComputeBuffer::getData(data);

        rawDeserializeDictionary(internal, data.data());
        return internal;
    }

private:

    /////  Serialize  /////

    static std::vector<uint8_t> rawSerializeVariant(const Variant& v){
        const Variant::Type t = v.getType();
        std::vector<uint8_t> out;

        int64_t d1;
        double d2;
        bool d3;
        std::string d4;

        switch(t){
        case Variant::INT:
            d1 = v;
            out.resize(sizeof(d1));
            memcpy(out.data(), &d1, sizeof(d1));
            break;
        case Variant::FLOAT:
            d2 = v;
            out.resize(sizeof(d2));
            memcpy(out.data(), &d2, sizeof(d2));
            break;
        case Variant::BOOL:
            d3 = v;
            out.resize(sizeof(d3));
            memcpy(out.data(), &d3, sizeof(d3));
            break;
        case Variant::STRING:
            d4 = v.getString();
            out.resize(d4.size() + 1);
            memcpy(out.data(), d4.c_str(), d4.size() + 1);
            break;
        case Variant::ARRAY:
            for(const Variant& curV: v.getArray()){
                std::vector<uint8_t> data = rawSerializeVariant(curV);

                out.reserve(out.size() + data.size());
                out.insert(out.end(), data.begin(), data.end());
            }
            break;
        default:
            break;
        }

        return out;
    }

    static std::vector<uint8_t> rawSerializeDictionary(const Dictionary& d){
        std::vector<uint8_t> out;

        for(const auto& pair: d){
            std::vector<uint8_t> data = rawSerializeVariant(pair.second);

            if(data.size()){
                out.reserve(out.size() + data.size());
                out.insert(out.end(), data.begin(), data.end());
            }
        }

        return out;
    }

    /////  Deserialize  /////

    static size_t rawDeserializeVariant(Variant& v, uint8_t* data){
        const Variant::Type t = v.getType();

        int64_t d1;
        double d2;
        bool d3;
        std::string d4;
        std::vector<Variant> d5;
        size_t size;

        switch(t){
        case Variant::INT:
            memcpy(&d1, data, sizeof(d1));
            v = d1;
            return sizeof(d1);
        case Variant::FLOAT:
            memcpy(&d2, data, sizeof(d2));
            v = d2;
            return sizeof(d2);
        case Variant::BOOL:
            memcpy(&d3, data, sizeof(d3));
            v = d3;
            return sizeof(d3);
        case Variant::STRING:
            v = d4 = std::string((const char*) data);
            return d4.size() + 1;
        case Variant::ARRAY:
            size = 0;

            d5 = v.getArray();
            for(Variant& cur: d5){
                size_t sizeIncrease = rawDeserializeVariant(cur, data);
                data += sizeIncrease;
                size += sizeIncrease;
            }

            v = d5;
            return size;
        default:
            v = mpark::monostate {};
            return 0;
        }
    }

    static void rawDeserializeDictionary(Dictionary& d, uint8_t* data){
        for(auto& pair: d)
            data += rawDeserializeVariant(pair.second, data);
    }
};


#endif /* end of include guard: __DICTIONARY_COMPUTE_BUFFER_H__ */
