//
// Created by julis.wang on 2022/1/8.
//

#include "yuv_player.h"

void createEGL(JNIEnv *env, jobject surface);

void render(FILE *fp);


/**
 * 顶点着色器，每个顶点执行一次，可以并行执行
 *
 * attribute：一般用于各个顶点各不相同的量。如顶点位置、纹理坐标、法向量、颜色等等。
 * varying：表示易变量，一般用于顶点着色器传递到片段着色器的量。
 * vec2：包含了2个浮点数的向量
 * vec3：包含了3个浮点数的向量
 * vec4：包含了4个浮点数的向量
 *
 */
#define GET_STR(x) #x
static const char *vertexShader = GET_STR(
        attribute
        vec4 aPosition;//输入的顶点坐标，会在程序指定将数据输入到该字段
        attribute
        vec2 aTextCoord;//输入的纹理坐标，会在程序指定将数据输入到该字段
        varying
        vec2 vTextCoord;//输出的纹理坐标
        void main() {
            //这里其实是将上下翻转过来（因为安卓图片会自动上下翻转，所以转回来）
            vTextCoord = vec2(aTextCoord.x, 1.0 - aTextCoord.y);
            //直接把传入的坐标值作为传入渲染管线。gl_Position是OpenGL内置的
            gl_Position = aPosition;
        }
);

/**
 * 图元被光栅化为多少片段，就被调用多少次
 *
 * uniform：一般用于对于物体中所有顶点或者所有的片段都相同的量。比如光源位置、统一变换矩阵、颜色等。
 * sampler1D：1D纹理着色器
 * sampler2D：2D纹理着色器
 * sampler3D：3D纹理着色器
 */
static const char *fragYUV420P = GET_STR(
        precision
        mediump float;
        varying
        vec2 vTextCoord;

        //输入的yuv三个纹理
        uniform
        sampler2D yTexture;//采样器
        uniform
        sampler2D uTexture;//采样器
        uniform
        sampler2D vTexture;//采样器
        void main() {
            vec3 yuv;
            vec3 rgb;

            //分别取yuv各个分量的采样纹理

            //因为OpenGl只支持RGB的渲染，所以需要将vec3类型的 yuv通过公式转为一个rgb 的vec3 类型变量。
            //最后将rgb 变量构建一个vec4变量，作为最终颜色赋值给gl_FragColor 。
            yuv.x = texture2D(yTexture, vTextCoord).g;
            yuv.y = texture2D(uTexture, vTextCoord).g - 0.5;
            yuv.z = texture2D(vTexture, vTextCoord).g - 0.5;
            rgb = mat3(
                    1.0, 1.0, 1.0,
                    0.0, -0.39465, 2.03211,
                    1.13983, -0.5806, 0.0
            ) * yuv;
            //gl_FragColor是OpenGL内置的
            gl_FragColor = vec4(rgb, 1.0);
        }
);


GLint yuv_player::initShader(const char *source, int type) {
    //创建shader
    GLint sh = glCreateShader(type);
    if (sh == 0) {
        LOGD("glCreateShader %d failed", type);
        return 0;
    }
    //加载shader
    glShaderSource(sh,
                   1,//shader数量
                   &source,
                   0);//代码长度，传0则读到字符串结尾

    //编译shader
    glCompileShader(sh);

    GLint status;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        LOGD("glCompileShader %d failed", type);
        LOGD("source %s", source);
        return 0;
    }

    LOGD("glCompileShader %d success", type);
    return sh;
}


EGLDisplay display;
EGLSurface winSurface;

