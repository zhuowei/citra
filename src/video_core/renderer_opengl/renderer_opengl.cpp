// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hw/gpu.h"
#include "core/mem_map.h"
#include "common/emu_window.h"
#include "video_core/pica.h"
#include "video_core/vertex_shader.h"
#include "video_core/vertex_shader_to_glsl.h"
#include "video_core/video_core.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/renderer_opengl/gl_shader_util.h"
#include "video_core/renderer_opengl/gl_shaders.h"
#include "video_core/debug_utils/debug_utils.h"

#include <algorithm>
//#include <dlfcn.h>
		    #include <GL/glx.h>

			#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte*)name)

/**
 * Vertex structure that the drawn screen rectangles are composed of.
 */
struct ScreenRectVertex {
    ScreenRectVertex(GLfloat x, GLfloat y, GLfloat u, GLfloat v) {
        position[0] = x;
        position[1] = y;
        tex_coord[0] = u;
        tex_coord[1] = v;
    }

    GLfloat position[2];
    GLfloat tex_coord[2];
};

/**
 * Defines a 1:1 pixel ortographic projection matrix with (0,0) on the top-left
 * corner and (width, height) on the lower-bottom.
 *
 * The projection part of the matrix is trivial, hence these operations are represented
 * by a 3x2 matrix.
 */
static std::array<GLfloat, 3*2> MakeOrthographicMatrix(const float width, const float height) {
    std::array<GLfloat, 3*2> matrix;

    matrix[0] = 2.f / width; matrix[2] = 0.f;           matrix[4] = -1.f;
    matrix[1] = 0.f;         matrix[3] = -2.f / height; matrix[5] = 1.f;
    // Last matrix row is implicitly assumed to be [0, 0, 1].

    return matrix;
}

/// RendererOpenGL constructor
RendererOpenGL::RendererOpenGL() {
    resolution_width  = std::max(VideoCore::kScreenTopWidth, VideoCore::kScreenBottomWidth);
    resolution_height = VideoCore::kScreenTopHeight + VideoCore::kScreenBottomHeight;
}

/// RendererOpenGL destructor
RendererOpenGL::~RendererOpenGL() {
}

/// Swap buffers (render frame)
void RendererOpenGL::SwapBuffers() {
    render_window->MakeCurrent();

    /*for(int i : {0, 1}) {
        const auto& framebuffer = GPU::g_regs.framebuffer_config[i];

        if (textures[i].width != (GLsizei)framebuffer.width ||
            textures[i].height != (GLsizei)framebuffer.height ||
            textures[i].format != framebuffer.color_format) {
            // Reallocate texture if the framebuffer size has changed.
            // This is expected to not happen very often and hence should not be a
            // performance problem.
            ConfigureFramebufferTexture(textures[i], framebuffer);
        }

        LoadFBToActiveGLTexture(GPU::g_regs.framebuffer_config[i], textures[i]);
    }*/

    DrawScreens();

    // Swap buffers
    render_window->PollEvents();
    glFlush();
    render_window->SwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * Loads framebuffer from emulated memory into the active OpenGL texture.
 */
void RendererOpenGL::LoadFBToActiveGLTexture(const GPU::Regs::FramebufferConfig& framebuffer,
                                             const TextureInfo& texture) {

    const VAddr framebuffer_vaddr = Memory::PhysicalToVirtualAddress(
        framebuffer.active_fb == 0 ? framebuffer.address_left1 : framebuffer.address_left2);

    LOG_TRACE(Render_OpenGL, "0x%08x bytes from 0x%08x(%dx%d), fmt %x",
        framebuffer.stride * framebuffer.height,
        framebuffer_vaddr, (int)framebuffer.width,
        (int)framebuffer.height, (int)framebuffer.format);

    const u8* framebuffer_data = Memory::GetPointer(framebuffer_vaddr);

    int bpp = GPU::Regs::BytesPerPixel(framebuffer.color_format);
    size_t pixel_stride = framebuffer.stride / bpp;

    // OpenGL only supports specifying a stride in units of pixels, not bytes, unfortunately
    ASSERT(pixel_stride * bpp == framebuffer.stride);

    // Ensure no bad interactions with GL_UNPACK_ALIGNMENT, which by default
    // only allows rows to have a memory alignement of 4.
    ASSERT(pixel_stride % 4 == 0);

    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)pixel_stride);

    // Update existing texture
    // TODO: Test what happens on hardware when you change the framebuffer dimensions so that they
    //       differ from the LCD resolution.
    // TODO: Applications could theoretically crash Citra here by specifying too large
    //       framebuffer sizes. We should make sure that this cannot happen.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer.width, framebuffer.height,
        texture.gl_format, texture.gl_type, framebuffer_data);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}
