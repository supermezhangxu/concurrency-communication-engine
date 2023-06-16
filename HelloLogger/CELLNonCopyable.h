#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

/*
��ֹ���������Ļ��࣬����ΪprotectȨ�޵ĳ�Ա����������������̳�
�����������������Ĺ��������
*/

// TODO: noncopyable ��ѧϰ
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif // NONCOPYABLE_H