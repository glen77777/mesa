/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_context.h"
#include "brw_blorp.h"
#include "brw_reg.h"
#include <stdio.h>

const unsigned *
brw_blorp_blit_test_compile(struct brw_context *brw,
                            const brw_blorp_blit_prog_key *key,
                            FILE *out);

static char *
fcopy(FILE *f, size_t *len)
{
   char *res;
   *len = (size_t)ftell(f);

   if (!*len)
      return NULL;

   res = (char *)malloc(*len);

   (void)fseek(f, 0L, SEEK_SET);

   if (fread(res, 1, *len, f) != *len) {
      fclose(f);
      free(res);
      return NULL;
   }

   fclose(f);

   return res;
}

static bool
check(struct brw_context *brw, const brw_blorp_blit_prog_key *key,
      const char *expected, size_t expected_len)
{
   char *s;
   size_t len;
   FILE *tmp = tmpfile();

   if (!tmp) {
      printf("failed to create temporary file for the dump\n");
      return false;
   }

   brw_blorp_blit_test_compile(brw, key, tmp);

   s = fcopy(tmp, &len);
   if (!s)
      return false;

   if (expected_len != len) {
      printf("length (%lu) mismatch (%lu expected)\n", len, expected_len);
      fwrite(s, 1, len, stdout);
      free(s);
      return false;
   }

   if (memcmp(s, expected, len)) {
      printf("content mismatch\n");
      fwrite(s, 1, len, stdout);
      free(s);
      return false;
   }

   return true;
}

/**
 * One of the flavours gotten when running piglit test:
 * "ext_framebuffer_multisample-blit-scaled 8"
 */
