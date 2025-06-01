#include <jni.h>
#include <stdlib.h>

#define LOG_TAG "c2bsh"
#ifndef LOGE
#define LOGE(...) printf(__VA_ARGS__)
#endif

#ifndef JNI_FUNC_SIGN_MAIN
#define JNI_FUNC_SIGN_MAIN Java_com_hack_Hack_showWindow
#endif

// The Java Method to create chunks
#ifndef JNI_FUNC_SIGN_GERAR_CHUNK
#define JNI_FUNC_SIGN_GERAR_CHUNK Java_com_hack_Hack_gerarChunk
#endif

#define jbool jboolean
#define true JNI_FALSE
#define false JNI_TRUE

typedef struct Window Window;
typedef struct ViewList ViewList;

// View types
typedef struct View View;
typedef View TextView;
typedef View LinearLayout;

// The hack data
typedef struct Hack Hack;

// Base View Data
struct View {
  jclass class;    // the android view java class reference
  jobject object;  // the android view java class instace
  jmethodID constructor;
  View* parent;
};
// View List data

struct ViewList {
  View** array;
  int count;     // the current views amount
  int capacity;  // the max capacity for views in memory, used for dynamic
                 // allocation
};

// Window
struct Window {
  jobject manager;  // the android java window manager
  jobject
      layoutParams;   // the window manager layout params, used to add root view
  ViewList viewList;  // the view list, see struct ViewList above
};

// App Data
struct Hack {
  JavaVM* jvm;      // the java vm
  Window window;    // the window, see above
  jobject context;  // android activity context, needed for views constructor
};

static Hack* hack = {0};

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  hack = (Hack*)malloc(sizeof(Hack));
  if (!hack) {
    LOGE("Failed to allocate memory for Hack*");
    return JNI_ERR;
  }
  hack->jvm = vm;
  return JNI_VERSION_1_6;
}

static JNIEnv* JNI_GetEnv() {
  JavaVM* jvm = hack->jvm;
  JNIEnv* env = NULL;
  if ((*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) != JNI_OK) {
      return NULL;
    }
  }
  return env;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
  JNIEnv* env = JNI_GetEnv();
  if (hack != NULL) {
    if (hack->window.viewList.array != NULL) {
      for (int i = 0; i < hack->window.viewList.count; ++i) {
        View* view = hack->window.viewList.array[i];
        if (view != NULL) {
          (*env)->DeleteGlobalRef(env, view->class);
          (*env)->DeleteGlobalRef(env, view->object);
          free(view);
          view = NULL;
        }
      }
      free(hack->window.viewList.array);
      hack->window.viewList.array = NULL;
    }

    (*env)->DeleteGlobalRef(env, hack->context);
    (*env)->DeleteGlobalRef(env, hack->window.manager);

    free(hack);
    hack = NULL;
  }
}

static TextView* TextView_Create(View* parent, const char* text) {
  JNIEnv* env = JNI_GetEnv();
  if (env == NULL) {
    LOGE("Failed to get JNIEnv*");
    return NULL;
  }

  TextView* textView = malloc(sizeof(TextView));
  textView->parent = parent;

  jclass localClass = (*env)->FindClass(env, "android/widget/TextView");
  textView->class = (*env)->NewGlobalRef(env, localClass);
  (*env)->DeleteLocalRef(env, localClass);

  textView->constructor = (*env)->GetMethodID(env, textView->class, "<init>",
                                              "(Landroid/content/Context;)V");
  jobject localObject = (*env)->NewObject(env, textView->class,
                                          textView->constructor, hack->context);
  textView->object = (*env)->NewGlobalRef(env, localObject);
  (*env)->DeleteLocalRef(env, localObject);

  jmethodID setText = (*env)->GetMethodID(env, textView->class, "setText",
                                          "(Ljava/lang/CharSequence;)V");
  if (setText == NULL) {
    LOGE("setText method not found");
    return textView;
  }
  jstring jtext = (*env)->NewStringUTF(env, text);
  (*env)->CallVoidMethod(env, textView->object, setText, jtext);
  return textView;
}

static void TextView_SetTextSize(TextView* textView, float size) {
  JNIEnv* env = JNI_GetEnv();
  if (env == NULL) {
    LOGE("Failed to get JNIEnv*");
    return;
  }

  jmethodID setTextSize =
      (*env)->GetMethodID(env, textView->class, "setTextSize", "(F)V");
  if (setTextSize == NULL) {
    LOGE("setTextSize method not found");
    return;
  }
  (*env)->CallVoidMethod(env, textView->object, setTextSize, size);
}

