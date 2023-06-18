#include "CELLFileUtil.h"
#include "CELLLogging.h"

FileUtil::FileUtil(std::string& fileName)
    : fp_(::fopen(fileName.c_str(), "a+")),
    writtenBytes_(0)
{
    // ��fd_����������Ϊ���ص�buffer_
    ::setvbuf(fp_, buffer_, _IOFBF, sizeof(buffer_));
}

FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

void FileUtil::append(const char* data, size_t len)
{
    // ��¼�Ѿ�д������ݴ�С
    size_t written = 0;

    while (written != len)
    {
        // ����д������ݴ�С
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
            }
        }
        // ����д������ݴ�С
        written += n;
    }
    // ��¼ĿǰΪֹд������ݴ�С���������ƻ������־
    writtenBytes_ += written;
}

void FileUtil::flush()
{
    ::fflush(fp_);
}

size_t FileUtil::write(const char* data, size_t len)
{
    /**
     * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
     * -- buffer:ָ�����ݿ��ָ��
     * -- size:ÿ�����ݵĴ�С����λΪByte(���磺sizeof(int)����4)
     * -- count:���ݸ���
     * -- stream:�ļ�ָ��
     */
    return ::_fwrite_nolock(data, 1, len, fp_);
}