static void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity,
GLsizei length, const char* message, void *userParam)
{

printf("%s\n", message);

}
extern void glDebugMessageCallback(void*, void*);
/**
 * Initializes the OpenGL state and creates persistent objects.
 */
static bool openglInit = false;
void RendererOpenGL::InitOpenGLObjects() {
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glViewport(0,0,640,480);
    void (*glDebugMessageCallback)(void*, void*) = (void (*)(void*, void*)) IntGetProcAddress("glDebugMessageCallback");
    glDebugMessageCallback((void*)&on_gl_error, nullptr);

    // Link shaders and get variable locations
    /*program_id = ShaderUtil::LoadShaders(GLShaders::g_vertex_shader, GLShaders::g_fragment_shader);
    uniform_modelview_matrix = glGetUniformLocation(program_id, "modelview_matrix");
    uniform_color_texture = glGetUniformLocation(program_id, "color_texture");
    attrib_position = glGetAttribLocation(program_id, "vert_position");
    attrib_tex_coord = glGetAttribLocation(program_id, "vert_tex_coord");

    // Generate VBO handle for drawing
    glGenBuffers(1, &vertex_buffer_handle);

    // Generate VAO
    glGenVertexArrays(1, &vertex_array_handle);
    glBindVertexArray(vertex_array_handle);

    // Attach vertex data to VAO
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenRectVertex) * 4, nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(attrib_position,  2, GL_FLOAT, GL_FALSE, sizeof(ScreenRectVertex), (GLvoid*)offsetof(ScreenRectVertex, position));
    glVertexAttribPointer(attrib_tex_coord, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenRectVertex), (GLvoid*)offsetof(ScreenRectVertex, tex_coord));
    glEnableVertexAttribArray(attrib_position);
    glEnableVertexAttribArray(attrib_tex_coord);

    // Allocate textures for each screen
    for (auto& texture : textures) {
        glGenTextures(1, &texture.handle);

        // Allocation of storage is deferred until the first frame, when we
        // know the framebuffer size.

        glBindTexture(GL_TEXTURE_2D, texture.handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);*/
    openglInit = true;
}

void RendererOpenGL::ConfigureFramebufferTexture(TextureInfo& texture,
                                                 const GPU::Regs::FramebufferConfig& framebuffer) {
    GPU::Regs::PixelFormat format = framebuffer.color_format;
    GLint internal_format;

    texture.format = format;
    texture.width = framebuffer.width;
    texture.height = framebuffer.height;

    switch (format) {
    case GPU::Regs::PixelFormat::RGBA8:
        internal_format = GL_RGBA;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_INT_8_8_8_8;
        break;

    case GPU::Regs::PixelFormat::RGB8:
        // This pixel format uses BGR since GL_UNSIGNED_BYTE specifies byte-order, unlike every
        // specific OpenGL type used in this function using native-endian (that is, little-endian
        // mostly everywhere) for words or half-words.
        // TODO: check how those behave on big-endian processors.
        internal_format = GL_RGB;
        texture.gl_format = GL_BGR;
        texture.gl_type = GL_UNSIGNED_BYTE;
        break;

    case GPU::Regs::PixelFormat::RGB565:
        internal_format = GL_RGB;
        texture.gl_format = GL_RGB;
        texture.gl_type = GL_UNSIGNED_SHORT_5_6_5;
        break;

    case GPU::Regs::PixelFormat::RGB5A1:
        internal_format = GL_RGBA;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_SHORT_5_5_5_1;
        break;

    case GPU::Regs::PixelFormat::RGBA4:
        internal_format = GL_RGBA;
        texture.gl_format = GL_RGBA;
        texture.gl_type = GL_UNSIGNED_SHORT_4_4_4_4;
        break;

    default:
        UNIMPLEMENTED();
    }

    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture.width, texture.height, 0,
            texture.gl_format, texture.gl_type, nullptr);
}

