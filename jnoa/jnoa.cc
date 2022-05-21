/*****************************************************************************
 *   Copyright (c) 2022, Roland Grinis, GrinisRIT ltd.                       *
 *   (roland.grinis@grinisrit.com)                                           *
 *   All rights reserved.                                                    *
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/**
 * Implemented by: Roland Grinis, Anastasia Golovina
 */

#include "jnoa.hh"
#include "space_kscience_kmath_noa_JNoa.h"

using namespace jnoa;

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_testException
        (JNIEnv *env, jclass, jint seed) {
    const auto res = safe_run<int>(env, test_exception, seed);
    return res.has_value() ? res.value() : 0;
}

JNIEXPORT jboolean JNICALL Java_space_kscience_kmath_noa_JNoa_cudaIsAvailable
        (JNIEnv *, jclass) {
    return torch::cuda::is_available();
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getNumThreads
        (JNIEnv *, jclass) {
    return torch::get_num_threads();
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setNumThreads
        (JNIEnv *, jclass, jint num_threads) {
    torch::set_num_threads(num_threads);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSeed
        (JNIEnv *, jclass, jint seed) {
    torch::manual_seed(seed);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    if (tensor_handle != 0L)
        dispose<Tensor>(tensor_handle);
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_emptyTensor
        (JNIEnv *, jclass) {
    return (long) new Tensor;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fromBlobDouble
        (JNIEnv *env, jclass, jdoubleArray array, jintArray shape, jint device) {
    auto data = env->GetDoubleArrayElements(array, nullptr);
    const auto res =
            safe_run<Tensor>(env,
                             from_blob<double>,
                             data,
                             to_shape(env, shape),
                             int_to_device(device));
    env->ReleaseDoubleArrayElements(array, data, JNI_ABORT);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fromBlobFloat
        (JNIEnv *env, jclass, jfloatArray array, jintArray shape, jint device) {
    auto data = env->GetFloatArrayElements(array, nullptr);
    const auto res =
            safe_run<Tensor>(env,
                             from_blob<float>,
                             data,
                             to_shape(env, shape),
                             int_to_device(device));
    env->ReleaseFloatArrayElements(array, data, JNI_ABORT);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fromBlobLong
        (JNIEnv *env, jclass, jlongArray array, jintArray shape, jint device) {
    auto data = env->GetLongArrayElements(array, nullptr);
    const auto res =
            safe_run<Tensor>(env,
                             from_blob<long>,
                             data,
                             to_shape(env, shape),
                             int_to_device(device));
    env->ReleaseLongArrayElements(array, data, JNI_ABORT);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fromBlobInt
        (JNIEnv *env, jclass, jintArray array, jintArray shape, jint device) {
    auto data = env->GetIntArrayElements(array, nullptr);
    const auto res =
            safe_run<Tensor>(env,
                             from_blob<int>,
                             data,
                             to_shape(env, shape),
                             int_to_device(device));
    env->ReleaseIntArrayElements(array, data, JNI_ABORT);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).clone());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyToDevice
        (JNIEnv *env, jclass, jlong tensor_handle, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const auto &device) {
                                 return tensor.to(device, false, true);
                             },
                             cast<Tensor>(tensor_handle),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyToDouble
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(
            cast<Tensor>(tensor_handle)
                    .to(dtype<double>(), false, false));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyToFloat
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(
            cast<Tensor>(tensor_handle)
                    .to(dtype<float>(), false, false));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyToLong
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(
            cast<Tensor>(tensor_handle)
                    .to(dtype<long>(), false, false));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_copyToInt
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(
            cast<Tensor>(tensor_handle)
                    .to(dtype<int>(), false, false));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_viewTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray shape) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const auto &shape) {
                                 return tensor.view(shape);
                             },
                             cast<Tensor>(tensor_handle),
                             to_shape(env, shape));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_viewAsTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jlong as_tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const auto &tensor_ref) {
                                 return tensor.view_as(tensor_ref);
                             },
                             cast<Tensor>(tensor_handle),
                             cast<Tensor>(as_tensor_handle));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jstring JNICALL Java_space_kscience_kmath_noa_JNoa_tensorToString
        (JNIEnv *env, jclass, jlong tensor_handle) {
    return env->NewStringUTF(tensor_to_string(cast<Tensor>(tensor_handle)).c_str());
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getDim
        (JNIEnv *, jclass, jlong tensor_handle) {
    return cast<Tensor>(tensor_handle).dim();
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getNumel
        (JNIEnv *, jclass, jlong tensor_handle) {
    return cast<Tensor>(tensor_handle).numel();
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getShapeAt
        (JNIEnv *, jclass, jlong tensor_handle, jint d) {
    return cast<Tensor>(tensor_handle).size(d);
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getStrideAt
        (JNIEnv *, jclass, jlong tensor_handle, jint d) {
    return cast<Tensor>(tensor_handle).stride(d);
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getDevice
        (JNIEnv *, jclass, jlong tensor_handle) {
    return device_to_int(cast<Tensor>(tensor_handle));
}

JNIEXPORT jdouble JNICALL Java_space_kscience_kmath_noa_JNoa_getItemDouble
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<double>(env,
                             [](const Tensor &tensor) { return tensor.item<double>(); },
                             cast<Tensor>(tensor_handle));
    return res.has_value() ? res.value() : 0.;
}

JNIEXPORT jfloat JNICALL Java_space_kscience_kmath_noa_JNoa_getItemFloat
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<float>(env,
                            [](const Tensor &tensor) { return tensor.item<float>(); },
                            cast<Tensor>(tensor_handle));
    return res.has_value() ? res.value() : 0.f;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getItemLong
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<long>(env,
                           [](const Tensor &tensor) { return tensor.item<long>(); },
                           cast<Tensor>(tensor_handle));
    return res.has_value() ? res.value() : 0L;
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getItemInt
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<int>(env,
                          [](const Tensor &tensor) { return tensor.item<int>(); },
                          cast<Tensor>(tensor_handle));
    return res.has_value() ? res.value() : 0;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getIndex
        (JNIEnv *env, jclass, jlong tensor_handle, jint index) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int index) {
                                 return tensor[index];
                             },
                             cast<Tensor>(tensor_handle), index);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jdouble JNICALL Java_space_kscience_kmath_noa_JNoa_getDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index) {
    auto i = env->GetIntArrayElements(index, nullptr);
    const auto res =
            safe_run<double>(env,
                             getter<double>,
                             cast<Tensor>(tensor_handle), i);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
    return res.has_value() ? res.value() : 0.;
}

JNIEXPORT jfloat JNICALL Java_space_kscience_kmath_noa_JNoa_getFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index) {
    auto i = env->GetIntArrayElements(index, nullptr);
    const auto res =
            safe_run<float>(env,
                            getter<float>,
                            cast<Tensor>(tensor_handle), i);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
    return res.has_value() ? res.value() : 0.f;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getLong
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index) {
    auto i = env->GetIntArrayElements(index, nullptr);
    const auto res =
            safe_run<long>(env,
                           getter<long>,
                           cast<Tensor>(tensor_handle), i);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
    return res.has_value() ? res.value() : 0L;
}

JNIEXPORT jint JNICALL Java_space_kscience_kmath_noa_JNoa_getInt
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index) {
    auto i = env->GetIntArrayElements(index, nullptr);
    const auto res =
            safe_run<int>(env,
                          getter<int>,
                          cast<Tensor>(tensor_handle), i);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
    return res.has_value() ? res.value() : 0;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index, jdouble value) {
    auto i = env->GetIntArrayElements(index, nullptr);
    safe_run(env,
             setter<double>,
             cast<Tensor>(tensor_handle), i, value);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index, jfloat value) {
    auto i = env->GetIntArrayElements(index, nullptr);
    safe_run(env,
             setter<float>,
             cast<Tensor>(tensor_handle), i, value);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setLong
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index, jlong value) {
    auto i = env->GetIntArrayElements(index, nullptr);
    safe_run(env,
             setter<long>,
             cast<Tensor>(tensor_handle), i, value);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setInt
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray index, jint value) {
    auto i = env->GetIntArrayElements(index, nullptr);
    safe_run(env,
             setter<int>,
             cast<Tensor>(tensor_handle), i, value);
    env->ReleaseIntArrayElements(index, i, JNI_ABORT);
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randDouble
        (JNIEnv *env, jclass, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_uniform<double>,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randnDouble
        (JNIEnv *env, jclass, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_normal<double>,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randFloat
        (JNIEnv *env, jclass, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_uniform<float>,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randnFloat
        (JNIEnv *env, jclass, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_normal<float>,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randintDouble
        (JNIEnv *env, jclass, jlong low, jlong high, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_discrete<double>,
                             low, high,
                             to_shape(env, shape),
                             int_to_device(device)
            );
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randintFloat
        (JNIEnv *env, jclass, jlong low, jlong high, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_discrete<float>,
                             low, high,
                             to_shape(env, shape),
                             int_to_device(device)
            );
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randintLong
        (JNIEnv *env, jclass, jlong low, jlong high, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_discrete<long>,
                             low, high,
                             to_shape(env, shape),
                             int_to_device(device)
            );
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randintInt
        (JNIEnv *env, jclass, jlong low, jlong high, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             rand_discrete<int>,
                             low, high,
                             to_shape(env, shape),
                             int_to_device(device)
            );
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randLike
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(torch::rand_like(cast<Tensor>(tensor_handle)));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_randLikeAssign
        (JNIEnv *, jclass, jlong tensor_handle) {
    cast<Tensor>(tensor_handle) = torch::rand_like(cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randnLike
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(torch::randn_like(cast<Tensor>(tensor_handle)));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_randnLikeAssign
        (JNIEnv *, jclass, jlong tensor_handle) {
    cast<Tensor>(tensor_handle) = torch::randn_like(cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_randintLike
        (JNIEnv *, jclass, jlong low, jlong high, jlong tensor_handle) {
    return (long) new Tensor(torch::randint_like(cast<Tensor>(tensor_handle), low, high));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_randintLikeAssign
        (JNIEnv *, jclass, jlong low, jlong high, jlong tensor_handle) {
    cast<Tensor>(tensor_handle) = torch::randint_like(cast<Tensor>(tensor_handle), low, high);
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fullDouble
        (JNIEnv *env, jclass, jdouble value, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             full<double>,
                             value,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fullFloat
        (JNIEnv *env, jclass, jfloat value, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             full<float>,
                             value,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fullLong
        (JNIEnv *env, jclass, jlong value, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             full<long>,
                             value,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_fullInt
        (JNIEnv *env, jclass, jint value, jintArray shape, jint device) {
    const auto res =
            safe_run<Tensor>(env,
                             full<int>,
                             value,
                             to_shape(env, shape),
                             int_to_device(device));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_timesDouble
        (JNIEnv *, jclass, jdouble value, jlong other) {
    return (long) new Tensor(value * cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_timesFloat
        (JNIEnv *, jclass, jfloat value, jlong other) {
    return (long) new Tensor(value * cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_timesLong
        (JNIEnv *, jclass, jlong value, jlong other) {
    return (long) new Tensor(value * cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_timesInt
        (JNIEnv *, jclass, jint value, jlong other) {
    return (long) new Tensor(value * cast<Tensor>(other));
}

JNIEXPORT void JNICALL
Java_space_kscience_kmath_noa_JNoa_timesDoubleAssign
        (JNIEnv *, jclass, jdouble value, jlong other) {
    cast<Tensor>(other) *= value;
}

JNIEXPORT void JNICALL
Java_space_kscience_kmath_noa_JNoa_timesFloatAssign
        (JNIEnv *, jclass, jfloat value, jlong other) {
    cast<Tensor>(other) *= value;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_timesLongAssign
        (JNIEnv *, jclass, jlong value, jlong other) {
    cast<Tensor>(other) *= value;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_timesIntAssign
        (JNIEnv *, jclass, jint value, jlong other) {
    cast<Tensor>(other) *= value;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_plusDouble
        (JNIEnv *, jclass, jdouble value, jlong other) {
    return (long) new Tensor(value + cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_plusFloat
        (JNIEnv *, jclass, jfloat value, jlong other) {
    return (long) new Tensor(value + cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_plusLong
        (JNIEnv *, jclass, jlong value, jlong other) {
    return (long) new Tensor(value + cast<Tensor>(other));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_plusInt
        (JNIEnv *, jclass, jint value, jlong other) {
    return (long) new Tensor(value + cast<Tensor>(other));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_plusDoubleAssign
        (JNIEnv *, jclass, jdouble value, jlong other) {
    cast<Tensor>(other) += value;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_plusFloatAssign
        (JNIEnv *, jclass, jfloat value, jlong other) {
    cast<Tensor>(other) += value;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_plusLongAssign
        (JNIEnv *, jclass, jlong value, jlong other) {
    cast<Tensor>(other) += value;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_plusIntAssign
        (JNIEnv *, jclass, jint value, jlong other) {
    cast<Tensor>(other) += value;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_timesTensor
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &lhs, const auto &rhs) {
                                 return lhs * rhs;
                             },
                             cast<Tensor>(lhs),
                             cast<Tensor>(rhs));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_timesTensorAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](auto &lhs, const auto &rhs) {
                 lhs *= rhs;
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_divTensor
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &lhs, const auto &rhs) {
                                 return lhs / rhs;
                             },
                             cast<Tensor>(lhs),
                             cast<Tensor>(rhs));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_divTensorAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](auto &lhs, const auto &rhs) {
                 lhs /= rhs;
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_plusTensor
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &lhs, const auto &rhs) {
                                 return lhs + rhs;
                             },
                             cast<Tensor>(lhs),
                             cast<Tensor>(rhs));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_plusTensorAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](auto &lhs, const auto &rhs) {
                 lhs += rhs;
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_minusTensor
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &lhs, const auto &rhs) {
                                 return lhs - rhs;
                             },
                             cast<Tensor>(lhs),
                             cast<Tensor>(rhs));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_minusTensorAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](auto &lhs, const auto &rhs) {
                 lhs -= rhs;
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_unaryMinus
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(-cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_transposeTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jint j) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const int j) {
                                 return tensor.transpose(i, j);
                             },
                             cast<Tensor>(tensor_handle),
                             i, j);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_absTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).abs());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_expTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).exp());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_lnTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).log());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sqrtTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).sqrt());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_cosTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).cos());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_acosTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).acos());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_coshTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).cosh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_acoshTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).acosh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sinTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).sin());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sinhTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).sinh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_asinhTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).asinh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_tanTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).tan());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_atanTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).atan());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_tanhTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).tanh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_atanhTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).atanh());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_ceilTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).ceil());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_floorTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).floor());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sumTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).sum());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sumDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return tensor.sum(i, keep);
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}


JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_minTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).min());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_minDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return std::get<0>(torch::min(tensor, i, keep));
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_maxTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).max());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_maxDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return std::get<0>(torch::max(tensor, i, keep));
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_meanTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).mean());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_meanDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return tensor.mean(i, keep);
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_stdTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).std());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_stdDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return tensor.std(i, keep);
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}


JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_varTensor
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).var());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_varDimTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return tensor.var(i, keep);
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_argMaxTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint dim, jboolean keep) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const bool keep) {
                                 return tensor.argmax(i, keep);
                             },
                             cast<Tensor>(tensor_handle),
                             dim, keep);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_flattenTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jint j) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor, const int i, const int j) {
                                 return tensor.flatten(i, j);
                             },
                             cast<Tensor>(tensor_handle),
                             i, j);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_matmul
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &lhs, const auto &rhs) {
                                 return lhs.matmul(rhs);
                             },
                             cast<Tensor>(lhs),
                             cast<Tensor>(rhs));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_matmulAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](auto &lhs, const auto &rhs) {
                 lhs = lhs.matmul(rhs);
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_matmulRightAssign
        (JNIEnv *env, jclass, jlong lhs, jlong rhs) {
    safe_run(env,
             [](const auto &lhs, auto &rhs) {
                 rhs = lhs.matmul(rhs);
             },
             cast<Tensor>(lhs),
             cast<Tensor>(rhs));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_diagEmbed
        (JNIEnv *env, jclass, jlong diags_handle, jint offset, jint dim1, jint dim2) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &diag_tensor, const int offset,
                                const int dim1, const int dim2) {
                                 return torch::diag_embed(diag_tensor, offset, dim1, dim2);
                             },
                             cast<Tensor>(diags_handle),
                             offset, dim1, dim2);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_detTensor
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor) {
                                 return torch::linalg::det(tensor);
                             },
                             cast<Tensor>(tensor_handle));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_invTensor
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor) {
                                 return torch::linalg::inv(tensor);
                             },
                             cast<Tensor>(tensor_handle));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_choleskyTensor
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor) {
                                 return torch::linalg::cholesky(tensor);
                             },
                             cast<Tensor>(tensor_handle));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_qrTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jlong Q_handle, jlong R_handle) {
    const auto res =
            safe_run<TensorPair>(env,
                                 [](const auto &tensor) {
                                     return torch::qr(tensor);
                                 },
                                 cast<Tensor>(tensor_handle));
    if (res.has_value()) {
        const auto &[Q, R] = res.value();
        cast<Tensor>(Q_handle) = Q;
        cast<Tensor>(R_handle) = R;
    }
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_luTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jlong P_handle, jlong L_handle, jlong U_handle) {
    const auto res =
            safe_run<TensorTriple>(env,
                                   [](const auto &tensor) {
                                       const auto[LU, pivots, _] = torch::_lu_with_info(tensor);
                                       return torch::lu_unpack(LU, pivots);
                                   },
                                   cast<Tensor>(tensor_handle));
    if (res.has_value()) {
        const auto &[P, L, U] = res.value();
        cast<Tensor>(P_handle) = P;
        cast<Tensor>(L_handle) = L;
        cast<Tensor>(U_handle) = U;
    }
}


JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_svdTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jlong U_handle, jlong S_handle, jlong V_handle) {
    const auto res =
            safe_run<TensorTriple>(env,
                                   [](const auto &tensor) {
                                       return torch::svd(tensor);
                                   },
                                   cast<Tensor>(tensor_handle));
    if (res.has_value()) {
        const auto &[U, S, V] = res.value();
        cast<Tensor>(U_handle) = U;
        cast<Tensor>(S_handle) = S;
        cast<Tensor>(V_handle) = V;
    }
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_symEigTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jlong S_handle, jlong V_handle) {
    const auto res =
            safe_run<TensorPair>(env,
                                 [](const auto &tensor) {
                                     return torch::linalg::eigh(tensor, "L");
                                 },
                                 cast<Tensor>(tensor_handle));
    if (res.has_value()) {
        const auto &[S, V] = res.value();
        cast<Tensor>(S_handle) = S;
        cast<Tensor>(V_handle) = V;
    }
}

JNIEXPORT jboolean JNICALL Java_space_kscience_kmath_noa_JNoa_requiresGrad
        (JNIEnv *, jclass, jlong tensor_handle) {
    return cast<Tensor>(tensor_handle).requires_grad();
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setRequiresGrad
        (JNIEnv *, jclass, jlong tensor_handle, jboolean status) {
    cast<Tensor>(tensor_handle).requires_grad_(status);
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_detachFromGraph
        (JNIEnv *, jclass, jlong tensor_handle) {
    return (long) new Tensor(cast<Tensor>(tensor_handle).detach());
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_autoGradTensor
        (JNIEnv *env, jclass, jlong value, jlong variable, jboolean retain_graph) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &value, const auto &variable, const bool retain) {
                                 return torch::autograd::grad(
                                         {value}, {variable}, {}, retain)[0];
                             },
                             cast<Tensor>(value),
                             cast<Tensor>(variable),
                             retain_graph);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_autoHessTensor
        (JNIEnv *env, jclass, jlong value, jlong variable) {
    const auto res =
            safe_run<Tensor>(env,
                             hess,
                             cast<Tensor>(value),
                             cast<Tensor>(variable));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_backwardPass
        (JNIEnv *env, jclass, jlong tensor_handle) {
    safe_run(env,
             [](const auto &tensor) {
                 tensor.backward();
             },
             cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_tensorGrad
        (JNIEnv *env, jclass, jlong tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](const auto &tensor) {
                                 return tensor.grad();
                             },
                             cast<Tensor>(tensor_handle));
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeJitModule
        (JNIEnv *, jclass, jlong jit_module_handle) {
    if (jit_module_handle != 0L)
        dispose<JitModule>(jit_module_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_trainMode
        (JNIEnv *, jclass, jlong jit_module_handle, jboolean status) {
    cast<JitModule>(jit_module_handle).jit_module.train(status);
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadJitModuleDouble
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<JitModule>(env, load_jit_module,
                                to_string(env, jpath),
                                torch::kDouble, int_to_device(device));

    return res.has_value() ? (long) new JitModule(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadJitModuleFloat
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<JitModule>(env, load_jit_module,
                                to_string(env, jpath),
                                torch::kFloat, int_to_device(device));

    return res.has_value() ? (long) new JitModule(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadJitModuleLong
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<JitModule>(env, load_jit_module,
                                to_string(env, jpath),
                                torch::kLong, int_to_device(device));

    return res.has_value() ? (long) new JitModule(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadJitModuleInt
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<JitModule>(env, load_jit_module,
                                to_string(env, jpath),
                                torch::kInt, int_to_device(device));

    return res.has_value() ? (long) new JitModule(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_forwardPass
        (JNIEnv *env, jclass, jlong jit_module_handle, jlong tensor_handle) {
    const auto res =
            safe_run<Tensor>(env,
                             [](auto &module, const auto &tensor) {
                                 return module.jit_module({tensor}).toTensor();
                             },
                             cast<JitModule>(jit_module_handle),
                             cast<Tensor>(tensor_handle)
            );

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_forwardPassAssign
        (JNIEnv *env, jclass, jlong jit_module_handle, jlong feature_handle, jlong preds_handle) {
    safe_run(env,
             [](auto &module, const auto &features, auto &predictions) {
                 predictions = module.jit_module({features}).toTensor();
             },
             cast<JitModule>(jit_module_handle),
             cast<Tensor>(feature_handle),
             cast<Tensor>(preds_handle)
    );
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getModuleParameter
        (JNIEnv *env, jclass, jlong jit_module_handle, jstring name) {
    const auto res =
            safe_run<Tensor>(env,
                             [](JitModule &module, const auto &name) {
                                 return module.get_parameter(name);
                             },
                             cast<JitModule>(jit_module_handle),
                             to_string(env, name)
            );

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setModuleParameter
        (JNIEnv *env, jclass, jlong jit_module_handle, jstring name, jlong tensor_handle) {
    safe_run(env,
             [](JitModule &module, const auto &name, const Tensor &tensor) {
                 module.get_parameter(name).set_data(tensor);
             },
             cast<JitModule>(jit_module_handle),
             to_string(env, name),
             cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getModuleBuffer
        (JNIEnv *env, jclass, jlong jit_module_handle, jstring name) {
    const auto res =
            safe_run<Tensor>(env,
                             [](JitModule &module, const auto &name) {
                                 return module.get_buffer(name);
                             },
                             cast<JitModule>(jit_module_handle),
                             to_string(env, name)
            );

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setModuleBuffer
        (JNIEnv *env, jclass, jlong jit_module_handle, jstring name, jlong tensor_handle) {
    safe_run(env,
             [](JitModule &module, const auto &name, const Tensor &tensor) {
                 module.get_buffer(name).set_data(tensor);
             },
             cast<JitModule>(jit_module_handle),
             to_string(env, name),
             cast<Tensor>(tensor_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_adamOptim
        (JNIEnv *, jclass, jlong jit_module_handle, jdouble learning_rate) {
    return (long) new AdamOptim(get_optim<AdamOptim, AdamOptimOpts>(
            cast<JitModule>(jit_module_handle), learning_rate));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeAdamOptim
        (JNIEnv *, jclass, jlong adam_optim_handle) {
    if (adam_optim_handle != 0L)
        dispose<AdamOptim>(adam_optim_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_stepAdamOptim
        (JNIEnv *env, jclass, jlong adam_optim_handle) {
    safe_run(env,
             [](AdamOptim &adam_optim) {
                 adam_optim.step();
             },
             cast<AdamOptim>(adam_optim_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_zeroGradAdamOptim
        (JNIEnv *, jclass, jlong adam_optim_handle) {
    cast<AdamOptim>(adam_optim_handle).zero_grad();
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_rmsOptim
        (JNIEnv *, jclass, jlong jit_module_handle, jdouble learning_rate, jdouble alpha, 
        jdouble eps, jdouble weight_decay, jdouble momentum, jboolean centered) {
    return (long) new RmsOptim(get_rms_optim<RmsOptim, RmsOptimOpts>(
            cast<JitModule>(jit_module_handle), learning_rate, alpha, 
            eps, weight_decay, momentum, centered));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeRmsOptim
        (JNIEnv *, jclass, jlong rms_optim_handle) {
    if (rms_optim_handle != 0L)
        dispose<RmsOptim>(rms_optim_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_stepRmsOptim
        (JNIEnv *env, jclass, jlong rms_optim_handle) {
    safe_run(env,
             [](RmsOptim &rms_optim) {
                 rms_optim.step();
             },
             cast<RmsOptim>(rms_optim_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_zeroGradRmsOptim
        (JNIEnv *, jclass, jlong rms_optim_handle) {
    cast<RmsOptim>(rms_optim_handle).zero_grad();
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_adamWOptim
        (JNIEnv *, jclass, jlong jit_module_handle, jdouble learning_rate, jdouble beta1,
        jdouble beta2, jdouble eps, jdouble weight_decay, jboolean amsgrad) {
    return (long) new AdamWOptim(get_adamw_optim<AdamWOptim, AdamWOptimOpts>(
            cast<JitModule>(jit_module_handle), learning_rate, beta1,
            beta2, eps, weight_decay, amsgrad));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeAdamWOptim
        (JNIEnv *, jclass, jlong adamW_optim_handle) {
    if (adamW_optim_handle != 0L)
        dispose<AdamWOptim>(adamW_optim_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_stepAdamWOptim
        (JNIEnv *env, jclass, jlong adamW_optim_handle) {
    safe_run(env,
             [](AdamWOptim &adamW_optim) {
                 adamW_optim.step();
             },
             cast<AdamWOptim>(adamW_optim_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_zeroGradAdamWOptim
        (JNIEnv *, jclass, jlong adamW_optim_handle) {
    cast<AdamWOptim>(adamW_optim_handle).zero_grad();
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_adagradOptim
        (JNIEnv *, jclass, jlong jit_module_handle, jdouble learning_rate, jdouble weight_decay,
        jdouble lr_decay, jdouble initial_accumulator_value, jdouble eps) {
    return (long) new AdagradOptim(get_adagrad_optim<AdagradOptim, AdagradOptimOpts>(
            cast<JitModule>(jit_module_handle), learning_rate, weight_decay,
        lr_decay, initial_accumulator_value, eps));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeAdagradOptim
        (JNIEnv *, jclass, jlong adagrad_optim_handle) {
    if (adagrad_optim_handle != 0L)
        dispose<AdagradOptim>(adagrad_optim_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_stepAdagradOptim
        (JNIEnv *env, jclass, jlong adagrad_optim_handle) {
    safe_run(env,
             [](AdagradOptim &adagrad_optim) {
                 adagrad_optim.step();
             },
             cast<AdagradOptim>(adagrad_optim_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_zeroGradAdagradOptim
        (JNIEnv *, jclass, jlong adagrad_optim_handle) {
    cast<AdagradOptim>(adagrad_optim_handle).zero_grad();
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_sgdOptim
        (JNIEnv *, jclass, jlong jit_module_handle, jdouble learning_rate, jdouble momentum,
        jdouble dampening, jdouble weight_decay, jboolean nesterov) {
    return (long) new SgdOptim(get_sgd_optim<SgdOptim, SgdOptimOpts>(
            cast<JitModule>(jit_module_handle), learning_rate, momentum,
            dampening, weight_decay, nesterov));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_disposeSgdOptim
        (JNIEnv *, jclass, jlong sgd_optim_handle) {
    if (sgd_optim_handle != 0L)
        dispose<SgdOptim>(sgd_optim_handle);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_stepSgdOptim
        (JNIEnv *env, jclass, jlong sgd_optim_handle) {
    safe_run(env,
             [](SgdOptim &sgd_optim) {
                 sgd_optim.step();
             },
             cast<SgdOptim>(sgd_optim_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_zeroGradSgdOptim
        (JNIEnv *, jclass, jlong sgd_optim_handle) {
    cast<SgdOptim>(sgd_optim_handle).zero_grad();
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_swapTensors
        (JNIEnv *, jclass, jlong lhs_handle, jlong rhs_handle) {
    std::swap(cast<Tensor>(lhs_handle), cast<Tensor>(rhs_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadTensorDouble
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<Tensor>(env, unsafe_load_tensor,
                             to_string(env, jpath),
                             torch::kDouble, int_to_device(device));

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadTensorFloat
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<Tensor>(env, unsafe_load_tensor,
                             to_string(env, jpath),
                             torch::kFloat, int_to_device(device));

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadTensorLong
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<Tensor>(env, unsafe_load_tensor,
                             to_string(env, jpath),
                             torch::kLong, int_to_device(device));

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_loadTensorInt
        (JNIEnv *env, jclass, jstring jpath, jint device) {
    const auto res =
            safe_run<Tensor>(env, unsafe_load_tensor,
                             to_string(env, jpath),
                             torch::kInt, int_to_device(device));

    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_saveTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jstring jpath) {
    safe_run(env,
             [](const Tensor &tensor, const auto &path) {
                 torch::save(tensor, path);
             },
             cast<Tensor>(tensor_handle),
             to_string(env, jpath));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_saveJitModule
        (JNIEnv *env, jclass, jlong jit_module_handle, jstring jpath) {
    safe_run(env,
             [](const JitModule &module, const auto &path) {
                 module.jit_module.save(path);
             },
             cast<JitModule>(jit_module_handle),
             to_string(env, jpath));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_assignBlobDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jdoubleArray array) {
    auto data = env->GetDoubleArrayElements(array, nullptr);
    safe_run(env,
             assign_blob<double>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseDoubleArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_assignBlobFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jfloatArray array) {
    auto data = env->GetFloatArrayElements(array, nullptr);
    safe_run(env,
             assign_blob<float>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseFloatArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_assignBlobLong
        (JNIEnv *env, jclass, jlong tensor_handle, jlongArray array) {
    auto data = env->GetLongArrayElements(array, nullptr);
    safe_run(env,
             assign_blob<long>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseLongArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_assignBlobInt
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray array) {
    auto data = env->GetIntArrayElements(array, nullptr);
    safe_run(env,
             assign_blob<int>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseIntArrayElements(array, data, JNI_ABORT);

}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setBlobDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jdoubleArray array) {
    auto data = env->GetDoubleArrayElements(array, nullptr);
    safe_run(env,
             set_blob<double>,
             cast<Tensor>(tensor_handle), i, data);
    env->ReleaseDoubleArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setBlobFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jfloatArray array) {
    auto data = env->GetFloatArrayElements(array, nullptr);
    safe_run(env,
             set_blob<float>,
             cast<Tensor>(tensor_handle), i, data);
    env->ReleaseFloatArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setBlobLong
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jlongArray array) {
    auto data = env->GetLongArrayElements(array, nullptr);
    safe_run(env,
             set_blob<long>,
             cast<Tensor>(tensor_handle), i, data);
    env->ReleaseLongArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setBlobInt
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jintArray array) {
    auto data = env->GetIntArrayElements(array, nullptr);
    safe_run(env,
             set_blob<int>,
             cast<Tensor>(tensor_handle), i, data);
    env->ReleaseIntArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_getBlobDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jdoubleArray array) {
    auto data = env->GetDoubleArrayElements(array, nullptr);
    safe_run(env,
             get_blob<double>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseDoubleArrayElements(array, data, JNI_COMMIT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_getBlobFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jfloatArray array) {
    auto data = env->GetFloatArrayElements(array, nullptr);
    safe_run(env,
             get_blob<float>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseFloatArrayElements(array, data, JNI_COMMIT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_getBlobLong
        (JNIEnv *env, jclass, jlong tensor_handle, jlongArray array) {
    auto data = env->GetLongArrayElements(array, nullptr);
    safe_run(env,
             get_blob<long>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseLongArrayElements(array, data, JNI_COMMIT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_getBlobInt
        (JNIEnv *env, jclass, jlong tensor_handle, jintArray array) {
    auto data = env->GetIntArrayElements(array, nullptr);
    safe_run(env,
             get_blob<int>,
             cast<Tensor>(tensor_handle), data);
    env->ReleaseIntArrayElements(array, data, JNI_COMMIT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint i, jlong value_handle) {
    safe_run(env,
             [](Tensor &tensor, int i, const Tensor &value) {
                 tensor[i] = value;
             },
             cast<Tensor>(tensor_handle), i, cast<Tensor>(value_handle));
}

JNIEXPORT jlong JNICALL Java_space_kscience_kmath_noa_JNoa_getSliceTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e) {
    const auto res =
            safe_run<Tensor>(env, [](const Tensor &tensor, int d, int s, int e) {
                return tensor.slice(d, s, e);
            }, cast<Tensor>(tensor_handle), d, s, e);
    return res.has_value() ? (long) new Tensor(res.value()) : 0L;
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSliceTensor
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e, jlong value_handle) {
    safe_run(env,
             [](Tensor &tensor, int d, int s, int e, const Tensor &value) {
                 tensor.slice(d, s, e) = value;
             },
             cast<Tensor>(tensor_handle), d, s, e, cast<Tensor>(value_handle));
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSliceBlobDouble
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e, jdoubleArray array) {
    auto data = env->GetDoubleArrayElements(array, nullptr);
    safe_run(env,
             set_slice_blob<double>,
             cast<Tensor>(tensor_handle), d, s, e, data);
    env->ReleaseDoubleArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSliceBlobFloat
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e, jfloatArray array) {
    auto data = env->GetFloatArrayElements(array, nullptr);
    safe_run(env,
             set_slice_blob<float>,
             cast<Tensor>(tensor_handle), d, s, e, data);
    env->ReleaseFloatArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSliceBlobLong
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e, jlongArray array) {
    auto data = env->GetLongArrayElements(array, nullptr);
    safe_run(env,
             set_slice_blob<long>,
             cast<Tensor>(tensor_handle), d, s, e, data);
    env->ReleaseLongArrayElements(array, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_space_kscience_kmath_noa_JNoa_setSliceBlobInt
        (JNIEnv *env, jclass, jlong tensor_handle, jint d, jint s, jint e, jintArray array) {
    auto data = env->GetIntArrayElements(array, nullptr);
    safe_run(env,
             set_slice_blob<int>,
             cast<Tensor>(tensor_handle), d, s, e, data);
    env->ReleaseIntArrayElements(array, data, JNI_ABORT);
}


