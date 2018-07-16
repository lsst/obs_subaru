# ybackground
# Copyright (C) 2017  Sogo Mineo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
CDF 9/7 wavelet transformation

See
   T.800: JPEG2000 image coding system: Core coding system
       https://www.itu.int/rec/T-REC-T.800
   OpenJPEG
       https://github.com/uclouvain/openjpeg
"""

import numpy

import itertools

# From Table F.4 in ITU-T Rec. T.800
# Definition of lifting parameters for the 9-7 irreversible filter
alpha = -1.586134342059924
beta = -0.052980118572961
gamma = 0.882911075530934
delta = 0.443506852043971

# "K" determines the normalization of biorthogonal filters.
# JPEG2000 uses the normalization:
#    H(0) = \sum_i h[i] = 1, \tilde{H}(0) = \sum_i \tilde{h}[i] = 2
# where h is the analysis low-pass filter, and \tilde{h} is the synthesis low-pass filter.
# The value K = 1.230174104914001 is for this normalization.
#
# We use the normalization:
#    H(0) = \sqrt{2}, \tilde{H}(0) = \sqrt{2}.
# For this normalization, K must be the following value.
K = 0.86986445162477855


def cdf_9_7(data, level):
    """
    Multi-dimensional forward wavelet transformation
    with the Cohen-Daubechies-Feauveau wavelet 9/7

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.
        If you want `data` kept, copy it before calling this function:
            cdf_9_7(numpy.copy(data), level)

    @param level (int or tuple)
        Level of wavelet analysis.
        If a tuple is given, its length must agree with the rank of `data`,
        and level[i] is to be the level for the i-th axis.

    @return (numpy.ndarray)
        Result of multiresolution analysis.
    """
    data = numpy.asarray(data, dtype=float)
    if numpy.asarray(level, dtype=int).shape == ():
        level = itertools.repeat(level)

    for axis, lev in zip(range(len(data.shape)), level):
        data = cdf_9_7_1d(data, lev, axis)

    return data


def cdf_9_7_1d(data, level, axis=0):
    """
    One-dimensional forward wavelet transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @param level (int)
        Level of wavelet analysis.

    @param axis (int)

    @return (numpy.ndarray)
        Result of multiresolution analysis.
    """
    data = numpy.asarray(data, dtype=float)
    data = numpy.swapaxes(data, 0, axis)
    size = len(data)

    for i in range(level):
        data[:size] = _cdf_9_7_1d_level1(data[:size])
        size = (size + 1)//2

    return numpy.swapaxes(data, 0, axis)


def _cdf_9_7_1d_level1(data):
    """
    Level 1 one-dimensional forward wavelet transformation.
    The 0th axis is used in the transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @return (numpy.ndarray)
        Result of transformation.
    """
    size = len(data)
    if size <= 1:
        return data

    isEven = (size % 2 == 0)

    # From equations (F-11) in ITU-T Rec. T.800

    # predict (1)
    data[1:-1:2] += alpha * (data[0:-2:2] + data[2::2])
    if isEven:
        data[-1] += 2*alpha * data[-2]

    # update (1)
    data[0] += 2*beta * data[1]
    data[2:-1:2] += beta * (data[1:-2:2] + data[3::2])
    if not isEven:
        data[-1] += 2*beta * data[-2]

    # predict (2)
    data[1:-1:2] += gamma * (data[0:-2:2] + data[2::2])
    if isEven:
        data[-1] += 2*gamma * data[-2]

    # update (2)
    data[0] += 2*delta * data[1]
    data[2:-1:2] += delta * (data[1:-2:2] + data[3::2])
    if not isEven:
        data[-1] += 2*delta * data[-2]

    # de-interleave
    scaling_size = (size + 1) // 2
    ret = numpy.empty_like(data)
    ret[:scaling_size] = (1/K) * data[0::2]
    ret[scaling_size:] = K * data[1::2]

    return ret


def icdf_9_7(data, level):
    """
    Multi-dimensional backword wavelet transformation
    with the Cohen-Daubechies-Feauveau wavelet 9/7

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.
        If you want `data` kept, copy it before calling this function:
            icdf_9_7(numpy.copy(data), level)

    @param level (int or tuple)
        Level of wavelet analysis.
        If a tuple is given, its length must agree with the rank of `data`,
        and level[i] is to be the level for the i-th axis.

    @return (numpy.ndarray)
        Result of multiresolution synthesis.
    """
    data = numpy.asarray(data, dtype=float)
    if numpy.asarray(level, dtype=int).shape == ():
        level = itertools.repeat(level)

    for axis, lev in zip(range(len(data.shape)), level):
        data = icdf_9_7_1d(data, lev, axis)

    return data


def icdf_9_7_1d(data, level, axis=0):
    """
    One-dimensional backword wavelet transformation.
    The 0th axis is used in the transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @param level (int)
        Level of wavelet analysis.

    @param axis (int)

    @return (numpy.ndarray)
        Result of multiresolution synthesis.
    """
    data = numpy.asarray(data, dtype=float)
    data = numpy.swapaxes(data, 0, axis)
    size = len(data)
    sizes = []
    for i in range(level):
        sizes.append(size)
        size = (size + 1) // 2

    for size in reversed(sizes):
        data[:size] = _icdf_9_7_1d_level1(data[:size])

    return numpy.swapaxes(data, 0, axis)


def _icdf_9_7_1d_level1(data):
    """
    Level 1 one-dimensional backword wavelet transformation.
    The 0th axis is used in the transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @return (numpy.ndarray)
        Result of transformation.
    """
    size = len(data)
    if size <= 1:
        return data

    isEven = (size % 2 == 0)

    # interleave
    scaling_size = (size + 1) // 2
    ret = data
    data = numpy.empty_like(ret)
    data[0::2] = K * ret[:scaling_size]
    data[1::2] = (1/K) * ret[scaling_size:]

    # From equations (F-7) in ITU-T Rec. T.800

    # update (2)
    data[0] -= 2*delta * data[1]
    data[2:-1:2] -= delta * (data[1:-2:2] + data[3::2])
    if not isEven:
        data[-1] -= 2*delta * data[-2]

    # predict (2)
    data[1:-1:2] -= gamma * (data[0:-2:2] + data[2::2])
    if isEven:
        data[-1] -= 2*gamma * data[-2]

    # update (1)
    data[0] -= 2*beta * data[1]
    data[2:-1:2] -= beta * (data[1:-2:2] + data[3::2])
    if not isEven:
        data[-1] -= 2*beta * data[-2]

    # predict (1)
    data[1:-1:2] -= alpha * (data[0:-2:2] + data[2::2])
    if isEven:
        data[-1] -= 2*alpha * data[-2]

    return data


def periodic_cdf_9_7_1d(data, level, axis=0):
    """
    One-dimensional forward wavelet transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.
        The size of this data must be divisible by 2**level.

    @param level (int)
        Level of wavelet analysis.

    @param axis (int)

    @return (numpy.ndarray)
        Result of multiresolution analysis.
    """
    data = numpy.asarray(data, dtype=float)
    data = numpy.swapaxes(data, 0, axis)
    size = len(data)

    if size % (2**level) != 0:
        raise ValueError("Size must be divisible by 2**level.")

    for i in range(level):
        data[:size] = _periodic_cdf_9_7_1d_level1(data[:size])
        size = (size + 1) // 2

    return numpy.swapaxes(data, 0, axis)


def _periodic_cdf_9_7_1d_level1(data):
    """
    Level 1 one-dimensional forward wavelet transformation.
    The 0th axis is used in the transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @return (numpy.ndarray)
        Result of transformation.
    """
    size = len(data)
    if size <= 1:
        return data

    assert(size % 2 == 0)

    # From equations (F-11) in ITU-T Rec. T.800

    # predict (1)
    data[1:-1:2] += alpha * (data[0:-2:2] + data[2::2])
    data[-1] += alpha * (data[0] + data[-2])

    # update (1)
    data[0] += beta * (data[1] + data[-1])
    data[2:-1:2] += beta * (data[1:-2:2] + data[3::2])

    # predict (2)
    data[1:-1:2] += gamma * (data[0:-2:2] + data[2::2])
    data[-1] += gamma * (data[0] + data[-2])

    # update (2)
    data[0] += delta * (data[1] + data[-1])
    data[2:-1:2] += delta * (data[1:-2:2] + data[3::2])

    # de-interleave
    scaling_size = (size + 1) // 2
    ret = numpy.empty_like(data)
    ret[:scaling_size] = (1/K) * data[0::2]
    ret[scaling_size:] = K * data[1::2]

    return ret


def periodic_icdf_9_7_1d(data, level, axis=0):
    """
    One-dimensional backword wavelet transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.
        The size of this data must be divisible by 2**level.

    @param level (int)
        Level of wavelet analysis.

    @param axis (int)

    @return (numpy.ndarray)
        Result of multiresolution synthesis.
    """
    data = numpy.asarray(data, dtype=float)
    data = numpy.swapaxes(data, 0, axis)
    size = len(data)

    if size % (2**level) != 0:
        raise ValueError("Size must be divisible by 2**level.")

    sizes = []
    for i in range(level):
        sizes.append(size)
        size = (size + 1) // 2

    for size in reversed(sizes):
        data[:size] = _periodic_icdf_9_7_1d_level1(data[:size])

    return numpy.swapaxes(data, 0, axis)


def _periodic_icdf_9_7_1d_level1(data):
    """
    Level 1 one-dimensional backword wavelet transformation.
    The 0th axis is used in the transformation.

    @param data (numpy.ndarray)
        N-dimensional array (N >= 1). This array will be *destroyed*.

    @return (numpy.ndarray)
        Result of transformation.
    """
    size = len(data)
    if size <= 1:
        return data

    assert(size % 2 == 0)

    # interleave
    scaling_size = (size + 1) // 2
    ret = data
    data = numpy.empty_like(ret)
    data[0::2] = K * ret[:scaling_size]
    data[1::2] = (1/K) * ret[scaling_size:]

    # From equations (F-7) in ITU-T Rec. T.800

    # update (2)
    data[0] -= delta * (data[1] + data[-1])
    data[2:-1:2] -= delta * (data[1:-2:2] + data[3::2])

    # predict (2)
    data[1:-1:2] -= gamma * (data[0:-2:2] + data[2::2])
    data[-1] -= gamma * (data[0] + data[-2])

    # update (1)
    data[0] -= beta * (data[1] + data[-1])
    data[2:-1:2] -= beta * (data[1:-2:2] + data[3::2])

    # predict (1)
    data[1:-1:2] -= alpha * (data[0:-2:2] + data[2::2])
    data[-1] -= alpha * (data[0] + data[-2])

    return data


def scaled_size(size, level):
    """
    Get the size of the level-n average portion
    (as opposed to the detail portion).
    @param size (int or tuple of int)
        Size of the original vector
    @param level (int or tuple of int)
        Level of wavelet transformation
    """
    size = numpy.asarray(size, dtype=int)
    level = numpy.asarray(level, dtype=int)
    return (size + (2**level - 1)) // (2**level)
