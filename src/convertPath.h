#pragma once
#include "common.h"

class convertPath {
public:
	static wchar_t* func(const wchar_t* path, bool islong) {//you need to delete the result yourself if it is not NULL
		wchar_t* tpath;
		DWORD sz = islong ? GetLongPathNameW(path, NULL, 0) : GetShortPathNameW(path, NULL, 0);
		if (sz > 0) {
			tpath = new wchar_t[sz];
			islong ? GetLongPathNameW(path, tpath, sz) : GetShortPathNameW(path, tpath, sz);
		} else {
			tpath = NULL;
		}
		return tpath;
	}
	static napi_value init(napi_env env, bool isSync = false) {
		napi_value f;
		napi_create_function(env, NULL, 0, isSync ? sync : async, NULL, &f);
		return f;
	}
private:
	const struct cbdata {
		napi_async_work work;
		napi_ref self;
		napi_ref cb;
		wchar_t* path;
		bool islong;
		wchar_t* result;
	};
	static napi_value sync(napi_env env, napi_callback_info info) {
		napi_value result;
		napi_get_new_target(env, info, &result);
		if (result) {
			result = NULL;
			napi_throw_error(env, SYB_EXP_INVAL, SYB_ERR_NOT_A_CONSTRUCTOR);
		} else {
			napi_value argv[2];
			size_t argc = 2;
			napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
			if (argc < 1) {
				napi_throw_error(env, SYB_EXP_INVAL, SYB_ERR_WRONG_ARGUMENTS);
			} else {
				size_t str_len;
				napi_value tmp;
				napi_coerce_to_string(env, argv[0], &tmp);
				napi_get_value_string_utf16(env, tmp, NULL, 0, &str_len);
				str_len += 1;
				wchar_t* str = new wchar_t[str_len];
				napi_get_value_string_utf16(env, tmp, (char16_t*)str, str_len, NULL);
				bool islong = false;
				if (argc > 1) {
					napi_coerce_to_bool(env, argv[1], &tmp);
					napi_get_value_bool(env, tmp, &islong);
				}
				wchar_t* s = func(str, islong);
				delete[]str;
				if (s) {
					napi_create_string_utf16(env, (char16_t*)s, wcslen(s), &result);
					delete[]s;
				} else {
					napi_get_null(env, &result);
				}
			}
		}
		return result;
	}
	static napi_value async(napi_env env, napi_callback_info info) {
		napi_value result;
		napi_get_new_target(env, info, &result);
		if (result) {
			result = NULL;
			napi_throw_error(env, SYB_EXP_INVAL, SYB_ERR_NOT_A_CONSTRUCTOR);
		} else {
			napi_value argv[3], self;
			size_t argc = 3;
			napi_get_cb_info(env, info, &argc, argv, &self, NULL);
			if (argc >= 2) {
				napi_valuetype t;
				napi_typeof(env, argv[1], &t);
				if (t == napi_function) {
					cbdata* data = new cbdata;
					data->islong = false;
					size_t str_len;
					napi_value tmp;
					napi_create_reference(env, argv[1], 1, &data->cb);
					napi_create_reference(env, self, 1, &data->self);
					napi_coerce_to_string(env, argv[0], &tmp);
					napi_get_value_string_utf16(env, tmp, NULL, 0, &str_len);
					str_len += 1;
					data->path = new wchar_t[str_len];
					napi_get_value_string_utf16(env, tmp, (char16_t*)data->path, str_len, NULL);
					if (argc > 2) {
						napi_coerce_to_bool(env, argv[2], &tmp);
						napi_get_value_bool(env, tmp, &data->islong);
					}
					napi_create_string_latin1(env, "fswin.convertPath", NAPI_AUTO_LENGTH, &tmp);
					napi_create_async_work(env, NULL, tmp, execute, complete, data, &data->work);
					if (napi_queue_async_work(env, data->work) == napi_ok) {
						napi_get_boolean(env, true, &result);
					} else {
						napi_get_boolean(env, false, &result);
						napi_delete_reference(env, data->cb);
						napi_delete_reference(env, data->self);
						napi_delete_async_work(env, data->work);
						delete[]data->path;
						delete data;
					}
				}
			}
			if (!result) {
				napi_throw_error(env, SYB_EXP_INVAL, SYB_ERR_WRONG_ARGUMENTS);
			}
		}
		return result;
	}
	static void execute(napi_env env, void* data) {
		cbdata* d = (cbdata*)data;
		d->result = func(d->path, d->islong);
	}
	static void complete(napi_env env, napi_status status, void* data) {
		cbdata* d = (cbdata*)data;
		delete[]d->path;
		napi_value cb, self, argv;
		napi_get_reference_value(env, d->cb, &cb);
		napi_get_reference_value(env, d->self, &self);
		if (status == napi_ok && d->result) {
			napi_create_string_utf16(env, (char16_t*)d->result, wcslen(d->result), &argv);
			delete[]d->result;
		} else {
			napi_get_null(env, &argv);
		}
		napi_call_function(env, self, cb, 1, &argv, NULL);
		napi_delete_reference(env, d->cb);
		napi_delete_reference(env, d->self);
		napi_delete_async_work(env, d->work);
		delete d;
	}
};