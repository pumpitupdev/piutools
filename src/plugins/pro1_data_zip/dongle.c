// vim: set et sw=4 ts=4:
#include <stdint.h>
#include "dongle.h"
#include "enc_zip_file.h"
#include "tomcrypt.h"
#include "ow/shaib.h"
#include "ow/ownet.h"
#include "PIUTools_Debug.h"

int derive_aes_key_from_ds1963s(const enc_zip_file_header *h, uint8_t out[24]) {
    uint8_t scratchpadWorkspace[60], sharesult[20];

    hash_state key_seed, key_iter1, key_iter2;
    sha1_init(&key_seed);
    sha1_init(&key_iter1);
    sha1_init(&key_iter2);

    // sha1(subkey), sha1(subkey+sha1(subkey)), sha1(subkey+sha1(subkey+sha1(subkey)))
    sha1_process(&key_seed, h->subkey, 1024);
    if (sha1_done(&key_seed, sharesult) != CRYPT_OK) {
        fprintf(stderr, "%s: sha1_done failed\n", __FUNCTION__);
        return -1;
    }
    memcpy(scratchpadWorkspace, sharesult, 20);

    sha1_process(&key_iter1, h->subkey, 1024);
    sha1_process(&key_iter1, scratchpadWorkspace, 20);
    if (sha1_done(&key_iter1, sharesult) != CRYPT_OK) {
        fprintf(stderr, "%s: sha1_done failed\n", __FUNCTION__);
        return -1;
    }
    memcpy(scratchpadWorkspace+20, sharesult, 20);

    sha1_process(&key_iter2, h->subkey, 1024);
    sha1_process(&key_iter2, scratchpadWorkspace, 40);
    if (sha1_done(&key_iter2, sharesult) != CRYPT_OK) {
        fprintf(stderr, "%s: sha1_done failed\n", __FUNCTION__);
        return -1;
    }
    memcpy(scratchpadWorkspace+40, sharesult, 20);

    return ds1963s_compute_data_sha(scratchpadWorkspace, out);
}

int ds1963s_compute_data_sha(const uint8_t *input, uint8_t out[24]) {
    uchar firstDataPage[32], firstScratchPad[32];
    SHACopr copr;

    memcpy(firstDataPage, input, 32);

    if ((copr.portnum = owAcquireEx("/dev/ttyS0")) == -1) {
        // HACK: see if the 1963s_in_ds2480b plugin is already operable
        if ((copr.portnum = owAcquireEx("/dev/pts/1")) == -1) {
            DBG_printf("getKey(): failed to acquire port.\nCheck to see that:\n=== [stupid boxor dongle meme goes here]\n");
            return -1;
        }
    }
    FindNewSHA(copr.portnum, copr.devAN, TRUE);
    owSerialNum(copr.portnum, copr.devAN, FALSE);
    WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
    memset(firstScratchPad, '\0', 32);
    memcpy(firstScratchPad+8, input+32, 15);
    WriteScratchpadSHA18(copr.portnum, 0, firstScratchPad, 32, 1);
    SHAFunction18(copr.portnum, SHA_SIGN_DATA_PAGE, 0, 1);
    ReadScratchpadSHA18(copr.portnum, 0, 0, firstScratchPad, 1);
    memset(firstDataPage, '\0', 32);
    WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
    owRelease(copr.portnum);
    memcpy(out, firstScratchPad+8, 24);
    return 0;
}