void createEGL(JNIEnv *env, jobject surface) {
    //1.获取原始窗口
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);

    //获取Display
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGD("egl display failed");
        return;
    }

    //2.初始化egl，后两个参数为主次版本号
    if (EGL_TRUE != eglInitialize(display, 0, 0)) {
        LOGD("eglInitialize failed");
        return;
    }

    //3.1 surface配置，可以理解为窗口
    EGLConfig eglConfig;
    EGLint configNum;
    EGLint configSpec[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    if (EGL_TRUE != eglChooseConfig(display, configSpec, &eglConfig, 1, &configNum)) {
        LOGD("eglChooseConfig failed");
        return;
    }

    //3.2创建surface(egl和NativeWindow进行关联。最后一个参数为属性信息，0表示默认版本)
    winSurface = eglCreateWindowSurface(display, eglConfig, native_window, 0);
    if (winSurface == EGL_NO_SURFACE) {
        LOGD("eglCreateWindowSurface failed");
        return;
    }

    //4 创建关联上下文
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    //EGL_NO_CONTEXT表示不需要多个设备共享上下文
    EGLContext context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        LOGD("eglCreateContext failed");
        return;
    }
    //将egl和opengl关联
    //两个surface一个读一个写。第二个一般用来离线渲染？
    if (EGL_TRUE != eglMakeCurrent(display, winSurface, winSurface, context)) {
        LOGD("eglMakeCurrent failed");
        return;
    }
}