/**
 * Draws a single texture to the emulator window, rotating the texture to correct for the 3DS's LCD rotation.
 */
void RendererOpenGL::DrawSingleScreenRotated(const TextureInfo& texture, float x, float y, float w, float h) {
    /*std::array<ScreenRectVertex, 4> vertices = {
        ScreenRectVertex(x,   y,   1.f, 0.f),
        ScreenRectVertex(x+w, y,   1.f, 1.f),
        ScreenRectVertex(x,   y+h, 0.f, 0.f),
        ScreenRectVertex(x+w, y+h, 0.f, 1.f),
    };

    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
    glBindVertexArray(vertex_array_handle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);*/
}

/**
 * Draws the emulated screens to the emulator window.
 */
void RendererOpenGL::DrawScreens() {
    auto viewport_extent = GetViewportExtent();
    //glViewport(viewport_extent.left, viewport_extent.top, viewport_extent.GetWidth(), viewport_extent.GetHeight()); // TODO: Or bottom?
    //glClear(GL_COLOR_BUFFER_BIT);

    //glUseProgram(program_id);

    // Set projection matrix
    /*std::array<GLfloat, 3*2> ortho_matrix = MakeOrthographicMatrix((float)resolution_width, (float)resolution_height);
    glUniformMatrix3x2fv(uniform_modelview_matrix, 1, GL_FALSE, ortho_matrix.data());

    // Bind texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uniform_color_texture, 0);

    const float max_width = std::max((float)VideoCore::kScreenTopWidth, (float)VideoCore::kScreenBottomWidth);
    const float top_x = 0.5f * (max_width - VideoCore::kScreenTopWidth);
    const float bottom_x = 0.5f * (max_width - VideoCore::kScreenBottomWidth);

    //DrawSingleScreenRotated(textures[0], top_x, 0,
    //    (float)VideoCore::kScreenTopWidth, (float)VideoCore::kScreenTopHeight);
    DrawSingleScreenRotated(textures[1], bottom_x, (float)VideoCore::kScreenTopHeight,
        (float)VideoCore::kScreenBottomWidth, (float)VideoCore::kScreenBottomHeight);*/

    m_current_frame++;
}

/// Updates the framerate
void RendererOpenGL::UpdateFramerate() {
}

/**
 * Set the emulator window to use for renderer
 * @param window EmuWindow handle to emulator window to use for rendering
 */
void RendererOpenGL::SetWindow(EmuWindow* window) {
    render_window = window;
}

MathUtil::Rectangle<unsigned> RendererOpenGL::GetViewportExtent() {
    unsigned framebuffer_width;
    unsigned framebuffer_height;
    std::tie(framebuffer_width, framebuffer_height) = render_window->GetFramebufferSize();

    float window_aspect_ratio = static_cast<float>(framebuffer_height) / framebuffer_width;
    float emulation_aspect_ratio = static_cast<float>(resolution_height) / resolution_width;

    MathUtil::Rectangle<unsigned> viewport_extent;
    if (window_aspect_ratio > emulation_aspect_ratio) {
        // Window is narrower than the emulation content => apply borders to the top and bottom
        unsigned viewport_height = static_cast<unsigned>(std::round(emulation_aspect_ratio * framebuffer_width));
        viewport_extent.left = 0;
        viewport_extent.top = (framebuffer_height - viewport_height) / 2;
        viewport_extent.right = viewport_extent.left + framebuffer_width;
        viewport_extent.bottom = viewport_extent.top + viewport_height;
    } else {
        // Otherwise, apply borders to the left and right sides of the window.
        unsigned viewport_width = static_cast<unsigned>(std::round(framebuffer_height / emulation_aspect_ratio));
        viewport_extent.left = (framebuffer_width - viewport_width) / 2;
        viewport_extent.top = 0;
        viewport_extent.right = viewport_extent.left + viewport_width;
        viewport_extent.bottom = viewport_extent.top + framebuffer_height;
    }

    return viewport_extent;
}

