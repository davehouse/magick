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
//#include <ctype.h>
#include <getopt.h>
#include <string.h>

#include <Magick++.h>

using namespace std; 
using namespace Magick; 

int main(int argc,char **argv)
{
    const char *input;
    char output[2000];
    char filename[2000];
    int x = 1;
    int y = 1;
    int c;
    int max = 3e6;
    int slices = 1;

    while ((c = getopt (argc, argv, "n::x::y::")) != -1) {
        if (optarg == NULL) {
            continue;
        }
        switch (c) {
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
        printf ("Usage: <input_image_file> <output_image_file> -x 10 -y 1\n");
        return 0;
    }

    input = argv[optind];

    if (optind + 1 < argc) {
        if (strstr(argv[optind + 1], "%d") != NULL) {
            sprintf(output, "%s", argv[optind + 1]);
        } else {
            char * pre = strtok(argv[optind + 1], ".");
            char * suf = strtok(NULL, ".");
            fprintf(stderr, "prefix:%s suffix:%s", pre, suf);
            sprintf(output, "%s.%s.%s",
                pre,
                "%d",
                suf
                );
        }
    } else {
        sprintf(output, "%s%s.png", input, "%d");
    }

    printf("image filename:%s\n", input);
    printf("output image filename:%s\n", output);
    printf("maximum pixel count for slices:%d\n", max);
    printf("target slices horizontally:%d\n", y);
    printf("target slices vertically:%d\n", x);

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

    try {
        unsigned int w = image.columns();
        unsigned int h = image.rows();
        printf("w:%d h:%d\n", w, h);

        float sw = w;
        float sh = h;

        int scale = 1;
        while (sw * sh > max) {
            if (x > 1) {
                sw = w / (x * scale);
            }
            if (y > 1) {
                sh = h / (y * scale);
            }
            else if (x <= 1) {
                sh = h / (y * scale);
            }
            scale++;
        }

        slices = (w / sw) * (h / sh);

        printf("slice w:%f h:%f\n", sw, sh);
        printf("slice count:%d\n", slices);
    } 
    catch( Exception &error_ ) 
    { 
        cout << "Error calculating image slice dimensions: " << error_.what() << endl; 
        return 1; 
    } 

    try {
        Image slice = image;
        while (slices) {
            sprintf(filename, output, slices);
            slices--;
            printf("writing %s slice\n", filename);
            fflush(stdout);
            slice.write(filename);
        }
    } 
    catch( Exception &error_ ) 
    { 
        cout << "Error creating image slices: " << error_.what() << endl; 
        return 1; 
    } 

    return 0; 
}