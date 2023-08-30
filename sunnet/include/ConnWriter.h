#pragma once
#include<list>
#include<stdint.h>
#include<memory>
using namespace std;

class WriteObject{
public:
    streamsize start;
    streamsize len;
    shared_ptr<char> buff;
};

class ConnWriter{
public:
    int fd;
private:
    bool isClosing = false;
    list<shared_ptr<WriteObject>> objs;
public:
    void EntireWrite(shared_ptr<char> buff,streamsize len);
    void LingegerClose();
    void OnWriteable();
};