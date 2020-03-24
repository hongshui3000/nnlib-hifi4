/*******************************************************************************
* Copyright (c) 2018-2020 Cadence Design Systems, Inc.
* 
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to use this Software with Cadence processor cores only and 
* not with any other processors and platforms, subject to
* the following conditions:
* 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************/
/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// See the header for documentation on the meaning of this data.

#include "tensorflow/lite/experimental/micro/examples/micro_speech/yes_30ms_sample_data.h"

const int g_yes_30ms_sample_data_size = 480;
const int16_t g_yes_30ms_sample_data[480] = {
    -876,   -470,   510,   803,   170,   -787,   -1568,  -1893,  -1598, -1027,
    -992,   -1803,  -2610, -2484, -1905, -2113,  -3113,  -3399,  -2267, -1261,
    -2007,  -3637,  -3909, -2340, -893,  -1158,  -2272,  -2486,  -1639, -915,
    -777,   -596,   -91,   196,   85,    210,    875,    1373,   1247,  1219,
    1958,   2718,   2328,  1196,  1008,  2350,   3677,   3269,   1503,  366,
    922,    2264,   2810,  1996,  608,   -168,   75,     680,    811,   395,
    -56,    -318,   -607,  -966,  -1108, -925,   -613,   -368,   -369,  -919,
    -1926,  -2460,  -1685, -300,  155,   -611,   -1524,  -2204,  -3227, -3859,
    -2037,  1622,   2382,  -2583, -8448, -7544,  -84,    4814,   915,   -6423,
    -7558,  -1746,  2515,  -59,   -4587, -3858,  1260,   3625,   187,   -4148,
    -3500,  1542,   5467,  4780,  1256,  -1127,  -403,   2481,   5332,  6346,
    5014,   2536,   1216,  2467,  5039,  6238,   5070,   3381,   3269,  4173,
    3905,   2248,   1586,  3299,  5240,  4362,   1004,   -1382,  -489,  2113,
    3168,   1620,   -742,  -1824, -1435, -897,   -1058,  -1500,  -1545, -1398,
    -1965,  -3266,  -4136, -3756, -2609, -1804,  -1986,  -3087,  -4599, -5296,
    -4051,  -1731,  -781,  -2228, -4092, -3977,  -2325,  -1353,  -1568, -1490,
    -428,   178,    -672,  -1650, -1058, 749,    2039,   2079,   1540,  897,
    310,    572,    2266,  4265,  4265,  1869,   -231,   559,    3332,  4752,
    3229,   768,    101,   1364,  2463,  1984,   819,    411,    723,   675,
    -162,   -923,   -743,  -32,   185,   -516,   -1653,  -2359,  -2103, -986,
    42,     -205,   -1702, -2870, -2337, -809,   -221,   -982,   -1544, -946,
    -598,   -2117,  -4291, -4100, -857,  1948,   338,    -4799,  -7972, -5403,
    173,    2371,   -1063, -5533, -5578, -1777,  605,    -985,   -3249, -2213,
    1184,   2691,   560,   -2356, -2288, 1233,   5244,   6441,   4004,  370,
    -663,   2555,   7404,  9282,  6573,  2612,   1836,   4662,   7467,  7393,
    5421,   4262,   4741,  5362,  4705,  3163,   2397,   3337,   4887,  4810,
    2254,   -749,   -1316, 772,   2706,  2016,   -573,   -2552,  -2746, -2012,
    -1647,  -1978,  -2579, -3105, -3473, -3911,  -4484,  -4891,  -4795, -4163,
    -3543,  -3538,  -4275, -5356, -5743, -4637,  -2614,  -1301,  -1825, -3341,
    -4011,  -2937,  -751,  1007,  1245,  235,    -639,   -61,    1626,  2864,
    2967,   2734,   3013,  3329,  2914,  2312,   2666,   3839,   4308,  3162,
    1453,   768,    1255,  1887,  2006,  1715,   1031,   -297,   -1660, -1690,
    -277,   813,    -30,   -2137, -3370, -2854,  -1553,  -593,   -413,  -1146,
    -2567,  -3440,  -2369, -205,  379,   -1258,  -2315,  -812,   262,   -3205,
    -8576,  -7894,  738,   7492,  1951,  -11595, -17098, -6934,  7139,  8065,
    -4575,  -14199, -8946, 3606,  7504,  -547,   -8242,  -5113,  4406,  8113,
    2134,   -5040,  -4089, 4157,  10934, 10158,  4167,   -565,   -192,  4428,
    9765,   12201,  9861,  4512,  1225,  3451,   8483,   10133,  6497,  2574,
    3333,   6806,   6986,  2487,  -1214, 623,    5416,   6647,   2204,  -3289,
    -4556,  -1565,  1544,  1525,  -1236, -4293,  -5695,  -5174,  -3995, -3403,
    -3449,  -3750,  -4505, -6014, -7296, -6523,  -3849,  -2096,  -3288, -5722,
    -6004,  -3581,  -1497, -1960, -3330, -2800,  -434,   964,    -111,  -1739,
    -1136,  1736,   4151,  3736,  1274,  -451,   469,    3386,   5833,  5898,
    3646,   1085,   272,   1743,  4061,  5108,   3837,   1490,   246,   967,
    1866,   859,    -1069, -974,  1542,  2835,   47,     -4285,  -5068, -1567,
    1781,   1223,   -1997, -4227, -3747, -1720,  41,     245,    -1228, -2972,
    -2673,  22,     1980,  -930,  -7721, -11271, -5725,  4974,   8484,  -2007,
    -16979, -19255, -4670, 11057, 9690,  -6417,  -17537, -10841, 4262,  9292,
};