static LinearLayout* LinearLayout_Create(View* parent, int background) {
  JNIEnv* env = JNI_GetEnv();
  if (env == NULL) {
    LOGE("Failed to get JNIEnv*");
    return NULL;
  }

  LinearLayout* linearLayout = malloc(sizeof(LinearLayout));
  linearLayout->parent = parent;

  jclass localClass = (*env)->FindClass(env, "android/widget/LinearLayout");
  linearLayout->class = (*env)->NewGlobalRef(env, localClass);
  (*env)->DeleteLocalRef(env, localClass);

  linearLayout->constructor = (*env)->GetMethodID(
      env, linearLayout->class, "<init>", "(Landroid/content/Context;)V");

  jobject localObject = (*env)->NewObject(
      env, linearLayout->class, linearLayout->constructor, hack->context);
  linearLayout->object = (*env)->NewGlobalRef(env, localObject);
  (*env)->DeleteLocalRef(env, localObject);

  jmethodID setBackgroundColor = (*env)->GetMethodID(
      env, linearLayout->class, "setBackgroundColor", "(I)V");
  if (setBackgroundColor == NULL) {
    LOGE("setBackgroundColor method not found");
    return linearLayout;
  }
  (*env)->CallVoidMethod(env, linearLayout->object, setBackgroundColor,
                         background);
  return linearLayout;
}

// Initialize Window ViewList
// Allocates memory for just 1
static void Window_ViewList_Init() {
  hack->window.viewList.count = 0;
  hack->window.viewList.capacity = 1;
  hack->window.viewList.array =
      (View**)malloc(sizeof(View*) * hack->window.viewList.capacity);
}

// Adds new View in Window ViewList
// Reallocates needed memory
static void Window_ViewList_Add(View* view) {
  if (view == NULL) {
    LOGE("Cant add null view in Window");
    return;
  }
  if (hack->window.viewList.count >= hack->window.viewList.capacity) {
    hack->window.viewList.capacity = hack->window.viewList.count + 1;
    hack->window.viewList.array =
        (View**)realloc(hack->window.viewList.array,
                        sizeof(View*) * hack->window.viewList.count);
  }
  hack->window.viewList.array[hack->window.viewList.count++] = view;
}

static void Window_ViewList_Draw() {
  JNIEnv* env = JNI_GetEnv();
  if (env == NULL) {
    LOGE("Failed to get JNIEnv*");
    return;
  }

  jclass wmClass = (*env)->GetObjectClass(env, hack->window.manager);
  jmethodID addView = (*env)->GetMethodID(
      env, wmClass, "addView",
      "(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V");

  for (int i = 0; i < hack->window.viewList.count; ++i) {
    View* view = hack->window.viewList.array[i];
    if (view->parent == NULL) {
      (*env)->CallVoidMethod(env, hack->window.manager, addView, view->object,
                             hack->window.layoutParams);
    } else {
      jmethodID addViewSingle = (*env)->GetMethodID(
          env, view->parent->class, "addView", "(Landroid/view/View;)V");
      (*env)->CallVoidMethod(env, view->parent->object, addViewSingle,
                             view->object);
    }
  }
}

