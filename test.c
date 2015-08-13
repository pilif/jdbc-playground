#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

static JNIEnv *env;
static JavaVM *jvm;

void load_driver(char* path, char* jdbc_driver_class);

int main(int argc, char * argv[]){
    JavaVMInitArgs  vm_args;
    JavaVMOption    *options;
    jint            res;

    options = (JavaVMOption*)malloc(sizeof(JavaVMOption));
    options[0].optionString = "-Djava.class.path=.";

    vm_args.version = 0x00010002;
    vm_args.options = options;
    vm_args.nOptions = 1;
    vm_args.ignoreUnrecognized = JNI_FALSE;

    res = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
    if (res != JNI_OK){
        printf("JNI_CreateJavaVM: %d\n", res);
        return 1;
    }

    printf("Java Initialized\n");

    load_driver("postgresql-9.4-1201.jdbc41.jar", "org.postgresql.Driver");

    (*jvm)->DestroyJavaVM(jvm);

    printf("Java destroyed\n");
    return 0;
}

void load_driver(char* path, char* jdbc_driver_class){
    jclass  loader_class;

    jmethodID add_path_method;
    jmethodID load_class_method;
    jmethodID constructor;
    jmethodID version_method;

    jstring jar_path;
    jstring driver;

    jobject loader_instance = NULL;
    jclass driver_class = NULL;
    jobject driver_instance = NULL;

    loader_class = (*env)->FindClass(env, "DriverLoader");
    if (loader_class == NULL){
        printf("Failed to load DriverLoader\n");
        return;
    }
    add_path_method = (*env)->GetMethodID(env, loader_class, "addPath", "(Ljava/lang/String;)V");
    if (add_path_method == NULL){
        printf("Failed to find addPath method\n");
        return;
    }
    load_class_method = (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (load_class_method == NULL){
        printf("Failed to find loadClass method\n");
        return;
    }
    constructor = (*env)->GetMethodID(env, loader_class, "<init>", "()V");
    if (constructor == NULL){
        printf("Failed to find constructor\n");
        return;
    }

    printf("Method found\n");
    jar_path = (*env)->NewStringUTF(env, path);
    driver = (*env)->NewStringUTF(env, jdbc_driver_class);

    loader_instance = (*env)->NewObject(env, loader_class, constructor);
    if (loader_instance == NULL){
        printf("Failed to instantiate loader");
        goto cleanup;
    }

    (*env)->CallObjectMethod(env, loader_instance, add_path_method, jar_path);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE){
        (*env)->ExceptionClear(env);
        printf("Exception in add_path\n");
        goto cleanup;
    }
    driver_class = (jclass)(*env)->CallObjectMethod(env, loader_instance, load_class_method, driver);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE){
        (*env)->ExceptionClear(env);
        printf("Exception in load_class\n");
        goto cleanup;
    }
    printf("Got %s class: %p\n", jdbc_driver_class, driver_class);

    constructor = (*env)->GetMethodID(env, driver_class, "<init>", "()V");
    if (constructor == NULL){
        printf("Failed to find constructor of driver\n");
        goto cleanup;
    }
    driver_instance = (*env)->NewObject(env, driver_class, constructor);
    if (driver_instance == NULL){
        printf("Failed to instantiate the driver\n");
        goto cleanup;
    }
    printf("Got %s instance: %p\n", jdbc_driver_class, driver_instance);

    version_method = (*env)->GetMethodID(env, driver_class, "getMajorVersion", "()I");
    if (version_method == NULL){
        printf("Failed to find getMajorVersion method");
        goto cleanup;
    }

    printf("got get major method. Calling it\n");
    jint major = (*env)->CallIntMethod(env, driver_instance, version_method);

    version_method = (*env)->GetMethodID(env, driver_class, "getMinorVersion", "()I");
    if (version_method == NULL){
        printf("Failed to find getMajorVersion method");
        goto cleanup;
    }
    jint minor = (*env)->CallIntMethod(env, driver_instance, version_method);

    printf("Driver-Version: %d.%d\n", major, minor);

cleanup:
    (*env)->DeleteLocalRef(env, jar_path);
    (*env)->DeleteLocalRef(env, driver);
    if (driver_class != NULL)
        (*env)->DeleteLocalRef(env, driver_class);
    if (driver_instance != NULL)
        (*env)->DeleteLocalRef(env, driver_instance);





}
