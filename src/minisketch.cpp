/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "../include/minisketch.h"
#include "sketch.h"

#ifdef __x86_64__
#include <cpuid.h>
#endif

Sketch* ConstructGeneric1Byte(int bits, int implementation);
Sketch* ConstructGeneric2Bytes(int bits, int implementation);
Sketch* ConstructGeneric3Bytes(int bits, int implementation);
Sketch* ConstructGeneric4Bytes(int bits, int implementation);
Sketch* ConstructGeneric5Bytes(int bits, int implementation);
Sketch* ConstructGeneric6Bytes(int bits, int implementation);
Sketch* ConstructGeneric7Bytes(int bits, int implementation);
Sketch* ConstructGeneric8Bytes(int bits, int implementation);

#ifdef __x86_64__
Sketch* ConstructClMul1Byte(int bits, int implementation);
Sketch* ConstructClMul2Bytes(int bits, int implementation);
Sketch* ConstructClMul3Bytes(int bits, int implementation);
Sketch* ConstructClMul4Bytes(int bits, int implementation);
Sketch* ConstructClMul5Bytes(int bits, int implementation);
Sketch* ConstructClMul6Bytes(int bits, int implementation);
Sketch* ConstructClMul7Bytes(int bits, int implementation);
Sketch* ConstructClMul8Bytes(int bits, int implementation);
Sketch* ConstructClMulTri1Byte(int bits, int implementation);
Sketch* ConstructClMulTri2Bytes(int bits, int implementation);
Sketch* ConstructClMulTri3Bytes(int bits, int implementation);
Sketch* ConstructClMulTri4Bytes(int bits, int implementation);
Sketch* ConstructClMulTri5Bytes(int bits, int implementation);
Sketch* ConstructClMulTri6Bytes(int bits, int implementation);
Sketch* ConstructClMulTri7Bytes(int bits, int implementation);
Sketch* ConstructClMulTri8Bytes(int bits, int implementation);
#endif

namespace {

enum class FieldImpl {
    GENERIC = 0,
#ifdef __x86_64__
    CLMUL,
    CLMUL_TRI,
#endif
};

Sketch* Construct(int bits, int impl)
{
    switch (FieldImpl(impl)) {
    case FieldImpl::GENERIC:
        switch ((bits + 7) / 8) {
        case 1:
            return ConstructGeneric1Byte(bits, impl);
        case 2:
            return ConstructGeneric2Bytes(bits, impl);
        case 3:
            return ConstructGeneric3Bytes(bits, impl);
        case 4:
            return ConstructGeneric4Bytes(bits, impl);
        case 5:
            return ConstructGeneric5Bytes(bits, impl);
        case 6:
            return ConstructGeneric6Bytes(bits, impl);
        case 7:
            return ConstructGeneric7Bytes(bits, impl);
        case 8:
            return ConstructGeneric8Bytes(bits, impl);
        default:
            return nullptr;
        }
#ifdef __x86_64__
    case FieldImpl::CLMUL:
    case FieldImpl::CLMUL_TRI: {
        uint32_t eax, ebx, ecx, edx;
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx & 0x2)) {
            switch ((bits + 7) / 8) {
            case 1:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul1Byte(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri1Byte(bits, impl);
            case 2:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul2Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri2Bytes(bits, impl);
            case 3:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul3Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri3Bytes(bits, impl);
            case 4:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul4Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri4Bytes(bits, impl);
            case 5:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul5Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri5Bytes(bits, impl);
            case 6:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul6Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri6Bytes(bits, impl);
            case 7:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul7Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri7Bytes(bits, impl);
            case 8:
                if (FieldImpl(impl) == FieldImpl::CLMUL) return ConstructClMul8Bytes(bits, impl);
                if (FieldImpl(impl) == FieldImpl::CLMUL_TRI) return ConstructClMulTri8Bytes(bits, impl);
            default:
                return nullptr;
            }
        }
    }
#endif
    }
    return nullptr;
}

}

extern "C" {

int minisketch_bits_supported(uint32_t bits) {
    return (bits >= 2) && (bits <= 64);
}

uint32_t minisketch_implementation_max() {
    uint32_t ret = 0;
#ifdef __x86_64__
    ret += 2;
#endif
    return ret;
}

minisketch* minisketch_create(uint32_t bits, uint32_t implementation, size_t capacity) {
    Sketch* sketch = Construct(bits, implementation);
    if (sketch) {
        sketch->Init(capacity);
        sketch->Ready();
    }
    return (minisketch*)sketch;
}

void minisketch_destroy(minisketch* sketch) {
    if (sketch) {
        Sketch* s = (Sketch*)sketch;
        s->UnReady();
        delete s;
    }
}

size_t minisketch_serialized_size(const minisketch* sketch) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    size_t bits = s->Bits();
    size_t syndromes = s->Syndromes();
    return (bits * syndromes + 7) / 8;
}

void minisketch_serialize(const minisketch* sketch, unsigned char* output) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    s->Serialize(output);
}

void minisketch_deserialize(minisketch* sketch, const unsigned char* input) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->Deserialize(input);
}

void minisketch_add_uint64(minisketch* sketch, uint64_t element) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->Add(element);
}

void minisketch_merge(minisketch* sketch, const minisketch* other_sketch) {
    Sketch* s1 = (Sketch*)sketch;
    const Sketch* s2 = (const Sketch*)other_sketch;
    s1->Check();
    s2->Check();
    if (s1->Bits() != s2->Bits()) return;
    if (s1->Implementation() != s2->Implementation()) return;
    s1->Merge(s2);
}

ssize_t minisketch_decode(const minisketch* sketch, size_t max_elements, uint64_t* output) {
    const Sketch* s = (const Sketch*)sketch;
    s->Check();
    return s->Decode(max_elements, output);
}

void minisketch_set_seed(minisketch* sketch, uint64_t seed) {
    Sketch* s = (Sketch*)sketch;
    s->Check();
    s->SetSeed(seed);
}

}
