#ifndef SCRYPT_JANE_H_
#define SCRYPT_JANE_H_
/*
	scrypt-jane by Andrew M, https://github.com/floodyberry/scrypt-jane

	Public Domain or MIT License, whichever is easier

    Modified to implement twist proof of work by Peer Node, 2014
    Modifications are distributed under the terms of the Gnu Affero GPLv3 License
*/

#include <string.h>

#include "twist-jane.h"
#include "code/scrypt-jane-portable.h"
#include "code/scrypt-jane-hash.h"
#include "code/twist-romix.h"
#include "code/scrypt-jane-test-vectors.h"

#if !defined(WORDSPERSEED)
#define WORDSPERSEED 3
#endif

#define scrypt_maxNfactor 30  /* (1 << (30 + 1)) = ~2 billion */
#if (SCRYPT_BLOCK_BYTES == 64)
#define scrypt_r_32kb 8 /* (1 << 8) = 256 * 2 blocks in a chunk * 64 bytes = Max of 32kb in a chunk */
#elif (SCRYPT_BLOCK_BYTES == 128)
#define scrypt_r_32kb 7 /* (1 << 7) = 128 * 2 blocks in a chunk * 128 bytes = Max of 32kb in a chunk */
#elif (SCRYPT_BLOCK_BYTES == 256)
#define scrypt_r_32kb 6 /* (1 << 6) = 64 * 2 blocks in a chunk * 256 bytes = Max of 32kb in a chunk */
#elif (SCRYPT_BLOCK_BYTES == 512)
#define scrypt_r_32kb 5 /* (1 << 5) = 32 * 2 blocks in a chunk * 512 bytes = Max of 32kb in a chunk */
#endif
#define scrypt_maxrfactor scrypt_r_32kb /* 32kb */
#define scrypt_maxpfactor 25  /* (1 << 25) = ~33 million */

#include <stdio.h>

