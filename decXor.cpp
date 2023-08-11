#include <iostream>
#include <fstream>

char* Xorcrypt(char* content, size_t length, const char* secretKey, size_t keyLength)
{
    for (size_t i = 0; i < length; i++)
    {
        content[i] ^= secretKey[i % keyLength];
    }

    return content;
}

void XorcryptFile(const std::string& filename, const char* secretKey, size_t keyLength)
{
    std::ifstream fileIn(filename, std::ios::binary);
    if (!fileIn)
    {
        std::cout << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Determine the size of the file
    fileIn.seekg(0, std::ios::end);
    size_t fileSize = fileIn.tellg();
    fileIn.seekg(0, std::ios::beg);

    // Allocate memory to hold the file content
    char* content = new char[fileSize];

    // Read the content of the file
    fileIn.read(content, fileSize);
    fileIn.close();

    // Encrypt or decrypt the content using XOR
    Xorcrypt(content, fileSize, secretKey, keyLength);

    std::ofstream fileOut(filename, std::ios::binary);
    if (!fileOut)
    {
        std::cout << "Failed to open file: " << filename << std::endl;
        delete[] content;
        return;
    }

    // Write the modified content back to the file
    fileOut.write(content, fileSize);
    fileOut.close();

    delete[] content;

    std::cout << "File " << filename << " has been XOR encrypted/decrypted." << std::endl;
}

int main(int args, char* argv[])
{
    std::string filename = argv[1];  // Replace with your file name
    const char* secretKey = argv[2];   // Replace with your secret key
    size_t keyLength = sizeof(secretKey);

    XorcryptFile(filename, secretKey, keyLength);

    return 0;
}
