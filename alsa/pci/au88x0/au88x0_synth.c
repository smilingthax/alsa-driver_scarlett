/*
 * Someday its supposed to make use of the WT DMA engine
 * for a Wavetable synthesizer.
 */
#include "au88x0.h"

void vortex_fifo_setwtvalid(vortex_t *vortex, int fifo, int en);
void vortex_connection_adb_mixin(vortex_t *vortex, int en, unsigned char channel,
    unsigned char source, unsigned char mixin);
void vortex_connection_mixin_mix(vortex_t *vortex, int en, unsigned char mixin, unsigned char mix, int a);
void vortex_fifo_wtinitialize(vortex_t *vortex, int fifo, int j);

/* WT */

void vortex_wt_setstereo(vortex_t *vortex, u32 wt, u32 stereo) {
	int temp;
	
	temp = hwread(vortex->mmio, 0x80 + ((wt >> 0x5)<< 0xf) + (((wt & 0x1f) >> 1) << 2));
	temp = (temp & 0xfe) | (stereo & 1);
	hwwrite(vortex->mmio, 0x80 + ((wt >> 0x5)<< 0xf) + (((wt & 0x1f) >> 1) << 2), temp);
}

void vortex_wt_setdsout(vortex_t *vortex, u32 wt, int en) {
#ifndef CHIP_AU8820
	int temp;

	temp = hwread(vortex->mmio, ((wt >> 0x5)<< 0xf) + 0x8);
	if (en)
		temp |= (1 << (wt & 0x1f));
	else
		temp &= (1 << ~(wt & 0x1f));
	hwwrite(vortex->mmio, ((wt >> 0x5)<< 0xf) + 0x8, temp);
#endif
}

// WT routing is still a mistery.
int  vortex_wt_allocroute(vortex_t *vortex, int dma, int nr_ch) {
    //FIXME: WT audio routing.
	if (nr_ch) {
		vortex_fifo_wtinitialize(vortex, dma, 1);
		vortex_wt_setstereo(vortex, dma, nr_ch-1);
	} else
		vortex_fifo_setwtvalid(vortex, dma, 0);
	vortex_wt_setdsout(vortex, dma, 1);
    return 0;
}
void vortex_wt_connect(vortex_t *vortex, int en, unsigned char mixers[]) {
	
	vortex->mixwt[0] = vortex_adb_checkinout(vortex, vortex->fixed_res, en, VORTEX_RESOURCE_MIXIN);
	vortex->mixwt[1] = vortex_adb_checkinout(vortex, vortex->fixed_res, en, VORTEX_RESOURCE_MIXIN);
	
	vortex_connection_adb_mixin(vortex, en, 0x11, ADB_WTOUT(0), vortex->mixwt[0]);
	vortex_connection_adb_mixin(vortex, en, 0x11, ADB_WTOUT(1), vortex->mixwt[1]);
	vortex_connection_mixin_mix(vortex, en, vortex->mixwt[0], mixers[0], 0);
	vortex_connection_mixin_mix(vortex, en, vortex->mixwt[1], mixers[1], 0);
	if (VORTEX_IS_QUAD(vortex)) {
		vortex_connection_mixin_mix(vortex, en, vortex->mixwt[0], mixers[2], 0);
		vortex_connection_mixin_mix(vortex, en, vortex->mixwt[1], mixers[3], 0);		
	}
}

/* Read WT Register */
int  vortex_wt_GetReg(vortex_t *vortex, char a, int *b) {
	int eax, esi;
	
	if (a != 4) {
		if (a == 7)
			return 0;
		return hwread(vortex->mmio, (*b << 0xf) + 0x10);
	}
	esi = b[1];
	eax = ((((a & 0x1f) + b[1]) / 32) & 0xff) << 0xb;
	if (esi < 0)
		esi = ((esi-1)|0x0FFFFFFE0)+1;
	esi &= 0xff;
	hwread(vortex->mmio, ((eax + esi) << 4) + 0x20c);
		
	return 8;
}

/*
 WT hardware abstraction layer generic register interface.
 a: type of register or register set to be accessed.
 b: Register offset
 c: Register value (or values) to be written.
*/
int  vortex_wt_SetReg2(vortex_t *vortex, unsigned char a, int b, unsigned short c) {
	int eax, edx;
	
	if (b >= NR_WT)  // 0x40 -> NR_WT
		return 0;
	
	/* cdq: 64 bit sign extender */
	eax = b;
	if (eax<0)
		edx = 0xffffffff & 0x1f;
	else
		edx = 0;
	eax += edx;
	edx = b & 0x8000001f;
	eax = (eax / 32) | ((char)b);
	if (eax < 0)
		edx = ((edx-1)|0x0FFFFFFE0)+1;
	
	// esp+4 = edx & 0xff
	
	if ((eax & 0xff) >= 2)
		return 0;
	
	if ((a & 0xff) - 0x20) {
		if ((a & 0xff) - 0x21) 
			return 0;
		eax = ((((b & 0xff) << 0xb) + (edx & 0xff)) << 4) + 0x208;
	} else {
		eax = ((((b & 0xff) << 0xb) + (edx & 0xff)) << 4) + 0x20a;
	}
	hwwrite(vortex->mmio, eax, c);
	return 1;
}


/*public: void __thiscall CWTHal::SetReg(unsigned char,int,unsigned long) */
int  vortex_wt_SetReg(vortex_t *vortex, unsigned char a, int b, unsigned long c) {
	// a : type
	// b : voice
	// c : data
	
	int  ecx, edx, esp4, eax;
	
	if (b >= NR_WT)  // 0x40 -> NR_WT
		return 0;

#if 0 //original_code	
	eax = ecx = b;
	/* cdq: 64 bit sign extender */
	if (eax<0)
		edx = 0xffffffff;
	else
		edx = 0;
	eax += edx  & 0x1f;
	edx = ecx & 0x8000001f;
	eax = (eax / 32) | ((char)b);
	// This "if" semes to be useless ...
	if (eax < 0)
		edx = ((edx-1)|0x0FFFFFFE0)+1;
	
	esp4 = (char)edx;
	
	// eax is the WT bank. AU8830 has 2 banks: 0 and 1.
	if ((char)eax >= 2)
		return 0;
	
#else //rewritting_code
	esp4 = b & 0x1f;
	b = b >> 5;
	
#endif
	
	if (a > 0xc)
		return 0;
	
	switch (a) {
		case 0 :
			eax = ((b << 0xd) + esp4) << 2;
			hwwrite(vortex->mmio, 0x180 + eax, c);
			return 0xc;
		break;
		case 1 :
			edx = ((b & 0xff) << 0xb) + (esp4 & 0xff);
			hwwrite(vortex->mmio, 0x20 + (edx << 4), c);
			return 0xc;			
		break;
		case 2 :
			eax = ((b & 0xff) << 0xb) + (esp4 & 0xff);
			hwwrite(vortex->mmio, 0x204 + (eax << 4), c);
			return 0xc;
		break;
		case 3 :
			eax = ((b & 0xff) << 0xb) + (esp4 & 0xff);
			hwwrite(vortex->mmio, 0x208 + (eax << 4), c);
			return 0xc;
		break;
		case 4 :
			eax = ((b & 0xff) << 0xb) + (esp4 & 0xff);
			hwwrite(vortex->mmio, 0x20c + (eax << 4), c);
			return 0xc;
		break;
		case 6 :
			edx = ((b & 0xff) << 0xd) + (esp4 & 0xff);
			hwwrite(vortex->mmio, 0x100 + (edx << 2), c);
			return 0xc;			
		break;
		case 0xb : 
			{
				int ebx, esi;
				// FIXME: Here is something wrong ... (check admaud30.asm)
				edx = ((b & 0xff) << 0xb) + (esp4 & 0xff);
				ebx = b + esp4;
				esi = ebx << 4;
				
				hwwrite(vortex->mmio, 0x40c + esi, c);
				hwwrite(vortex->mmio, 0x408 + esi, c);
				hwwrite(vortex->mmio, 0x404 + esi, c);
				hwwrite(vortex->mmio, 0x400 + esi, c);
				return 0xc;			
			}
		break;
		case 5 :
			ecx = (b << 0xf) + 0x4;
		break;
		case 8 :
			ecx = (b << 0xf) + 0x14;
		break;
		case 9 :
			ecx = (b << 0xf) + 0xc;
		break;
		case 0xa :
			ecx = (b << 0xf) + 0x0;
		break;
		case 0xc :
			ecx = (b << 0xf) + 0x8;
		break;
		default:
			return 0;
		break;
	}
	eax = ecx;
	hwwrite(vortex->mmio, eax, c);
	return 1;
}