static bool
test_gen7_blend_scaled_msaa_8(struct brw_context *brw)
{
   static const char expected[] =
      "0x00000000: add(16)         g44<1>UW        g1.4<2,4,0>UW   0x10101010V     { align1 WE_normal 1H };\n"
      "0x00000010: add(16)         g46<1>UW        g1.5<2,4,0>UW   0x11001100V     { align1 WE_normal 1H };\n"
      "0x00000020: mov(16)         g48<1>UD        g44<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000030: mov(16)         g50<1>UD        g46<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000040: mov(16)         g44<1>F         g48<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000050: mov(16)         g46<1>F         g50<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000060: mul(16)         g48<1>F         g44<8,8,1>F     g2.6<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x00000070: mul(16)         g50<1>F         g46<8,8,1>F     g3<0,1,0>F      { align1 WE_normal 1H };\n"
      "0x00000080: add(16)         g48<1>F         g48<8,8,1>F     g2.7<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x00000090: add(16)         g50<1>F         g50<8,8,1>F     g3.1<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000a0: mul(16)         g48<1>F         g48<8,8,1>F     2F              { align1 WE_normal 1H };\n"
      "0x000000b0: mul(16)         g50<1>F         g50<8,8,1>F     4F              { align1 WE_normal 1H };\n"
      "0x000000c0: add(16)         g48<1>F         g48<8,8,1>F     -0.5F           { align1 WE_normal 1H };\n"
      "0x000000d0: add(16)         g50<1>F         g50<8,8,1>F     -0.5F           { align1 WE_normal 1H };\n"
      "0x000000e0: cmp.l.f0(16)    null            g48<8,8,1>F     0F              { align1 WE_normal 1H switch };\n"
      "0x000000f0: (+f0) mov(16)   g48<1>F         0F                              { align1 WE_normal 1H };\n"
      "0x00000100: cmp.g.f0(16)    null            g48<8,8,1>F     g2.4<0,1,0>F    { align1 WE_normal 1H switch };\n"
      "0x00000110: (+f0) mov(16)   g48<1>F         g2.4<0,1,0>F                    { align1 WE_normal 1H };\n"
      "0x00000120: cmp.l.f0(16)    null            g50<8,8,1>F     0F              { align1 WE_normal 1H switch };\n"
      "0x00000130: (+f0) mov(16)   g50<1>F         0F                              { align1 WE_normal 1H };\n"
      "0x00000140: cmp.g.f0(16)    null            g50<8,8,1>F     g2.5<0,1,0>F    { align1 WE_normal 1H switch };\n"
      "0x00000150: (+f0) mov(16)   g50<1>F         g2.5<0,1,0>F                    { align1 WE_normal 1H };\n"
      "0x00000160: frc(16)         g56<1>F         g48<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000170: frc(16)         g58<1>F         g50<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000180: rndd(16)        g44<1>F         g48<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000190: rndd(16)        g46<1>F         g50<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x000001a0: mul(16)         g48<1>F         g44<8,8,1>F     0.5F            { align1 WE_normal 1H };\n"
      "0x000001b0: mul(16)         g50<1>F         g46<8,8,1>F     0.25F           { align1 WE_normal 1H };\n"
      "0x000001c0: add(16)         g52<1>F         g48<8,8,1>F     0F              { align1 WE_normal 1H };\n"
      "0x000001d0: add(16)         g54<1>F         g50<8,8,1>F     0F              { align1 WE_normal 1H };\n"
      "0x000001e0: mov(16)         g44<1>UD        g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x000001f0: mov(16)         g46<1>UD        g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000200: frc(16)         g62<1>F         g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000210: frc(16)         g64<1>F         g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000220: mul(16)         g62<1>F         g62<8,8,1>F     2F              { align1 WE_normal 1H };\n"
      "0x00000230: mul(16)         g64<1>F         g64<8,8,1>F     8F              { align1 WE_normal 1H };\n"
      "0x00000240: add(16)         g62<1>F         g62<8,8,1>F     g64<8,8,1>F     { align1 WE_normal 1H };\n"
      "0x00000250: mov(16)         g60<1>UD        g62<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000260: cmp.l.f0(16)    null            g60<8,8,1>UD    4D              { align1 WE_normal 1H switch };\n"
      "0x00000270: (+f0) if(16) 32 18              null            0x00200012UD    { align1 WE_normal 1H switch };\n"
      "0x00000280: mov(16)         g64<1>UD        5D                              { align1 WE_normal 1H };\n"
      "0x00000290: cmp.e.f0(16)    null            g60<8,8,1>UD    1D              { align1 WE_normal 1H switch };\n"
      "0x000002a0: (+f0) mov(16)   g64<1>UD        2D                              { align1 WE_normal 1H };\n"
      "0x000002b0: cmp.e.f0(16)    null            g60<8,8,1>UD    2D              { align1 WE_normal 1H switch };\n"
      "0x000002c0: (+f0) mov(16)   g64<1>UD        4D                              { align1 WE_normal 1H };\n"
      "0x000002d0: cmp.e.f0(16)    null            g60<8,8,1>UD    3D              { align1 WE_normal 1H switch };\n"
      "0x000002e0: (+f0) mov(16)   g64<1>UD        6D                              { align1 WE_normal 1H };\n"
      "0x000002f0: else(16) 16                     null            0x00000010UD    { align1 WE_normal 1H switch };\n"
      "0x00000300: mov(16)         g64<1>UD        0D                              { align1 WE_normal 1H };\n"
      "0x00000310: cmp.e.f0(16)    null            g60<8,8,1>UD    5D              { align1 WE_normal 1H switch };\n"
      "0x00000320: (+f0) mov(16)   g64<1>UD        3D                              { align1 WE_normal 1H };\n"
      "0x00000330: cmp.e.f0(16)    null            g60<8,8,1>UD    6D              { align1 WE_normal 1H switch };\n"
      "0x00000340: (+f0) mov(16)   g64<1>UD        7D                              { align1 WE_normal 1H };\n"
      "0x00000350: cmp.e.f0(16)    null            g60<8,8,1>UD    7D              { align1 WE_normal 1H switch };\n"
      "0x00000360: (+f0) mov(16)   g64<1>UD        1D                              { align1 WE_normal 1H };\n"
      "0x00000370: endif(16) 2                     null            0x00000002UD    { align1 WE_normal 1H switch };\n"
      "0x00000380: mov(16)         g60<1>UD        g64<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000390: mov(16)         g114<1>UD       g60<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000003a0: mov(16)         g116<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000003b0: mov(16)         g118<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000003c0: send(16)        g4<1>UW         g114<8,8,1>F\n"
      "                sampler (0, 0, 31, 2) mlen 6 rlen 8             { align1 WE_normal 1H };\n"
      "0x000003d0: add(16)         g52<1>F         g48<8,8,1>F     0.5F            { align1 WE_normal 1H };\n"
      "0x000003e0: add(16)         g54<1>F         g50<8,8,1>F     0F              { align1 WE_normal 1H };\n"
      "0x000003f0: mov(16)         g44<1>UD        g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000400: mov(16)         g46<1>UD        g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000410: frc(16)         g62<1>F         g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000420: frc(16)         g64<1>F         g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000430: mul(16)         g62<1>F         g62<8,8,1>F     2F              { align1 WE_normal 1H };\n"
      "0x00000440: mul(16)         g64<1>F         g64<8,8,1>F     8F              { align1 WE_normal 1H };\n"
      "0x00000450: add(16)         g62<1>F         g62<8,8,1>F     g64<8,8,1>F     { align1 WE_normal 1H };\n"
      "0x00000460: mov(16)         g60<1>UD        g62<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000470: cmp.l.f0(16)    null            g60<8,8,1>UD    4D              { align1 WE_normal 1H switch };\n"
      "0x00000480: (+f0) if(16) 32 18              null            0x00200012UD    { align1 WE_normal 1H switch };\n"
      "0x00000490: mov(16)         g64<1>UD        5D                              { align1 WE_normal 1H };\n"
      "0x000004a0: cmp.e.f0(16)    null            g60<8,8,1>UD    1D              { align1 WE_normal 1H switch };\n"
      "0x000004b0: (+f0) mov(16)   g64<1>UD        2D                              { align1 WE_normal 1H };\n"
      "0x000004c0: cmp.e.f0(16)    null            g60<8,8,1>UD    2D              { align1 WE_normal 1H switch };\n"
      "0x000004d0: (+f0) mov(16)   g64<1>UD        4D                              { align1 WE_normal 1H };\n"
      "0x000004e0: cmp.e.f0(16)    null            g60<8,8,1>UD    3D              { align1 WE_normal 1H switch };\n"
      "0x000004f0: (+f0) mov(16)   g64<1>UD        6D                              { align1 WE_normal 1H };\n"
      "0x00000500: else(16) 16                     null            0x00000010UD    { align1 WE_normal 1H switch };\n"
      "0x00000510: mov(16)         g64<1>UD        0D                              { align1 WE_normal 1H };\n"
      "0x00000520: cmp.e.f0(16)    null            g60<8,8,1>UD    5D              { align1 WE_normal 1H switch };\n"
      "0x00000530: (+f0) mov(16)   g64<1>UD        3D                              { align1 WE_normal 1H };\n"
      "0x00000540: cmp.e.f0(16)    null            g60<8,8,1>UD    6D              { align1 WE_normal 1H switch };\n"
      "0x00000550: (+f0) mov(16)   g64<1>UD        7D                              { align1 WE_normal 1H };\n"
      "0x00000560: cmp.e.f0(16)    null            g60<8,8,1>UD    7D              { align1 WE_normal 1H switch };\n"
      "0x00000570: (+f0) mov(16)   g64<1>UD        1D                              { align1 WE_normal 1H };\n"
      "0x00000580: endif(16) 2                     null            0x00000002UD    { align1 WE_normal 1H switch };\n"
      "0x00000590: mov(16)         g60<1>UD        g64<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000005a0: mov(16)         g114<1>UD       g60<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000005b0: mov(16)         g116<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000005c0: mov(16)         g118<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000005d0: send(16)        g12<1>UW        g114<8,8,1>F\n"
      "                sampler (0, 0, 31, 2) mlen 6 rlen 8             { align1 WE_normal 1H };\n"
      "0x000005e0: add(16)         g52<1>F         g48<8,8,1>F     0F              { align1 WE_normal 1H };\n"
      "0x000005f0: add(16)         g54<1>F         g50<8,8,1>F     0.25F           { align1 WE_normal 1H };\n"
      "0x00000600: mov(16)         g44<1>UD        g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000610: mov(16)         g46<1>UD        g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000620: frc(16)         g62<1>F         g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000630: frc(16)         g64<1>F         g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000640: mul(16)         g62<1>F         g62<8,8,1>F     2F              { align1 WE_normal 1H };\n"
      "0x00000650: mul(16)         g64<1>F         g64<8,8,1>F     8F              { align1 WE_normal 1H };\n"
      "0x00000660: add(16)         g62<1>F         g62<8,8,1>F     g64<8,8,1>F     { align1 WE_normal 1H };\n"
      "0x00000670: mov(16)         g60<1>UD        g62<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000680: cmp.l.f0(16)    null            g60<8,8,1>UD    4D              { align1 WE_normal 1H switch };\n"
      "0x00000690: (+f0) if(16) 32 18              null            0x00200012UD    { align1 WE_normal 1H switch };\n"
      "0x000006a0: mov(16)         g64<1>UD        5D                              { align1 WE_normal 1H };\n"
      "0x000006b0: cmp.e.f0(16)    null            g60<8,8,1>UD    1D              { align1 WE_normal 1H switch };\n"
      "0x000006c0: (+f0) mov(16)   g64<1>UD        2D                              { align1 WE_normal 1H };\n"
      "0x000006d0: cmp.e.f0(16)    null            g60<8,8,1>UD    2D              { align1 WE_normal 1H switch };\n"
      "0x000006e0: (+f0) mov(16)   g64<1>UD        4D                              { align1 WE_normal 1H };\n"
      "0x000006f0: cmp.e.f0(16)    null            g60<8,8,1>UD    3D              { align1 WE_normal 1H switch };\n"
      "0x00000700: (+f0) mov(16)   g64<1>UD        6D                              { align1 WE_normal 1H };\n"
      "0x00000710: else(16) 16                     null            0x00000010UD    { align1 WE_normal 1H switch };\n"
      "0x00000720: mov(16)         g64<1>UD        0D                              { align1 WE_normal 1H };\n"
      "0x00000730: cmp.e.f0(16)    null            g60<8,8,1>UD    5D              { align1 WE_normal 1H switch };\n"
      "0x00000740: (+f0) mov(16)   g64<1>UD        3D                              { align1 WE_normal 1H };\n"
      "0x00000750: cmp.e.f0(16)    null            g60<8,8,1>UD    6D              { align1 WE_normal 1H switch };\n"
      "0x00000760: (+f0) mov(16)   g64<1>UD        7D                              { align1 WE_normal 1H };\n"
      "0x00000770: cmp.e.f0(16)    null            g60<8,8,1>UD    7D              { align1 WE_normal 1H switch };\n"
      "0x00000780: (+f0) mov(16)   g64<1>UD        1D                              { align1 WE_normal 1H };\n"
      "0x00000790: endif(16) 2                     null            0x00000002UD    { align1 WE_normal 1H switch };\n"
      "0x000007a0: mov(16)         g60<1>UD        g64<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000007b0: mov(16)         g114<1>UD       g60<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000007c0: mov(16)         g116<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000007d0: mov(16)         g118<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000007e0: send(16)        g20<1>UW        g114<8,8,1>F\n"
      "                sampler (0, 0, 31, 2) mlen 6 rlen 8             { align1 WE_normal 1H };\n"
      "0x000007f0: add(16)         g52<1>F         g48<8,8,1>F     0.5F            { align1 WE_normal 1H };\n"
      "0x00000800: add(16)         g54<1>F         g50<8,8,1>F     0.25F           { align1 WE_normal 1H };\n"
      "0x00000810: mov(16)         g44<1>UD        g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000820: mov(16)         g46<1>UD        g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000830: frc(16)         g62<1>F         g52<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000840: frc(16)         g64<1>F         g54<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000850: mul(16)         g62<1>F         g62<8,8,1>F     2F              { align1 WE_normal 1H };\n"
      "0x00000860: mul(16)         g64<1>F         g64<8,8,1>F     8F              { align1 WE_normal 1H };\n"
      "0x00000870: add(16)         g62<1>F         g62<8,8,1>F     g64<8,8,1>F     { align1 WE_normal 1H };\n"
      "0x00000880: mov(16)         g60<1>UD        g62<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000890: cmp.l.f0(16)    null            g60<8,8,1>UD    4D              { align1 WE_normal 1H switch };\n"
      "0x000008a0: (+f0) if(16) 32 18              null            0x00200012UD    { align1 WE_normal 1H switch };\n"
      "0x000008b0: mov(16)         g64<1>UD        5D                              { align1 WE_normal 1H };\n"
      "0x000008c0: cmp.e.f0(16)    null            g60<8,8,1>UD    1D              { align1 WE_normal 1H switch };\n"
      "0x000008d0: (+f0) mov(16)   g64<1>UD        2D                              { align1 WE_normal 1H };\n"
      "0x000008e0: cmp.e.f0(16)    null            g60<8,8,1>UD    2D              { align1 WE_normal 1H switch };\n"
      "0x000008f0: (+f0) mov(16)   g64<1>UD        4D                              { align1 WE_normal 1H };\n"
      "0x00000900: cmp.e.f0(16)    null            g60<8,8,1>UD    3D              { align1 WE_normal 1H switch };\n"
      "0x00000910: (+f0) mov(16)   g64<1>UD        6D                              { align1 WE_normal 1H };\n"
      "0x00000920: else(16) 16                     null            0x00000010UD    { align1 WE_normal 1H switch };\n"
      "0x00000930: mov(16)         g64<1>UD        0D                              { align1 WE_normal 1H };\n"
      "0x00000940: cmp.e.f0(16)    null            g60<8,8,1>UD    5D              { align1 WE_normal 1H switch };\n"
      "0x00000950: (+f0) mov(16)   g64<1>UD        3D                              { align1 WE_normal 1H };\n"
      "0x00000960: cmp.e.f0(16)    null            g60<8,8,1>UD    6D              { align1 WE_normal 1H switch };\n"
      "0x00000970: (+f0) mov(16)   g64<1>UD        7D                              { align1 WE_normal 1H };\n"
      "0x00000980: cmp.e.f0(16)    null            g60<8,8,1>UD    7D              { align1 WE_normal 1H switch };\n"
      "0x00000990: (+f0) mov(16)   g64<1>UD        1D                              { align1 WE_normal 1H };\n"
      "0x000009a0: endif(16) 2                     null            0x00000002UD    { align1 WE_normal 1H switch };\n"
      "0x000009b0: mov(16)         g60<1>UD        g64<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000009c0: mov(16)         g114<1>UD       g60<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000009d0: mov(16)         g116<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000009e0: mov(16)         g118<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000009f0: send(16)        g28<1>UW        g114<8,8,1>F\n"
      "                sampler (0, 0, 31, 2) mlen 6 rlen 8             { align1 WE_normal 1H };\n"
      "0x00000a00: lrp(8)          g20<1>F         g56<4,1,1>F     g28<4,1,1>F     g20<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a10: lrp(8)          g21<1>F         g57<4,1,1>F     g29<4,1,1>F     g21<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a20: lrp(8)          g22<1>F         g56<4,1,1>F     g30<4,1,1>F     g22<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a30: lrp(8)          g23<1>F         g57<4,1,1>F     g31<4,1,1>F     g23<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a40: lrp(8)          g24<1>F         g56<4,1,1>F     g32<4,1,1>F     g24<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a50: lrp(8)          g25<1>F         g57<4,1,1>F     g33<4,1,1>F     g25<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a60: lrp(8)          g26<1>F         g56<4,1,1>F     g34<4,1,1>F     g26<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a70: lrp(8)          g27<1>F         g57<4,1,1>F     g35<4,1,1>F     g27<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a80: lrp(8)          g4<1>F          g56<4,1,1>F     g12<4,1,1>F     g4<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000a90: lrp(8)          g5<1>F          g57<4,1,1>F     g13<4,1,1>F     g5<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000aa0: lrp(8)          g6<1>F          g56<4,1,1>F     g14<4,1,1>F     g6<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000ab0: lrp(8)          g7<1>F          g57<4,1,1>F     g15<4,1,1>F     g7<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000ac0: lrp(8)          g8<1>F          g56<4,1,1>F     g16<4,1,1>F     g8<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000ad0: lrp(8)          g9<1>F          g57<4,1,1>F     g17<4,1,1>F     g9<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000ae0: lrp(8)          g10<1>F         g56<4,1,1>F     g18<4,1,1>F     g10<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000af0: lrp(8)          g11<1>F         g57<4,1,1>F     g19<4,1,1>F     g11<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b00: lrp(8)          g4<1>F          g58<4,1,1>F     g20<4,1,1>F     g4<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b10: lrp(8)          g5<1>F          g59<4,1,1>F     g21<4,1,1>F     g5<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b20: lrp(8)          g6<1>F          g58<4,1,1>F     g22<4,1,1>F     g6<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b30: lrp(8)          g7<1>F          g59<4,1,1>F     g23<4,1,1>F     g7<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b40: lrp(8)          g8<1>F          g58<4,1,1>F     g24<4,1,1>F     g8<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b50: lrp(8)          g9<1>F          g59<4,1,1>F     g25<4,1,1>F     g9<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b60: lrp(8)          g10<1>F         g58<4,1,1>F     g26<4,1,1>F     g10<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b70: lrp(8)          g11<1>F         g59<4,1,1>F     g27<4,1,1>F     g11<4,1,1>F { align16 WE_normal 1Q };\n"
      "0x00000b80: mov(16)         g114<1>F        g4<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000b90: mov(16)         g116<1>F        g6<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000ba0: mov(16)         g118<1>F        g8<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000bb0: mov(16)         g120<1>F        g10<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000bc0: sendc(16)       null            g114<8,8,1>F\n"
      "                render ( RT write, 1, 0, 12) mlen 8 rlen 0      { align1 WE_normal 1H EOT };\n";
   struct brw_blorp_blit_prog_key key;

   key.tex_samples = 8;
   key.tex_layout = INTEL_MSAA_LAYOUT_UMS;
   key.src_samples = 8;
   key.src_layout = INTEL_MSAA_LAYOUT_UMS;
   key.rt_samples = 0;
   key.dst_samples = 0;
   key.texture_data_type = BRW_REGISTER_TYPE_F;
   key.src_tiled_w = false;
   key.dst_tiled_w = false;
   key.blend = true;
   key.use_kill = false;
   key.persample_msaa_dispatch = false;
   key.blit_scaled = true;
   key.x_scale = 2.000000;
   key.y_scale = 4.000000;
   key.bilinear_filter = false;

   return check(brw, &key, expected, sizeof(expected) - 1);
}