static GLenum picaToGLTopology(Pica::Regs::TriangleTopology topology) {
    using Pica::Regs;
    switch (topology) {
        case Regs::TriangleTopology::List:
        case Regs::TriangleTopology::ListIndexed:
            return GL_TRIANGLES;
        case Regs::TriangleTopology::Strip:
            return GL_TRIANGLE_STRIP;
        case Regs::TriangleTopology::Fan:
            return GL_TRIANGLE_FAN;
        default:
            LOG_ERROR(HW_GPU, "Unknown triangle topology %x:", (int)topology);
            break;
    }
}

static GLenum picaToGLFormat(int format) {
    using Pica::Regs;
/*
        enum class Format : u64 {
            BYTE = 0,
            UBYTE = 1,
            SHORT = 2,
            FLOAT = 3,
        };
*/
    switch (format) {
        case 0: //Regs::Format::BYTE:
            return GL_BYTE;
        case 1: //Regs::Format::UBYTE:
            return GL_UNSIGNED_BYTE;
        case 2: //Regs::Format::SHORT:
            return GL_SHORT;
        case 3: //Regs::Format::FLOAT:
            return GL_FLOAT;
        default:
            return GL_FLOAT; // not reached.
    }
}

static GLenum picaToGLTextureFormat(Pica::Regs::TextureFormat format, GLenum* type) {
	using TextureFormat = Pica::Regs::TextureFormat;
	// yes, I know this duplicates code from the framebuffer format config
	switch (format) {
		case TextureFormat::RGBA8:
			*type = GL_UNSIGNED_BYTE;
			return GL_RGBA;
		case TextureFormat::RGB8:
			*type = GL_UNSIGNED_BYTE;
			return GL_RGB;
		case TextureFormat::RGBA5551:
			*type = GL_UNSIGNED_SHORT_5_5_5_1;
			return GL_RGBA;
		case TextureFormat::RGBA4:
			*type = GL_UNSIGNED_SHORT_4_4_4_4;
			return GL_RGBA;
		default:
			LOG_ERROR(Render_OpenGL, "Unsupported texture format: %d", format);
			*type = GL_UNSIGNED_BYTE;
			return GL_RGBA;
	}
}

static bool checkGL(int line) {
    GLenum error;
    bool once = false;
    while ((error = glGetError())) {
        LOG_ERROR(Render_OpenGL, "GL Error: %x line %d", error, line);
        once = true;
    }
    return once;
}

