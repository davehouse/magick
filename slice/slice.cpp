/*
 * =====================================================================================
 *
 *       Filename:  canvas_slice.cpp
 *
 *    Description:  Slice input image into max_pixel slices for drawing to html canvas with overlap.
 *
 *        Version:  0.0.1
 *        Created:  10/15/2014 04:31:18 PM
 *       Revision:  10/16/2014 06:26:00 AM
 *       Compiler:  gcc
 *
 *         Author:  David House (dave.a.house@gmail.com)
 *
 * =====================================================================================
 */

/*
COMPILE:
c++ `Magick++-config --cxxflags --cppflags` -O2 -o slice slice.cpp   `Magick++-config --ldflags --libs`
*/
#include <stdlib.h>

#include <iostream>

//#include <stdio.h>
//#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <math.h>

#include <Magick++.h>

using namespace std;
using namespace Magick;

#ifdef DEBUG
#define DPRINTF(s) printf(s)
#else
#define DPRINTF(s)
#endif

int main(int argc,char **argv)
{
    const char *input;
    char output[2000];
    char filename[2000];
    int x = 0;
    int y = 0;
    int c;
    int max = 3e6;
    int slices = 1;
    int overlap = 0;

    const int record_max_length = 1024 * 1024 * 1024;
    char* record = (char*) malloc(record_max_length);
    int record_length = 0;


    while ((c = getopt (argc, argv, "n::o::x::y::")) != -1) {
        if (optarg == NULL) {
            continue;
        }
        switch (c) {
        case 'o':
            overlap = atoi(optarg);
            break;
        case 'n':
            max = atoi(optarg);
            break;
        case 'x':
            x = atoi(optarg);
            break;
        case 'y':
            y = atoi(optarg);
            break;
        case '?':
            if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            return 1;
        default:
            break;
        }
    }

    if (optind == argc) {
        printf ("Usage: <input_image_file> <output_image_file> -x10 -y1\n");
        return 0;
    }

    input = argv[optind];

    char *pre;
    char *suf;
    if (optind + 1 < argc) {
        if (strstr(argv[optind + 1], "%d") != NULL) {
            sprintf(output, "%s", argv[optind + 1]);
        } else {
            pre = strtok(argv[optind + 1], ".");
            suf = strtok(NULL, ".");
            sprintf(output, "%s.%s.%s",
                pre,
                "%d",
                suf
                );
        }
    } else {
        sprintf(output, "%s%s.png", input, "%d");
    }

    DPRINTF("image filename:%s\n", input);
    DPRINTF("output image filename:%s\n", output);
    DPRINTF("maximum pixel count for slices:%d\n", max);
    DPRINTF("target slices horizontally:%d\n", y);
    DPRINTF("target slices vertically:%d\n", x);
    DPRINTF("slice overlap:%d\n", overlap);

    InitializeMagick(*argv);

    Image image;
    try {
        image.read(input);
    }
    catch( Exception &error_ )
    {
        cout << "Error reading image: " << error_.what() << endl;
        return 1;
    }

    unsigned int w = 0;
    unsigned int h = 0;
    int sw, sh;
    try {
        w = image.columns();
        h = image.rows();
        printf("w:%d h:%d\n", w, h);

        sw = w;
        sh = h;

        int scale = 1;
        do {
            if (x >= 1) {
                sw = ceilf(float(w) / (x * scale));
            }
            if (y >= 1 || x < 1) {
                sh = ceilf(float(h) / (y * scale));
            }
            scale++;
        } while (sw * sh > max);

        slices = (w / sw) * (h / sh);

        printf("slice w:%d h:%d\n", sw, sh);
        printf("slice count:%d\n", slices);

        record_length = sprintf(record, "{\"original\":\"%s\"", input);
        record_length += sprintf(record + record_length, ",\"mime_type\":\"application/json\"");
        record_length += sprintf(record + record_length, ",\"width\":%d,\"height\":%d", w, h);
        record_length += sprintf(record + record_length, ",\"url\":\"%s\"", output);
        record_length += sprintf(record + record_length, "}");

        record_length = sprintf(record, "{\"status\":200,\"result\":{");
        record_length += sprintf(record + record_length, "\"content_type\":\"canvas\"");
        record_length += sprintf(record + record_length, ",\"fonts\":\"[]\"");
        record_length += sprintf(record + record_length, ",\"metrics\":\"[]\"");
        record_length += sprintf(record + record_length, ",\"links\":\"[]\"");
        record_length += sprintf(record + record_length, ",\"canvas_width\":%d,\"canvas_height\":%d", w, h);
        record_length += sprintf(record + record_length, ",\"draw_instructions\":\"(function(c,d){");

        fprintf(stdout, "%s\n", record);

    }
    catch( Exception &error_ )
    {
        cout << "Error calculating image slice dimensions: " << error_.what() << endl;
        return 1;
    }

    try {
        int xoffset = 0;
        int yoffset = 0;
        while (slices) {
            sprintf(filename, output, slices);
            slices--;
            printf("writing %s slice\n", filename);
            fflush(stdout);
            /*
            slice.crop("%dx%d+%d+%d",
                sw + overlap, sh + overlap,
                xoffset, yoffset);
            */
            // size_t width_, size_t height_, ssize_t xOff_ = 0, ssize_t yOff_
            // = 0, bool xNegative_ = false, bool yNegative_ = false
            printf("%d %d, %d %d", sw + overlap, sh + overlap, xoffset, yoffset);
            Image slice = image;
            slice.crop(
                Geometry(sw + overlap, sh + overlap, xoffset, yoffset));
            xoffset += sw;
            if (xoffset >= w) {
                xoffset = 0;
                yoffset += sh;
            }
            if (yoffset >= h) {
                printf("finished slicing xoff:%d yoff:%d slices remain:%d last filename:%s\n",
                    xoffset, yoffset, slices, filename);
            }
            slice.write(filename);

            char *tile_name = "tilenamehere";
            record_length += sprintf(record + record_length, "d[\'%s\'](c,%d,%d);",
                tile_name, xoffset, yoffset);

        }
    }
    catch( Exception &error_ )
    {
        cout << "Error creating image slices: " << error_.what() << endl;
        return 1;
    }

    char *images = "imagelisthere";
    record_length += sprintf(record + record_length, "})\",\"images\":\"[%s]\"",
        images);
    record_length += sprintf(record + record_length, "}}");
    fprintf(stdout, "%s\n", record);

    sprintf(filename, "%s.json", pre);
    FILE *page_file;
    page_file = fopen (filename, "w");
    fprintf(stdout, "%s\n", record);
    fprintf(page_file, "%s\n", record);
    fclose (page_file);

    sprintf(filename, "%s.json", pre);
    page_file = fopen (filename, "w");
    fprintf(stdout, "%s\n", record);
    fprintf(page_file, "%s\n", record);
    fclose (page_file);

    return 0;
}