static void NORETURN
scrypt_fatal_error_default(const char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

static scrypt_fatal_errorfn scrypt_fatal_error = scrypt_fatal_error_default;

static void
scrypt_set_fatal_error(scrypt_fatal_errorfn fn) {
	scrypt_fatal_error = fn;
}

static int
scrypt_power_on_self_test(void) {
	const scrypt_test_setting *t;
	uint8_t test_digest[64];
	uint32_t i;
	int res = 7, scrypt_valid;

	if (!scrypt_test_mix()) {
#if !defined(SCRYPT_TEST)
		scrypt_fatal_error("scrypt: mix function power-on-self-test failed");
#endif
		res &= ~1;
	}

	if (!scrypt_test_hash()) {
#if !defined(SCRYPT_TEST)
		scrypt_fatal_error("scrypt: hash function power-on-self-test failed");
#endif
		res &= ~2;
	}

	for (i = 0, scrypt_valid = 1; post_settings[i].pw; i++) {
		t = post_settings + i;
		scrypt((uint8_t *)t->pw, strlen(t->pw), (uint8_t *)t->salt, strlen(t->salt), t->Nfactor, t->rfactor, t->pfactor, test_digest, sizeof(test_digest));
		scrypt_valid &= scrypt_verify(post_vectors[i], test_digest, sizeof(test_digest));
	}
	
	if (!scrypt_valid) {
#if !defined(SCRYPT_TEST)
		scrypt_fatal_error("scrypt: scrypt power-on-self-test failed");
#endif
		res &= ~4;
	}

	return res;
}

#if defined(SCRYPT_TEST_SPEED)
static uint8_t *mem_base = (uint8_t *)0;
static size_t mem_bump = 0;

/* allocations are assumed to be multiples of 64 bytes and total allocations not to exceed ~1.01gb */
static scrypt_aligned_alloc
scrypt_alloc(uint64_t size) {
	scrypt_aligned_alloc aa;
    uint64_t allocated_size = (1024 * 1024 * 1024) + (1024 * 1024) + (SCRYPT_BLOCK_BYTES - 1);
	if (!mem_base) {
		mem_base = (uint8_t *)malloc(allocated_size);
		if (!mem_base)
			scrypt_fatal_error("scrypt: out of memory");
		mem_base = (uint8_t *)(((size_t)mem_base + (SCRYPT_BLOCK_BYTES - 1)) & ~(SCRYPT_BLOCK_BYTES - 1));
	}
    if (mem_base + mem_bump >= allocated_size)
        scrypt_fatal_error("scrypt: out of memory");
	aa.mem = mem_base + mem_bump;
	aa.ptr = aa.mem;
	mem_bump += (size_t)size;
	return aa;
}

static void
scrypt_free(scrypt_aligned_alloc *aa) {
	mem_bump = 0;
}
#else

#endif

static void
scrypt(const uint8_t *password, size_t password_len, const uint8_t *salt, 
       size_t salt_len, uint8_t Nfactor, uint8_t rfactor, uint8_t pfactor, 
       uint8_t *out, size_t bytes) {
	scrypt_aligned_alloc YX, V;
	uint8_t *X, *Y;
	uint32_t N, r, p, chunk_bytes, i;

#if !defined(SCRYPT_CHOOSE_COMPILETIME)
	scrypt_ROMixfn scrypt_ROMix = scrypt_getROMix();
#endif

#if !defined(SCRYPT_TEST)
	static int power_on_self_test = 0;
	if (!power_on_self_test) {
		power_on_self_test = 1;
		if (!scrypt_power_on_self_test())
			scrypt_fatal_error("scrypt: power on self test failed");
	}
#endif

	if (Nfactor > scrypt_maxNfactor)
		scrypt_fatal_error("scrypt: N out of range");
	if (rfactor > scrypt_maxrfactor)
		scrypt_fatal_error("scrypt: r out of range");
	if (pfactor > scrypt_maxpfactor)
		scrypt_fatal_error("scrypt: p out of range");

	N = (1 << (Nfactor + 1));
	r = (1 << rfactor);
	p = (1 << pfactor);

	chunk_bytes = SCRYPT_BLOCK_BYTES * r * 2;
	V = scrypt_alloc((uint64_t)N * chunk_bytes);
	YX = scrypt_alloc((p + 1) * chunk_bytes);

	/* 1: X = PBKDF2(password, salt) */
	Y = YX.ptr;
	X = Y + chunk_bytes;
	scrypt_pbkdf2(password, password_len, salt, salt_len, 1, X, chunk_bytes * p);

	/* 2: X = ROMix(X) */
	for (i = 0; i < p; i++)
		scrypt_ROMix((scrypt_mix_word_t *)(X + (chunk_bytes * i)), (scrypt_mix_word_t *)Y, (scrypt_mix_word_t *)V.ptr, N, r);

	/* 3: Out = PBKDF2(password, X) */
	scrypt_pbkdf2(password, password_len, X, chunk_bytes * p, 1, out, bytes);

	scrypt_ensure_zero(YX.ptr, (p + 1) * chunk_bytes);

	scrypt_free(&V);
	scrypt_free(&YX);
}

static void
twist_prepare(const uint8_t *password, 
              size_t password_len, 
              const uint8_t *salt, 
              size_t salt_len, 
              uint8_t Nfactor, 
              uint8_t rfactor, 
              uint32_t numSegments,
              scrypt_aligned_alloc *V,
              scrypt_aligned_alloc *VSeeds) {

	scrypt_aligned_alloc YX;
	uint8_t *X, *Y;
	uint32_t N, r, chunk_bytes, numSeeds;

#if !defined(SCRYPT_TEST)
	static int power_on_self_test = 0;
	if (!power_on_self_test) {
		power_on_self_test = 1;
		if (!scrypt_power_on_self_test())
			scrypt_fatal_error("twist: power on self test failed");
	}
#endif

	if (Nfactor > scrypt_maxNfactor)
		scrypt_fatal_error("twist: N out of range");
	if (rfactor > scrypt_maxrfactor)
		scrypt_fatal_error("twist: r out of range");

	N = (1 << (Nfactor + 1));
	r = (1 << rfactor);
	chunk_bytes = SCRYPT_BLOCK_BYTES * r * 2;
	*V = scrypt_alloc((uint64_t)(N + 1) * chunk_bytes);
	*VSeeds = scrypt_alloc((uint64_t)numSegments 
                           * WORDSPERSEED * sizeof(scrypt_mix_word_t));
	YX = scrypt_alloc(2 * chunk_bytes);

	Y = YX.ptr;
	X = Y + chunk_bytes;
	scrypt_pbkdf2(password, password_len, salt, salt_len, 1, X, chunk_bytes);

    TWIST_PREPARE_FN((scrypt_mix_word_t *)X, 
                     (scrypt_mix_word_t *)(*V).ptr, 
                     (uint8_t *)(*VSeeds).ptr, 
                     &numSeeds,
                     N, r, numSegments);

	scrypt_ensure_zero(YX.ptr, 2 * chunk_bytes);

	scrypt_free(&YX);
}

static void
twist_work(uint8_t *pFirstLink,
           uint8_t Nfactor, 
           uint8_t rfactor, 
           uint32_t numSegments,
           scrypt_aligned_alloc *V, 
           uint64_t *Links, 
           uint32_t *linkLengths, 
           uint32_t *numLinks, 
           uint32_t W,
           uint128_t T,
           uint128_t Target,
           uint128_t *quick_verifier,
           uint8_t *pfWorking)
{
	uint32_t N, r, currentLinkLength, chunk_bytes;
	scrypt_aligned_alloc YX;
	uint8_t *Y;
    currentLinkLength = 0;

	N = (1 << (Nfactor + 1));
	r = (1 << rfactor);

	chunk_bytes = SCRYPT_BLOCK_BYTES * r * 2;
	YX = scrypt_alloc(chunk_bytes);
	Y = YX.ptr;
    
    
	TWIST_WORK_FN((scrypt_mix_word_t *)pFirstLink, 
                  (scrypt_mix_word_t *)Y,
                  (scrypt_mix_word_t *)(*V).ptr, 
                  N, 
                  r,
                  numSegments,
                  Links, 
                  linkLengths, 
                  numLinks,
                  W,
                  T,
                  &currentLinkLength,
                  Target,
                  quick_verifier,
                  pfWorking);
	scrypt_free(&YX);
}

static uint32_t
twist_verify(uint8_t *pHashInput,
             uint8_t Nfactor, 
             uint8_t rfactor, 
             uint32_t numSegments,
             scrypt_aligned_alloc *VSeeds,
             uint32_t startSeed,
             uint32_t numSeedsToUse,
             uint64_t *Links, 
             uint32_t *linkLengths, 
             uint32_t *numLinks, 
             uint32_t startCheckLink,
             uint32_t endCheckLink,
             uint128_t T,
             uint128_t Target,
             uint32_t *workStep,
             uint32_t *link,
             uint32_t *seed)
{
	uint32_t N, r, result, chunk_bytes, segmentSize, j;
    uint32_t bytesPerLink = 8;
	scrypt_aligned_alloc YX, V;
	uint8_t *X, *Y;

	N = (1 << (Nfactor + 1));
	r = (1 << rfactor);

    segmentSize = N / numSegments;
	chunk_bytes = SCRYPT_BLOCK_BYTES * r * 2;
	YX = scrypt_alloc(2 * chunk_bytes);
	Y = YX.ptr;
	X = Y + chunk_bytes;
	V = scrypt_alloc((uint64_t)numSeedsToUse 
                     * (uint64_t)chunk_bytes 
                     * (1 + (uint64_t)(N / numSegments)));

    scrypt_pbkdf2((const uint8_t *)pHashInput, 32, (const uint8_t *)"", 0, 
                  1, X, chunk_bytes);

    result = TWIST_VERIFY_FN((scrypt_mix_word_t *)X, 
                             (scrypt_mix_word_t *)Y,
                             (scrypt_mix_word_t *)V.ptr, 
                             (uint8_t *)(*VSeeds).ptr, 
                             startSeed,
                             numSeedsToUse,
                             N, 
                             r,
                             numSegments,
                             Links, 
                             linkLengths, 
                             numLinks,
                             startCheckLink,
                             endCheckLink,
                             T,
                             Target,
                             workStep,
                             link,
                             &j);
    *seed = (bytesPerLink * j) / (segmentSize * chunk_bytes);
	scrypt_free(&V);
	scrypt_free(&YX);
    return result;
}

static uint128_t
twist_quickcheck(uint8_t *pHashInput,
                 uint8_t Nfactor, 
                 uint8_t rfactor, 
                 uint32_t numSegments,
                 scrypt_aligned_alloc *VSeeds,
                 uint64_t *Links, 
                 uint32_t *linkLengths, 
                 uint32_t *numLinks, 
                 uint128_t T,
                 uint128_t Target,
                 uint128_t *quick_verifier)
{
	uint32_t N, r, chunk_bytes;
    uint128_t result;
	scrypt_aligned_alloc YX, V;
	uint8_t *X, *Y;

	N = (1 << (Nfactor + 1));
	r = (1 << rfactor);

	chunk_bytes = SCRYPT_BLOCK_BYTES * r * 2;
	YX = scrypt_alloc(2 * chunk_bytes);
	Y = YX.ptr;
	X = Y + chunk_bytes;
	V = scrypt_alloc((uint64_t)chunk_bytes 
                     * (1 + (uint64_t)(N / numSegments)));

    scrypt_pbkdf2((const uint8_t *)pHashInput, 32, (const uint8_t *)"", 0, 
                  1, X, chunk_bytes);

    result = TWIST_QUICKCHECK_FN((scrypt_mix_word_t *)X, 
                                 (scrypt_mix_word_t *)Y,
                                 (scrypt_mix_word_t *)V.ptr, 
                                 (uint8_t *)(*VSeeds).ptr, 
                                 N, 
                                 r,
                                 numSegments,
                                 Links, 
                                 linkLengths, 
                                 numLinks,
                                 T,
                                 Target,
                                 quick_verifier);
	scrypt_free(&V);
	scrypt_free(&YX);
    return result;
}

#endif
