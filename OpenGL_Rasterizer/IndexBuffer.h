#include <glad/glad.h>

class IndexBuffer {
private:
    unsigned int renderer_id; // for implementation of a renderer with ability to utilize different graphics APIs
public:
    IndexBuffer(const unsigned int* data, unsigned int numbOfElements); // constructor
    ~IndexBuffer(); // destructor

    // methods
    void Bind() const; // binds vbo
    void Unbind() const; // unbinds vbo
};