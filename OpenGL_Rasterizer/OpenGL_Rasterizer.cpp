// OpenGL_Rasterizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <glad/glad.h> // obtains GPU openGL api function pointers for machine being used 
#include <GLFW/glfw3.h> // defines openGL context, handles IO and basic window operations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "stb_image.h"

#include "Camera.h"

#include "VertexBuffer.h"

// shader struct for convenient returning for ParseShader below
struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};

// Screen settings/instance fields
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// method/function headers
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // handles resizing
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
static ShaderProgramSource ParseShader(const std::string& filepath);
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
static unsigned int CompileShader(unsigned int type, const std::string& source);

// method definitions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    // if the escape key is pressed while the window is open, instruct it to close
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W)) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S)) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A)) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D)) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // casting input to float from double
    float xPos = static_cast<float>(xpos);
    float yPos = static_cast<float>(ypos);

    // ensure no sudden jump of camera when mouse cursor enters window boundaries
    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    // calculating the offsets
    float xoffset = xPos - lastX;
    float yoffset = lastY - yPos; // y is reversed, so calculation is too
    
    // update lastX and lastY
    lastX = xPos;
    lastY = yPos;

    // actually perform the camera calculations using inputs
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// Shader loading in from file methods below (following 3 methods) implemented from openGL lecture series
static ShaderProgramSource ParseShader(const std::string& filepath) {
    std::ifstream stream(filepath); // opens the file

    enum class ShaderType {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2]; // one array for vertex, another for fragment
    ShaderType type = ShaderType::NONE; // sets the default shadertype to NONE

    while (getline(stream, line)) {
        // if #shader HAS been found, then set mode, else set the shader elements
        if (line.find("#shader") != std::string::npos) {
            // if vertex is found, set to vertex mode, else if fragment found, fragment mode
            if (line.find("vertex") != std::string::npos) {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos) {
                type = ShaderType::FRAGMENT;
            }
        }
        else {
            // adds the line into the string stream for correct position and adds a new line to cap it off
            ss[(int)type] << line << '\n'; // cast the shader type to an int in order to index into string stream array for correct shader - clever
        }
    }

    return { ss[0].str(), ss[1].str() };
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vShader = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return program;
}

static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // error checking 
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char)); // allows us to set up a char array of length size
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

