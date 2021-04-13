#pragma once
#include <cstdint>

typedef void MonoAssembly;
typedef void MonoDomain;
typedef void MonoImage;
typedef void MonoClass;
typedef void MonoType;
typedef void MonoImageOpenStatus;
typedef void MonoThread;
typedef void MonoObject;
typedef void MonoClassField;
typedef void MonoMethod;
typedef void MonoMethodDesc;
typedef void MonoProperty;
typedef void MonoError;
typedef void MonoString;
typedef void MonoJitInfo;
typedef void MonoVTable;
typedef void MonoAssemblyName;
typedef void MonoReflectionMethod;
typedef void MonoReflectionType;
typedef void MonoArray;

typedef const void* gconstpointer;
typedef void gpointer;
typedef bool gboolean;

typedef bool mono_bool;

typedef uint32_t guint32;
typedef int32_t gint32;


namespace Mira
{
    namespace Mono
    {
        class MonoApi
        {
        public:
            static bool Init();
            
        public:
            typedef MonoClass* (*bind_generic_parameters_t)(MonoClass *klass, int type_argc, MonoType **types, gboolean is_dynamic);
            static bind_generic_parameters_t mono_class_bind_generic_parameters;

            typedef MonoAssembly *(*mono_assembly_open_full_t)(const char *filename, MonoImageOpenStatus *status, mono_bool refonly);
            static mono_assembly_open_full_t mono_assembly_open_full;

            typedef MonoAssembly *(*mono_domain_assembly_open_t)(MonoDomain *domain, const char *name);
            static mono_domain_assembly_open_t mono_domain_assembly_open;
            
            typedef MonoAssembly *(*mono_assembly_load_from_t)(MonoImage *image, const char *fname, MonoImageOpenStatus *status);
            static mono_assembly_load_from_t mono_assembly_load_from;

            typedef MonoDomain* (*mono_get_root_domain_t)();
            static mono_get_root_domain_t mono_get_root_domain;

            typedef MonoDomain* (* mono_domain_get_by_id_t)(gint32 domainid);
            static mono_domain_get_by_id_t mono_domain_get_by_id;

            typedef MonoImage *(*mono_assembly_get_image_t)(MonoAssembly *assembly);
            static mono_assembly_get_image_t mono_assembly_get_image;

            typedef MonoImage* (*mono_image_open_from_data_with_name_t)(char *data, uint32_t data_len, mono_bool need_copy, MonoImageOpenStatus *status, mono_bool refonly, const char *name);
            static mono_image_open_from_data_with_name_t mono_image_open_from_data_with_name;

            typedef MonoThread* (*mono_thread_get_main_t)();
            static mono_thread_get_main_t mono_thread_get_main;

            typedef MonoThread* (*mono_thread_set_main_t)(MonoThread* thread);
            static mono_thread_set_main_t mono_thread_set_main;

            typedef MonoThread* (*mono_thread_current_t)();
            static mono_thread_current_t mono_thread_current;

            typedef MonoThread* (*mono_thread_attach_t)(MonoDomain* domain);
            static mono_thread_attach_t mono_thread_attach;

            typedef MonoClass* (*mono_class_from_name_t)(void* image, const char* namespacee, const char* name);
            static mono_class_from_name_t mono_class_from_name;

            typedef MonoClass* (*mono_object_get_class_t)(MonoObject *obj);
            static mono_object_get_class_t mono_object_get_class;

            typedef MonoClass* (*_mono_class_from_mono_type)(MonoType *type);
            typedef MonoType* (*_mono_class_get_type)(MonoClass *klass);

            typedef MonoClassField* (*mono_class_get_field_from_name_t)(MonoClass* mclass, const char* name);
            static mono_class_get_field_from_name_t mono_class_get_field_from_name;

            typedef MonoClassField* (*mono_class_get_fields_t)(MonoClass *klass, gpointer *iter);
            static mono_class_get_fields_t mono_class_get_fields;

            typedef MonoMethod* (*mono_class_get_method_from_name_t)(void* _class, const char* name, uint32_t param_count);
            static mono_class_get_method_from_name_t mono_class_get_method_from_name;

            typedef MonoMethod* (*mono_method_desc_search_in_class_t)(MonoMethodDesc* desc,void* _class);
            static mono_method_desc_search_in_class_t mono_method_desc_search_in_class;

            typedef MonoMethod* (*mono_property_get_get_method_t)(MonoProperty* prop);
            static mono_property_get_get_method_t mono_property_get_get_method;

            typedef MonoMethod* (*mono_property_get_set_method_t)(MonoProperty* prop);
            static mono_property_get_set_method_t mono_property_get_set_method;

            typedef MonoMethod* (*mono_get_delegate_invoke_t)(MonoClass *klass);
            static mono_get_delegate_invoke_t mono_get_delegate_invoke;

            typedef MonoMethod* (*mono_class_get_methods_t)(MonoClass *klass, gpointer *iter);
            static mono_class_get_methods_t mono_class_get_methods;

            typedef MonoMethod* (*mono_class_get_virtual_methods_t)(MonoClass *klass, gpointer *iter);
            static mono_class_get_virtual_methods_t mono_class_get_virtual_methods;

            typedef MonoMethodDesc* (*mono_method_desc_new_t)(const char *name, gboolean include_namespace);
            static mono_method_desc_new_t mono_method_desc_new;

            typedef MonoProperty* (*mono_class_get_property_from_name_t)(MonoClass* mclass, const char* name);
            static mono_class_get_property_from_name_t mono_class_get_property_from_name;

