/*
 *  PStringTable.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_STRINGTABLE_HEADER
#define PIKA_STRINGTABLE_HEADER

namespace pika
{

// StringTable /////////////////////////////////////////////////////////////////////////////////////
// TODO: Slots should be resizable.

class StringTable
{
    static size_t const ENTRY_SIZE = 2048;
public:
    StringTable(Engine*);

    ~StringTable();

    String* Get(const char* cstr);
    String* Get(const char*, size_t);
private:
    friend class Engine;

    void Sweep();
    void SweepAll();

    void Clear();

    String* Slots[ENTRY_SIZE];
    Engine* engine;
};

}

#endif
