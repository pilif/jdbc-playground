#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

static JNIEnv *env;
static JavaVM *jvm;

jobject load_driver(char* path, char* jdbc_driver_class);
void print_version(jobject driver);
void connect(jobject driver, char* url, char* user, char* password);

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

    jobject driver = load_driver("postgresql-9.4-1201.jdbc41.jar", "org.postgresql.Driver");
    if (driver != NULL){
        print_version(driver);
        connect(driver, "jdbc:postgresql:pilif", "pilif", NULL);

        (*env)->DeleteLocalRef(env, driver);
        driver = NULL;
    }

    (*jvm)->DestroyJavaVM(jvm);

    printf("Java destroyed\n");
    return 0;
}

void connect(jobject driver, char* url, char* user, char* password){
    jclass driver_class = (*env)->GetObjectClass(env, driver);
    jmethodID method;

    jclass prop_class = (*env)->FindClass(env, "java/util/Properties");
    if (prop_class == NULL){
        printf("Failed to find Properties class\n");
        return;
    }

    method = (*env)->GetMethodID(env, prop_class, "<init>", "()V");
    if (method == NULL){
        printf("Failed to find Properties constructor\n");
        return;
    }
    jobject properties = (*env)->NewObject(env, prop_class, method);
    if (properties == NULL){
        printf("Failed to instantiate Properties instance\n");
        return;
    }
    method = (*env)->GetMethodID(env, prop_class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (method == NULL){
        printf("Failed t find put method on Properties\n");
        return;
    }

    jstring prop_key = (*env)->NewStringUTF(env, "user");
    jstring prop_value = (*env)->NewStringUTF(env, user);
    (*env)->CallObjectMethod(env, properties, method, prop_key, prop_value);
    (*env)->DeleteLocalRef(env, prop_key);
    (*env)->DeleteLocalRef(env, prop_value);

    prop_key = (*env)->NewStringUTF(env, "ApplicationName");
    prop_value = (*env)->NewStringUTF(env, "pilif's fun project");
    (*env)->CallObjectMethod(env, properties, method, prop_key, prop_value);
    (*env)->DeleteLocalRef(env, prop_key);
    (*env)->DeleteLocalRef(env, prop_value);

    method = (*env)->GetMethodID(env, driver_class, "connect", "(Ljava/lang/String;Ljava/util/Properties;)Ljava/sql/Connection;");
    if (method == NULL){
        printf("Failed to find connect method\n");
        return;
    }

    jstring url_string = (*env)->NewStringUTF(env, url);
    jobject connection = (*env)->CallObjectMethod(env, driver, method, url_string, properties);
    (*env)->DeleteLocalRef(env, url_string);

    if (connection == NULL){
        printf("Failed to connect\n");
        return;
    }

    printf("Connection established. Instance: %p\nPress any key...", connection);
    getchar();
    jclass connection_class = (*env)->GetObjectClass(env, connection);
    method = (*env)->GetMethodID(env, connection_class, "close", "()V");
    if (method == NULL){
        printf("Failed to find close method\n");
        return;
    }

    (*env)->CallVoidMethod(env, connection, method);
    printf("Closed again\n");
    (*env)->DeleteLocalRef(env, connection);
    connection = NULL;
}

void print_version(jobject driver){
    jclass cls = (*env)->GetObjectClass(env, driver);
    jmethodID method;

    method = (*env)->GetMethodID(env, cls, "getMajorVersion", "()I");
    if (method == NULL){
        printf("Failed to find getMajorVersion method\n");
        return;
    }
    jint major = (*env)->CallIntMethod(env, driver, method);

    method = (*env)->GetMethodID(env, cls, "getMinorVersion", "()I");
    if (method == NULL){
        printf("Failed to find getMinorVersion method\n");
        return;
    }
    jint minor = (*env)->CallIntMethod(env, driver, method);

    printf("Driver-Version: %d.%d\n", major, minor);
}

jobject load_driver(char* path, char* jdbc_driver_class){
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
        return NULL;
    }
    add_path_method = (*env)->GetMethodID(env, loader_class, "addPath", "(Ljava/lang/String;)V");
    if (add_path_method == NULL){
        printf("Failed to find addPath method\n");
        return NULL;
    }
    load_class_method = (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (load_class_method == NULL){
        printf("Failed to find loadClass method\n");
        return NULL;
    }
    constructor = (*env)->GetMethodID(env, loader_class, "<init>", "()V");
    if (constructor == NULL){
        printf("Failed to find constructor\n");
        return NULL;
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

cleanup:
    (*env)->DeleteLocalRef(env, jar_path);
    (*env)->DeleteLocalRef(env, driver);
    if (driver_class != NULL)
        (*env)->DeleteLocalRef(env, driver_class);

    return driver_instance;
}
