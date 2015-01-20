/*
 * jpegslice.c
 *
 * lossless cropping of JPEG data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// #include <sys/stat.h>
// #include <stddef.h>
// #include <fcntl.h> // open
#include <errno.h>
#include <jpeglib.h>
#include <turbojpeg.h>

static char outfilename[255];
static char *outfilename2 = "outjpg";

int main (int argc, char **argv) {
    FILE * file;
    long fsize;
    unsigned char *jpegBuf;
    FILE * fp;

    char *filename;
    unsigned long max = (1024 * 1024) * 3;
    int overlap = 0;

    if (argc > 1) {
        filename = argv[1];
    } else {
        fprintf(stderr,
            "jpegslice [input.jpg] [output_prefix] [max_pixels] [overlap]");
    }
    fprintf(stderr, "input file: %s\n", filename);
    if ((file = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "can't open %s for reading\n", filename);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    jpegBuf = malloc(fsize + 1);
    fread(jpegBuf, fsize, 1, file);
    fclose(file);

    if (argc > 2) {
        outfilename2 = argv[2];
    }

    if (argc > 3) {
        max = atoi(argv[3]);
    }
    fprintf(stderr, "max pixel count: %lu\n", max);

    if (argc > 4) {
        overlap = atoi(argv[4]);
    }
    fprintf(stderr, "slice overlap: %d\n", overlap);

    tjtransform transforms[20];
    tjregion rect;

    unsigned char *dstBufs[20];
    unsigned long dstSizes[20];

    unsigned long jpegSize = fsize;
    int n;

    tjhandle handle = tjInitTransform();

    int x, y, w, h, sliceh;
    int jpegSubsamp = 0;
    int jpegColorspace = 0;
    if (-1 == tjDecompressHeader2(handle, jpegBuf, jpegSize, &w, &h, &jpegSubsamp)) {
        fprintf(stderr, "Error with decompress header: %s\n", tjGetErrorStr());
    }
    fprintf(stderr, "input image: %d x %d subsamp:%d colorspace:%d MCUWidth:%d\n",
            w, h, jpegSubsamp, jpegColorspace, tjMCUWidth[jpegSubsamp]);
    sliceh = max / w;

    if (sliceh > h) {
        fprintf(stderr, "Original image is smaller than maximum slice pixel count.\n");
        exit(1);
    }

    int offset = h % sliceh;
    if (offset != 0) {
        fprintf(stderr, "offset: %d\n", offset);
        n = ceil((float)h / sliceh);
    } else {
        n = h / sliceh;
    }

    int adjusted = 0;
    int MCUHeight = tjMCUHeight[jpegSubsamp];
    if (sliceh % MCUHeight) {
        fprintf(stderr, "adjusting slice height %d to match MCUHeight: %d\n", sliceh, MCUHeight);
        adjusted = sliceh % MCUHeight;
        sliceh = sliceh - adjusted;
    }
    fprintf(stderr, "slice height: %d\n", sliceh);
    fprintf(stderr, "slice count: %d\n", n);

    int last_height = h - (n - 1) * sliceh;
    fprintf(stderr, "width:%d\n", w);
    fprintf(stderr, "last height:%d\n", last_height);
    x = 0;
    w = 0;
    h = sliceh + overlap;
    for (int i = 0; i < n; ++i) {
        y = i * sliceh;
        fprintf(stderr, "%d * %d = %d  \t", i, sliceh, y);
        if (i == n - 1) {
            rect = (tjregion) { x, y, w, last_height };
        } else {
            rect = (tjregion) { x, y, w, h };
        }
        transforms[i] = (tjtransform) {
            rect, TJXOP_NONE, TJXOPT_CROP, NULL, NULL
        };
        fprintf(stderr, "%d x %d n=%d %dx%d\n", x, y, i, rect.x, rect.y);
        fflush(stderr);

        dstBufs[i] = NULL;
        dstSizes[i] = 0;
    }

    fprintf(stderr, "Next, perform transforms ...\n");
    int flags = 0;
    if (-1 == tjTransform(handle, jpegBuf, jpegSize,
                     n, dstBufs, dstSizes, transforms, flags, outfilename2)) {
        fprintf(stderr, "Error with transform: %s\n", tjGetErrorStr());
        exit(1);
    }

    fprintf(stderr, "destroy transform handler ...\n");
    tjDestroy(handle);

    /*
    fprintf(stderr, "write buffers to files ...\n");
    for (int i = 0; i < n; ++i) {
        sprintf(outfilename, "%s-%d.jpg", outfilename2, i);
        fprintf(stderr, "size=%lu fn=%s\n", dstSizes[i], outfilename);

        fp = fopen(outfilename, "w");
        if (fp == NULL) {
            fprintf(stderr, "errno: %d \n", errno);
            fprintf(stderr, "Can't open %s for writing\n", outfilename);
            exit(EXIT_FAILURE);
        }
        fwrite(dstBufs[i], dstSizes[i], 1, fp);

        fclose(fp);
    }

    for (int i=n-1; i >= 0; --i) {
//        tjFree(dstBufs[i]);
    }
    */

    return 0;
}
