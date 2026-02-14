#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// OPENGL
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

// STB IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// --- SETTINGS UI ---
const int TOP_BAR_HEIGHT = 40;
const int SIDEBAR_WIDTH = 300;

// --- STRUCTURE IMAGE ---
struct ImageEditor {
    unsigned int textureID;
    int width, height, channels;
    bool isLoaded = false;

    // Filtres
    bool enableGrayscale = false; float intensityGrayscale = 1.0f;
    bool enableBlur = false;      float intensityBlur = 0.0f;
    bool enableMosaic = false;    float intensityMosaic = 0.0f;
    bool enableEdge = false;      float intensityEdge = 0.0f;
    bool enableInvert = false;    float intensityInvert = 0.0f;

    bool load(const char* filepath) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            isLoaded = true;
            return true;
        }
        return false;
    }
};

// --- SHADERS ---
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
    uniform float u_width; uniform float u_height;
    uniform int u_enableGray;    uniform float u_intGray;
    uniform int u_enableBlur;    uniform float u_intBlur;
    uniform int u_enableMosaic;  uniform float u_intMosaic;
    uniform int u_enableEdge;    uniform float u_intEdge;
    uniform int u_enableInvert;  uniform float u_intInvert;
    const float PI = 3.14159265359;

    vec4 applyBlur(vec4 col, vec2 uv, float intensity) {
        float radius = intensity * 25.0;
        if (radius < 1.0) return col;
        vec4 sum = vec4(0.0);
        float totalWeight = 0.0;
        float sigma = radius / 2.0;
        float sigmaRoot = sqrt(2.0 * PI * sigma * sigma);
        int r = int(radius);
        for(int x = -r; x <= r; x+=2) {
            for(int y = -r; y <= r; y+=2) {
                float weight = exp(-float(x*x + y*y) / (2.0 * sigma * sigma)) / sigmaRoot;
                sum += texture(screenTexture, uv + vec2(float(x)/u_width, float(y)/u_height)) * weight;
                totalWeight += weight;
            }
        }
        return sum / totalWeight;
    }

    vec4 applyEdge(vec4 col, vec2 uv, float intensity) {
        float ox = 1.0/u_width; float oy = 1.0/u_height;
        vec2 off[9] = vec2[](vec2(-ox,oy), vec2(0,oy), vec2(ox,oy), vec2(-ox,0), vec2(0,0), vec2(ox,0), vec2(-ox,-oy), vec2(0,-oy), vec2(ox,-oy));
        float k[9] = float[](1,1,1,1,-8,1,1,1,1);
        vec3 sum = vec3(0.0);
        for(int i=0; i<9; i++) sum += vec3(texture(screenTexture, uv+off[i])) * k[i];
        return mix(col, vec4(sum, 1.0), intensity);
    }

    void main() {
        vec2 uv = TexCoord;
        if (u_enableMosaic==1 && u_intMosaic>0.0) {
            float b = 200.0 * (1.0 - u_intMosaic) + 5.0;
            vec2 d = vec2(1.0/b, 1.0/(b*(u_width/u_height)));
            uv = d * floor(uv/d) + d*0.5;
        }
        vec4 c = texture(screenTexture, uv);
        if (u_enableBlur==1) c = applyBlur(c, uv, u_intBlur);
        if (u_enableGray==1) c = mix(c, vec4(vec3(dot(c.rgb, vec3(0.299,0.587,0.114))), 1.0), u_intGray);
        if (u_enableEdge==1) c = applyEdge(c, uv, u_intEdge);
        if (u_enableInvert==1) c = mix(c, vec4(1.0-c.rgb, 1.0), u_intInvert);
        FragColor = c;
    }
)";

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    return id;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // On retire la décoration de fenêtre native (titre) pour faire la nôtre
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "GPU Studio", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    unsigned int p = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);

    ImageEditor editor;
    if (!editor.load("input.jpg")) cout << "Input.jpg missing" << endl;

    float vertices[] = { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  -1.0f, 1.0f, 0.0f, 0.0f, 1.0f };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);

    // BOUCLE PRINCIPALE
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. RECUPERER LES DIMENSIONS CORRECTES
        int display_w, display_h; // En Pixels (Pour OpenGL)
        glfwGetFramebufferSize(window, &display_w, &display_h);

        int window_w, window_h;   // En Points (Pour ImGui / UI)
        glfwGetWindowSize(window, &window_w, &window_h);

        // ==========================================================
        // 2. BARRE SUPERIEURE (HEADER)
        // ==========================================================
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)window_w, (float)TOP_BAR_HEIGHT)); // Utilise window_w
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::Begin("Header", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(" X ", ImVec2(30, 25))) {
            glfwSetWindowShouldClose(window, true);
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        ImGui::SetCursorPosY(8);
        ImGui::Text("GPU STUDIO");

        ImGui::End();
        ImGui::PopStyleColor();

        // ==========================================================
        // 3. SIDEBAR DROITE (FILTRES)
        // ==========================================================
        // Correction ici : on utilise window_w pour positionner
        ImGui::SetNextWindowPos(ImVec2((float)window_w - SIDEBAR_WIDTH, (float)TOP_BAR_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2((float)SIDEBAR_WIDTH, (float)window_h - TOP_BAR_HEIGHT));

        ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "REGLAGES FILTRES");
        ImGui::Separator();
        ImGui::Spacing();

        if (editor.isLoaded) {
            // N&B
            ImGui::Checkbox("Noir & Blanc", &editor.enableGrayscale);
            if(editor.enableGrayscale) ImGui::SliderFloat("Intensite N&B", &editor.intensityGrayscale, 0.0f, 1.0f);
            ImGui::Spacing();
            // FLOU
            ImGui::Checkbox("Flou Gaussien", &editor.enableBlur);
            if(editor.enableBlur) ImGui::SliderFloat("Rayon Blur", &editor.intensityBlur, 0.0f, 1.0f);
            ImGui::Spacing();
            // MOSAIC
            ImGui::Checkbox("Mosaique", &editor.enableMosaic);
            if(editor.enableMosaic) ImGui::SliderFloat("Pixel Size", &editor.intensityMosaic, 0.0f, 1.0f);
            ImGui::Spacing();
            // EDGE
            ImGui::Checkbox("Contours", &editor.enableEdge);
            if(editor.enableEdge) ImGui::SliderFloat("Mix Edge", &editor.intensityEdge, 0.0f, 1.0f);
            ImGui::Spacing();
            // INVERT
            ImGui::Checkbox("Negatif", &editor.enableInvert);
            if(editor.enableInvert) ImGui::SliderFloat("Mix Negatif", &editor.intensityInvert, 0.0f, 1.0f);
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "ERREUR: Image non chargee");
        }
        ImGui::End();

        // ==========================================================
        // 4. RENDU OPENGL (CENTRE)
        // ==========================================================
        // On calcule le ratio Retina pour adapter le glViewport
        float retinaScale = (float)display_w / (float)window_w;

        // Zone de dessin : Tout l'écran moins la sidebar
        int viewWidth = (int)((window_w - SIDEBAR_WIDTH) * retinaScale);
        int viewHeight = display_h; // Toute la hauteur (le header est par dessus)

        glViewport(0, 0, viewWidth, viewHeight);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (editor.isLoaded) {
            glUseProgram(p);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, editor.textureID);

            glUniform1f(glGetUniformLocation(p, "u_width"), (float)editor.width);
            glUniform1f(glGetUniformLocation(p, "u_height"), (float)editor.height);
            glUniform1i(glGetUniformLocation(p, "u_enableGray"), editor.enableGrayscale);
            glUniform1f(glGetUniformLocation(p, "u_intGray"), editor.intensityGrayscale);
            glUniform1i(glGetUniformLocation(p, "u_enableBlur"), editor.enableBlur);
            glUniform1f(glGetUniformLocation(p, "u_intBlur"), editor.intensityBlur);
            glUniform1i(glGetUniformLocation(p, "u_enableMosaic"), editor.enableMosaic);
            glUniform1f(glGetUniformLocation(p, "u_intMosaic"), editor.intensityMosaic);
            glUniform1i(glGetUniformLocation(p, "u_enableEdge"), editor.enableEdge);
            glUniform1f(glGetUniformLocation(p, "u_intEdge"), editor.intensityEdge);
            glUniform1i(glGetUniformLocation(p, "u_enableInvert"), editor.enableInvert);
            glUniform1f(glGetUniformLocation(p, "u_intInvert"), editor.intensityInvert);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // ==========================================================
        // 5. RENDU GUI FINAL
        // ==========================================================
        // Rendu ImGui sur TOUTE la fenêtre (avec scaling correct)
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}