void RendererOpenGL::handleVirtualGPUDraw(u32 vertex_attribute_sources[16],
            u32 vertex_attribute_strides[16],
            u32 vertex_attribute_formats[16],
            u32 vertex_attribute_elements[16],
            u32 vertex_attribute_element_size[16],
            bool is_indexed) {
    static GLint shader = -1;
    static GLuint vertexAttribIndex[8];
    static GLuint vertexBuffers[8];
    static GLuint vertexArrays[8];
    static GLuint shaderUniforms[96];
    static GLuint shaderTextureUniforms[3];
    static GLuint shaderTextures[3];
    static bool hasInit = false;
static GLuint tbo;
static bool texed = false;
    using namespace Pica;

    if (!openglInit) return;

    VideoCore::g_emu_window->MakeCurrent();

    while(glGetError()) {};

    if (!hasInit) {
        glGenBuffers(8, vertexBuffers);
        if (checkGL(__LINE__)) exit(1);
        if (vertexBuffers[0] == 0) exit(1);
        glGenVertexArrays(8, vertexArrays);
        if (checkGL(__LINE__)) exit(1);
#ifdef OPENGL_ACCEL_DEBUG
        for (int i = 0; i < 8; i++) {
            printf("Buffers: %d %d %d\n", i, vertexBuffers[i], vertexArrays[i]);
        }
#endif

glGenBuffers(1, &tbo);
glBindBuffer(GL_ARRAY_BUFFER, tbo);
glBufferData(GL_ARRAY_BUFFER, 0x100000, nullptr, GL_STATIC_READ);
glGenTextures(3, shaderTextures);
for (int i = 0; i < 3; i++) {
        glBindTexture(GL_TEXTURE_2D, shaderTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
        glBindTexture(GL_TEXTURE_2D, 0);
        hasInit = true;
    }

    if (VertexShader::shader_changed) {
        shader = VertexShaderToGLSL::CompileGlsl(VertexShader::GetShaderBinary(), VertexShader::GetSwizzlePatterns());
        checkGL(__LINE__);
        char attribName[16];
        for (int i = 0; i <= 7; i++) {
            snprintf(attribName, sizeof(attribName), "v%d", i);
            vertexAttribIndex[i] = glGetAttribLocation(shader, attribName);
            checkGL(__LINE__);
            printf("Attrib index: %s %d\n", attribName, vertexAttribIndex[i]);
        }
        for (int i = 0; i <= 95; i++) {
            snprintf(attribName, sizeof(attribName), "c%d", i);
            shaderUniforms[i] = glGetUniformLocation(shader, attribName);
            printf("Shader uniform: %s %d\n", attribName, shaderUniforms[i]);
            checkGL(__LINE__);
        }
	for (int i = 0; i < 3; i++) {
		snprintf(attribName, sizeof(attribName), "tex%d", i);
		shaderTextureUniforms[i] = glGetUniformLocation(shader, attribName);
            printf("Shader texture uniform: %s %d\n", attribName, shaderTextureUniforms[i]);
	}
        checkGL(__LINE__);
        VertexShader::shader_changed = false;
    }
    glUseProgram(shader);
    checkGL(__LINE__);
#ifdef OPENGL_ACCEL_DEBUG
    printf("Number of vertices: %d\n", registers.num_vertices);
#endif
    auto& attribute_config = registers.vertex_attributes;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBindVertexArray(vertexArrays[0]);
    size_t bufferPtr = 0;
    for (int i = 0; i < attribute_config.GetNumTotalAttributes(); i++) {
        if (vertexAttribIndex[i] == -1) continue;
        //glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[i]);
        checkGL(__LINE__);
        //glBufferData(GL_ARRAY_BUFFER, registers.num_vertices * vertex_attribute_strides[i], ptr, GL_STREAM_DRAW);
        checkGL(__LINE__);
        checkGL(__LINE__);
#ifdef OPENGL_ACCEL_DEBUG
printf("Elements %d Stride %d\n", vertex_attribute_elements[i], vertex_attribute_strides[i]);
#endif
        glVertexAttribPointer(vertexAttribIndex[i], vertex_attribute_elements[i], picaToGLFormat(vertex_attribute_formats[i]),
            GL_FALSE, vertex_attribute_strides[i], (void*) bufferPtr);
        bufferPtr += registers.num_vertices * vertex_attribute_strides[i];
        checkGL(__LINE__);
        glEnableVertexAttribArray(vertexAttribIndex[i]);
        checkGL(__LINE__);
	//printf("bound %d first value %f\n", i, ((float*)ptr)[0]);
    }
    glBufferData(GL_ARRAY_BUFFER, bufferPtr, nullptr, GL_STREAM_DRAW);
    bufferPtr = 0;
    for (int i = 0; i < attribute_config.GetNumTotalAttributes(); i++) {
        if (vertexAttribIndex[i] == -1) continue;
        void* ptr = Memory::GetPointer(PAddrToVAddr(vertex_attribute_sources[i]));
        size_t bufferSize = registers.num_vertices * vertex_attribute_strides[i];
        glBufferSubData(GL_ARRAY_BUFFER, bufferPtr, bufferSize, ptr);
        bufferPtr += bufferSize;
#ifdef OPENGL_ACCEL_DEBUG
	float* asdf = (float*) ptr;
for (int i = 0; i < 40; i+=vertex_attribute_strides[i]/4 != 0? vertex_attribute_strides[i]/4: 4) {
	printf("Bound: %f %f %f %f\n", asdf[i], asdf[i+1], asdf[i+2], asdf[i+3]);
}
#endif
    }
    for (int i = 0; i < 96; i++) { // todo: find out which uniforms are actually used
        if (shaderUniforms[i] == -1) continue;
        auto& uniform = VertexShader::GetFloatUniform(i);
#ifdef OPENGL_ACCEL_DEBUG
	printf("Uniform %d: %f, %f, %f, %f\n", i, uniform.x.ToFloat32(), uniform.y.ToFloat32(), uniform.z.ToFloat32(), uniform.w.ToFloat32());
#endif
        glUniform4f(shaderUniforms[i], uniform.x.ToFloat32(), uniform.y.ToFloat32(), uniform.z.ToFloat32(), uniform.w.ToFloat32());
        checkGL(__LINE__);
    }
    auto textures = registers.GetTextures();
    for (int i = 0; i < 3; i++) {
        if (shaderTextureUniforms[i] == -1) continue;
        glUniform1i(shaderTextureUniforms[i], i);
        checkGL(__LINE__);
        glActiveTexture(GL_TEXTURE0 + i);
        checkGL(__LINE__);
        glBindTexture(GL_TEXTURE_2D, shaderTextures[i]);
        checkGL(__LINE__);
	//GLenum type;
	//GLenum format = picaToGLTextureFormat(textures[i].format, &type);
	void* pointer = Memory::GetPointer(PAddrToVAddr(textures[i].config.GetPhysicalAddress()));
	//printf("Pointer for tex%d: %p %x %x %x\n", i, pointer, ((int*)pointer)[0], ((int*)pointer)[1], ((int*)pointer)[2]);
        checkGL(__LINE__);
if (!texed) {
	Math::Vec4<u8> buffer[textures[i].config.width * textures[i].config.height];
	u8* source = (u8*) pointer;
	int index = 0;
	DebugUtils::TextureInfo info = DebugUtils::TextureInfo::FromPicaRegister(textures[i].config, textures[i].format);
	for (int y = textures[i].config.height - 1; y >= 0; --y) {
		for (int x = 0; x < textures[i].config.width; ++x) {
			buffer[index++] = DebugUtils::LookupTexture(source, x, y, info, false);
		}
	}
        //glTexImage2D(GL_TEXTURE_2D, 0, format, textures[i].config.width, textures[i].config.height, 0, format, type, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i].config.width, textures[i].config.height,
		0, GL_RGBA, GL_UNSIGNED_BYTE, (void*) buffer);
texed = true;
}
        checkGL(__LINE__);
    }
glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
#ifdef OPENGL_ACCEL_DEBUG
glBeginTransformFeedback(GL_TRIANGLES);
#endif
    glDrawArrays(picaToGLTopology(registers.triangle_topology.Value()), 0, registers.num_vertices);
#ifdef OPENGL_ACCEL_DEBUG
glEndTransformFeedback();
#endif
#if 0
glFlush();
GLfloat feedback[400];
glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(feedback), feedback);
for (int i = 0; i < 4*registers.num_vertices; i+=4) {
	printf("Feedback: %f %f %f %f\n", feedback[i], feedback[i+1], feedback[i+2], feedback[i+3]);
}
#endif
    checkGL(__LINE__);
#ifdef OPENGL_ACCEL_DEBUG
    LOG_INFO(Render_OpenGL, "Frame!");
#endif
}

/// Initialize the renderer
void RendererOpenGL::Init() {
    render_window->MakeCurrent();

    int err = ogl_LoadFunctions();
    if (ogl_LOAD_SUCCEEDED != err) {
        LOG_CRITICAL(Render_OpenGL, "Failed to initialize GL functions! Exiting...");
        exit(-1);
    }

    LOG_INFO(Render_OpenGL, "GL_VERSION: %s", glGetString(GL_VERSION));
    InitOpenGLObjects();
}

/// Shutdown the renderer
void RendererOpenGL::ShutDown() {
}
