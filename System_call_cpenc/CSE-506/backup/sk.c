struct crypto_skcipher *skcipher = NULL;
struct skcipher_request *req = NULL;
char *scratchpad = NULL;
char *ivdata = NULL;
unsigned char key[16]="itshouldnotcorru";

skcipher = crypto_alloc_skcipher("ctr(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
        pr_info("could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }

req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate skcipher request\n");
        ret = -ENOMEM;
        goto out;
    }

    if (crypto_skcipher_setkey(skcipher, key, 16)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }

    ivdata = kmalloc(16, GFP_KERNEL);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    get_random_bytes(ivdata, 16);
    ivdata will be incremented automatically as soon as the encrypt is called. memcopy or strcpy of ivdata is needed

struct scatterlist sg;
sg_init_one(&sg, scratchpad, 16);
skcipher_request_set_crypt(req, &sg, &sg, 16, ivdata);
int ret=crypto_skcipher_encrypt(req);
if(ret)
    goto out;
printk("Encryption succeeded\n");

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (ivdata)
        kfree(ivdata);
    if (scratchpad)
        kfree(scratchpad);
    return ret;
}