void vortex_wt_WriteReg(vortex_t *vortex, int wt, char addr, int data) {
	hwwrite(vortex->mmio, ((wt << 0xf) + wt) + addr, data);
}
void vortex_wt_ReadReg(vortex_t *vortex, int wt, char addr, int *data) {
	*data = hwread(vortex->mmio, ((wt << 0xf) + wt) + addr);
}

void vortex_wt_InitializeWTRegs(vortex_t * vortex) {
	int ebx=0, var4, var8, varc, var10=0, edi;
	
	var10 &= 0xe3;
	var10 |= 0x22;
	var10 &= 0xFFFFFEBF;
	var10 |= 0x80;
	var10 |= 0x2;
	var10 &= 0xfffffffe;
	var10 &= 0xfffffffb;
	var10 |= 0x18;
	var4 = 0x10000000;
	varc = 0x00830000;
	var8 = 0x00830000;
	
	for (edi=0; edi<2; edi++) {
		vortex_wt_SetReg(vortex, 0xc, edi, ebx);
		vortex_wt_SetReg(vortex, 0xa, edi, var10);
		vortex_wt_SetReg(vortex, 0x9, edi, var4);
		vortex_wt_SetReg(vortex, 0x8, edi, varc);
		vortex_wt_SetReg(vortex, 0x5, edi, var8);
	}
	
	for (edi=0; edi<NR_WT; edi++)  {
		vortex_wt_SetReg(vortex, 0x4, edi, ebx);
		vortex_wt_SetReg(vortex, 0x3, edi, ebx);
		vortex_wt_SetReg(vortex, 0x2, edi, ebx);
		vortex_wt_SetReg(vortex, 0x1, edi, ebx);
		vortex_wt_SetReg(vortex, 0xb, edi, ebx);
	}
	var10 |= 1;
	for (edi=0; edi<2; edi++)
		vortex_wt_SetReg(vortex, 0xa, edi, var10);
}
/* Extract of CAdbTopology::SetVolume(struct _ASPVOLUME *) */
void vortex_wt_SetVolume(vortex_t *vortex, int wt, int a, int vol[]) {
	int x=0, y=0;
	// CAdbTopology->this_1E4, CAdbTopology->this_1E8
	vortex_wt_WriteReg(vortex, wt, 0x200 + (a<<4), (x & 0xff) << 0x10);
	vortex_wt_WriteReg(vortex, wt, 0x204 + (a<<4), ((x & 0xff) << 0x10) | 1);
	// In this case x semes to a set of 7 bit values.
	// y = CAdbTopology->this_1F0
	vortex_wt_WriteReg(vortex, wt, 0x20c + (a<<4), y);	
}

/* The rest of this file is only for information purpose. */
#if 0
void Asp4SynthTopology::Asp4SynthTopology(class	CResource * res, class Asp4Topology * asp) {
	this00 = res;
	this04 = asp;
	
	this08 = 0xff; // MixIn 0x65
	this0c = 0xff; // MixIn 0x66
	this10 = 0xff; // MixIn 0x67
	this14 = 0xff; // MixIn 0x68
	this18 = 0xff; // MixIn 0x6b
	this1c = 0xff; // MixIn 0x6c
	this20 = 0xff; // MixIn 0x6d
	this24 = 0xff; // MixIn 0x6e
	this28 = 0xff; // MixIn 0x6f
	this2c = 0xff; // MixIn 0x70
	this30 = 0; // MixOut 0x8b
	this34 = 0; // MixOut 0x8c
	this38 = 0; // MixOut 0x81
	this3c = 0; // MixOut 0x82
	this40 = 0; // MixOut 0x8f
	this44 = 0; // MixOut 0x90
	this48 = 0;
	this4c = 0;
	this50 = 0; // ADB DMA
	this54 = 0; // ADB DMA
	this58 = 0;
	this5c = 0;
	this60 = 0;
	this64 = 0;
	thisa0 = 0;
	
	//GetMixObject@CResource@@QAEJW4_eReservedFor@@PAPAVCAsp4Mix@@K@
}

void Asp4SynthTopology::Create(int a, int b, int c, int d) {
	
	eax = Asp4SynthTopology::GetMixerResources(a, b)
	if (eax == 0)
		return eax;
	if ((a==0)||(b==0))
		return eax;
	/* Here goes a condition thats allways true ... */
	if (eax) {
		eax = _OsLibAllocate@12(0x4000, 0, 0x46425846, 8);
		*esi = eax;
		eax = __imp__IoAllocateMdl@20(eax, 0x4000, 0,0,0);
		*(esi + 4) = eax;
		__imp__MmBuildMdlForNonPagedPool@4(eax);
	} else
		esi = 0;
	this_60 = esi;
	if (esi == 0)
		return eax;
	CResource = this_00;
	eax = CResource::GetAdbDmaObject(&this_50, 2);
	if (eax == 0)
		return 0;
	esi = operator new(8);
	if (esi) {
		eax = _OsLibAllocate@12(0x4000, 0, 0x46425846, 8);
		*esi = eax;
		eax = __imp__IoAllocateMdl@20(eax, 0x4000, 0,0,0);
		*(esi + 4) = eax;
		__imp__MmBuildMdlForNonPagedPool@4(eax);
	} else
		esi = 0;
	this_64 = esi;
	if (esi == 0)
		return eax;
	CResource = this_00;
	eax = CResource::GetAdbDmaObject(&this_54, 2);
	if (eax == 0)
		return 0;
	return 1;
}

void ~Asp4SynthTopology(void) {
	
	asp = this04;
	edi = asp->this74; /* CAsp4HwIO */
	asp->this9c = 0x3e8;
	
	if (this38  && (this08 != 0xff))
		asp->this64 = ReadDWORD(VORTEX_WT_BASE + ((this38->this08 << 5)+this08)<<2) & 0xff;
	 (0x420 << 7)
	if (this3c  && (this0c != 0xff))
		asp->this68 = ReadDWORD(VORTEX_WT_BASE + ((this3c->this08 << 5)+this0c)<<2) & 0xff;
	
	if (this08 != 0xff) {
#ifdef CHIP_AU8830
		Route(0, 0x11, 0x62, this08 + 0x50);
#elifdef CHIP_AU8820
		Route(0, 0x11, 0x00, this08);
#endif
		if (this38)
			DisableInput(this38, this08, 0);
		if (this40)
			DisableInput(this40, this08, 0);
	}
	if (this0c != 0xff) {
#ifdef CHIP_AU8830
		Route(0, 0x11, 0x62, this0c + 0x50);
#elifdef CHIP_AU8820
		Route(0, 0x11, 0x00, this0c);
#endif
		if (this3c)
			DisableInput(this3c, this0c, 0);
		if (this44)
			DisableInput(this44, this0c, 0);
	}
#ifdef CHIP_AU8830
	if (this18 != 0xff) {
		Route(0, 0x11, 0xA2, this18 + 0x50);
		if (this38)
			DisableInput(this38, this18, 0);
		if (this40)
			DisableInput(this40, this18, 0);
	}
	if (this1c != 0xff) {
		Route(0, 0x11, 0xA2, this1c + 0x50);
		if (this38)
			DisableInput(this38, this1c, 0);
		if (this40)
			DisableInput(this40, this1c, 0);
	}
#endif
	if (this10 != 0xff) {
		if (this38)
			DisableInput(this38, this10, 0);
		if (this40)
			DisableInput(this40, this10, 0);
	}
	if (this14 != 0xff) {
		if (this38)
			DisableInput(this38, this14, 0);
		if (this40)
			DisableInput(this40, this14, 0);
	}
	if (this28 != 0xff) {
		if (GetInputChannelRefCount(this28) == 1) {
			Route(0, 0x11, 0x64, this28 + 0x50);
			if (this34)
				DisableInput(this34, this28, 0);
		}
	}
	if (this2c != 0xff) {
		if (GetInputChannelRefCount(this2c) == 1) {
			Route(0, 0x11, 0xa4, this2c + 0x50);
			if (this34)
				DisableInput(this34, this2c, 0);
		}
	}
	if (this20 != 0xff) {
		if (GetInputChannelRefCount(this20) == 1) {
			Route(0, 0x11, 0x65, this20 + 0x50);
			if (this30)
				DisableInput(this30, this20, 0);
		}
	}
	if (this24 != 0xff) {
		if (GetInputChannelRefCount(this24) == 1) {
			Route(0, 0x11, 0xa5, this24 + 0x50);
			if (this30)
				DisableInput(this30, this24, 0);
		}
	}
	
	if (this50) {
		SetADBValid(asp->thiscc, this50->this04, 0);
		if (this30 && this34)
			Asp4Topology::Connection(this04, 0, 0x11, (CAsp4Mix *) this34, (int) this30, (CAsp4AdbDma *)this50)
	}
	if (this54) {
		SetADBValid(asp->thiscc, this54->this04, 0);
		if (this10 && this14)
			Asp4Topology::Connection(this04, 0, 0x11, (CAsp4Mix *) this14, (int) this10, (CAsp4AdbDma *)this54)
	}
	Asp4SynthTopology::ReleaseMixerResources(void);
	if (this50)
		CAsp4DmaChannel::CheckIn(this50);
	if (this54)
		CAsp4DmaChannel::CheckIn(this54);
	free(thsis60);
	free(tthis64);
}