            typedef MonoProperty* (*mono_class_get_properties_t)(MonoClass *klass, gpointer *iter);
            static mono_class_get_properties_t mono_class_get_properties;

            typedef MonoObject* (*mono_runtime_delegate_invoke_t)(MonoObject *delegate, void **params, MonoObject **exc);
            static mono_runtime_delegate_invoke_t mono_runtime_delegate_invoke;

            typedef MonoObject* (*mono_field_get_value_object_t)(MonoDomain *domain, MonoClassField *field, MonoObject *obj);
            static mono_field_get_value_object_t mono_field_get_value_object;

            typedef MonoObject* (*mono_runtime_invoke_t)(void* method, void* instance, void* params, void* exc);
            static mono_runtime_invoke_t mono_runtime_invoke;

            typedef MonoObject* (*mono_object_new_t)(MonoDomain* domain, void* _class);
            static mono_object_new_t mono_object_new;

            typedef MonoObject* (*mono_runtime_delegate_invoke_checked_t)(MonoObject *delegate, void **params, MonoError *error);
            static mono_runtime_delegate_invoke_checked_t mono_runtime_delegate_invoke_checked;

            typedef MonoString* (*mono_string_new_t)(MonoDomain* domain, const char* string);
            static mono_string_new_t mono_string_new;

            typedef MonoString* (*mono_object_to_string_t)(MonoObject *obj, MonoObject **exc);
            static mono_object_to_string_t mono_object_to_string;

            typedef MonoJitInfo* (*mono_jit_info_table_find_t)(MonoDomain *domain, char *addr);
            static mono_jit_info_table_find_t mono_jit_info_table_find;

            typedef MonoVTable* (*mono_class_vtable_t)(MonoDomain *domain, MonoClass *klass);
            static mono_class_vtable_t mono_class_vtable;

            typedef gpointer* (*mono_object_unbox_t)(MonoObject* obj);
            static mono_object_unbox_t mono_object_unbox;

            typedef gpointer (*mono_compile_method_t)(MonoMethod *method);
            static mono_compile_method_t mono_compile_method;

            typedef gboolean (*mono_assembly_name_parse_full_t)(const char *name, MonoAssemblyName *aname, gboolean save_public_key, gboolean *is_version_defined, gboolean *is_token_defined);
            static mono_assembly_name_parse_full_t mono_assembly_name_parse_full;

            typedef MonoReflectionMethod* (*mono_method_get_object_t)(MonoDomain *domain, MonoMethod *method, MonoClass *refclass);
            static mono_method_get_object_t mono_method_get_object;

            typedef MonoReflectionType* (*mono_type_get_object_t)(MonoDomain *domain, MonoType *type);
            static mono_type_get_object_t mono_type_get_object;

            typedef MonoType* (*mono_reflection_type_from_name_t)(const char *name, MonoImage *image);
            static mono_reflection_type_from_name_t mono_reflection_type_from_name;

            typedef MonoArray* (*mono_array_new_t)(MonoDomain *domain, MonoClass *eclass, uintptr_t n);
            static mono_array_new_t mono_array_new;

            typedef char* (*mono_array_addr_with_size_t)(MonoArray *array, int size, uintptr_t idx);
            static mono_array_addr_with_size_t mono_array_addr_with_size;

            typedef char* (*mono_string_to_utf8_t)(MonoString *s);
            static mono_string_to_utf8_t mono_string_to_utf8;

            typedef char* (*mono_field_full_name_t)(MonoClassField *method);
            static mono_field_full_name_t mono_field_full_name;

            typedef char* (*mono_method_full_name_t)(MonoMethod *method, gboolean signature);
            static mono_method_full_name_t mono_method_full_name;

            typedef const char* (*mono_property_get_name_t)(MonoProperty *prop);
            static mono_property_get_name_t mono_property_get_name;

            typedef void (*mono_jit_set_aot_only_t)(gboolean val);
            static mono_jit_set_aot_only_t mono_jit_set_aot_only;

            typedef void (*mono_add_internal_call_t)(const char* name, gconstpointer NativeMethod);
            static mono_add_internal_call_t mono_add_internal_call;

            typedef void (*mono_property_set_value_t)(MonoProperty *prop, void *obj, void *params, MonoObject **exc);
            static mono_property_set_value_t mono_property_set_value;

            typedef void (*mono_field_get_value_t)(MonoObject *obj, MonoClassField *field, void *value);
            static mono_field_get_value_t mono_field_get_value;

            typedef void (*mono_field_set_value_t)(MonoObject *obj, MonoClassField *field, void *value);
            static mono_field_set_value_t mono_field_set_value;
            
            typedef void (*mono_field_static_set_value_t)(MonoVTable *vt, MonoClassField *field, void *value);
            static mono_field_static_set_value_t mono_field_static_set_value;

            typedef int	(*mono_main_t)(int argc, const char* const argv[]);
            static mono_main_t mono_main;

            typedef guint32 (*mono_gchandle_new_t)(MonoObject *obj, gboolean pinned);
            static mono_gchandle_new_t mono_gchandle_new;

            typedef guint32 (*mono_gchandle_new_weakref_t)(MonoObject *obj, gboolean pinned);
            static mono_gchandle_new_weakref_t mono_gchandle_new_weakref;

            typedef void (*mono_gchandle_free_t)(guint32 obj);
            static mono_gchandle_free_t mono_gchandle_free;

            typedef MonoObject* (*mono_gchandle_get_target_t)(guint32 gchandle);
            static mono_gchandle_get_target_t mono_gchandle_get_target;
        };
    }
}