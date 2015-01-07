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
char *version = "0.0.2";

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
    char *images = (char*) malloc(2048);
    int images_length = 0;

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
            dprintf("directory output is:%s\n", dir);
            pre = strtok(argv[optind + 1], ".");
            suf = strtok(NULL, ".");
            if (suf == NULL) {
                suf = "jpg";
            }
            dprintf("pre:%s suf:%s", pre, suf);
            sprintf(output, "%s/%s.%s",
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
        fflush(stdout);
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
                dprintf("\nw:%d x*scale:%f w/(x*scale):%f ceilf(...):%f",
                    w, float(x * scale),
                    float(w) / float(x * scale),
                    ceilf(float(w) / float(x * scale)));
                sw = int(ceilf(float(w) / float(x * scale)));
            }
            if (y >= 1 || x < 1) {
                dprintf("\nh:%d y*scale:%f h/(y*scale):%f ceilf(...):%f",
                    h, float(y * scale),
                    float(h) / float(y * scale),
                    ceilf(float(h) / float(y * scale)));
                sh = int(ceilf(float(h) / float(y * scale)));
            }
            scale++;
            dprintf("\nscale:%d, w:%d h:%d - sw*sh:%f max:%d", scale, w, h, float(sw * sh), max);
        } while (sw * sh > max);

        slices = int(ceilf(float(float(w) / float(sw)) * float(float(h) / float(sh))));
        dprintf("(w / sw):%f (h / sh):%f", float(float(w) / float(sw)), float(float(h) / float(sh)));

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
        fflush(stdout);
        return 1;
    }

    try {
        int xoffset = 0;
        int yoffset = 0;
        while (slices) {
            dprintf("creating %d slice\n", slices);
            dprintf("output %s\n", output);
            sprintf(filename, output, slices);
            dprintf("filename: %s\n", filename);
            try {
                images_length += sprintf(images + images_length,
                    "{\\\"id\\\":\\\"%d\\\",\\\"url\\\":\\\"%s\\\"},", slices, filename);
            dprintf("images: %s\n", images);
            }
            catch( Exception &error_ )
            {
                printf("Error adding image to list: %s\n", error_.what());
        fflush(stdout);
                return 1;
            }
            slices--;
            dprintf("writing %s slice\n", filename);
            dprintf("%d %d, %d %d", sw + overlap, sh + overlap, xoffset, yoffset);
            Image slice = image;
            try {
            slice.crop(
                Geometry(sw + overlap, sh + overlap, xoffset, yoffset));
            }
            catch( Exception &error_ )
            {
                printf("Error creating cropped slice: %s crop(%d, %d, %d, %d)\n",
                    error_.what(),
                    sw + overlap, sh + overlap, xoffset, yoffset);
        fflush(stdout);
                return 1;
            }
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

            record_length += sprintf(record + record_length, "d[\'%d\'](c,%d,%d);",
                slices, xoffset, yoffset);
        }
    }
    catch( Exception &error_ )
    {
        printf("Error creating image slices: %s\n", error_.what());
        fflush(stdout);
        return 1;
    }

    try {
    record_length += sprintf(record + record_length, "})\",\"images\":\"[%s{}]\"",
        images);
    record_length += sprintf(record + record_length, "}}");
    dprintf("%s\n", record);

    sprintf(filename, "%s/1", dir);
    }
    catch( Exception &error_ )
    {
        printf("Error creating page json: %s\n", error_.what());
        fflush(stdout);
        return 1;
    }
    FILE *page_file;
    try {
        page_file = fopen (filename, "w");
        dprintf("%s\n", record);
        fprintf(page_file, "%s\n", record);
        fclose (page_file);
    }
    catch( Exception &error_ )
    {
        printf("Error writing page json: %s\n", error_.what());
        fflush(stdout);
        return 1;
    }

    try {
        sprintf(filename, "%s/toc.json", dir);
        page_file = fopen (filename, "w");
        record_length = sprintf(record, "{\"version\":{\"Branch\":\"%s\"},\"meta\":\"\"", version);
        record_length += sprintf(record + record_length, ",\"max_page_width\":%d,\"max_page_height\":%d", w, h);
        record_length += sprintf(record + record_length, ",\"pages\":[{\"url\":\"rendered/1\",\"width\":%d,\"height\":%d,\"mime_type\":\"application/json\"}]", w, h);
// {"max_page_height":2696,"max_page_width":4000,"pages":[{"url":"rendered/page1","width":4000,"height":2696,"mime_type":"application/json"}],"meta":"","version":{"Branch":"narcissus"}}
        record_length = sprintf(record + record_length, "}");
        fprintf(page_file, "%s\n", record);
        fclose (page_file);
    }
    catch( Exception &error_ )
    {
        printf("Error writing toc: %s\n", error_.what());
        fflush(stdout);
        return 1;
    }

    return 0;
}