// admaud30.asm: line 67609
void Asp4Topology::ConfigureWT(int arg_0 ,int arg_4 ,int arg_8) {
	// ...
	ebx = arg_0;
	// ...
	
	arg_0 = some_weird_operations(ebx) + 0xA4;
	Asp4Topology::Route((int)ebp ,(uchar) 0x11, (uchar) arg_0, (uchar) (*edi + 0x50));
	arg_0 = some_weird_operations(ebx) + 0xA5;
	Asp4Topology::Route((int)ebp ,(uchar) 0x11, (uchar) arg_0, (uchar) (*(esi+ebx*4+0x16C) + 0x50));

}

int CResource::GetMixObject(enum	_eReservedFor, class CAsp4Mix **CAsp4Mix,unsigned long) {
	
	if (thisc4 == 0)
		return 0x92EB0014;
	if (CAsp4Mix == 0)
		return 0x80070057;
	
	*CAsp4Mix = CAsp4Mixer::GetMixObject(_eReservedFor, ulong 0xFFFFFFFF)
	
	eax = ~CAsp4Mix;
	
	neg	eax
	sbb	eax, eax
	and	eax, 6D14FFF6h
	add	eax, 92EB000Ah
	
	return eax;
}

CAsp4Mix * CAsp4Mixer::GetMixObject(_eReservedFor, ulong a) {
	//CAsp4Mixer->this188 CMixArray.
	
	var04 = CMixArray::GetOutput(_eReservedFor, ulong a);
	if (var04 == 0xff)
		return 0;
	var_A40 = *(this_188 + var04*0xc + 0x188);
	if (var_a40 == 1) {	
		CMixArray::GetPropertyIndexes(_eReservedFor, (_mixIndex *)&var_A1C);
		
		for (var08=0; var08<var_A1C; var08++) {
			
			eax = CMixArray::GetPendingProperty((ulong)var08*4+var_A18, (_mixProperty *)&var_A34);
			if (eax == 0)
				return 0;
			var_A38 == 0;
			var_A48 = var_A34;
			if (var_A48 == 1) {
				CAsp4Mix = var04*0x18 + this_08;
				CAsp4Mix::SetVolume((ulong) var_A28);
				var_A38 = 1;
			}
			if (var_A48 == 0) {
				var_A3C = CMixArray::SearchInput(_eReservedFor,(ulong) 0x0FFFFFFFF);
				if (var_a3c != 0xff) {
					CAsp4Mix = var04*0x18 + this_08;
					CAsp4Mix::SetInputVolume(var_A3C, var_A28, var_A24, 0);
					var_A38 = 1;
					
				}
			}
			if (var_A38 == 0) {
				CMixArray = this_188;
				CMixArray::DeletePendingProperty((_mixProperty *)&var_A34);
			}
		}
	}
	return (var04*0x18 + &this_08);
}

void CMixArray::GetOutput(enum  _eReservedFor,unsigned long a) {
	
	eax = CMixArray::GiveIndex(_eReservedFor, ulong a);
	if (eax != 0FFFFFFFF) {
		*(eax*3*4 + CMixArray->this_188)++;
		return;
	}
	eax = 0;
	ecx = &this_180;
	{
		if (*ecx != 0xff) {
			ecx = &this_00 + eax*3*4;
			*(ecx + 0x184) = a;
			*((eax*3 + 0x60)*4 + this_00) = _eReservedFor;
			*(ecx + 0x188)++;
			return;
		}	
		eax++;
		ecx += 0xc;
	} while (eax < 0x10);
	return 0xff;
}

int CMixArray::GiveIndex(enum  _eReservedFor, unsigned long a) {
	eax = 0;
	if (_eReservedFor < 0x80) {
		ecx = &this00;
		{
			if ((_eReservedFor == *ecx) && (a == *(ecx+4)))
				return eax;
			ecx += 0xc;
			eax++;
		} while(eax < 0x20);
	} else {
		ecx = &this184;
		{
			if ((_eReservedFor == *(ecx-4)) && (a == *ecx))
				return eax;
			ecx += 0xc;
			eax++;
		} while(eax < 0x10);	
	}
	eax |= 0xFFFFFFFF;
}




// Voice related stuff.
void vortex_AspSynth_VoiceInit(void) {
	int i;
	struct voice_t *voice; 
	//Asp4Synth::ResetPerformanceStats(void);
	//Asp4Synth::ProfileSynth(void);
	
	// Hardware Voices.
	for (i=0; i<NR_WT; i++) {
		voice = (this_bc + i*0x198);
		voice->this_b8 = 0xffff;
		voice->this_114 = 0xffff;
		
		voice->this_174 = 0;
		voice->this_5d = 0;
		voice->this_60 = 0;
		voice->this_190 = 0;
		voice->this_58 = i;
		voice->this_62 = 0;
		voice->this_4c = 0;
		voice->this_50 = 0;
		
		if (voice->this_180) {
			vortex_SetReg(vortex, 1, i, 0);
			vortex_SetReg(vortex, 2, i, 0);
			//CSynth::Stop(void);
			//CSynth::Release(void);
			voice->this_180 = 0;
			vortex_SetReg(vortex, 0, i, 1);
		}
	}
	// Software voices ...
	
	//CDmDlsDownloadManager::DownloadInstrument(void * *,void	* *,_DMUS_DOWNLOADINFO *,void *	* const,void *);
}

void Asp4Synth::MuteVoices(uchar a,ulong b) {
	int ebx=0;
	if (ebx != this_180) {
		vortex_SetReg(vortex, 6, i, 1);
		vortex_SetReg(vortex, 1, i, ebx);
		vortex_SetReg(vortex, 2, i, ebx);
		CSynth::Stop(void);
		CSynth::Release(void);
		this_180 = ebx;
	}
	
}
void Asp4Synth::ResetControllers(unsigned char) {
	
}

int CAdmHilObject::wtMidiDeviceOpen(void) {
	if (this_2c == 0)
		return 0;
	this_2c->this52794 = 0;
	if (this_2c->this52790)
		return 0;
	this_2c->this52790 = 1;
	this_2c->Asp4Synth::VoiceInit(void);
	this_2c->Asp4Synth::BankChange((CAbeSndFont *)this_2c->this_08, (int) 0);
	if (this_2c) {
		var_c = __imp__RtlConvertLongToLargeInteger@4(0x0FFFFB1E0);
		__imp__KeDelayExecutionThread@12(0, 0, *var_c);
		this_2c->Asp4Synth::Close((int) 0);
		eax = this_2c->Asp4Synth::Open(void);
		if (eax == 0)
			return 0C0000001;
		this_2c->this_53888 = 1;
	}
	// Some "repe" things i dont understand. Basically semes to be initializers.
	
}
void Asp4MidiEvent::handler(void *) {
	switch (a) {
		
		case x:
			if (this_60)
				CWTHal::SetReg((uchar) 6, (int) this_58, (ulong) 1);
		break;
		
	}
	
}