JNIEXPORT void JNICALL JNI_FUNC_SIGN_MAIN(JNIEnv* env,
                                          jclass clazz,
                                          jobject context) {
  //////////////////////// GET NEEDED CLASSES ////////////////////////
  jclass contextClass = (*env)->GetObjectClass(env, context);
  jmethodID getSystemService =
      (*env)->GetMethodID(env, contextClass, "getSystemService",
                          "(Ljava/lang/String;)Ljava/lang/Object;");

  hack->context = (*env)->NewGlobalRef(env, context);
  jstring service = (*env)->NewStringUTF(env, "window");
  jobject manager =
      (*env)->CallObjectMethod(env, context, getSystemService, service);
  hack->window.manager = (*env)->NewGlobalRef(env, manager);

  /////////////////////////////////////////////////////////////

  //////////////////////// BEGIN LAYOUT PARAMS ////////////////////////
  jclass layoutParamsClass =
      (*env)->FindClass(env, "android/view/WindowManager$LayoutParams");
  jmethodID constructor =
      (*env)->GetMethodID(env, layoutParamsClass, "<init>", "(IIIII)V");

  jfieldID typeField =
      (*env)->GetStaticFieldID(env, layoutParamsClass, "TYPE_APPLICATION", "I");
  jint type = (*env)->GetStaticIntField(env, layoutParamsClass, typeField);

  jfieldID wrapContentField =
      (*env)->GetStaticFieldID(env, layoutParamsClass, "WRAP_CONTENT", "I");
  jint WRAP_CONTENT =
      (*env)->GetStaticIntField(env, layoutParamsClass, wrapContentField);

  jfieldID flagField = (*env)->GetStaticFieldID(env, layoutParamsClass,
                                                "FLAG_NOT_FOCUSABLE", "I");
  jint FLAG_NOT_FOCUSABLE =
      (*env)->GetStaticIntField(env, layoutParamsClass, flagField);

  jclass pixelFormatClass =
      (*env)->FindClass(env, "android/graphics/PixelFormat");
  jfieldID typeField5 =
      (*env)->GetStaticFieldID(env, pixelFormatClass, "TRANSLUCENT", "I");
  jint format = (*env)->GetStaticIntField(env, pixelFormatClass, typeField5);

  int width = WRAP_CONTENT;
  int height = WRAP_CONTENT;
  int flags = FLAG_NOT_FOCUSABLE;

  hack->window.layoutParams = (*env)->NewObject(
      env, layoutParamsClass, constructor, width, height, type, flags, format);
  //////////////////////// END LAYOUT PARAMS ////////////////////////

  //////////////////////// BEGIN WINDOW //////////////////////

  Window_ViewList_Init();

  Window_ViewList_Add(LinearLayout_Create(NULL, 0xFffF0000));
  Window_ViewList_Add(
      TextView_Create(hack->window.viewList.array[0], "Hello world"));

  TextView* aim = TextView_Create(NULL, "±");
  TextView_SetTextSize(aim, 25.0f);
  Window_ViewList_Add(aim);

  //////////////////////// END WINDOW //////////////////////

  //////////////////////// BEGIN DRAW VIEWS //////////////////////
  Window_ViewList_Draw();
  //////////////////////// END DRAW VIEWS //////////////////////
}

static float Math_Lerp(float t, float a, float b) {
  return a + t * (b - a);
}

static float Math_Floor(double x) {
  int i = (int)x;
  return (x < 0 && x != i) ? i - 1 : i;
}

static float Math_Fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

static const int permutacao[] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
    225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
    6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
    35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
    171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
    231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
    245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
    58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
    154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
    19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
    84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
    180};

