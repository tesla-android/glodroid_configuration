/* SPDX-License-Identifier: Apache-2.0 */
/* (c) 2020 Roman Stratiienko r.stratiienko@gmail.com */

#pragma once

#define N(...) __VA_ARGS__
#define Q() \"

#define q() "

#define BO() "${
#define BOQ() "\${
#define BC() }"

#define EXTENV(var_name, extension) setenv var_name BO()N(var_name)BC()N(extension)
#define FEXTENV(var_name, extension) setenv var_name BOQ()N(var_name)BC()N(extension)

#define FUNC_BEGIN(name) setenv name '
#define FUNC_END() '

#define STR(...) q()N(__VA_ARGS__)q()
#define STRESC(...) Q()N(__VA_ARGS__)Q()

