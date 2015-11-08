#if !defined(SCRYPT_CHOOSE_COMPILETIME)
/* function type returned by scrypt_getROMix, used with cpu detection */
typedef void (FASTCALL *scrypt_ROMixfn)(scrypt_mix_word_t *X/*[chunkWords]*/, scrypt_mix_word_t *Y/*[chunkWords]*/, scrypt_mix_word_t *V/*[chunkWords * N]*/, uint32_t N, uint32_t r);

typedef void (FASTCALL *twist_ROMixfn_prepare)(
        scrypt_mix_word_t *X/*[chunkWords]*/, 
        scrypt_mix_word_t *V/*[chunkWords * N]*/, 
        uint8_t *VSeeds/*[chunkWords * numSegments]*/, 
        uint32_t *numSeeds, 
        uint32_t N, 
        uint32_t r, 
        uint32_t numSegments);

typedef void (FASTCALL *twist_ROMixfn_work)(
        scrypt_mix_word_t *X/*[chunkWords]*/, 
        scrypt_mix_word_t *Y/*[2]*/, 
        scrypt_mix_word_t *V/*[chunkWords * N]*/, 
        uint32_t N, 
        uint32_t r,
        uint32_t numSegments,
        uint64_t *Links, 
        uint32_t *linkLengths, 
        uint32_t *numLinks, 
        uint32_t W, 
        uint64_t T, 
        uint32_t *currentLinkLength, 
        uint64_t Target, 
        uint8_t *pfWorking,
        uint8_t *pResetSignal);

typedef uint32_t (FASTCALL *twist_ROMixfn_verify)(
        scrypt_mix_word_t *X/*[chunkWords]*/, 
        scrypt_mix_word_t *Y/*[2]*/, 
        scrypt_mix_word_t *V/*[chunkWords * ?]*/, 
        uint8_t *VSeeds/*[chunkWords * numSegments]*/, 
        uint32_t startSeed, 
        uint32_t numSeedsToUse, 
        uint32_t N, 
        uint32_t r, 
        uint32_t numSegments,
        uint64_t *Links, 
        uint32_t *linkLengths, 
        uint32_t *numLinks, 
        uint32_t startCheckLink, 
        uint32_t endCheckLink, 
        uint64_t T, 
        uint64_t Target, 
        uint32_t *workStep, 
        uint32_t *link,
        uint32_t *j,
        uint32_t numStepsToRecord,
        uint32_t *stepsToRecord,
        scrypt_mix_word_t *recordedSteps);

typedef uint32_t (FASTCALL *twist_ROMixfn_checkrecordedsteps)(
        scrypt_mix_word_t *X/*[chunkWords]*/, 
        scrypt_mix_word_t *Y/*[2]*/, 
        scrypt_mix_word_t *V/*[chunkWords * ?]*/, 
        uint8_t *VSeeds/*[chunkWords * numSegments]*/, 
        uint32_t startSeed, 
        uint32_t numSeedsToUse, 
        uint32_t N, 
        uint32_t r, 
        uint32_t numSegments,
        uint64_t *Links, 
        uint32_t *linkLengths, 
        uint32_t *numLinks, 
        uint64_t T, 
        uint64_t Target, 
        uint32_t *workStep, 
        uint32_t *link,
        uint32_t *j,
        uint32_t numStepsToRecord,
        uint32_t *stepsToRecord,
        scrypt_mix_word_t *newRecordedSteps,
        uint32_t numRecordedOldSteps,
        uint32_t *oldRecordedStepPositions,
        scrypt_mix_word_t *oldRecordedSteps);

typedef uint64_t (FASTCALL *twist_ROMixfn_quickcheck)(
        scrypt_mix_word_t *X/*[chunkWords]*/, 
        scrypt_mix_word_t *Y/*[2]*/, 
        scrypt_mix_word_t *V/*[chunkWords * ?]*/, 
        uint8_t *VSeeds/*[chunkWords * numSegments]*/, 
        uint32_t N, 
        uint32_t r, 
        uint32_t numSegments,
        uint64_t *Links, 
        uint32_t *linkLengths, 
        uint32_t *numLinks, 
        uint64_t T, 
        uint64_t Target, 
        uint32_t *workStep, 
        uint32_t *link);
#endif

/* romix pre/post nop function */
static void asm_calling_convention
scrypt_romix_nop(scrypt_mix_word_t *blocks, size_t nblocks) {
	(void)blocks; (void)nblocks;
}

/* romix pre/post endian conversion function */
static void asm_calling_convention
scrypt_romix_convert_endian(scrypt_mix_word_t *blocks, size_t nblocks) {
#if !defined(CPU_LE)
	static const union { uint8_t b[2]; uint16_t w; } endian_test = {{1,0}};
	size_t i;
	if (endian_test.w == 0x100) {
		nblocks *= SCRYPT_BLOCK_WORDS;
		for (i = 0; i < nblocks; i++) {
			SCRYPT_WORD_ENDIAN_SWAP(blocks[i]);
		}
	}
#else
	(void)blocks; (void)nblocks;
#endif
}

/* chunkmix test function */
typedef void (asm_calling_convention *chunkmixfn)(scrypt_mix_word_t *Bout/*[chunkWords]*/, scrypt_mix_word_t *Bin/*[chunkWords]*/, scrypt_mix_word_t *Bxor/*[chunkWords]*/, uint32_t r);
typedef void (asm_calling_convention *blockfixfn)(scrypt_mix_word_t *blocks, size_t nblocks);

static int
scrypt_test_mix_instance(chunkmixfn mixfn, blockfixfn prefn, blockfixfn postfn, const uint8_t expected[16]) {
	/* r = 2, (2 * r) = 4 blocks in a chunk, 4 * SCRYPT_BLOCK_WORDS total */
	const uint32_t r = 2, blocks = 2 * r, words = blocks * SCRYPT_BLOCK_WORDS;
#if (defined(X86ASM_AVX2) || defined(X86_64ASM_AVX2) || defined(X86_INTRINSIC_AVX2))
	scrypt_mix_word_t ALIGN22(32) chunk[2][4 * SCRYPT_BLOCK_WORDS], v;
#else
	scrypt_mix_word_t ALIGN22(16) chunk[2][4 * SCRYPT_BLOCK_WORDS], v;
#endif
	uint8_t final[16];
	size_t i;

	for (i = 0; i < words; i++) {
		v = (scrypt_mix_word_t)i;
		v = (v << 8) | v;
		v = (v << 16) | v;
		chunk[0][i] = v;
	}

	prefn(chunk[0], blocks);
	mixfn(chunk[1], chunk[0], NULL, r);
	postfn(chunk[1], blocks);

	/* grab the last 16 bytes of the final block */
	for (i = 0; i < 16; i += sizeof(scrypt_mix_word_t)) {
		SCRYPT_WORDTO8_LE(final + i, chunk[1][words - (16 / sizeof(scrypt_mix_word_t)) + (i / sizeof(scrypt_mix_word_t))]);
	}

	return scrypt_verify(expected, final, 16);
}

/* returns a pointer to item i, where item is len scrypt_mix_word_t's long */
static scrypt_mix_word_t *
scrypt_item(scrypt_mix_word_t *base, scrypt_mix_word_t i, scrypt_mix_word_t len) {
	return base + (i * len);
}

/* returns a pointer to block i */
static scrypt_mix_word_t *
scrypt_block(scrypt_mix_word_t *base, scrypt_mix_word_t i) {
	return base + (i * SCRYPT_BLOCK_WORDS);
}