static const int PerlinNoise2D_GRADIENTS[8][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

static const int PerlinNoise3D_GRADIENTS[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

static float PerlinNoise2D_Math_Grad(int hash, float x, float y) {
  int h = hash & 7;
  const int* g = PerlinNoise2D_GRADIENTS[h];
  return g[0] * x + g[1] * y;
}

static float PerlinNoise3D_Math_Grad(int hash, float x, float y, float z) {
  int h = hash & 15;
  const int* g = PerlinNoise3D_GRADIENTS[h % 12];
  return g[0] * x + g[1] * y + g[2] * z;
}

static int PerlinNoise2D_p[512];
static int PerlinNoise3D_p[512];

static void PerlinNoise_InitPermutationTables() {
  for (int i = 0; i < 256; i++) {
    PerlinNoise2D_p[i] = permutacao[i];
    PerlinNoise2D_p[256 + i] = permutacao[i];
    PerlinNoise3D_p[i] = permutacao[i];
    PerlinNoise3D_p[256 + i] = permutacao[i];
  }
}

static float PerlinNoise2D_ruido(int x, int z, int seed) {
  int X = ((int)Math_Floor(x) + seed) & 255;
  int Z = ((int)Math_Floor(z) + seed) & 255;
  float xf = x - (int)Math_Floor(x);
  float zf = z - (int)Math_Floor(z);

  float u = Math_Fade(xf);
  float v = Math_Fade(zf);

  int A = PerlinNoise2D_p[X] + Z;
  int B = PerlinNoise2D_p[X + 1] + Z;

  float gradAA = PerlinNoise2D_Math_Grad(PerlinNoise2D_p[A], xf, zf);
  float gradBA = PerlinNoise2D_Math_Grad(PerlinNoise2D_p[B], xf - 1, zf);
  float gradAB = PerlinNoise2D_Math_Grad(PerlinNoise2D_p[A + 1], xf, zf - 1);
  float gradBB =
      PerlinNoise2D_Math_Grad(PerlinNoise2D_p[B + 1], xf - 1, zf - 1);

  float lerpX1 = Math_Lerp(u, gradAA, gradBA);
  float lerpX2 = Math_Lerp(u, gradAB, gradBB);
  return Math_Lerp(v, lerpX1, lerpX2);
}

static float PerlinNoise3D_ruido(int x, int y, int z, int seed) {
  int X = ((int)Math_Floor(x) + seed) & 255;
  int Y = ((int)Math_Floor(y) + seed) & 255;
  int Z = ((int)Math_Floor(z) + seed) & 255;
  float xf = x - (int)Math_Floor(x);
  float yf = y - (int)Math_Floor(y);
  float zf = z - (int)Math_Floor(z);

  float u = Math_Fade(xf);
  float v = Math_Fade(yf);
  float w = Math_Fade(zf);

  int A = PerlinNoise3D_p[X] + Y;
  int AA = PerlinNoise3D_p[A] + Z;
  int AB = PerlinNoise3D_p[A + 1] + Z;
  int B = PerlinNoise3D_p[X + 1] + Y;
  int BA = PerlinNoise3D_p[B] + Z;
  int BB = PerlinNoise3D_p[B + 1] + Z;

  float g000 = PerlinNoise3D_Math_Grad(PerlinNoise3D_p[AA], xf, yf, zf);
  float g001 = PerlinNoise3D_Math_Grad(PerlinNoise3D_p[AA + 1], xf, yf, zf - 1);
  float g010 = PerlinNoise3D_Math_Grad(PerlinNoise3D_p[AB], xf, yf - 1, zf);
  float g011 =
      PerlinNoise3D_Math_Grad(PerlinNoise3D_p[AB + 1], xf, yf - 1, zf - 1);
  float g100 = PerlinNoise3D_Math_Grad(PerlinNoise3D_p[BA], xf - 1, yf, zf);
  float g101 =
      PerlinNoise3D_Math_Grad(PerlinNoise3D_p[BA + 1], xf - 1, yf, zf - 1);
  float g110 = PerlinNoise3D_Math_Grad(PerlinNoise3D_p[BB], xf - 1, yf - 1, zf);
  float g111 =
      PerlinNoise3D_Math_Grad(PerlinNoise3D_p[BB + 1], xf - 1, yf - 1, zf - 1);

  float lerpX00 = Math_Lerp(u, g000, g100);
  float lerpX01 = Math_Lerp(u, g001, g101);
  float lerpX10 = Math_Lerp(u, g010, g110);
  float lerpX11 = Math_Lerp(u, g011, g111);

  float lerpY0 = Math_Lerp(v, lerpX00, lerpX10);
  float lerpY1 = Math_Lerp(v, lerpX01, lerpX11);

  return Math_Lerp(w, lerpY0, lerpY1);
}

JNIEXPORT jobjectArray JNICALL JNI_FUNC_SIGN_GERAR_CHUNK(JNIEnv* env,
                                                         jclass clazz,
                                                         jobject worldInstance,
                                                         jint chunkX,
                                                         jint chunkZ) {
  PerlinNoise_InitPermutationTables();
  //////////////////////// BEGIN CONSTS //////////////////////////////////
  const int CHUNK_TAMANHO = 8;
  const int MUNDO_LATERAL = 120;
  //////////////////////// END CONSTS ////////////////////////////////////

  //////////////////////// BEGIN CLASSES AND METHODS ///////////////////
  jclass worldClass = (*env)->GetObjectClass(env, worldInstance);
  jclass blocoClass = (*env)->FindClass(env, "com/minimine/engine/Bloco");
  jclass blocoArrayClass =
      (*env)->FindClass(env, "[Lcom/minimine/engine/Bloco;");
  jclass bloco2DArrayClass =
      (*env)->FindClass(env, "[[Lcom/minimine/engine/Bloco;");
  jmethodID ctorBloco = (*env)->GetMethodID(env, blocoClass, "<init>",
                                            "(IIILjava/lang/String;)V");

  //////////////////////// END CLASSES AND METHODS ///////////////////////

  //////////////////////// BEGIN GET FIELDS FROM WORLDINSTANCE ///////////
  jfieldID seedField = (*env)->GetFieldID(env, worldClass, "seed", "I");
  jint seed = (*env)->GetIntField(env, worldInstance, seedField);

  jbool plano = false;

  jfieldID escala2DField =
      (*env)->GetFieldID(env, worldClass, "ESCALA_2D", "F");
  jfloat escala2D = (*env)->GetFloatField(env, worldInstance, escala2DField);

  jfieldID escala3DField =
      (*env)->GetFieldID(env, worldClass, "ESCALA_3D", "F");
  jfloat escala3D = (*env)->GetFloatField(env, worldInstance, escala3DField);

  jfieldID estruturasField =
      (*env)->GetFieldID(env, worldClass, "estruturas", "Ljava/util/List;");
  jobject estruturas =
      (*env)->GetObjectField(env, worldInstance, estruturasField);

  // Structures methods (in ptbr)
  jclass listClass = (*env)->FindClass(env, "java/util/List");
  jmethodID listGet =
      (*env)->GetMethodID(env, listClass, "get", "(I)Ljava/lang/Object;");

  jmethodID spawnEstruturaMethod =
      (*env)->GetMethodID(env, worldClass, "spawnEstrutura", "(FIII)Z");

  jmethodID adicionarEstruturaMethod = (*env)->GetMethodID(
      env, worldClass, "adicionarEstrutura",
      "(IIILjava/lang/String;[[[Lcom/minimine/engine/Bloco;)V");

  //////////////////////// END GET FIELDS FROM WORLDINSTANCE /////////////

  //////////////////////// BEGIN INITIALIZATIONS /////////////////////////
  int baseX = chunkX * CHUNK_TAMANHO;
  int baseZ = chunkZ * CHUNK_TAMANHO;

  int alturas[CHUNK_TAMANHO][CHUNK_TAMANHO];

  // create chunk[x][y][z]
  jobjectArray chunk =
      (*env)->NewObjectArray(env, CHUNK_TAMANHO, bloco2DArrayClass, NULL);

  for (int x = 0; x < CHUNK_TAMANHO; x++) {
    jobjectArray yArray =
        (*env)->NewObjectArray(env, MUNDO_LATERAL, blocoArrayClass, NULL);

    for (int z = 0; z < CHUNK_TAMANHO; z++) {
      int globalX = baseX + x;
      int globalZ = baseZ + z;

      float noise2D = plano ? 0.001f
                            : (PerlinNoise2D_ruido(globalX * escala2D,
                                                   globalZ * escala2D, seed) +
                               1.0f) *
                                  0.5f;
      int altura = (int)(noise2D * 16.0f + 32.0f);
      alturas[x][z] = altura;

      for (int y = 0; y < MUNDO_LATERAL; y++) {
        jobjectArray zArray =
            (*env)->NewObjectArray(env, CHUNK_TAMANHO, blocoClass, NULL);

        const char* tipo;
        if (y == 0) {
          tipo = "BEDROCK";
        } else {
          float noise3D = PerlinNoise3D_ruido(globalX * escala3D, y * escala3D,
                                              globalZ * escala3D, seed + 100);
          if (noise3D > 0.15f && y < altura)
            tipo = "AR";
          else if (y < altura - 1)
            tipo = "PEDRA";
          else if (y < altura)
            tipo = "TERRA";
          else if (y == altura)
            tipo = "GRAMA";
          else
            tipo = "AR";
        }

        jstring tipoStr = (*env)->NewStringUTF(env, tipo);
        jobject bloco = (*env)->NewObject(env, blocoClass, ctorBloco, globalX,
                                          y, globalZ, tipoStr);
        (*env)->SetObjectArrayElement(env, zArray, z, bloco);
        (*env)->DeleteLocalRef(env, tipoStr);
        (*env)->DeleteLocalRef(env, bloco);

        (*env)->SetObjectArrayElement(env, yArray, y, zArray);
        (*env)->DeleteLocalRef(env, zArray);
      }
    }

    (*env)->SetObjectArrayElement(env, chunk, x, yArray);
    (*env)->DeleteLocalRef(env, yArray);
  }

  //////////////////////// END INITIALIZATIONS ///////////////////////////

  //////////////////////// BEGIN STRUCTURE ///////////////////////////////
  // Add structures
  for (int x = 0; x < CHUNK_TAMANHO; x++) {
    for (int z = 0; z < CHUNK_TAMANHO; z++) {
      int globalX = baseX + x;
      int globalZ = baseZ + z;
      int altura = alturas[x][z];

      float chances[] = {0.1f, 0.01f, 0.009f};
      for (int i = 0; i < 3; i++) {
        jbool deveSpawnar =
            (*env)->CallBooleanMethod(env, worldInstance, spawnEstruturaMethod,
                                      chances[i], globalX, globalZ, seed);
        if (deveSpawnar) {
          jstring estrutura =
              (*env)->CallObjectMethod(env, estruturas, listGet, i);
          (*env)->CallVoidMethod(env, worldInstance, adicionarEstruturaMethod,
                                 globalX, altura, globalZ, estrutura, chunk);
          (*env)->DeleteLocalRef(env, estrutura);
        }
      }
    }
  }

  //////////////////////// END STRUCTURE /////////////////////////////////

  return chunk;
}