/**
 * One of the flavours gotten when running piglit test:
 * "ext_framebuffer_multisample-blit-scaled 8"
 */
static bool
test_gen7_msaa_8_ums_to_cms(struct brw_context *brw)
{
   static const char expected[] =
      "0x00000000: add(16)         g44<1>UW        g1.4<2,4,0>UW   0x10101010V     { align1 WE_normal 1H };\n"
      "0x00000010: add(16)         g46<1>UW        g1.5<2,4,0>UW   0x11001100V     { align1 WE_normal 1H };\n"
      "0x00000020: mov(16)         g48<1>UD        g44<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000030: mov(16)         g50<1>UD        g46<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000040: and(1)          g54<1>UD        g0<0,1,0>UD     0x000000c0UD    { align1 WE_normal };\n"
      "0x00000050: shr(1)          g54<1>UD        g54<0,1,0>UD    0x00000005UD    { align1 WE_normal };\n"
      "0x00000060: mov(16)         g56<1>UW        0x00003210V                     { align1 WE_normal 1H };\n"
      "0x00000070: add(16)         g52<1>UD        g54<0,1,0>UW    g56<1,4,0>UW    { align1 WE_normal 1H };\n"
      "0x00000080: add(8)          g53<1>UD        g54<0,1,0>UW    g56.2<1,4,0>UW  { align1 WE_normal 1Q };\n"
      "0x00000090: mov(16)         g44<1>F         g48<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000000a0: mov(16)         g46<1>F         g50<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000000b0: mul(16)         g48<1>F         g44<8,8,1>F     g2.6<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000c0: mul(16)         g50<1>F         g46<8,8,1>F     g3<0,1,0>F      { align1 WE_normal 1H };\n"
      "0x000000d0: add(16)         g48<1>F         g48<8,8,1>F     g2.7<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000e0: add(16)         g50<1>F         g50<8,8,1>F     g3.1<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000f0: mov(16)         g44<1>UD        g48<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000100: mov(16)         g46<1>UD        g50<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000110: mov(16)         g114<1>UD       g52<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000120: mov(16)         g116<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000130: mov(16)         g118<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000140: send(16)        g4<1>UW         g114<8,8,1>F\n"
      "                sampler (0, 0, 31, 2) mlen 6 rlen 8             { align1 WE_normal 1H };\n"
      "0x00000150: mov(16)         g114<1>F        g4<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000160: mov(16)         g116<1>F        g6<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000170: mov(16)         g118<1>F        g8<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x00000180: mov(16)         g120<1>F        g10<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000190: sendc(16)       null            g114<8,8,1>F\n"
      "                render ( RT write, 1, 0, 12) mlen 8 rlen 0      { align1 WE_normal 1H EOT };\n";
   struct brw_blorp_blit_prog_key key;

   key.tex_samples = 8;
   key.tex_layout = INTEL_MSAA_LAYOUT_UMS;
   key.src_samples = 8;
   key.src_layout = INTEL_MSAA_LAYOUT_UMS;
   key.rt_samples = 8;
   key.rt_layout = INTEL_MSAA_LAYOUT_CMS;
   key.dst_samples = 8;
   key.dst_layout = INTEL_MSAA_LAYOUT_CMS;
   key.texture_data_type = BRW_REGISTER_TYPE_F;
   key.src_tiled_w = false;
   key.dst_tiled_w = false;
   key.blend = false;
   key.use_kill = false;
   key.persample_msaa_dispatch = true;
   key.blit_scaled = false;
   key.x_scale = 2.000000;
   key.y_scale = 4.000000;
   key.bilinear_filter = false;

   return check(brw, &key, expected, sizeof(expected) - 1);
}

