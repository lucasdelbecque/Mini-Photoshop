#include <iostream>
#include <vector>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main() {
        gl_Position = vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;

    uniform sampler2D screenTexture;
    uniform float u_intensity;
    uniform int u_mode;
    uniform float u_width;
    uniform float u_height;

    const float PI = 3.14159265359;

    void main() {
        vec4 originalColor = texture(screenTexture, TexCoord);
        vec4 resultColor = originalColor;

        if (u_mode == 0) {
            resultColor = originalColor;
        }
        else if (u_mode == 1) {
            float gray = dot(originalColor.rgb, vec3(0.299, 0.587, 0.114));
            resultColor = mix(originalColor, vec4(vec3(gray), 1.0), u_intensity);
        }
        else if (u_mode == 2) {
            float radius = u_intensity * 15.0;

            if (radius < 1.0) {
                resultColor = originalColor;
            } else {
                vec4 sum = vec4(0.0);
                float totalWeight = 0.0;

                float sigma = radius / 2.0;
                float twoSigmaSq = 2.0 * sigma * sigma;
                float sigmaRoot = sqrt(2.0 * PI * sigma * sigma);


                int r = int(radius);
                for(int x = -r; x <= r; x++) {
                    for(int y = -r; y <= r; y++) {
                        float distSq = float(x*x + y*y);
                        float weight = exp(-distSq / twoSigmaSq) / sigmaRoot;

                        vec2 offset = vec2(float(x) / u_width, float(y) / u_height);
                        sum += texture(screenTexture, TexCoord + offset) * weight;
                        totalWeight += weight;
                    }
                }
                resultColor = sum / totalWeight;
            }
        }
        else if (u_mode == 3) {
            if (u_intensity > 0.01) {
                float blocks = 200.0 * (1.0 - u_intensity) + 5.0;
                float dx = 1.0 / blocks;
                float dy = 1.0 / (blocks * (u_width / u_height));

                vec2 coord;
                coord.x = dx * floor(TexCoord.x / dx) + (dx * 0.5);
                coord.y = dy * floor(TexCoord.y / dy) + (dy * 0.5);

                resultColor = texture(screenTexture, coord);
            }
        }
        else if (u_mode == 4) {
            float offset_x = 1.0 / u_width;
            float offset_y = 1.0 / u_height;
            vec2 offsets[9] = vec2[](
                vec2(-offset_x,  offset_y), vec2( 0.0f,    offset_y), vec2( offset_x,  offset_y),
                vec2(-offset_x,  0.0f),     vec2( 0.0f,    0.0f),     vec2( offset_x,  0.0f),
                vec2(-offset_x, -offset_y), vec2( 0.0f,   -offset_y), vec2( offset_x, -offset_y)
            );
            float kernel[9] = float[](1, 1, 1, 1, -8, 1, 1, 1, 1);

            vec3 col = vec3(0.0);
            for(int i = 0; i < 9; i++)
                col += vec3(texture(screenTexture, TexCoord + offsets[i])) * kernel[i];

            resultColor = mix(originalColor, vec4(col, 1.0), u_intensity);
        }
        else if (u_mode == 5) {
            vec4 inverted = vec4(1.0 - originalColor.rgb, 1.0);
            resultColor = mix(originalColor, inverted, u_intensity);
        }

        FragColor = resultColor;
    }
)";

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        cerr << "ERREUR SHADER:\n" << infoLog << endl;
    }
    return id;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Adobe GPU Engine v2", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    unsigned int vShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("input.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        cerr << "ERREUR CHARGEMENT IMAGE" << endl;
    }
    stbi_image_free(data);

    float vertices[] = {
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // VARIABLES ETAT
    int currentMode = 0;
    float intensity = 0.0f;
    const char* filterItems[] = { "Normal", "Noir & Blanc", "Flou Gaussien", "Mosaique", "Contours", "Negatif" };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Filtres Avances", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Combo("Effet", &currentMode, filterItems, IM_ARRAYSIZE(filterItems))) {
            intensity = 0.5f;
        }

        if (currentMode == 2) {
            ImGui::Text("Rayon du flou (Attention GPU!)");
            ImGui::SliderFloat("Puissance", &intensity, 0.0f, 1.0f);
        } else if (currentMode == 3) {
            ImGui::Text("Taille des pixels");
            ImGui::SliderFloat("Taille", &intensity, 0.0f, 1.0f);
        } else {
            ImGui::Text("Opacite / Mix");
            ImGui::SliderFloat("Intensite", &intensity, 0.0f, 1.0f);
        }

        ImGui::Text("Resolution: %dx%d", width, height);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_mode"), currentMode);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_intensity"), intensity);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_width"), (float)width);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_height"), (float)height);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}