void render(FILE *fp) {
    GLint vsh = yuv_player::initShader(vertexShader, GL_VERTEX_SHADER);
    GLint fsh = yuv_player::initShader(fragYUV420P, GL_FRAGMENT_SHADER);

    //创建渲染程序
    GLint program = glCreateProgram();
    if (program == 0) {
        LOGD("glCreateProgram failed");
        return;
    }

    //向渲染程序中加入着色器
    glAttachShader(program, vsh);
    glAttachShader(program, fsh);

    //链接程序
    glLinkProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == 0) {
        LOGD("glLinkProgram failed");
        return;
    }
    LOGD("glLinkProgram success");
    //激活渲染程序
    glUseProgram(program);
    //在glUseProgram函数调用之后，每个着色器调用和渲染调用都会使用这个程序对象（也就是之前写的着色器)了。

    //在把着色器对象链接到程序对象以后，删除着色器对象，我们不再需要它们了
    glDeleteShader(vsh);
    glDeleteShader(fsh);

    //加入三维顶点数据。这里就是整个屏幕的矩形。
    static float ver[] = {
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f
    };

    auto apos = static_cast<GLuint>(glGetAttribLocation(program, "aPosition"));
    glEnableVertexAttribArray(apos);
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 0, ver);

    //加入纹理坐标数据
    static float fragment[] = {
            1.0f, 0.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
    };
    auto aTex = static_cast<GLuint>(glGetAttribLocation(program, "aTextCoord"));
    glEnableVertexAttribArray(aTex);
    glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 0, fragment);

    int width = 176;
    int height = 144;

    //纹理初始化
    //设置纹理层对应的对应采样器？

    /**
     * 纹理对象、纹理目标、纹理单元
     *
     *   1.纹理对象是我们创建的用来存储纹理的显存，在实际使用过程中使用的是创建后返回的纹理ID。
     *   2.纹理目标可以简单理解为纹理的类型，比如指定是渲染2D还是3D等。
     *   3.纹理单元：纹理的操作容器，有GL_TEXTURE0、GL_TEXTURE1、GL_TEXTURE2等，纹理单元的数量是有限的，最多16个。
     *   所以在最多只能同时操作16个纹理。可以简单理解为第几层纹理。
     */

    //指定纹理变量在哪一层纹理单元渲染
    //对sampler变量，使用函数glUniform1i和glUniform1iv进行设置,
    glUniform1i(glGetUniformLocation(program, "yTexture"), 0);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 1);
    glUniform1i(glGetUniformLocation(program, "vTexture"), 2);
    //纹理ID
    GLuint texts[3] = {0};
    //创建若干个纹理对象，并且得到纹理ID
    glGenTextures(3, texts);

    //绑定纹理。后面的的设置和加载全部作用于当前绑定的纹理对象
    //GL_TEXTURE0、GL_TEXTURE1、GL_TEXTURE2 的就是纹理单元，GL_TEXTURE_1D、GL_TEXTURE_2D、CUBE_MAP为纹理目标
    //通过 glBindTexture 函数将纹理目标和纹理绑定后，对纹理目标所进行的操作都反映到对纹理上

    //第一层Y
    glBindTexture(GL_TEXTURE_2D, texts[0]);
    //缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //放大的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //设置纹理的格式和大小
    // 加载纹理到 OpenGL，读入 buffer 定义的位图数据，并把它复制到当前绑定的纹理对象
    // 当前绑定的纹理对象就会被附加上纹理图像。
    //width,height表示每几个像素公用一个yuv元素？比如width / 2表示横向每两个像素使用一个元素？
    glTexImage2D(GL_TEXTURE_2D,
                 0,//细节基本 默认0
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图（这里就是只取一个亮度的颜色通道的意思）
                 width,//加载的纹理宽度。最好为2的次幂(这里对y分量数据当做指定尺寸算，但显示尺寸会拉伸到全屏？)
                 height,//加载的纹理高度。最好为2的次幂
                 0,//纹理边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图
                 GL_UNSIGNED_BYTE,//像素点存储的数据类型
                 NULL //纹理的数据（先不传）
    );

    //绑定纹理
    //第一层U
    glBindTexture(GL_TEXTURE_2D, texts[1]);
    //缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,//细节基本 默认0
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图（这里就是只取一个颜色通道的意思）
                 width / 2,//u数据数量为屏幕的4分之1
                 height / 2,
                 0,//边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图
                 GL_UNSIGNED_BYTE,//像素点存储的数据类型
                 NULL //纹理的数据（先不传）
    );


    //绑定纹理
    //第三层V
    glBindTexture(GL_TEXTURE_2D, texts[2]);
    //缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,//细节基本 默认0
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图（这里就是只取一个颜色通道的意思）
                 width / 2,
                 height / 2,//v数据数量为屏幕的4分之1
                 0,//边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图
                 GL_UNSIGNED_BYTE,//像素点存储的数据类型
                 NULL //纹理的数据（先不传）
    );


    //从视频文件中读取yuv数据到内存中
    unsigned char *buf[3] = {0};
    buf[0] = new unsigned char[width * height];//y
    buf[1] = new unsigned char[width * height / 4];//u
    buf[2] = new unsigned char[width * height / 4];//v

    for (int index = 0; index < 10000; ++index) {
        //读一帧yuv420p数据
        if (feof(fp) == 0) {
            fread(buf[0], 1, width * height, fp);
            fread(buf[1], 1, width * height / 4, fp);
            fread(buf[2], 1, width * height / 4, fp);
        }

        //激活第一层纹理，绑定到创建的纹理
        //下面的width,height主要是显示尺寸？
        glActiveTexture(GL_TEXTURE0);
        //绑定y对应的纹理
        glBindTexture(GL_TEXTURE_2D, texts[0]);
        //替换纹理，比重新使用glTexImage2D性能高多
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0,//相对原来的纹理的offset
                        width, height,//加载的纹理宽度、高度。最好为2的次幂
                        GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        buf[0]);

        //激活第二层纹理，绑定到创建的纹理
        glActiveTexture(GL_TEXTURE1);
        //绑定u对应的纹理
        glBindTexture(GL_TEXTURE_2D, texts[1]);
        //替换纹理，比重新使用glTexImage2D性能高
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        buf[1]);

        //激活第三层纹理，绑定到创建的纹理
        glActiveTexture(GL_TEXTURE2);
        //绑定v对应的纹理
        glBindTexture(GL_TEXTURE_2D, texts[2]);
        //替换纹理，比重新使用glTexImage2D性能高
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        buf[2]);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //窗口显示，交换双缓冲区
        eglSwapBuffers(display, winSurface);
    }
}

void yuv_player::load_yuv(JNIEnv *env, jobject thiz, jstring jUrl, jobject surface) {
    const char *url = env->GetStringUTFChars(jUrl, 0);
    FILE *fp = fopen(url, "rb");
    if (!fp) {
        LOGD("Open file %s fail", url);
        return;
    }
    LOGD("Open url is %s", url);

    createEGL(env, surface);
    render(fp);
}

int yuv_player::run() {
    return 0;
}


