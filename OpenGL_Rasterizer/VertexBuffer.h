#include <glad/glad.h>

class VertexBuffer {
private:
    unsigned int renderer_id; // for implementation of a renderer with ability to utilize different graphics APIs
public:
    VertexBuffer(const void* data, unsigned int size); // constructor
    ~VertexBuffer(); // destructor

    // methods
    void Bind() const; // binds vbo
    void Unbind() const; // unbinds vbo
};