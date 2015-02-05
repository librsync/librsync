#pragma once
#ifndef __RDIFF_H__
#define __RDIFF_H__

#ifdef BUILDDLL
#define DLLEXPORT __declspec(dllexport)

DLLEXPORT rs_result rdiff_sig(const char *baseFile, const char *sigFile);
DLLEXPORT rs_result rdiff_delta(const char *sigFile, const char *newFile, const char *deltaFile);
DLLEXPORT rs_result rdiff_patch(const char *baseFile, const char *deltaFile, const char *newFile);

#endif

#endif