void handleVAO() {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void handleLightVAO() {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void handleTextures(unsigned int& texture1, const std::string& location) {
    // generate texture IDs
    glGenTextures(1, &texture1);
    const char* loc = location.c_str();

    // handling first texture (rug)
    glBindTexture(GL_TEXTURE_2D, texture1); // binding current texture
    // setting texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // setting texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // load the image
    int imWidth, imHeight, nrChannels;
    unsigned char* data = stbi_load(loc, &imWidth, &imHeight, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imWidth, imHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Initialize window through GLFW
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Learning OpenGL Project", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window. " << std::endl; // System.out.println(); equivalent
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ensures the mouse cursor doesn't display and applies to window
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Ensure GLAD is initialized
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // enable depth testing
    glEnable(GL_DEPTH_TEST); 

    // read in shader from file
    ShaderProgramSource source = ParseShader("res/shaders/BasicShaders.shader");
    ShaderProgramSource sourceLight = ParseShader("res/shaders/BasicShadersLight.shader");
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    unsigned int lightShader = CreateShader(sourceLight.VertexSource, sourceLight.FragmentSource);

    float vertices[] = {
    0.5f, 0.5f, -2.0f,      0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
    0.5f, -0.5f, -2.0f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    -0.5f, 0.5f, -2.0f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
    0.5f, -0.5f, -2.0f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    -0.5f, -0.5f, -2.0f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
    -0.5f, 0.5f, -2.0f,     0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,     0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,    1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,    1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,     1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,     0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,     1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,    0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,    1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,    1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,    1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,    0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,    0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,     0.0f, 1.0f
    };

    float cubeLightVertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int VAO0, VAO1, VAO2;
    glGenVertexArrays(1, &VAO0);
    glBindVertexArray(VAO0);
    VertexBuffer vbo0(vertices, sizeof(vertices));
    handleVAO();
    glGenVertexArrays(1, &VAO1);
    glBindVertexArray(VAO1);
    VertexBuffer vbo1(cubeVertices, sizeof(cubeVertices));
    handleVAO();
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);
    VertexBuffer vbo2(cubeLightVertices, sizeof(cubeLightVertices));
    handleLightVAO();

    glBindVertexArray(VAO0);

    // handling textures
    unsigned int texture1, texture1Specular, texture2, texture2Specular;
    const std::string texture1Location = "res/textures/carpet_texture.png";
    const std::string texture1SpecularLocation = "res/textures/carpet_texture_specular.png";
    const std::string texture2Location = "res/textures/blanket_texture.png";
    const std::string texture2SpecularLocation = "res/textures/blanket_texture_specular.png";
    handleTextures(texture1, texture1Location);
    handleTextures(texture1Specular, texture1SpecularLocation);
    handleTextures(texture2, texture2Location);
    handleTextures(texture2Specular, texture2SpecularLocation);

    // creating the model matrix (transform to global world space), the view matrix (transform to camera view), and the projection matrix (transform to screen)
    glm::mat4 model = glm::mat4(1.0f);
    //glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::mat4(1.0f);
    // rotating to make a floor
    model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0, 3.0f, -1.7f));
    model = glm::scale(model, glm::vec3(18.0f, 18.0f, 1.0f));
    projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    // getting matrix uniform locations
    // vertex shader uniform locations
    unsigned int modelLoc = glGetUniformLocation(shader, "model");
    unsigned int viewLoc = glGetUniformLocation(shader, "view");
    unsigned int projectionLoc = glGetUniformLocation(shader, "projection");
    unsigned int lightColorLoc = glGetUniformLocation(shader, "lightColor");
    // fragment shader uniform locations - directional light
    unsigned int dirLightDirectionLoc = glGetUniformLocation(shader, "directionalLight.direction");
    unsigned int dirLightAmbientLoc = glGetUniformLocation(shader, "directionalLight.ambient");
    unsigned int dirLightDiffuseLoc = glGetUniformLocation(shader, "directionalLight.diffuse");
    unsigned int dirLightSpecularLoc = glGetUniformLocation(shader, "directionalLight.specular");
    // fragment shader uniform locations - point light
    unsigned int pointLightPositionLoc0 = glGetUniformLocation(shader, "pointLight[0].position");
    unsigned int pointLightAmbientLoc0 = glGetUniformLocation(shader, "pointLight[0].ambient");
    unsigned int pointLightDiffuseLoc0 = glGetUniformLocation(shader, "pointLight[0].diffuse");
    unsigned int pointLightSpecularLoc0 = glGetUniformLocation(shader, "pointLight[0].specular");
    unsigned int pointLightConstantLoc0 = glGetUniformLocation(shader, "pointLight[0].constant");
    unsigned int pointLightLinearLoc0 = glGetUniformLocation(shader, "pointLight[0].linear");
    unsigned int pointLightQuadraticLoc0 = glGetUniformLocation(shader, "pointLight[0].quadratic");
    unsigned int pointLightPositionLoc1 = glGetUniformLocation(shader, "pointLight[1].position");
    unsigned int pointLightAmbientLoc1 = glGetUniformLocation(shader, "pointLight[1].ambient");
    unsigned int pointLightDiffuseLoc1 = glGetUniformLocation(shader, "pointLight[1].diffuse");
    unsigned int pointLightSpecularLoc1 = glGetUniformLocation(shader, "pointLight[1].specular");
    unsigned int pointLightConstantLoc1 = glGetUniformLocation(shader, "pointLight[1].constant");
    unsigned int pointLightLinearLoc1 = glGetUniformLocation(shader, "pointLight[1].linear");
    unsigned int pointLightQuadraticLoc1 = glGetUniformLocation(shader, "pointLight[1].quadratic");
    // fragment shader uniform locations - spot light
    unsigned int spotLightPositionLoc = glGetUniformLocation(shader, "spotLight.position");
    unsigned int spotLightDirectionLoc = glGetUniformLocation(shader, "spotLight.direction");
    unsigned int spotLightCutoffLoc = glGetUniformLocation(shader, "spotLight.cutoff");
    unsigned int spotLightOuterCutoffLoc = glGetUniformLocation(shader, "spotLight.outerCutoff");
    unsigned int spotLightAmbientLoc = glGetUniformLocation(shader, "spotLight.ambient");
    unsigned int spotLightDiffuseLoc = glGetUniformLocation(shader, "spotLight.diffuse");
    unsigned int spotLightSpecularLoc = glGetUniformLocation(shader, "spotLight.specular");

    unsigned int viewPositionLoc = glGetUniformLocation(shader, "viewPosition");

    unsigned int modelLocLight = glGetUniformLocation(lightShader, "model");
    unsigned int viewLocLight = glGetUniformLocation(lightShader, "view");
    unsigned int projectionLocLight = glGetUniformLocation(lightShader, "projection");

    glm::vec3 cubePointLightPos[] = {
        glm::vec3(-2.0f, 3.3f, -2.3f), 
        glm::vec3(1.7f, 2.7f, 2.5f)
    };

    // keep the window open in the render loop until instructed to close
    // glfwWindowShouldClose checks whether the window should close each loop iteration
    while (!glfwWindowShouldClose(window)) {
        // time calculations
        float timeOfCurrentFrame = static_cast<float>(glfwGetTime());
        deltaTime = timeOfCurrentFrame - lastFrame;
        lastFrame = timeOfCurrentFrame;

        processInput(window); // handles input - currently checking for closing via escape key

        // rendering commands should appear below here, above glfwSwapBuffers(window)
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // test that rendering commands are working - clears color buffer with color specified in this function
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // the actual clear instruction, specified to the color buffer bit

        // drawing the triangle
        glUseProgram(shader);

        // passing uniforms to the shaders
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        glUniform3f(viewPositionLoc, camera.Position[0], camera.Position[1], camera.Position[2]);
        // setting directional light uniforms
        glUniform3f(dirLightDirectionLoc, -0.1f, -1.0f, 0.4f);
        glUniform3f(dirLightAmbientLoc, 0.05f, 0.05f, 0.05f);
        glUniform3f(dirLightDiffuseLoc, 0.125f, 0.125f, 0.125f);
        glUniform3f(dirLightSpecularLoc, 0.25f, 0.25f, 0.25f);
        // setting point light uniforms
        glUniform3f(pointLightPositionLoc0, cubePointLightPos[0][0], cubePointLightPos[0][1], cubePointLightPos[0][2]);
        glUniform3f(pointLightAmbientLoc0, 0.2f, 0.2f, 0.2f);
        glUniform3f(pointLightDiffuseLoc0, 0.5f, 0.5f, 0.5f);
        glUniform3f(pointLightSpecularLoc0, 1.0f, 1.0f, 1.0f);
        glUniform1f(pointLightConstantLoc0, 1.0f);
        glUniform1f(pointLightLinearLoc0, 0.045f);
        glUniform1f(pointLightQuadraticLoc0, 0.0075f);
        glUniform3f(pointLightPositionLoc1, cubePointLightPos[1][0], cubePointLightPos[1][1], cubePointLightPos[1][2]);
        glUniform3f(pointLightAmbientLoc1, 0.4f, 0.4f, 0.7f);
        glUniform3f(pointLightDiffuseLoc1, 0.4f, 0.4f, 0.7f);
        glUniform3f(pointLightSpecularLoc1, 0.4f, 0.4f, 0.7f);
        glUniform1f(pointLightConstantLoc1, 1.0f);
        glUniform1f(pointLightLinearLoc1, 0.045f);
        glUniform1f(pointLightQuadraticLoc1, 0.0075f);
        // setting spot light uniforms
        glUniform3f(spotLightPositionLoc, 0.4f, 3.0f, -6.4f);
        glUniform3f(spotLightDirectionLoc, -0.1f, -1.0f, 0.4f);
        glUniform1f(spotLightCutoffLoc, glm::cos(glm::radians(13.5f)));
        glUniform1f(spotLightOuterCutoffLoc, glm::cos(glm::radians(18.7f)));
        glUniform3f(spotLightAmbientLoc, 0.2f, 0.2f, 0.2f);
        glUniform3f(spotLightDiffuseLoc, 0.5f, 0.5f, 0.5f);
        glUniform3f(spotLightSpecularLoc, 1.0f, 1.0f, 1.0f);
        
        glUniform1i(glGetUniformLocation(shader, "material.diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glUniform1i(glGetUniformLocation(shader, "material.specular"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture1Specular);

        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = camera.GetViewMatrix();

        glDrawArrays(GL_TRIANGLES, 0, 6); // for plane

        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // what we're drawing, how many verts, data type, specified offset
        glBindVertexArray(VAO1);
        glUniform1i(glGetUniformLocation(shader, "material.diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glUniform1i(glGetUniformLocation(shader, "material.specular"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2Specular);
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glUseProgram(lightShader);
        glBindVertexArray(VAO2);
        // cube point light 1
        model = glm::mat4(1.0f);
        model = glm::translate(model, cubePointLightPos[0]);
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, -0.3f, 0.0f));
        glUniformMatrix4fv(modelLocLight, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocLight, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projectionLocLight, 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // cube point light 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, cubePointLightPos[1]);
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, -0.3f, 0.0f));
        glUniformMatrix4fv(modelLocLight, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocLight, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projectionLocLight, 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // cube spot light
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.4f, 3.0f, -6.4f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, -0.3f, 0.0f));
        glUniformMatrix4fv(modelLocLight, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocLight, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projectionLocLight, 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(VAO0);
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0, 1.0f, -0.7f));
        model = glm::scale(model, glm::vec3(18.0f, 18.0f, 1.0f));

        // a single buffer image draws the image pixel by pixel, which can cause flickering. double buffered images handle this with back and front buffers
        // the back buffer goes pixel by pixel, while the front buffer is what is shown on screen in the window. the back is swapped to front when ready
        glfwSwapBuffers(window); // handles the buffer containing the window's pixel color values and swaps it ouch each frame for the new one
        glfwPollEvents(); // checks for keyboard/mouse inputs
    }

    // cleanly de allocating no longer needed buffers and vertex arrays
    glDeleteVertexArrays(1, &VAO0);
    //glDeleteBuffers(1, &VBO0);
    glDeleteVertexArrays(1, &VAO1);
    //glDeleteBuffers(1, &VBO1);
    glDeleteVertexArrays(1, &VAO2);
    //glDeleteBuffers(1, &VBO2);
    // glDeleteBuffers(1, &EBO);

    glfwTerminate();// properly de-allocate allocated resources in GLFW, called when render loop is over
    return 0;
}
