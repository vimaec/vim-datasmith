#include "Utils.h"

#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <stdio.h>

namespace Utils
{

    const char* ReadFile(const std::string& fileName, off_t* size)
    {
        std::cout << "Reading file: " << fileName << std::endl;

        struct stat stat_buf;
        int rc = stat(fileName.c_str(), &stat_buf);
        if (rc == 0)
        {
            off_t fileSize = stat_buf.st_size;

            FILE* file;
			file = fopen(fileName.c_str(), "rb");
            if (file != nullptr)
            {
                auto buffer = tracked_new char[fileSize + 1];
                memset((void*)buffer, 0, fileSize + 1);
                fread((void*)buffer, 1, fileSize, file);

                if (size)
                {
                    *size = fileSize;
                }

                fclose(file);

                return buffer;
            }
        }

        std::cout << "Failed to read file: " << fileName << std::endl;

        return nullptr;
    }
}