void CAsp4Core::CreateSynthBuffer(_ASPWAVEFORMAT *,CSynth * *) {
	
	
}

void fragment_of_wtcallback(void) { 
	vortex_SetReg(vortex, 0xb, i, 0);
	vortex_SetReg2(vortex, 0x21, i, 0);
	CSynth::Start(void);
	// ..
	vortex_SetReg(vortex, 1, i, 0x50000000);
	vortex_SetReg(vortex, 2, i, 0x50000000);
	// ..
	vortex_SetReg(vortex, 5, 0, 0x8800);
	vortex_SetReg(vortex, 5, 1, 0x8800);
	
}
#endif

/*
CSynth constructor

; public: class	Asp4SynthTopology * __thiscall Asp4Topology::AllocSynth(class CResource	*)

?AllocSynth@Asp4Topology@@QAEPAVAsp4SynthTopology@@PAVCResource@@@Z proc near
					; CODE XREF: CAsp4Core::SetWavetableTopology(int)+3Fp
					; .text:0002565Dp

var_14		= dword	ptr -14h
var_10		= dword	ptr -10h
var_C		= dword	ptr -0Ch
var_8		= dword	ptr -8
var_4		= dword	ptr -4
arg_0		= dword	ptr  4

		sub	esp, 14h
		push	ebx
		push	ebp
		push	esi
		push	edi
		mov	edi, ecx
		push	70h
		call	??2@YAPAXI@Z	; operator new(uint)

		add	esp, 4
		test	eax, eax
		jz	short loc_2F3BA

		mov	ebp, [esp+24h+arg_0]
		push	edi
		push	ebp
		mov	ecx, eax
		call	??0Asp4SynthTopology@@QAE@PAVCResource@@PAVAsp4Topology@@@Z ; Asp4SynthTopology::Asp4SynthTopology(CResource *,Asp4Topology *)

		mov	esi, eax
		test	esi, esi
		jnz	short loc_2F3C6


loc_2F3BA:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+15j
		pop	edi
		pop	esi
		pop	ebp
		xor	eax, eax
		pop	ebx
		add	esp, 14h
		retn	4

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F3C6:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+28j
		mov	eax, [ebp+3A4h]
		mov	ecx, [ebp+3ACh]
		mov	ebx, [ebp+3A8h]
		lea	edx, [esp+24h+var_14]
		push	edx
		push	offset aA_0	; "A"
		push	offset asc_814A0 ; "s"
		push	80000002h
		mov	[esp+34h+var_4], eax
		mov	[esp+34h+arg_0], ecx
		call	_AspReadRegDwordValue@16

		test	eax, eax
		jz	short loc_2F403

		mov	eax, [esp+24h+var_14]
		jmp	short loc_2F41C

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F403:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+6Bj
		cmp	ebx, 0C8h
		jb	short loc_2F41A

		mov	eax, [esp+24h+arg_0]
		test	eax, eax
		jz	short loc_2F41A

		mov	eax, 1
		jmp	short loc_2F41C

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F41A:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+79j
					; Asp4Topology::AllocSynth(CResource *)+81j
		xor	eax, eax

loc_2F41C:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+71j
					; Asp4Topology::AllocSynth(CResource *)+88j
		lea	ecx, [esp+24h+var_14]
		mov	[edi+88h], eax
		push	ecx
		push	offset asc_81514 ; "S"
		push	offset asc_81528 ; "s"
		push	80000002h
		call	_AspReadRegDwordValue@16

		test	eax, eax
		jz	short loc_2F44B

		mov	edx, [esp+24h+var_14]
		mov	[edi+8Ch], edx
		jmp	short loc_2F46A

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F44B:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+ADj
		cmp	ebx, 0C8h
		jb	short loc_2F462

		mov	eax, [esp+24h+arg_0]
		test	eax, eax
		jz	short loc_2F462

		mov	eax, 1
		jmp	short loc_2F464

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F462:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+C1j
					; Asp4Topology::AllocSynth(CResource *)+C9j
		xor	eax, eax

loc_2F464:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+D0j
		mov	[edi+8Ch], eax

loc_2F46A:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+B9j
		lea	eax, [esp+24h+var_14]
		push	eax
		push	offset asc_8159C ; "H"
		push	offset asc_815B8 ; "s"
		push	80000002h
		call	_AspReadRegDwordValue@16

		test	eax, eax
		jz	short loc_2F493

		mov	ecx, [esp+24h+var_14]
		mov	[edi+90h], ecx
		jmp	short loc_2F49D

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F493:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+F5j
		mov	dword ptr [edi+90h], 1

loc_2F49D:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+101j
		lea	edx, [esp+24h+var_14]
		push	edx
		push	offset aG_15	; "G"
		push	offset asc_81640 ; "s"
		push	80000002h
		call	_AspReadRegDwordValue@16

		test	eax, eax
		jz	short loc_2F4C8

		mov	eax, [esp+24h+var_14]
		lea	ebx, [edi+94h]
		mov	[ebx], eax
		jmp	short loc_2F4F9

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F4C8:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+128j
		cmp	[esp+24h+var_4], 6
		jnb	short loc_2F4ED

		cmp	ebx, 0C8h
		jb	short loc_2F4ED

		mov	eax, [esp+24h+arg_0]
		test	eax, eax
		jz	short loc_2F4ED

		lea	ebx, [edi+94h]
		mov	dword ptr [ebx], 1
		jmp	short loc_2F4F9

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F4ED:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+13Dj
					; Asp4Topology::AllocSynth(CResource *)+145j ...
		lea	ebx, [edi+94h]
		mov	dword ptr [ebx], 0

loc_2F4F9:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+136j
					; Asp4Topology::AllocSynth(CResource *)+15Bj
		xor	eax, eax
		mov	dword ptr [edi+9Ch], 3E8h
		mov	[esp+24h+var_10], eax
		mov	[esp+24h+var_C], eax
		cmp	dword ptr [edi+64h], 0FFFFFFFFh
		jnz	short loc_2F564

		lea	ecx, [esp+24h+var_10]
		push	0FFFFFFFFh
		push	ecx
		push	81h
		mov	ecx, ebp
		call	?SearchMixerOutputChannel@CResource@@QAEJW4_eReservedFor@@PAKK@Z ; CResource::SearchMixerOutputChannel(_eReservedFor,ulong *,ulong)

		test	eax, eax
		jnz	short loc_2F564

		lea	edx, [esp+24h+var_C]
		push	0FFFFFFFFh
		push	edx
		push	65h
		mov	ecx, ebp
		call	?SearchMixerInputChannel@CResource@@QAEJW4_eReservedFor@@PAKK@Z	; CResource::SearchMixerInputChannel(_eReservedFor,ulong *,ulong)

		test	eax, eax
		jnz	short loc_2F564

		mov	eax, [esp+24h+var_10]
		mov	edx, [esp+24h+var_C]
		add	eax, 420h
		mov	ecx, [edi+74h]
		shl	eax, 5
		add	eax, edx
		shl	eax, 2
		push	eax
		call	?ReadDWORD@CAsp4HwIO@@QAEKK@Z ;	CAsp4HwIO::ReadDWORD(ulong)

		and	eax, 0FFh
		mov	[edi+64h], eax

loc_2F564:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+181j
					; Asp4Topology::AllocSynth(CResource *)+198j ...
		cmp	dword ptr [edi+68h], 0FFFFFFFFh
		jnz	short loc_2F5BB

		lea	ecx, [esp+24h+var_10]
		push	0FFFFFFFFh
		push	ecx
		push	82h
		mov	ecx, ebp
		call	?SearchMixerOutputChannel@CResource@@QAEJW4_eReservedFor@@PAKK@Z ; CResource::SearchMixerOutputChannel(_eReservedFor,ulong *,ulong)

		test	eax, eax
		jnz	short loc_2F5BB

		lea	edx, [esp+24h+var_C]
		push	0FFFFFFFFh
		push	edx
		push	66h
		mov	ecx, ebp
		call	?SearchMixerInputChannel@CResource@@QAEJW4_eReservedFor@@PAKK@Z	; CResource::SearchMixerInputChannel(_eReservedFor,ulong *,ulong)

		test	eax, eax
		jnz	short loc_2F5BB

		mov	eax, [esp+24h+var_10]
		mov	edx, [esp+24h+var_C]
		add	eax, 420h
		mov	ecx, [edi+74h]
		shl	eax, 5
		add	eax, edx
		shl	eax, 2
		push	eax
		call	?ReadDWORD@CAsp4HwIO@@QAEKK@Z ;	CAsp4HwIO::ReadDWORD(ulong)

		and	eax, 0FFh
		mov	[edi+68h], eax

loc_2F5BB:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+1D8j
					; Asp4Topology::AllocSynth(CResource *)+1EFj ...
		mov	ecx, [ebp+0C4h]
		lea	edx, [esp+24h+var_8]
		push	edx
		push	66h
		push	82h
		mov	[esp+30h+var_8], 0
		call	?IsInputVolPending@CAsp4Mixer@@QAEHW4_eReservedFor@@0PAK@Z ; CAsp4Mixer::IsInputVolPending(_eReservedFor,_eReservedFor,ulong *)

		test	eax, eax
		jz	short loc_2F5F0

		mov	eax, [esp+24h+var_8]
		push	eax
		call	?LinearFrac2WtFP@@YGEK@Z ; LinearFrac2WtFP(ulong)

		and	eax, 0FFh
		mov	[edi+68h], eax

loc_2F5F0:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+24Cj
		mov	ecx, [ebp+0C4h]
		lea	edx, [esp+24h+var_8]
		push	edx
		push	65h
		push	81h
		call	?IsInputVolPending@CAsp4Mixer@@QAEHW4_eReservedFor@@0PAK@Z ; CAsp4Mixer::IsInputVolPending(_eReservedFor,_eReservedFor,ulong *)

		test	eax, eax
		jz	short loc_2F61D

		mov	eax, [esp+24h+var_8]
		push	eax
		call	?LinearFrac2WtFP@@YGEK@Z ; LinearFrac2WtFP(ulong)

		and	eax, 0FFh
		mov	[edi+64h], eax

loc_2F61D:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+279j
		mov	ecx, [ebx]
		mov	edx, [edi+90h]
		mov	eax, [edi+8Ch]
		push	ecx
		mov	ecx, [edi+88h]
		push	edx
		push	eax
		push	ecx
		mov	ecx, esi
		call	?Create@Asp4SynthTopology@@QAEHHHHH@Z ;	Asp4SynthTopology::Create(int,int,int,int)

		test	eax, eax
		jnz	loc_2F719

		mov	ecx, esi
		call	??1Asp4SynthTopology@@QAE@XZ ; Asp4SynthTopology::~Asp4SynthTopology(void)

		push	esi
		call	??3@YAXPAX@Z	; operator delete(void *)

		add	esp, 4
		lea	eax, [edi+88h]
		lea	esi, [edi+8Ch]
		mov	dword ptr [ebx], 0
		push	eax
		push	offset asc_816B4 ; "A"
		push	offset asc_816E0 ; "s"
		push	80000002h
		mov	dword ptr [eax], 0
		mov	dword ptr [esi], 0
		call	_AspSetRegDwordValue@16

		push	esi
		push	offset asc_81754 ; "S"
		push	offset asc_81768 ; "s"
		push	80000002h
		call	_AspSetRegDwordValue@16

		push	ebx
		push	offset aG_16	; "G"
		push	offset asc_817F0 ; "s"
		push	80000002h
		call	_AspSetRegDwordValue@16

		push	70h
		call	??2@YAPAXI@Z	; operator new(uint)

		add	esp, 4
		test	eax, eax
		jz	short loc_2F6CE

		push	edi
		push	ebp
		mov	ecx, eax
		call	??0Asp4SynthTopology@@QAE@PAVCResource@@PAVAsp4Topology@@@Z ; Asp4SynthTopology::Asp4SynthTopology(CResource *,Asp4Topology *)

		mov	esi, eax
		test	esi, esi
		jnz	short loc_2F6DA


loc_2F6CE:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+32Dj
		pop	edi
		pop	esi
		pop	ebp
		xor	eax, eax
		pop	ebx
		add	esp, 14h
		retn	4

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F6DA:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+33Cj
		mov	edx, [ebx]
		mov	eax, [edi+90h]
		mov	ecx, [edi+8Ch]
		push	edx
		mov	edx, [edi+88h]
		push	eax
		push	ecx
		push	edx
		mov	ecx, esi
		call	?Create@Asp4SynthTopology@@QAEHHHHH@Z ;	Asp4SynthTopology::Create(int,int,int,int)

		test	eax, eax
		jnz	short loc_2F719

		mov	ecx, esi
		call	??1Asp4SynthTopology@@QAE@XZ ; Asp4SynthTopology::~Asp4SynthTopology(void)

		push	esi
		call	??3@YAXPAX@Z	; operator delete(void *)

		add	esp, 4
		xor	eax, eax
		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		add	esp, 14h
		retn	4

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2F719:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+2AEj
					; Asp4Topology::AllocSynth(CResource *)+36Bj
		mov	eax, [edi+90h]
		test	eax, eax
		jz	loc_2F823

		mov	eax, [esi+8]
		mov	ecx, edi
		add	al, 50h
		push	eax
		push	62h
		push	11h
		push	1
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	ecx, [esi+0Ch]
		add	cl, 50h
		push	ecx
		push	63h
		push	11h
		push	1
		mov	ecx, edi
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	edx, [esi+18h]
		mov	ecx, edi
		add	dl, 50h
		push	edx
		push	0A2h
		push	11h
		push	1
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	eax, [esi+1Ch]
		mov	ecx, edi
		add	al, 50h
		push	eax
		push	0A3h
		push	11h
		push	1
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		push	2
		push	1
		mov	ecx, ebp
		call	?GetWTRefCount@CResource@@QAEKHW4WTSOURCE@@@Z ;	CResource::GetWTRefCount(int,WTSOURCE)

		test	eax, eax
		jnz	short loc_2F7CF

		mov	eax, [edi+194h]
		test	eax, eax
		jnz	short loc_2F7CF

		mov	ecx, [esi+18h]
		push	ecx
		mov	ecx, [esi+38h]
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	edx, [esi+1Ch]
		mov	ecx, [esi+3Ch]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	ecx, [esi+40h]
		test	ecx, ecx
		jz	short loc_2F7CF

		mov	eax, [esi+44h]
		test	eax, eax
		jz	short loc_2F7CF

		mov	eax, [esi+18h]
		push	eax
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	ecx, [esi+1Ch]
		push	ecx
		mov	ecx, [esi+44h]
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)


loc_2F7CF:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+3F8j
					; Asp4Topology::AllocSynth(CResource *)+402j ...
		push	2
		push	0
		mov	ecx, ebp
		call	?GetWTRefCount@CResource@@QAEKHW4WTSOURCE@@@Z ;	CResource::GetWTRefCount(int,WTSOURCE)

		test	eax, eax
		jnz	short loc_2F823

		mov	eax, [edi+190h]
		test	eax, eax
		jnz	short loc_2F823

		mov	edx, [esi+8]
		mov	ecx, [esi+38h]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	eax, [esi+0Ch]
		mov	ecx, [esi+3Ch]
		push	eax
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	ecx, [esi+40h]
		test	ecx, ecx
		jz	short loc_2F823

		mov	eax, [esi+44h]
		test	eax, eax
		jz	short loc_2F823

		mov	edx, [esi+8]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	eax, [esi+0Ch]
		mov	ecx, [esi+44h]
		push	eax
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)


loc_2F823:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+391j
					; Asp4Topology::AllocSynth(CResource *)+44Cj ...
		mov	eax, [edi+8Ch]
		test	eax, eax
		jnz	short loc_2F83B

		mov	eax, [edi+88h]
		test	eax, eax
		jz	loc_2FB41


loc_2F83B:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+49Bj
		mov	ecx, [esi+10h]
		push	ecx
		mov	ecx, [esi+38h]
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	edx, [esi+14h]
		mov	ecx, [esi+3Ch]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	ecx, [esi+40h]
		test	ecx, ecx
		jz	short loc_2F876

		mov	eax, [esi+44h]
		test	eax, eax
		jz	short loc_2F876

		mov	eax, [esi+10h]
		push	eax
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	ecx, [esi+14h]
		push	ecx
		mov	ecx, [esi+44h]
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)


loc_2F876:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+4C8j
					; Asp4Topology::AllocSynth(CResource *)+4CFj
		mov	eax, [esi+28h]
		mov	ecx, ebp
		push	eax
		call	?GetInputChannelRefCount@CResource@@QAEKK@Z ; CResource::GetInputChannelRefCount(ulong)

		cmp	eax, 1
		jnz	short loc_2F8C4

		mov	edx, [esi+28h]
		mov	ecx, edi
		add	dl, 50h
		push	edx
		push	64h
		push	11h
		push	eax
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	eax, [esi+20h]
		mov	ecx, edi
		add	al, 50h
		push	eax
		push	65h
		push	11h
		push	1
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	ecx, [esi+28h]
		push	ecx
		mov	ecx, [esi+34h]
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	edx, [esi+20h]
		mov	ecx, [esi+30h]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)


loc_2F8C4:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+4F4j
		mov	eax, [esi+2Ch]
		mov	ecx, ebp
		push	eax
		call	?GetInputChannelRefCount@CResource@@QAEKK@Z ; CResource::GetInputChannelRefCount(ulong)

		cmp	eax, 1
		jnz	short loc_2F919

		mov	eax, [esi+2Ch]
		mov	ecx, edi
		add	al, 50h
		push	eax
		push	0A4h
		push	11h
		push	1
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	ecx, [esi+24h]
		add	cl, 50h
		push	ecx
		push	0A5h
		push	11h
		push	1
		mov	ecx, edi
		call	?Route@Asp4Topology@@QAEXHEEE@Z	; Asp4Topology::Route(int,uchar,uchar,uchar)

		mov	edx, [esi+2Ch]
		mov	ecx, [esi+34h]
		push	edx
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)

		mov	eax, [esi+24h]
		mov	ecx, [esi+30h]
		push	eax
		call	?EnableInput@CAsp4Mix@@QAEXH@Z ; CAsp4Mix::EnableInput(int)


loc_2F919:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+542j
		mov	ecx, [esi+50h]
		mov	edx, [esi+30h]
		mov	eax, [esi+34h]
		push	ecx
		push	edx
		push	eax
		push	11h
		push	1
		mov	ecx, edi
		call	?Connection@Asp4Topology@@QAEXHEPAVCAsp4Mix@@0PAVCAsp4AdbDma@@@Z ; Asp4Topology::Connection(int,uchar,CAsp4Mix *,int,CAsp4AdbDma *)

		mov	ecx, [esi+50h]
		test	ecx, ecx
		jz	loc_2FA2D

		mov	edx, [ecx]
		push	0
		push	0
		push	1
		push	8
		push	0
		push	0
		call	dword ptr [edx+4]

		mov	eax, [esi+50h]
		push	56FFFFFFh
		mov	ecx, [eax+4]
		lea	edx, ds:27800h[ecx*8]
		mov	ecx, [edi+74h]
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+50h]
		push	74FFFFFFh
		mov	ecx, [eax+4]
		lea	edx, ds:27804h[ecx*8]
		mov	ecx, [edi+74h]
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+60h]
		mov	ecx, [eax+4]
		mov	eax, [esi+50h]
		mov	edx, [ecx+1Ch]
		mov	ecx, [eax+4]
		add	ecx, 2740h
		shl	edx, 0Ch
		shl	ecx, 4
		push	edx
		push	ecx
		mov	ecx, [edi+74h]
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	edx, [esi+60h]
		mov	eax, [edx+4]
		mov	edx, [esi+50h]
		mov	ecx, [eax+20h]
		mov	eax, [edx+4]
		shl	eax, 4
		shl	ecx, 0Ch
		add	eax, (offset loc_27402+2)
		push	ecx
		mov	ecx, [edi+74h]
		push	eax
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+60h]
		mov	edx, [ecx+4]
		mov	ecx, [esi+50h]
		mov	eax, [edx+24h]
		mov	edx, [ecx+4]
		mov	ecx, [edi+74h]
		shl	edx, 4
		shl	eax, 0Ch
		add	edx, offset nullsub_2
		push	eax
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+60h]
		mov	ecx, [eax+4]
		mov	eax, [esi+50h]
		mov	edx, [ecx+28h]
		mov	ecx, [eax+4]
		shl	ecx, 4
		shl	edx, 0Ch
		add	ecx, offset dword_2740C
		push	edx
		push	ecx
		mov	ecx, [edi+74h]
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	edx, [esi+50h]
		push	0
		mov	eax, [edx+4]
		lea	ecx, ds:27C00h[eax*4]
		push	ecx
		mov	ecx, [edi+74h]
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)


loc_2FA2D:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+5A5j
		mov	edx, [esi+14h]
		mov	eax, [esi+10h]
		mov	ecx, [esi+54h]
		add	dl, 50h
		push	edx
		add	al, 50h
		mov	edx, [ecx+4]
		push	eax
		push	edx
		push	11h
		push	1
		mov	ecx, edi
		call	?Route@Asp4Topology@@QAEXHEEEE@Z ; Asp4Topology::Route(int,uchar,uchar,uchar,uchar)

		mov	ecx, [esi+54h]
		push	0
		push	0
		push	1
		mov	eax, [ecx]
		push	8
		push	1
		push	0
		call	dword ptr [eax+4]

		mov	ecx, [esi+54h]
		push	56FFFFFFh
		mov	edx, [ecx+4]
		mov	ecx, [edi+74h]
		lea	eax, ds:27800h[edx*8]
		push	eax
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+54h]
		push	74FFFFFFh
		mov	edx, [ecx+4]
		mov	ecx, [edi+74h]
		lea	eax, ds:27804h[edx*8]
		push	eax
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+64h]
		mov	edx, [ecx+4]
		mov	ecx, [esi+54h]
		mov	eax, [edx+1Ch]
		mov	edx, [ecx+4]
		mov	ecx, [edi+74h]
		add	edx, 2740h
		shl	eax, 0Ch
		shl	edx, 4
		push	eax
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+64h]
		mov	ecx, [eax+4]
		mov	eax, [esi+54h]
		mov	edx, [ecx+20h]
		mov	ecx, [eax+4]
		shl	ecx, 4
		shl	edx, 0Ch
		add	ecx, (offset loc_27402+2)
		push	edx
		push	ecx
		mov	ecx, [edi+74h]
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	edx, [esi+64h]
		mov	eax, [edx+4]
		mov	edx, [esi+54h]
		mov	ecx, [eax+24h]
		mov	eax, [edx+4]
		shl	eax, 4
		shl	ecx, 0Ch
		add	eax, offset nullsub_2
		push	ecx
		mov	ecx, [edi+74h]
		push	eax
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+64h]
		mov	edx, [ecx+4]
		mov	ecx, [esi+54h]
		mov	eax, [edx+28h]
		mov	edx, [ecx+4]
		shl	eax, 0Ch
		shl	edx, 4
		push	eax
		add	edx, offset dword_2740C
		mov	ecx, [edi+74h]
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+54h]
		push	0
		mov	ecx, [eax+4]
		lea	edx, ds:27C00h[ecx*4]
		mov	ecx, [edi+74h]
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)


loc_2FB41:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+4A5j
		mov	ecx, [esi+38h]
		mov	ebx, [esi+10h]
		mov	eax, [edi+64h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+3Ch]
		mov	eax, [edi+68h]
		push	eax
		mov	eax, [esi+14h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+40h]
		test	eax, eax
		jz	short loc_2FBDC

		mov	ecx, [esi+44h]
		test	ecx, ecx
		jz	short loc_2FBDC

		mov	edx, [eax+8]
		mov	eax, [esi+10h]
		mov	ecx, [edi+64h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		push	ecx
		mov	ecx, [edi+74h]
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+44h]
		mov	ebx, [esi+14h]
		mov	eax, [edi+68h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)


loc_2FBDC:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+7FEj
					; Asp4Topology::AllocSynth(CResource *)+805j
		mov	ecx, [esi+38h]
		mov	ebx, [esi+8]
		mov	eax, [edi+64h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+3Ch]
		mov	eax, [edi+68h]
		push	eax
		mov	eax, [esi+0Ch]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+40h]
		test	eax, eax
		jz	short loc_2FC77

		mov	ecx, [esi+44h]
		test	ecx, ecx
		jz	short loc_2FC77

		mov	edx, [eax+8]
		mov	eax, [esi+8]
		mov	ecx, [edi+64h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		push	ecx
		mov	ecx, [edi+74h]
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+44h]
		mov	ebx, [esi+0Ch]
		mov	eax, [edi+68h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)


loc_2FC77:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+899j
					; Asp4Topology::AllocSynth(CResource *)+8A0j
		mov	ecx, [esi+38h]
		mov	ebx, [esi+18h]
		mov	eax, [edi+64h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+3Ch]
		mov	eax, [edi+68h]
		push	eax
		mov	eax, [esi+1Ch]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	eax, [esi+40h]
		test	eax, eax
		jz	short loc_2FD12

		mov	ecx, [esi+44h]
		test	ecx, ecx
		jz	short loc_2FD12

		mov	edx, [eax+8]
		mov	eax, [esi+18h]
		mov	ecx, [edi+64h]
		add	edx, 420h
		shl	edx, 5
		add	edx, eax
		push	ecx
		mov	ecx, [edi+74h]
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)

		mov	ecx, [esi+44h]
		mov	ebx, [esi+1Ch]
		mov	eax, [edi+68h]
		mov	edx, [ecx+8]
		mov	ecx, [edi+74h]
		add	edx, 420h
		push	eax
		shl	edx, 5
		add	edx, ebx
		shl	edx, 2
		push	edx
		call	?Write@CAsp4HwIO@@QAEXKK@Z ; CAsp4HwIO::Write(ulong,ulong)


loc_2FD12:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+934j
					; Asp4Topology::AllocSynth(CResource *)+93Bj
		mov	eax, [edi+8Ch]
		test	eax, eax
		jnz	short loc_2FD26

		mov	eax, [edi+88h]
		test	eax, eax
		jz	short loc_2FD72


loc_2FD26:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+98Aj
		mov	eax, [esi+28h]
		mov	ecx, [esi+34h]
		push	8
		push	eax
		call	?SetInputVolumeByte@CAsp4Mix@@QAEXHE@Z ; CAsp4Mix::SetInputVolumeByte(int,uchar)

		mov	ecx, [esi+2Ch]
		push	8
		push	ecx
		mov	ecx, [esi+34h]
		call	?SetInputVolumeByte@CAsp4Mix@@QAEXHE@Z ; CAsp4Mix::SetInputVolumeByte(int,uchar)

		mov	edx, [esi+20h]
		mov	ecx, [esi+30h]
		push	8
		push	edx
		call	?SetInputVolumeByte@CAsp4Mix@@QAEXHE@Z ; CAsp4Mix::SetInputVolumeByte(int,uchar)

		mov	eax, [esi+24h]
		mov	ecx, [esi+30h]
		push	8
		push	eax
		call	?SetInputVolumeByte@CAsp4Mix@@QAEXHE@Z ; CAsp4Mix::SetInputVolumeByte(int,uchar)

		mov	ecx, [esi+30h]
		push	8
		call	?SetVolumeByte@CAsp4Mix@@QAEXE@Z ; CAsp4Mix::SetVolumeByte(uchar)

		mov	ecx, [esi+34h]
		push	8
		call	?SetVolumeByte@CAsp4Mix@@QAEXE@Z ; CAsp4Mix::SetVolumeByte(uchar)


loc_2FD72:				; CODE XREF: Asp4Topology::AllocSynth(CResource	*)+994j
		mov	[edi+7Ch], esi
		mov	eax, esi
		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		add	esp, 14h
		retn	4

?AllocSynth@Asp4Topology@@QAEPAVAsp4SynthTopology@@PAVCResource@@@Z endp ; sp =	-30h

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		align 10h

; ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ S U B	R O U T	I N E ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ


; public: void __thiscall Asp4Topology::DeleteSynth(void)

?DeleteSynth@Asp4Topology@@QAEXXZ proc near
					; CODE XREF: CAsp4Core::SetWavetableTopology(int)+25p
					; .text:0002564Bp
		push	esi
		push	edi
		mov	edi, ecx
		mov	esi, [edi+7Ch]
		test	esi, esi
		jz	short loc_2FDAB

		mov	ecx, esi
		call	??1Asp4SynthTopology@@QAE@XZ ; Asp4SynthTopology::~Asp4SynthTopology(void)

		push	esi
		call	??3@YAXPAX@Z	; operator delete(void *)

		add	esp, 4

loc_2FDAB:				; CODE XREF: Asp4Topology::DeleteSynth(void)+9j
		mov	dword ptr [edi+7Ch], 0
		pop	edi
		pop	esi
		retn

?DeleteSynth@Asp4Topology@@QAEXXZ endp

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		align 10h

; public: static void __cdecl Asp4Topology::Appy_Handler(void *,unsigned long)
?Appy_Handler@Asp4Topology@@SAXPAXK@Z:
		retn

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		align 10h

; ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ S U B	R O U T	I N E ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ


; public: void __thiscall Asp4Topology::Route(int,unsigned char,unsigned char,unsigned char)

?Route@Asp4Topology@@QAEXHEEE@Z	proc near
					; CODE XREF: Asp4Topology::MakeDefaultConnections(void)+2Ap
					; Asp4Topology::MakeDefaultConnections(void)+75p ...

arg_0		= dword	ptr  8
arg_4		= dword	ptr  0Ch
arg_8		= dword	ptr  10h
arg_C		= dword	ptr  14h

		push	ebx
		mov	ebx, [esp+arg_8]
		push	ebp
		push	esi
		push	edi
		mov	edi, ebx
		mov	esi, ecx
		and	edi, 0FFh
		mov	eax, [esp+0Ch+arg_8]
		mov	ecx, edi
		and	ah, 0
		shl	ecx, 8
		or	eax, ecx
		mov	[esp+0Ch+arg_8], eax
		mov	edx, [esp+0Ch+arg_8]
		mov	eax, [esp+0Ch+arg_C]
		and	dl, 0
		and	eax, 0FFh
		or	edx, eax
		mov	[esp+0Ch+arg_8], edx
		mov	eax, [esp+0Ch+arg_0]
		test	eax, eax
		jz	short loc_2FE74

		mov	ebp, [esp+0Ch+arg_4]
		lea	ecx, [esp+0Ch+arg_8]
		push	1
		push	ecx
		mov	ecx, [esi+6Ch]
		push	ebp
		call	?AddRoutes@CAsp4Adb@@QAEXEPATADBRamLink@@H@Z ; CAsp4Adb::AddRoutes(uchar,ADBRamLink *,int)

		cmp	bl, 20h
		jb	short loc_2FE4D

		lea	edx, [edi-20h]
		cmp	edx, 10h
		jge	short loc_2FE4D

		mov	eax, [esi+70h]
		sub	bl, 20h
		push	ebp
		push	ebx
		mov	ecx, [eax+0C8h]
		call	?AddWTD@CAsp4SrcBlock@@QAEHEE@Z	; CAsp4SrcBlock::AddWTD(uchar,uchar)

		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		retn	10h

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2FE4D:				; CODE XREF: Asp4Topology::Route(int,uchar,uchar,uchar)+59j
					; Asp4Topology::Route(int,uchar,uchar,uchar)+61j
		cmp	bl, 30h
		jb	short loc_2FECE

		add	edi, 0FFFFFFD0h
		cmp	edi, 10h
		jge	short loc_2FECE

		mov	ecx, [esi+70h]
		sub	bl, 30h
		push	ebp
		push	ebx
		mov	ecx, [ecx+0C4h]
		call	?AddWTD@CAsp4Mixer@@QAEHEE@Z ; CAsp4Mixer::AddWTD(uchar,uchar)

		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		retn	10h

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2FE74:				; CODE XREF: Asp4Topology::Route(int,uchar,uchar,uchar)+40j
		mov	eax, [esp+0Ch+arg_8]
		mov	ebp, [esp+0Ch+arg_4]
		mov	ecx, [esi+6Ch]
		push	eax
		push	eax
		push	ebp
		call	?DeleteRoutes@CAsp4Adb@@QAEXETADBRamLink@@0@Z ;	CAsp4Adb::DeleteRoutes(uchar,ADBRamLink,uchar)

		cmp	bl, 20h
		jb	short loc_2FEAE

		lea	edx, [edi-20h]
		cmp	edx, 10h
		jge	short loc_2FEAE

		mov	eax, [esi+70h]
		sub	bl, 20h
		push	ebp
		push	ebx
		mov	ecx, [eax+0C8h]
		call	?DeleteWTD@CAsp4SrcBlock@@QAEHEE@Z ; CAsp4SrcBlock::DeleteWTD(uchar,uchar)

		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		retn	10h

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_2FEAE:				; CODE XREF: Asp4Topology::Route(int,uchar,uchar,uchar)+BAj
					; Asp4Topology::Route(int,uchar,uchar,uchar)+C2j
		cmp	bl, 30h
		jb	short loc_2FECE

		add	edi, 0FFFFFFD0h
		cmp	edi, 10h
		jge	short loc_2FECE

		mov	ecx, [esi+70h]
		sub	bl, 30h
		push	ebp
		push	ebx
		mov	ecx, [ecx+0C4h]
		call	?DeleteWTD@CAsp4Mixer@@QAEHEE@Z	; CAsp4Mixer::DeleteWTD(uchar,uchar)


loc_2FECE:				; CODE XREF: Asp4Topology::Route(int,uchar,uchar,uchar)+80j
					; Asp4Topology::Route(int,uchar,uchar,uchar)+88j ...
		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		retn	10h

?Route@Asp4Topology@@QAEXHEEE@Z	endp

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		align 10h




*/

