#include <jni.h>
#include <stdlib.h>

#define LOG_TAG "c2bsh"
#define LOGE(...) printf(__VA_ARGS__)

#ifndef JNI_MAIN_FUNC_SIGN
#define JNI_MAIN_FUNC_SIGN Java_com_minimine_Native_showWindow
#endif

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
  int count;    // the current views amount
  int capacity; // the max capacity for views in memory, used for dynamic allocation
};

// Window
struct Window {
  jobject manager;      // the android java window manager 
  jobject layoutParams; // the window manager layout params, used to add root view
  ViewList viewList;    // the view list, see struct ViewList above
};

// App Data
struct Hack {
  JavaVM* jvm;     // the java vm
  Window window;   // the window, see above
  jobject context; // android activity context, needed for views constructor
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

JNIEXPORT void JNICALL JNI_MAIN_FUNC_SIGN(JNIEnv* env,
                                             jobject thiz,
                                             jobject context) {
  //////////////////////// GET NEEDED CLASSES ////////////////////////
  jclass contextClass = (*env)->GetObjectClass(env, context);
  jmethodID getSystemService =
      (*env)->GetMethodID(env, contextClass, "getSystemService",
                          "(Ljava/lang/String;)Ljava/lang/Object;");

  hack->context = (*env)->NewGlobalRef(env, context);
  jstring service = (*env)->NewStringUTF(env, "window");
  jobject manager = (*env)->CallObjectMethod(env, context, getSystemService,
                                             service);
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