static bool
test_gen7_msaa_8_cms_to_cms(struct brw_context *brw)
{
   static const char expected[] =
      "0x00000000: add(16)         g44<1>UW        g1.4<2,4,0>UW   0x10101010V     { align1 WE_normal 1H };\n"
      "0x00000010: add(16)         g46<1>UW        g1.5<2,4,0>UW   0x11001100V     { align1 WE_normal 1H };\n"
      "0x00000020: mov(16)         g48<1>UD        g44<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000030: mov(16)         g50<1>UD        g46<8,8,1>UW                    { align1 WE_normal 1H };\n"
      "0x00000040: and(1)          g54<1>UD        g0<0,1,0>UD     0x000000c0UD    { align1 WE_normal };\n"
      "0x00000050: shr(1)          g54<1>UD        g54<0,1,0>UD    0x00000005UD    { align1 WE_normal };\n"
      "0x00000060: mov(16)         g56<1>UW        0x00003210V                     { align1 WE_normal 1H };\n"
      "0x00000070: add(16)         g52<1>UD        g54<0,1,0>UW    g56<1,4,0>UW    { align1 WE_normal 1H };\n"
      "0x00000080: add(8)          g53<1>UD        g54<0,1,0>UW    g56.2<1,4,0>UW  { align1 WE_normal 1Q };\n"
      "0x00000090: mov(16)         g44<1>F         g48<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000000a0: mov(16)         g46<1>F         g50<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x000000b0: mul(16)         g48<1>F         g44<8,8,1>F     g2.6<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000c0: mul(16)         g50<1>F         g46<8,8,1>F     g3<0,1,0>F      { align1 WE_normal 1H };\n"
      "0x000000d0: add(16)         g48<1>F         g48<8,8,1>F     g2.7<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000e0: add(16)         g50<1>F         g50<8,8,1>F     g3.1<0,1,0>F    { align1 WE_normal 1H };\n"
      "0x000000f0: mov(16)         g44<1>UD        g48<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000100: mov(16)         g46<1>UD        g50<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x00000110: mov(16)         g114<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000120: mov(16)         g116<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000130: send(16)        g36<1>UW        g114<8,8,1>F\n"
      "                sampler (0, 0, 29, 2) mlen 4 rlen 8             { align1 WE_normal 1H };\n"
      "0x00000140: mov(16)         g114<1>UD       g52<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000150: mov(16)         g116<1>UD       g36<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000160: mov(16)         g118<1>UD       g44<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000170: mov(16)         g120<1>UD       g46<8,8,1>UD                    { align1 WE_normal 1H };\n"
      "0x00000180: send(16)        g4<1>UW         g114<8,8,1>F\n"
      "                sampler (0, 0, 30, 2) mlen 8 rlen 8             { align1 WE_normal 1H };\n"
      "0x00000190: mov(16)         g114<1>F        g4<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x000001a0: mov(16)         g116<1>F        g6<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x000001b0: mov(16)         g118<1>F        g8<8,8,1>F                      { align1 WE_normal 1H };\n"
      "0x000001c0: mov(16)         g120<1>F        g10<8,8,1>F                     { align1 WE_normal 1H };\n"
      "0x000001d0: sendc(16)       null            g114<8,8,1>F\n"
      "                render ( RT write, 1, 0, 12) mlen 8 rlen 0      { align1 WE_normal 1H EOT };\n";
   struct brw_blorp_blit_prog_key key;

   key.tex_samples = 8;
   key.tex_layout = INTEL_MSAA_LAYOUT_CMS;
   key.src_samples = 8;
   key.src_layout = INTEL_MSAA_LAYOUT_CMS;
   key.rt_samples = 8;
   key.rt_layout = INTEL_MSAA_LAYOUT_CMS;
   key.dst_samples = 8;
   key.dst_layout = INTEL_MSAA_LAYOUT_CMS;
   key.texture_data_type = BRW_REGISTER_TYPE_F;
   key.src_tiled_w = false;
   key.dst_tiled_w = false;
   key.blend = false;
   key.use_kill = false;
   key.persample_msaa_dispatch = true;
   key.blit_scaled = false;
   key.x_scale = 2.000000;
   key.y_scale = 4.000000;
   key.bilinear_filter = false;

   return check(brw, &key, expected, sizeof(expected) - 1);
}

int
main(int argc, char **argv)
{
   bool pass = true;
   struct brw_context brw;

   memset(&brw, 0, sizeof(brw));
   brw.gen = 7;

   pass = test_gen7_blend_scaled_msaa_8(&brw) && pass;
   pass = test_gen7_msaa_8_ums_to_cms(&brw) && pass;
   pass = test_gen7_msaa_8_cms_to_cms(&brw) && pass;

   /* Test suite expects zero for success */
   return !pass;
}