/*



; public: void __thiscall CWTHal::SetReg(unsigned char,int,unsigned short)

?SetReg@CWTHal@@QAEXEHG@Z proc near	; CODE XREF: Asp4Synth::WTTimerCallback(ulong)+E45p
					; Asp4Synth::WTTimerCallback(ulong)+1294p

arg_0		= dword	ptr  0Ch
arg_4		= dword	ptr  10h
arg_8		= dword	ptr  14h

		push	ecx
		push	esi
		mov	esi, [esp+arg_4]
		cmp	esi, 40h
		jge	loc_54507

		mov	eax, esi
		cdq
		and	edx, 1Fh
		add	eax, edx
		mov	edx, esi
		sar	eax, 5
		and	edx, 8000001Fh
		mov	byte ptr [esp+arg_4], al
		jns	short loc_5448D

		dec	edx
		or	edx, 0FFFFFFE0h
		inc	edx

loc_5448D:				; CODE XREF: CWTHal::SetReg(uchar,int,ushort)+26j
		cmp	al, 2
		mov	[esp+4], dl
		jnb	short loc_54507

		mov	eax, [esp+arg_0]
		and	eax, 0FFh
		sub	eax, 20h
		jz	short loc_544D9

		dec	eax
		jnz	short loc_54507

		mov	eax, [esp+arg_4]
		mov	edx, [esp+4]
		and	eax, 0FFh
		and	edx, 0FFh
		shl	eax, 0Bh
		add	eax, edx
		mov	edx, [esp+arg_8]
		shl	eax, 4
		add	eax, 208h
		push	edx
		push	eax
		mov	eax, [ecx]
		push	eax
		call	?WriteWORD@CAsp4HIO@@QAGXKG@Z ;	CAsp4HIO::WriteWORD(ulong,ushort)

		pop	esi
		pop	ecx
		retn	0Ch

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

loc_544D9:				; CODE XREF: CWTHal::SetReg(uchar,int,ushort)+41j
		mov	eax, [esp+arg_4]
		mov	edx, [esp+4]
		and	eax, 0FFh
		and	edx, 0FFh
		shl	eax, 0Bh
		add	eax, edx
		mov	edx, [esp+arg_8]
		shl	eax, 4
		add	eax, 20Ah
		push	edx
		push	eax
		mov	eax, [ecx]
		push	eax
		call	?WriteWORD@CAsp4HIO@@QAGXKG@Z ;	CAsp4HIO::WriteWORD(ulong,ushort)


loc_54507:				; CODE XREF: CWTHal::SetReg(uchar,int,ushort)+9j
					; CWTHal::SetReg(uchar,int,ushort)+33j	...
		pop	esi
		pop	ecx
		retn	0Ch

?SetReg@CWTHal@@QAEXEHG@Z endp

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		align 8

; ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ S U B	R O U T	I N E ÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ


; public: void __thiscall CWTHal::InitializeWTRegs(void)

?InitializeWTRegs@CWTHal@@QAEXXZ proc near ; CODE XREF:	Asp4Synth::init(void)+242p

var_10		= dword	ptr -10h
var_C		= dword	ptr -0Ch
var_8		= dword	ptr -8
var_4		= dword	ptr -4

		sub	esp, 10h
		push	ebx
		push	ebp
		push	esi
		xor	ebx, ebx
		push	edi
		mov	esi, ecx
		mov	[esp+20h+var_10], ebx
		mov	eax, [esp+20h+var_10]
		and	al, 0E3h
		or	al, 22h
		mov	[esp+20h+var_10], eax
		mov	ecx, [esp+20h+var_10]
		and	ecx, 0FFFFFEBFh
		or	cl, 80h
		mov	[esp+20h+var_10], ecx
		mov	edx, [esp+20h+var_10]
		or	dh, 2
		mov	[esp+20h+var_10], edx
		mov	eax, [esp+20h+var_10]
		and	al, 0FEh
		mov	[esp+20h+var_10], eax
		mov	ecx, [esp+20h+var_10]
		and	ch, 0FBh
		or	ch, 18h
		mov	[esp+20h+var_10], ecx
		mov	[esp+20h+var_8], ebx
		mov	edx, [esp+20h+var_8]
		mov	[esp+20h+var_C], edx
		mov	eax, [esp+20h+var_C]
		mov	[esp+20h+var_4], eax
		mov	word ptr [esp+20h+var_4+2], 1000h
		mov	eax, 83h
		mov	word ptr [esp+20h+var_C+2], ax
		mov	word ptr [esp+20h+var_8+2], ax
		xor	edi, edi
		mov	ebp, 2

loc_54590:				; CODE XREF: CWTHal::InitializeWTRegs(void)+C9j
		push	ebx
		push	edi
		push	0Ch
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		mov	ecx, [esp+20h+var_10]
		push	ecx
		push	edi
		push	0Ah
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		mov	edx, [esp+20h+var_4]
		push	edx
		push	edi
		push	9
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		mov	eax, [esp+20h+var_C]
		push	eax
		push	edi
		push	8
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		mov	ecx, [esp+20h+var_8]
		push	ecx
		push	edi
		push	5
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		inc	edi
		dec	ebp
		jnz	short loc_54590

		mov	eax, ?g_num_hw_voices@@3HA ; int g_num_hw_voices
		xor	bl, bl
		test	eax, eax
		jle	short loc_5463D

		xor	edi, edi

loc_545E8:				; CODE XREF: CWTHal::InitializeWTRegs(void)+12Bj
		push	0
		push	edi
		push	4
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		push	0
		push	edi
		push	3
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		push	0
		push	edi
		push	2
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		push	0
		push	edi
		push	1
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		push	0
		push	edi
		push	0Bh
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		mov	eax, ?g_num_hw_voices@@3HA ; int g_num_hw_voices
		inc	bl
		mov	byte ptr [esp+20h+var_4], bl
		mov	edi, [esp+20h+var_4]
		and	edi, 0FFh
		cmp	edi, eax
		jl	short loc_545E8


loc_5463D:				; CODE XREF: CWTHal::InitializeWTRegs(void)+D4j
		mov	edx, [esp+20h+var_10]
		or	edx, 1
		mov	[esp+20h+var_10], edx
		xor	edi, edi
		mov	ebx, 2

loc_5464F:				; CODE XREF: CWTHal::InitializeWTRegs(void)+150j
		mov	eax, [esp+20h+var_10]
		push	eax
		push	edi
		push	0Ah
		mov	ecx, esi
		call	?SetReg@CWTHal@@QAEXEHK@Z ; CWTHal::SetReg(uchar,int,ulong)

		inc	edi
		dec	ebx
		jnz	short loc_5464F

		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		add	esp, 10h
		retn

?InitializeWTRegs@CWTHal@@QAEXXZ endp



*/
