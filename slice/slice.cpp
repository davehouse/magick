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
#include <getopt.h>
#include <string.h>
#include <math.h>

#include <Magick++.h>

using namespace std;
using namespace Magick;

#ifdef DEBUG
#define dprintf(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
#define dprintf(...)
#endif

void list_formats(bool verbose=false) {
    list<CoderInfo> coderList;

    coderInfoList( &coderList,
        CoderInfo::TrueMatch,   // readable
        CoderInfo::AnyMatch,    // writable
        CoderInfo::AnyMatch);   // multi-frame

    list<CoderInfo>::iterator entry = coderList.begin();

    printf("Supported input formats: ");
    if (verbose) {
        printf("EXT: (description) readable writable multiframe");
    }
    while( entry != coderList.end() )
    {
        if (verbose) {
            printf("%s: (%s) %b %b %b", entry->name().c_str(),
                entry->description().c_str(),
                entry->isReadable(), entry->isWritable(), entry->isMultiFrame());
        } else {
            printf("%s%s ", entry->name().c_str(),
                entry->isWritable() ? "" : "*");
        }
        entry++;
    }
    printf("\n* : only as input image (not writable)\n\n");
}

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

    bool help = false;

    while ((c = getopt (argc, argv, "n::o::x::y::h")) != -1) {
        if (optarg == NULL) {
            continue;
        }
        switch (c) {
        case 'h':
            help = true;
            break;
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
            help = true;
        default:
            break;
        }
    }

    if (optind == argc || help == true) {
        printf("\nUsage: [ options ] <input_image_file> [ <output_image_file> ]\n\n");
        printf("Options:\n");
        printf("\t-x\t:\thorizontal slicing target count/ratio ( 0: no horizontal cuts )\n");
        printf("\t-y\t:\tvertical slicing target count/ratio ( 0: no vertical cuts )\n");
        printf("Examples:\n");
        printf("\t-x5 -y3  # Cuts are 5:3. For a 500x300px image this would return 100px square slices.\n");
        printf("\t-x0 -y1  # Cuts are 0:1. Slices will have original image width.\n");
        printf("\t-x2 -y1  # Cuts are 2:1. Slices will have width double their height.\n");
        list_formats();
        return 0;
    }

    input = argv[optind];

    char *pre;
    char *suf;
    char *dir;
    if (optind + 1 < argc) {
        if (strstr(argv[optind + 1], "%d") != NULL) {
            sprintf(output, "%s", argv[optind + 1]);
        } else {
            dir = strtok(argv[optind + 1], "/");
            dprintf("directory output is:%s", dir);
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

    dprintf("image filename:%s\n", input);
    dprintf("output image filename:%s\n", output);
    dprintf("maximum pixel count for slices:%d\n", max);
    dprintf("target slices horizontally:%d\n", y);
    dprintf("target slices vertically:%d\n", x);
    dprintf("slice overlap:%d\n", overlap);

    InitializeMagick(*argv);

    Image image;
    try {
        image.read(input);
    }
    catch( Exception &error_ )
    {
        printf("Error reading image: %s\n", error_.what());
        return 1;
    }

    unsigned int w = 0;
    unsigned int h = 0;
    int sw, sh;
    try {
        w = image.columns();
        h = image.rows();
        dprintf("w:%d h:%d\n", w, h);

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

        dprintf("slice w:%d h:%d\n", sw, sh);
        dprintf("slice count:%d\n", slices);

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

        dprintf("%s\n", record);

    }
    catch( Exception &error_ )
    {
        printf("Error calculating image slice dimensions: %s\n", error_.what());
        return 1;
    }

    try {
        int xoffset = 0;
        int yoffset = 0;
        while (slices) {
            sprintf(filename, output, slices);
            slices--;
            dprintf("writing %s slice\n", filename);
            /*
            slice.crop("%dx%d+%d+%d",
                sw + overlap, sh + overlap,
                xoffset, yoffset);
            */
            // size_t width_, size_t height_, ssize_t xOff_ = 0, ssize_t yOff_
            // = 0, bool xNegative_ = false, bool yNegative_ = false
            dprintf("%d %d, %d %d", sw + overlap, sh + overlap, xoffset, yoffset);
            Image slice = image;
            slice.crop(
                Geometry(sw + overlap, sh + overlap, xoffset, yoffset));
            xoffset += sw;
            if (xoffset >= w) {
                xoffset = 0;
                yoffset += sh;
            }
            if (yoffset >= h) {
                dprintf("finished slicing xoff:%d yoff:%d slices remain:%d last filename:%s\n",
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
        printf("Error creating image slices: %s\n", error_.what());
        return 1;
    }

    char *images = "imagelisthere";
    record_length += sprintf(record + record_length, "})\",\"images\":\"[%s]\"",
        images);
    record_length += sprintf(record + record_length, "}}");
    dprintf("%s\n", record);

    sprintf(filename, "%s/1", dir);
    FILE *page_file;
    page_file = fopen (filename, "w");
    dprintf("%s\n", record);
    fprintf(page_file, "%s\n", record);
    fclose (page_file);

    sprintf(filename, "%s/toc.json", dir);
    page_file = fopen (filename, "w");
    dprintf("%s\n", record);
    fprintf(page_file, "%s\n", record);
    fclose (page_file);

    return 0;
}