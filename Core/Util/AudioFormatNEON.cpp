// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include "ppsspp_config.h"
#if PPSSPP_ARCH(ARM_NEON)

#if defined(_MSC_VER) && PPSSPP_ARCH(ARM64)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
#include "Common/CommonTypes.h"
#include "Core/Util/AudioFormat.h"
#include "Core/Util/AudioFormatNEON.h"

#if !PPSSPP_ARCH(ARM) && !PPSSPP_ARCH(ARM64)
#error Should not be compiled on non-ARM.
#endif

alignas(16) static s16 volumeValues[4] = {};

void AdjustVolumeBlockNEON(s16 *out, s16 *in, size_t size, int leftVol, int rightVol) {
	if (leftVol <= 0xFFFF && -leftVol <= 0x10000 && rightVol <= 0xFFFF && -rightVol <= 0x10000) {
		// Note: vqshrn_n_s32 takes a const argument, so we always go with 1 here, 15 there.
		volumeValues[0] = leftVol >> 1;
		volumeValues[1] = rightVol >> 1;
		volumeValues[2] = leftVol >> 1;
		volumeValues[3] = rightVol >> 1;

		const int16x4_t vol = vld1_s16(volumeValues);

		while (size >= 16) {
			int16x8_t indata1 = vld1q_s16(in);
			int16x8_t indata2 = vld1q_s16(in + 8);

			int32x4_t outh1 = vmull_s16(vget_high_s16(indata1), vol);
			int32x4_t outh2 = vmull_s16(vget_high_s16(indata2), vol);
			int32x4_t outl1 = vmull_s16(vget_low_s16(indata1), vol);
			int32x4_t outl2 = vmull_s16(vget_low_s16(indata2), vol);

			int16x8_t outdata1 = vcombine_s16(vqshrn_n_s32(outl1, 15), vqshrn_n_s32(outh1, 15));
			int16x8_t outdata2 = vcombine_s16(vqshrn_n_s32(outl2, 15), vqshrn_n_s32(outh2, 15));
			vst1q_s16(out, outdata1);
			vst1q_s16(out + 8, outdata2);
			in += 16;
			out += 16;
			size -= 16;
		}
	}

	if (leftVol <= 0x7fff && -leftVol <= 0x8000 && rightVol <= 0x7fff && -rightVol <= 0x8000) {
		for (size_t i = 0; i < size; i += 2) {
			out[i] = ApplySampleVolume(in[i], leftVol);
			out[i + 1] = ApplySampleVolume(in[i + 1], rightVol);
		}
	} else {
		for (size_t i = 0; i < size; i += 2) {
			out[i] = ApplySampleVolume20Bit(in[i], leftVol);
			out[i + 1] = ApplySampleVolume20Bit(in[i + 1], rightVol);
		}
	}
}

#endif // PPSSPP_ARCH(ARM_NEON)
